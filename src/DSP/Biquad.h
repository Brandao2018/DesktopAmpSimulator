#pragma once

#include <cmath>

// RBJ-cookbook biquad filter (transposed direct form II). Framework-free and
// allocation-free: safe to recompute coefficients and process on the
// real-time audio thread.

namespace ampsim
{
    struct BiquadCoeffs
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
    };

    struct BiquadState
    {
        float z1 = 0.0f;
        float z2 = 0.0f;

        void reset() noexcept { z1 = z2 = 0.0f; }

        float process(float x, const BiquadCoeffs& c) noexcept
        {
            const float y = c.b0 * x + z1;
            z1 = c.b1 * x - c.a1 * y + z2;
            z2 = c.b2 * x - c.a2 * y;
            return y;
        }
    };

    // Low shelf at `freq` with `gainDb` boost/cut (slope 1).
    inline BiquadCoeffs makeLowShelf(double sampleRate, float freq, float gainDb) noexcept
    {
        const float A = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * 3.14159265f * freq / static_cast<float>(sampleRate);
        const float cosW = std::cos(w0);
        const float sinW = std::sin(w0);
        const float alpha = sinW / 2.0f * std::sqrt(2.0f);
        const float sqrtA2a = 2.0f * std::sqrt(A) * alpha;

        const float a0 = (A + 1.0f) + (A - 1.0f) * cosW + sqrtA2a;
        BiquadCoeffs c;
        c.b0 = (A * ((A + 1.0f) - (A - 1.0f) * cosW + sqrtA2a)) / a0;
        c.b1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosW)) / a0;
        c.b2 = (A * ((A + 1.0f) - (A - 1.0f) * cosW - sqrtA2a)) / a0;
        c.a1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cosW)) / a0;
        c.a2 = ((A + 1.0f) + (A - 1.0f) * cosW - sqrtA2a) / a0;
        return c;
    }

    // High shelf at `freq` with `gainDb` boost/cut (slope 1).
    inline BiquadCoeffs makeHighShelf(double sampleRate, float freq, float gainDb) noexcept
    {
        const float A = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * 3.14159265f * freq / static_cast<float>(sampleRate);
        const float cosW = std::cos(w0);
        const float sinW = std::sin(w0);
        const float alpha = sinW / 2.0f * std::sqrt(2.0f);
        const float sqrtA2a = 2.0f * std::sqrt(A) * alpha;

        const float a0 = (A + 1.0f) - (A - 1.0f) * cosW + sqrtA2a;
        BiquadCoeffs c;
        c.b0 = (A * ((A + 1.0f) + (A - 1.0f) * cosW + sqrtA2a)) / a0;
        c.b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosW)) / a0;
        c.b2 = (A * ((A + 1.0f) + (A - 1.0f) * cosW - sqrtA2a)) / a0;
        c.a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cosW)) / a0;
        c.a2 = ((A + 1.0f) - (A - 1.0f) * cosW - sqrtA2a) / a0;
        return c;
    }

    // Peaking EQ at `freq` with `gainDb` boost/cut and quality `q`.
    inline BiquadCoeffs makePeak(double sampleRate, float freq, float gainDb, float q) noexcept
    {
        const float A = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * 3.14159265f * freq / static_cast<float>(sampleRate);
        const float cosW = std::cos(w0);
        const float alpha = std::sin(w0) / (2.0f * q);

        const float a0 = 1.0f + alpha / A;
        BiquadCoeffs c;
        c.b0 = (1.0f + alpha * A) / a0;
        c.b1 = (-2.0f * cosW) / a0;
        c.b2 = (1.0f - alpha * A) / a0;
        c.a1 = (-2.0f * cosW) / a0;
        c.a2 = (1.0f - alpha / A) / a0;
        return c;
    }

    // Constant-skirt-gain bandpass (peak gain = Q) at `freq` — the resonant
    // sweep filter behind a wah pedal.
    inline BiquadCoeffs makeBandpass(double sampleRate, float freq, float q) noexcept
    {
        const float w0 = 2.0f * 3.14159265f * freq / static_cast<float>(sampleRate);
        const float cosW = std::cos(w0);
        const float alpha = std::sin(w0) / (2.0f * q);

        const float a0 = 1.0f + alpha;
        BiquadCoeffs c;
        c.b0 = alpha / a0;
        c.b1 = 0.0f;
        c.b2 = -alpha / a0;
        c.a1 = (-2.0f * cosW) / a0;
        c.a2 = (1.0f - alpha) / a0;
        return c;
    }
}
