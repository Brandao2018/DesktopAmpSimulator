// Tests for the pedalboard effects added alongside "Valve One": Wah,
// Whammy (pitch shift), Phaser, Chorus, and Delay. Runs headless — no audio
// device, Qt, or JUCE required.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <array>
#include <cmath>
#include <vector>

#include "DSP/Chorus.h"
#include "DSP/Delay.h"
#include "DSP/Phaser.h"
#include "DSP/PitchShifter.h"
#include "DSP/Wah.h"
#include "Shared/MeterProcessor.h"

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kNumSamples = 1024;

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

    float rms(const std::vector<float>& samples)
    {
        return ampsim::channelRms(samples.data(), samples.size());
    }

    float maxAbsDiff(const std::vector<float>& a, const std::vector<float>& b)
    {
        float diff = 0.0f;
        for (size_t i = 0; i < a.size(); ++i)
            diff = std::max(diff, std::abs(a[i] - b[i]));
        return diff;
    }
}

//==============================================================================
// Wah
//==============================================================================

TEST_CASE("Wah bypass (disabled) leaves audio unchanged", "[wah]")
{
    ampsim::Wah wah;
    wah.prepare(kSampleRate);
    wah.setEnabled(false);
    wah.setPositionPct(80.0f);

    auto samples = makeSine(1000.0f, kSampleRate, kNumSamples, 0.7f);
    const auto original = samples;

    std::array<float*, 1> channels { samples.data() };
    wah.process(channels.data(), 1, kNumSamples);

    for (size_t i = 0; i < samples.size(); ++i)
        REQUIRE(samples[i] == Catch::Approx(original[i]).margin(1.0e-6f));
}

TEST_CASE("Wah heel-down favors low frequencies, toe-down favors high frequencies", "[wah]")
{
    auto gainAt = [](float positionPct, float freq)
    {
        ampsim::Wah wah;
        wah.prepare(kSampleRate);
        wah.setEnabled(true);
        wah.setPositionPct(positionPct);

        auto samples = makeSine(freq, kSampleRate, kNumSamples, 0.5f);
        std::array<float*, 1> channels { samples.data() };
        wah.process(channels.data(), 1, kNumSamples);

        std::vector<float> settled(samples.begin() + 200, samples.end());
        return peakAbs(settled) / 0.5f;
    };

    const float heelLowFreqGain   = gainAt(0.0f, 500.0f);
    const float heelHighFreqGain  = gainAt(0.0f, 3000.0f);
    const float toeLowFreqGain    = gainAt(100.0f, 500.0f);
    const float toeHighFreqGain   = gainAt(100.0f, 3000.0f);

    REQUIRE(heelLowFreqGain > heelHighFreqGain);
    REQUIRE(toeHighFreqGain > toeLowFreqGain);
}

//==============================================================================
// Whammy (PitchShifter)
//==============================================================================

TEST_CASE("Whammy bypass (disabled) leaves audio unchanged", "[whammy]")
{
    ampsim::PitchShifter whammy;
    whammy.prepare(kSampleRate);
    whammy.setEnabled(false);
    whammy.setSemitones(12.0f);

    auto samples = makeSine(220.0f, kSampleRate, kNumSamples, 0.8f);
    const auto original = samples;

    std::array<float*, 1> channels { samples.data() };
    whammy.process(channels.data(), 1, kNumSamples);

    for (size_t i = 0; i < samples.size(); ++i)
        REQUIRE(samples[i] == Catch::Approx(original[i]).margin(1.0e-6f));
}

TEST_CASE("Whammy at 0 semitones is transparent even when enabled", "[whammy]")
{
    ampsim::PitchShifter whammy;
    whammy.prepare(kSampleRate);
    whammy.setEnabled(true);
    whammy.setSemitones(0.0f);

    auto samples = makeSine(220.0f, kSampleRate, kNumSamples, 0.8f);
    const auto original = samples;

    std::array<float*, 1> channels { samples.data() };
    whammy.process(channels.data(), 1, kNumSamples);

    for (size_t i = 0; i < samples.size(); ++i)
        REQUIRE(samples[i] == Catch::Approx(original[i]).margin(1.0e-6f));
}

TEST_CASE("Whammy shifts pitch while roughly preserving energy", "[whammy]")
{
    // The two crossfading taps are two time-shifted copies of the same
    // resampled signal, so their relative phase sweeps a full circle every
    // grain cycle and they periodically interfere (louder here, quieter
    // there) — the characteristic "warble" of a simple two-tap pitch
    // shifter. A single short block can land in a destructive-interference
    // dip, so measure steady-state RMS over several full cycles instead.
    constexpr int kLongSamples = 16384;

    // Bulls on Parade-style octave-up dive (+12 st) and a -12 st dive down.
    for (float semitones : { -12.0f, 12.0f, 24.0f })
    {
        ampsim::PitchShifter whammy;
        whammy.prepare(kSampleRate);
        whammy.setEnabled(true);
        whammy.setSemitones(semitones);

        auto samples = makeSine(220.0f, kSampleRate, kLongSamples, 0.7f);
        const auto original = samples;

        std::array<float*, 1> channels { samples.data() };
        whammy.process(channels.data(), 1, kLongSamples);

        std::vector<float> settledOut(samples.begin() + 2000, samples.end());
        std::vector<float> settledIn(original.begin() + 2000, original.end());

        REQUIRE(rms(settledOut) > 0.2f * rms(settledIn));
        REQUIRE(rms(settledOut) < 2.0f * rms(settledIn));
        REQUIRE(maxAbsDiff(samples, original) > 0.05f);
    }
}

//==============================================================================
// Phaser
//==============================================================================

TEST_CASE("Phaser bypass (disabled) leaves audio unchanged", "[phaser]")
{
    ampsim::Phaser phaser;
    phaser.prepare(kSampleRate);
    phaser.setEnabled(false);
    phaser.setRateHz(2.0f);

    auto samples = makeSine(440.0f, kSampleRate, kNumSamples, 0.6f);
    const auto original = samples;

    std::array<float*, 1> channels { samples.data() };
    phaser.process(channels.data(), 1, kNumSamples);

    for (size_t i = 0; i < samples.size(); ++i)
        REQUIRE(samples[i] == Catch::Approx(original[i]).margin(1.0e-6f));
}

TEST_CASE("Phaser colors the signal while staying stable", "[phaser]")
{
    ampsim::Phaser phaser;
    phaser.prepare(kSampleRate);
    phaser.setEnabled(true);
    phaser.setRateHz(1.0f);

    auto samples = makeSine(440.0f, kSampleRate, kNumSamples, 0.6f);
    const auto original = samples;

    std::array<float*, 1> channels { samples.data() };
    phaser.process(channels.data(), 1, kNumSamples);

    REQUIRE(maxAbsDiff(samples, original) > 0.01f);
    REQUIRE(peakAbs(samples) < 2.0f * peakAbs(original));
}

//==============================================================================
// Chorus
//==============================================================================

TEST_CASE("Chorus bypass (disabled) leaves audio unchanged", "[chorus]")
{
    ampsim::Chorus chorus;
    chorus.prepare(kSampleRate);
    chorus.setEnabled(false);
    chorus.setRateHz(1.0f);
    chorus.setDepthPct(80.0f);

    auto samples = makeSine(440.0f, kSampleRate, kNumSamples, 0.6f);
    const auto original = samples;

    std::array<float*, 1> channels { samples.data() };
    chorus.process(channels.data(), 1, kNumSamples);

    for (size_t i = 0; i < samples.size(); ++i)
        REQUIRE(samples[i] == Catch::Approx(original[i]).margin(1.0e-6f));
}

TEST_CASE("Chorus modulates the signal while staying near unity level", "[chorus]")
{
    ampsim::Chorus chorus;
    chorus.prepare(kSampleRate);
    chorus.setEnabled(true);
    chorus.setRateHz(1.5f);
    chorus.setDepthPct(100.0f);

    auto samples = makeSine(440.0f, kSampleRate, kNumSamples, 0.6f);
    const auto original = samples;

    std::array<float*, 1> channels { samples.data() };
    chorus.process(channels.data(), 1, kNumSamples);

    REQUIRE(maxAbsDiff(samples, original) > 0.01f);
    REQUIRE(peakAbs(samples) < 1.5f * peakAbs(original));
}

//==============================================================================
// Delay
//==============================================================================

TEST_CASE("Delay bypass (disabled) leaves audio unchanged", "[delay]")
{
    ampsim::Delay delay;
    delay.prepare(kSampleRate);
    delay.setEnabled(false);
    delay.setTimeMs(300.0f);
    delay.setFeedbackPct(50.0f);
    delay.setMixPct(50.0f);

    auto samples = makeSine(440.0f, kSampleRate, kNumSamples, 0.6f);
    const auto original = samples;

    std::array<float*, 1> channels { samples.data() };
    delay.process(channels.data(), 1, kNumSamples);

    for (size_t i = 0; i < samples.size(); ++i)
        REQUIRE(samples[i] == Catch::Approx(original[i]).margin(1.0e-6f));
}

TEST_CASE("Delay produces an echo at the expected time offset", "[delay]")
{
    ampsim::Delay delay;
    delay.prepare(kSampleRate);
    delay.setEnabled(true);
    delay.setTimeMs(100.0f);     // exactly 4800 samples @ 48 kHz
    delay.setFeedbackPct(0.0f);     // isolate a single, clean echo
    delay.setMixPct(50.0f);

    const int delaySamples = 4800;
    std::vector<float> samples(static_cast<size_t>(delaySamples) + 200, 0.0f);
    samples[0] = 1.0f;   // impulse

    std::array<float*, 1> channels { samples.data() };
    delay.process(channels.data(), 1, static_cast<int>(samples.size()));

    REQUIRE(samples[0] == Catch::Approx(0.5f).margin(1.0e-3f));
    REQUIRE(samples[static_cast<size_t>(delaySamples)] == Catch::Approx(0.5f).margin(1.0e-3f));
}

TEST_CASE("Delay feedback produces repeating, decaying echoes", "[delay]")
{
    ampsim::Delay delay;
    delay.prepare(kSampleRate);
    delay.setEnabled(true);
    delay.setTimeMs(100.0f);     // 4800 samples
    delay.setFeedbackPct(50.0f);
    delay.setMixPct(50.0f);

    const int delaySamples = 4800;
    std::vector<float> samples(static_cast<size_t>(delaySamples) * 3 + 50, 0.0f);
    samples[0] = 1.0f;

    std::array<float*, 1> channels { samples.data() };
    delay.process(channels.data(), 1, static_cast<int>(samples.size()));

    const float firstEcho  = std::abs(samples[static_cast<size_t>(delaySamples)]);
    const float secondEcho = std::abs(samples[static_cast<size_t>(delaySamples) * 2]);

    REQUIRE(firstEcho > 0.01f);
    REQUIRE(secondEcho > 0.01f);
    REQUIRE(secondEcho < firstEcho);   // feedback decays, doesn't grow
}
