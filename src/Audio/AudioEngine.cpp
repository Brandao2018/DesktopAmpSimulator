#include "AudioEngine.h"

#include "Shared/Constants.h"
#include "Shared/Logger.h"
#include "Shared/MeterProcessor.h"

AudioEngine::AudioEngine()
{
    deviceManager.addChangeListener(this);
}

AudioEngine::~AudioEngine()
{
    deviceManager.removeChangeListener(this);
    shutdownAudio();
}

//==============================================================================
// Real-time audio thread
//==============================================================================

void AudioEngine::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    meters_.sampleRate.store(sampleRate, std::memory_order_release);
    meters_.bufferSize.store(samplesPerBlockExpected, std::memory_order_release);
    smoothingCoeff_ = ampsim::smoothingCoefficient(ampsim::kMeterSmoothingMs,
                                                   sampleRate, samplesPerBlockExpected);

    for (auto& io : smoothedPeak_)
        for (auto& ch : io)
            ch = ampsim::kMeterFloorDb;

    refreshLatencyFromDevice();
    meters_.running.store(true, std::memory_order_release);
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto& buffer = *bufferToFill.buffer;
    const int numChannels = juce::jmin(buffer.getNumChannels(), ampsim::kNumChannels);
    const auto numSamples = static_cast<size_t>(bufferToFill.numSamples);

    // The buffer arrives pre-filled with the device input. Measure it.
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* data = buffer.getReadPointer(ch, bufferToFill.startSample);
        const float peakDb = ampsim::gainToDb(ampsim::channelPeak(data, numSamples));
        const float rmsDb  = ampsim::gainToDb(ampsim::channelRms(data, numSamples));

        smoothedPeak_[0][ch] = ampsim::updateSmoothedLevel(smoothedPeak_[0][ch], peakDb, smoothingCoeff_);
        auto& meter = (ch == 0) ? meters_.input.left : meters_.input.right;
        meter.store(smoothedPeak_[0][ch], rmsDb);
    }

    // Phase 1: pass-through (input stays in the buffer, unchanged).
    ampsim::processPassThrough(buffer.getArrayOfWritePointers(),
                               numChannels, bufferToFill.numSamples);

    // Output meters (identical to input in Phase 1, but measured separately
    // so the pipeline is honest once processing exists).
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* data = buffer.getReadPointer(ch, bufferToFill.startSample);
        const float peakDb = ampsim::gainToDb(ampsim::channelPeak(data, numSamples));
        const float rmsDb  = ampsim::gainToDb(ampsim::channelRms(data, numSamples));

        smoothedPeak_[1][ch] = ampsim::updateSmoothedLevel(smoothedPeak_[1][ch], peakDb, smoothingCoeff_);
        auto& meter = (ch == 0) ? meters_.output.left : meters_.output.right;
        meter.store(smoothedPeak_[1][ch], rmsDb);
    }
}

void AudioEngine::releaseResources()
{
    meters_.running.store(false, std::memory_order_release);
    meters_.reset();
}

//==============================================================================
// Message/UI thread
//==============================================================================

bool AudioEngine::start()
{
    setAudioChannels(ampsim::kNumChannels, ampsim::kNumChannels);

    if (deviceManager.getCurrentAudioDevice() == nullptr)
    {
        setLastError("No audio device available. Check connections and try again.");
        ampsim::Logger::error("AudioEngine::start: no audio device available");
        return false;
    }

    setLastError({});
    ampsim::Logger::info("Audio started on device: " + getCurrentDeviceName());
    return true;
}

void AudioEngine::stop()
{
    shutdownAudio();
    meters_.running.store(false, std::memory_order_release);
    meters_.reset();
    ampsim::Logger::info("Audio stopped");
}

bool AudioEngine::setAudioDevice(const juce::String& deviceName)
{
    auto setup = deviceManager.getAudioDeviceSetup();
    setup.inputDeviceName = deviceName;
    setup.outputDeviceName = deviceName;
    setup.useDefaultInputChannels = true;
    setup.useDefaultOutputChannels = true;

    const juce::String error = deviceManager.setAudioDeviceSetup(setup, true);
    if (error.isNotEmpty())
    {
        setLastError(error.toStdString());
        ampsim::Logger::error("setAudioDevice failed: " + error.toStdString());
        return false;
    }

    setLastError({});
    refreshLatencyFromDevice();
    return true;
}

bool AudioEngine::setBufferSize(int numSamples)
{
    auto setup = deviceManager.getAudioDeviceSetup();
    setup.bufferSize = numSamples;

    const juce::String error = deviceManager.setAudioDeviceSetup(setup, true);
    if (error.isNotEmpty())
    {
        setLastError(error.toStdString());
        ampsim::Logger::error("setBufferSize failed: " + error.toStdString());
        return false;
    }

    setLastError({});
    refreshLatencyFromDevice();
    return true;
}

std::vector<std::string> AudioEngine::getAvailableDeviceNames() const
{
    std::vector<std::string> names;

    // const_cast: scanForDevices/getCurrentDeviceTypeObject are non-const in
    // JUCE, but enumeration does not mutate engine state we care about.
    auto& manager = const_cast<juce::AudioDeviceManager&>(deviceManager);

    if (auto* type = manager.getCurrentDeviceTypeObject())
    {
        type->scanForDevices();
        for (const auto& name : type->getDeviceNames(false))
            names.push_back(name.toStdString());
    }

    return names;
}

std::string AudioEngine::getCurrentDeviceName() const
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
        return device->getName().toStdString();
    return {};
}

std::string AudioEngine::getLastError() const
{
    std::lock_guard<std::mutex> lock(errorMutex_);
    return lastError_;
}

void AudioEngine::setLastError(const std::string& message)
{
    std::lock_guard<std::mutex> lock(errorMutex_);
    lastError_ = message;
}

void AudioEngine::changeListenerCallback(juce::ChangeBroadcaster*)
{
    // Runs on the JUCE message thread (pumped from the Qt event loop) when a
    // device is added/removed or the configuration changes.
    deviceListChanged_.store(true, std::memory_order_release);
    refreshLatencyFromDevice();
}

void AudioEngine::refreshLatencyFromDevice()
{
    // Device-reported roundtrip latency: input + output + one buffer.
    // (An active chirp-loopback measurement is planned for a later phase;
    // it needs a physical loopback cable to be meaningful.)
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const double rate = device->getCurrentSampleRate();
        if (rate > 0.0)
        {
            const int samples = device->getInputLatencyInSamples()
                              + device->getOutputLatencyInSamples()
                              + device->getCurrentBufferSizeSamples();
            meters_.latencyMs.store(static_cast<float>(1000.0 * samples / rate),
                                    std::memory_order_release);
            meters_.sampleRate.store(rate, std::memory_order_release);
            meters_.bufferSize.store(device->getCurrentBufferSizeSamples(),
                                     std::memory_order_release);
        }
    }
}
