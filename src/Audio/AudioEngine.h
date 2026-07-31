#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include <juce_audio_utils/juce_audio_utils.h>

#include "Shared/ThreadSafeBuffer.h"

// Real-time audio I/O for Phase 1: opens the system audio device, passes
// input to output unchanged, and publishes meter/latency data through
// lock-free atomics for the Qt UI thread.
//
// Threading contract:
//  - getNextAudioBlock() runs on the real-time audio thread. It never locks,
//    allocates, or touches Qt.
//  - Everything else runs on the (JUCE) message thread, which Main.cpp pumps
//    from the Qt event loop, so device changes are safe.

class AudioEngine : public juce::AudioAppComponent,
                    private juce::ChangeListener
{
public:
    AudioEngine();
    ~AudioEngine() override;

    // juce::AudioAppComponent
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    // Meter data (lock-free, read by the UI thread). All values in dBFS.
    float getInputLevelL() const  { return meters_.input.left.loadPeak(); }
    float getInputLevelR() const  { return meters_.input.right.loadPeak(); }
    float getOutputLevelL() const { return meters_.output.left.loadPeak(); }
    float getOutputLevelR() const { return meters_.output.right.loadPeak(); }
    float getInputRmsL() const    { return meters_.input.left.loadRms(); }
    float getInputRmsR() const    { return meters_.input.right.loadRms(); }
    float getOutputRmsL() const   { return meters_.output.left.loadRms(); }
    float getOutputRmsR() const   { return meters_.output.right.loadRms(); }

    double getCurrentSampleRate() const { return meters_.sampleRate.load(std::memory_order_acquire); }
    int    getCurrentBufferSize() const { return meters_.bufferSize.load(std::memory_order_acquire); }
    float  getMeasuredLatency() const   { return meters_.latencyMs.load(std::memory_order_acquire); }
    bool   isRunning() const            { return meters_.running.load(std::memory_order_acquire); }

    // Processing parameters (any thread; the audio thread reads atomics).
    // Gain = input trim before processing, Volume = output level. Both in dB.
    void setGainDb(float db)   { gainDb_.store(db, std::memory_order_release); }
    void setVolumeDb(float db) { volumeDb_.store(db, std::memory_order_release); }
    float getGainDb() const    { return gainDb_.load(std::memory_order_acquire); }
    float getVolumeDb() const  { return volumeDb_.load(std::memory_order_acquire); }

    // Control (message/UI thread only).
    bool start();                                   // returns false on device error
    void stop();
    bool setAudioDevice(const std::string& deviceName);
    bool setBufferSize(int numSamples);

    // Device enumeration (message/UI thread only).
    std::vector<std::string> getAvailableDeviceNames() const;
    std::string getCurrentDeviceName() const;

    // Set when the OS device list changes (hot-plug); UI polls and clears it.
    bool consumeDeviceListChanged() { return deviceListChanged_.exchange(false); }

    // Last device/engine error, empty if none. Message/UI thread only.
    std::string getLastError() const;

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void refreshLatencyFromDevice();
    void setLastError(const std::string& message);

    ampsim::MeterExchange meters_;

    // Per-channel smoothed levels, audio thread only.
    float smoothedPeak_[2][2] { { ampsim::kMeterFloorDb, ampsim::kMeterFloorDb },
                                { ampsim::kMeterFloorDb, ampsim::kMeterFloorDb } };
    float smoothingCoeff_ = 0.0f;

    std::atomic<float> gainDb_   { 0.0f };
    std::atomic<float> volumeDb_ { 0.0f };

    std::atomic<bool> deviceListChanged_ { false };

    mutable std::mutex errorMutex_;      // never touched by the audio thread
    std::string lastError_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
