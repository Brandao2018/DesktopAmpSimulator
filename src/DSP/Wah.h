#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>

#include "Biquad.h"
#include "Shared/Constants.h"

// A manual (pedal-rock-position) wah: a resonant bandpass filter swept from
// a dark, bassy heel-down position to a bright, nasal toe-down position,
// exactly like a Cry Baby with the treadle set by hand instead of a foot.
// The whole signal passes through the filter in series (no dry/wet mix) —
// that is how a real wah pedal sits in the signal chain.

namespace ampsim
{
    class Wah
    {
    public:
        // UI thread (or tests).
        void setEnabled(bool on)        { enabled_.store(on, std::memory_order_release); }
        void setPositionPct(float pct)  { positionPct_.store(pct, std::memory_order_release); }

        bool  isEnabled() const         { return enabled_.load(std::memory_order_acquire); }
        float getPositionPct() const    { return positionPct_.load(std::memory_order_acquire); }

        // Audio thread.
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            for (auto& s : states_)
                s.reset();
            lastPosition_ = -1000.0f;   // force recompute
        }

        void process(float* const* channels, int numChannels, int numSamples) noexcept
        {
            if (!enabled_.load(std::memory_order_acquire) || sampleRate_ <= 0.0)
                return;

            updateCoeffs();

            const int maxCh = numChannels < kNumChannels ? numChannels : kNumChannels;
            for (int ch = 0; ch < maxCh; ++ch)
            {
                float* data = channels[ch];
                auto& state = states_[ch];
                for (int i = 0; i < numSamples; ++i)
                    data[i] = state.process(data[i], coeffs_);
            }
        }

        static constexpr float kMinFreq = 450.0f;
        static constexpr float kMaxFreq = 2200.0f;

    private:
        void updateCoeffs() noexcept
        {
            const float pos = std::clamp(positionPct_.load(std::memory_order_acquire), 0.0f, 100.0f);
            if (pos != lastPosition_)
            {
                const float t = pos / 100.0f;
                const float freq = kMinFreq * std::pow(kMaxFreq / kMinFreq, t);   // log sweep
                coeffs_ = makeBandpass(sampleRate_, freq, kQ);
                lastPosition_ = pos;
            }
        }

        static constexpr float kQ = 2.2f;

        std::atomic<bool>  enabled_     { true };
        std::atomic<float> positionPct_ { 50.0f };

        double sampleRate_ = 0.0;
        BiquadCoeffs coeffs_;
        BiquadState  states_[kNumChannels];
        float lastPosition_ = -1000.0f;
    };
}
