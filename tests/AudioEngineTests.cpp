// Verifies the Phase 1 processing pipeline: audio must pass through
// completely unchanged. Tests the same processPassThrough() the engine's
// real-time callback uses, without needing an audio device.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <array>
#include <cmath>
#include <vector>

#include "Shared/MeterProcessor.h"

namespace
{
    constexpr int   kNumSamples = 256;
    constexpr float kPi = 3.14159265358979f;

    std::vector<float> makeSine(float frequency, double sampleRate, int numSamples)
    {
        std::vector<float> samples(static_cast<size_t>(numSamples));
        for (int i = 0; i < numSamples; ++i)
        {
            const float phase = 2.0f * kPi * frequency * static_cast<float>(i)
                              / static_cast<float>(sampleRate);
            samples[static_cast<size_t>(i)] = std::sin(phase);
        }
        return samples;
    }
}

TEST_CASE("Audio passes through unchanged", "[audio]")
{
    auto left  = makeSine(1000.0f, 48000.0, kNumSamples);
    auto right = makeSine(440.0f, 48000.0, kNumSamples);
    const auto leftOriginal = left;
    const auto rightOriginal = right;

    std::array<float*, 2> channels { left.data(), right.data() };
    ampsim::processPassThrough(channels.data(), 2, kNumSamples);

    for (int i = 0; i < kNumSamples; ++i)
    {
        const auto idx = static_cast<size_t>(i);
        REQUIRE(left[idx]  == Catch::Approx(leftOriginal[idx]).margin(0.001f));
        REQUIRE(right[idx] == Catch::Approx(rightOriginal[idx]).margin(0.001f));
    }
}

TEST_CASE("Pass-through preserves silence", "[audio]")
{
    std::vector<float> silence(kNumSamples, 0.0f);
    std::array<float*, 1> channels { silence.data() };

    ampsim::processPassThrough(channels.data(), 1, kNumSamples);

    for (float sample : silence)
        REQUIRE(sample == 0.0f);
}

TEST_CASE("applyGain scales samples by the expected linear factor", "[audio]")
{
    std::vector<float> samples(kNumSamples, 0.5f);
    std::array<float*, 1> channels { samples.data() };

    // +6.02 dB ≈ factor of 2.
    ampsim::applyGain(channels.data(), 1, kNumSamples, ampsim::dbToGain(6.0206f));

    for (float sample : samples)
        REQUIRE(sample == Catch::Approx(1.0f).margin(0.001f));
}

TEST_CASE("dbToGain and gainToDb are inverse operations", "[audio]")
{
    for (float db : { -24.0f, -6.0f, 0.0f, 6.0f, 24.0f })
        REQUIRE(ampsim::gainToDb(ampsim::dbToGain(db)) == Catch::Approx(db).margin(0.01f));
}

TEST_CASE("Full-scale sine has expected peak and RMS", "[audio]")
{
    // Use a whole number of cycles so RMS is exact.
    const auto sine = makeSine(1500.0f, 48000.0, kNumSamples);   // 8 cycles in 256 samples

    const float peak = ampsim::channelPeak(sine.data(), sine.size());
    const float rms  = ampsim::channelRms(sine.data(), sine.size());

    REQUIRE(peak == Catch::Approx(1.0f).margin(0.01f));
    REQUIRE(rms  == Catch::Approx(1.0f / std::sqrt(2.0f)).margin(0.01f));
}
