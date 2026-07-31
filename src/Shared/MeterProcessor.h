#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "Constants.h"

// Pure, framework-free DSP helpers used by AudioEngine on the real-time
// thread and exercised directly by the unit tests. Everything here must be
// allocation-free and safe to call from a real-time audio callback.

namespace ampsim
{
    // Convert a linear gain value to dBFS, clamped at the meter floor.
    inline float gainToDb(float gain) noexcept
    {
        if (gain <= 0.0f)
            return kMeterFloorDb;

        const float db = 20.0f * std::log10(gain);
        return std::max(db, kMeterFloorDb);
    }

    // Peak absolute sample value of one channel.
    inline float channelPeak(const float* samples, size_t numSamples) noexcept
    {
        float peak = 0.0f;
        for (size_t i = 0; i < numSamples; ++i)
            peak = std::max(peak, std::abs(samples[i]));
        return peak;
    }

    // Root-mean-square of one channel.
    inline float channelRms(const float* samples, size_t numSamples) noexcept
    {
        if (numSamples == 0)
            return 0.0f;

        float sum = 0.0f;
        for (size_t i = 0; i < numSamples; ++i)
            sum += samples[i] * samples[i];
        return std::sqrt(sum / static_cast<float>(numSamples));
    }

    // One-pole smoother used for meter ballistics. Returns the coefficient
    // for a given smoothing time at a given block rate.
    inline float smoothingCoefficient(float smoothingMs, double sampleRate, int blockSize) noexcept
    {
        if (sampleRate <= 0.0 || blockSize <= 0 || smoothingMs <= 0.0f)
            return 0.0f;

        const float blocksPerSecond = static_cast<float>(sampleRate) / static_cast<float>(blockSize);
        const float samplesEquivalent = smoothingMs * 0.001f * blocksPerSecond;
        return std::exp(-1.0f / std::max(samplesEquivalent, 1.0e-6f));
    }

    // Smoothed level update: fast attack (jump up instantly), smooth release.
    inline float updateSmoothedLevel(float previous, float incoming, float coeff) noexcept
    {
        if (incoming >= previous)
            return incoming;
        return incoming + coeff * (previous - incoming);
    }

    // Phase 1 "processing": pass audio through unchanged. Exists as an
    // explicit function so the pipeline shape is already in place for the
    // amp models of Phase 2, and so tests can verify transparency.
    inline void processPassThrough(float* const* channels, int numChannels, int numSamples) noexcept
    {
        // Intentionally a no-op on the sample data.
        (void) channels;
        (void) numChannels;
        (void) numSamples;
    }
}
