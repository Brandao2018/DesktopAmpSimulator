// Tests for the "Valve One" amp model: saturation, tone stack, and bypass.
// Runs headless — no audio device, Qt, or JUCE required.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <array>
#include <cmath>
#include <vector>

#include "DSP/AmpModel.h"
#include "DSP/Biquad.h"

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kNumSamples = 512;

    std::vector<float> makeSine(float frequency, double sampleRate, int numSamples, float amplitude = 1.0f)
    {
        std::vector<float> samples(static_cast<size_t>(numSamples));
        for (int i = 0; i < numSamples; ++i)
        {
            const float phase = 2.0f * 3.14159265f * frequency * static_cast<float>(i)
                              / static_cast<float>(sampleRate);
            samples[static_cast<size_t>(i)] = amplitude * std::sin(phase);
        }
        return samples;
    }

    float peakAbs(const std::vector<float>& samples)
    {
        return ampsim::channelPeak(samples.data(), samples.size());
    }
}

TEST_CASE("AmpModel bypass (disabled) leaves audio unchanged", "[amp]")
{
    ampsim::AmpModel amp;
    amp.prepare(kSampleRate);
    amp.setEnabled(false);
    amp.setDriveDb(24.0f);   // would clearly alter the signal if active

    auto samples = makeSine(220.0f, kSampleRate, kNumSamples, 0.8f);
    const auto original = samples;

    std::array<float*, 1> channels { samples.data() };
    amp.process(channels.data(), 1, kNumSamples);

    for (size_t i = 0; i < samples.size(); ++i)
        REQUIRE(samples[i] == Catch::Approx(original[i]).margin(1.0e-6f));
}

TEST_CASE("AmpModel saturation is peak-normalized at any drive setting", "[amp]")
{
    // A full-scale input's peak should map back to ~full scale regardless
    // of Drive — that's what the makeup gain guarantees (loud passages
    // don't jump in level as Drive is turned up).
    for (float driveDb : { 0.0f, 12.0f, 24.0f, 36.0f })
    {
        ampsim::AmpModel amp;
        amp.prepare(kSampleRate);
        amp.setEnabled(true);
        amp.setDriveDb(driveDb);
        amp.setBassDb(0.0f);
        amp.setMidDb(0.0f);
        amp.setTrebleDb(0.0f);

        auto samples = makeSine(1000.0f, kSampleRate, kNumSamples, 1.0f);
        std::array<float*, 1> channels { samples.data() };
        amp.process(channels.data(), 1, kNumSamples);

        // Skip the tone stack's short settling transient at the start.
        std::vector<float> settled(samples.begin() + 64, samples.end());
        REQUIRE(peakAbs(settled) == Catch::Approx(1.0f).margin(0.03f));
    }
}

TEST_CASE("AmpModel saturation compresses peaks as drive increases", "[amp]")
{
    ampsim::AmpModel amp;
    amp.prepare(kSampleRate);
    amp.setEnabled(true);
    amp.setBassDb(0.0f);
    amp.setMidDb(0.0f);
    amp.setTrebleDb(0.0f);

    auto lowDrive = makeSine(220.0f, kSampleRate, kNumSamples, 0.9f);
    auto highDrive = lowDrive;

    amp.setDriveDb(0.0f);
    std::array<float*, 1> lowChannels { lowDrive.data() };
    amp.process(lowChannels.data(), 1, kNumSamples);

    ampsim::AmpModel amp2;
    amp2.prepare(kSampleRate);
    amp2.setEnabled(true);
    amp2.setBassDb(0.0f);
    amp2.setMidDb(0.0f);
    amp2.setTrebleDb(0.0f);
    amp2.setDriveDb(24.0f);
    std::array<float*, 1> highChannels { highDrive.data() };
    amp2.process(highChannels.data(), 1, kNumSamples);

    // tanh() saturation flattens the waveform — RMS/peak ratio rises
    // (the signal spends more time near its peak) as drive increases.
    const float lowPeak = peakAbs(lowDrive);
    const float highPeak = peakAbs(highDrive);
    const float lowRms = ampsim::channelRms(lowDrive.data(), lowDrive.size());
    const float highRms = ampsim::channelRms(highDrive.data(), highDrive.size());

    REQUIRE(highRms / highPeak > lowRms / lowPeak);
}

TEST_CASE("Bass shelf boosts low frequencies more than high frequencies", "[amp][biquad]")
{
    const auto coeffs = ampsim::makeLowShelf(kSampleRate, ampsim::AmpModel::kBassFreq, 12.0f);

    ampsim::BiquadState lowState, highState;
    auto low = makeSine(60.0f, kSampleRate, kNumSamples, 0.5f);
    auto high = makeSine(8000.0f, kSampleRate, kNumSamples, 0.5f);

    for (auto& s : low)  s = lowState.process(s, coeffs);
    for (auto& s : high) s = highState.process(s, coeffs);

    // Settle past the filter's transient, then compare gain via peak ratio.
    const float lowGain = peakAbs(std::vector<float>(low.begin() + 100, low.end())) / 0.5f;
    const float highGain = peakAbs(std::vector<float>(high.begin() + 100, high.end())) / 0.5f;

    REQUIRE(lowGain > highGain);
}

TEST_CASE("Treble shelf boosts high frequencies more than low frequencies", "[amp][biquad]")
{
    const auto coeffs = ampsim::makeHighShelf(kSampleRate, ampsim::AmpModel::kTrebleFreq, 12.0f);

    ampsim::BiquadState lowState, highState;
    auto low = makeSine(80.0f, kSampleRate, kNumSamples, 0.5f);
    auto high = makeSine(10000.0f, kSampleRate, kNumSamples, 0.5f);

    for (auto& s : low)  s = lowState.process(s, coeffs);
    for (auto& s : high) s = highState.process(s, coeffs);

    const float lowGain = peakAbs(std::vector<float>(low.begin() + 100, low.end())) / 0.5f;
    const float highGain = peakAbs(std::vector<float>(high.begin() + 100, high.end())) / 0.5f;

    REQUIRE(highGain > lowGain);
}

TEST_CASE("Peaking mid EQ at 0 dB gain is near-transparent", "[amp][biquad]")
{
    const auto coeffs = ampsim::makePeak(kSampleRate, ampsim::AmpModel::kMidFreq, 0.0f, ampsim::AmpModel::kMidQ);

    ampsim::BiquadState state;
    auto samples = makeSine(ampsim::AmpModel::kMidFreq, kSampleRate, kNumSamples, 0.5f);
    const auto original = samples;

    for (auto& s : samples)
        s = state.process(s, coeffs);

    for (size_t i = 100; i < samples.size(); ++i)
        REQUIRE(samples[i] == Catch::Approx(original[i]).margin(0.01f));
}

//==============================================================================
// Amp voicings (Fender / Marshall / Mesa / Orange)
//==============================================================================

TEST_CASE("AmpModel defaults to the Fender voicing", "[amp][voicing]")
{
    ampsim::AmpModel amp;
    REQUIRE(amp.getVoicing() == ampsim::AmpVoicing::Fender);
}

TEST_CASE("Different voicings use different tone-stack corner frequencies", "[amp][voicing]")
{
    // Same knob settings, different voicing: since each voicing's tone
    // stack sits at a different corner frequency, the measured response at
    // a fixed test frequency should differ between voicings.
    auto processedPeak = [](ampsim::AmpVoicing voicing, float freq)
    {
        ampsim::AmpModel amp;
        amp.prepare(kSampleRate);
        amp.setVoicing(voicing);
        amp.setDriveDb(0.0f);
        amp.setBassDb(0.0f);
        amp.setMidDb(9.0f);
        amp.setTrebleDb(0.0f);

        auto samples = makeSine(freq, kSampleRate, kNumSamples, 0.4f);
        std::array<float*, 1> channels { samples.data() };
        amp.process(channels.data(), 1, kNumSamples);

        std::vector<float> settled(samples.begin() + 100, samples.end());
        return peakAbs(settled);
    };

    // Fender's mid peak sits at 500 Hz, Marshall's at 800 Hz — a +9 dB mid
    // boost should raise 500 Hz noticeably more under Fender's voicing.
    const float fenderAt500 = processedPeak(ampsim::AmpVoicing::Fender, 500.0f);
    const float marshallAt500 = processedPeak(ampsim::AmpVoicing::Marshall, 500.0f);

    REQUIRE(fenderAt500 > marshallAt500);
}

TEST_CASE("setVoicing takes effect on the next process() call", "[amp][voicing]")
{
    ampsim::AmpModel amp;
    amp.prepare(kSampleRate);
    amp.setDriveDb(0.0f);
    amp.setBassDb(0.0f);
    amp.setTrebleDb(0.0f);
    amp.setMidDb(9.0f);

    auto renderAt500Hz = [&]()
    {
        auto samples = makeSine(500.0f, kSampleRate, kNumSamples, 0.4f);
        std::array<float*, 1> channels { samples.data() };
        amp.process(channels.data(), 1, kNumSamples);
        std::vector<float> settled(samples.begin() + 100, samples.end());
        return peakAbs(settled);
    };

    amp.setVoicing(ampsim::AmpVoicing::Fender);
    const float fenderPeak = renderAt500Hz();

    amp.setVoicing(ampsim::AmpVoicing::Marshall);
    const float marshallPeak = renderAt500Hz();

    REQUIRE(fenderPeak != Catch::Approx(marshallPeak).margin(0.01f));
}

TEST_CASE("Mesa's asymmetric saturation breaks positive/negative peak symmetry", "[amp][voicing]")
{
    // Fender's curve (asymmetry = 0) is an odd function of the input, so a
    // symmetric sine stays symmetric. Mesa's biased curve should not.
    auto peakAsymmetry = [](ampsim::AmpVoicing voicing)
    {
        ampsim::AmpModel amp;
        amp.prepare(kSampleRate);
        amp.setVoicing(voicing);
        amp.setDriveDb(24.0f);
        amp.setBassDb(0.0f);
        amp.setMidDb(0.0f);
        amp.setTrebleDb(0.0f);

        auto samples = makeSine(220.0f, kSampleRate, kNumSamples, 0.8f);
        std::array<float*, 1> channels { samples.data() };
        amp.process(channels.data(), 1, kNumSamples);

        float posPeak = 0.0f, negPeak = 0.0f;
        for (size_t i = 100; i < samples.size(); ++i)
        {
            posPeak = std::max(posPeak, samples[i]);
            negPeak = std::max(negPeak, -samples[i]);
        }
        return std::abs(posPeak - negPeak);
    };

    REQUIRE(peakAsymmetry(ampsim::AmpVoicing::Fender) < 0.02f);
    REQUIRE(peakAsymmetry(ampsim::AmpVoicing::Mesa) > 0.02f);
}
