#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <vector>

#include "DelayLine.h"
#include "Shared/Constants.h"

// A CE-2-style chorus: a short delay line (a few milliseconds) modulated by
// a sine LFO, mixed 50/50 with the dry signal. Two knobs (Rate, Depth), no
// mix control, matching the pedal it emulates.

namespace ampsim
{
    class Chorus
    {
    public:
        // UI thread (or tests).
        void setEnabled(bool on)     { enabled_.store(on, std::memory_order_release); }
        void setRateHz(float hz)     { rateHz_.store(hz, std::memory_order_release); }
        void setDepthPct(float pct)  { depthPct_.store(pct, std::memory_order_release); }

        bool  isEnabled() const      { return enabled_.load(std::memory_order_acquire); }

        // Audio thread.
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            const float maxDelayMs = kBaseDelayMs + kDepthMs;
            bufferSize_ = static_cast<size_t>(maxDelayMs * 0.001 * sampleRate) + 8;

            for (auto& buf : buffer_)
                buf.assign(bufferSize_, 0.0f);

            writePos_ = 0;
            lfoPhase_ = 0.0f;
        }

        void process(float* const* channels, int numChannels, int numSamples) noexcept
        {
            if (!enabled_.load(std::memory_order_acquire) || sampleRate_ <= 0.0)
                return;

            const float rateHz = std::clamp(rateHz_.load(std::memory_order_acquire), 0.02f, 8.0f);
            const float depth = std::clamp(depthPct_.load(std::memory_order_acquire), 0.0f, 100.0f) / 100.0f;
            const float lfoInc = kTwoPi * rateHz / static_cast<float>(sampleRate_);
            const float startPhase = lfoPhase_;

            const int maxCh = numChannels < kNumChannels ? numChannels : kNumChannels;
            for (int ch = 0; ch < maxCh; ++ch)
            {
                auto& buf = buffer_[ch];
                float* data = channels[ch];
                float phase = startPhase;

                for (int i = 0; i < numSamples; ++i)
                {
                    const size_t wp = (writePos_ + static_cast<size_t>(i)) % bufferSize_;
                    buf[wp] = data[i];

                    const float lfo = 0.5f * (1.0f + std::sin(phase));
                    const float delayMs = kBaseDelayMs + depth * kDepthMs * lfo;
                    const float delaySamples = delayMs * 0.001f * static_cast<float>(sampleRate_);
                    const float wet = readInterpolated(buf.data(), bufferSize_,
                                                        static_cast<float>(wp) - delaySamples);

                    data[i] = data[i] * 0.5f + wet * 0.5f;

                    phase += lfoInc;
                    if (phase > kTwoPi)
                        phase -= kTwoPi;
                }
            }

            lfoPhase_ = std::fmod(startPhase + static_cast<float>(numSamples) * lfoInc, kTwoPi);
            writePos_ = (writePos_ + static_cast<size_t>(numSamples)) % bufferSize_;
        }

    private:
        static constexpr float kBaseDelayMs = 7.0f;
        static constexpr float kDepthMs     = 6.0f;
        static constexpr float kTwoPi       = 6.28318530718f;

        std::atomic<bool>  enabled_   { true };
        std::atomic<float> rateHz_    { 0.8f };
        std::atomic<float> depthPct_  { 50.0f };

        double sampleRate_ = 0.0;
        size_t bufferSize_ = 0;
        size_t writePos_   = 0;
        float  lfoPhase_   = 0.0f;
        std::vector<float> buffer_[kNumChannels];
    };
}
