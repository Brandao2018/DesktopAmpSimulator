#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>

#include "Shared/Constants.h"

// A classic 4-stage analog-style phaser (MXR Phase 90 territory): a chain
// of first-order allpass filters whose corner frequency is swept by a
// shared sine LFO, with feedback around the chain for resonance and a
// fixed 50/50 dry/wet mix. Single knob (Rate), like the pedal it emulates.

namespace ampsim
{
    struct AllpassState
    {
        float xz1 = 0.0f;
        float yz1 = 0.0f;

        void reset() noexcept { xz1 = yz1 = 0.0f; }

        float process(float x, float a) noexcept
        {
            const float y = a * x + xz1 - a * yz1;
            xz1 = x;
            yz1 = y;
            return y;
        }
    };

    class Phaser
    {
    public:
        // UI thread (or tests).
        void setEnabled(bool on)   { enabled_.store(on, std::memory_order_release); }
        void setRateHz(float hz)   { rateHz_.store(hz, std::memory_order_release); }

        bool  isEnabled() const    { return enabled_.load(std::memory_order_acquire); }
        float getRateHz() const    { return rateHz_.load(std::memory_order_acquire); }

        // Audio thread.
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            for (auto& channel : states_)
                for (auto& s : channel)
                    s.reset();
            for (auto& fb : lastOutput_)
                fb = 0.0f;
            lfoPhase_ = 0.0f;
        }

        void process(float* const* channels, int numChannels, int numSamples) noexcept
        {
            if (!enabled_.load(std::memory_order_acquire) || sampleRate_ <= 0.0)
                return;

            const float rateHz = std::clamp(rateHz_.load(std::memory_order_acquire), 0.02f, 10.0f);
            const float lfoInc = kTwoPi * rateHz / static_cast<float>(sampleRate_);
            const float startPhase = lfoPhase_;

            const int maxCh = numChannels < kNumChannels ? numChannels : kNumChannels;
            for (int ch = 0; ch < maxCh; ++ch)
            {
                float phase = startPhase;
                float* data = channels[ch];
                auto& stages = states_[ch];
                float feedbackState = lastOutput_[ch];

                for (int i = 0; i < numSamples; ++i)
                {
                    const float lfo = 0.5f * (1.0f + std::sin(phase));
                    const float fc = kMinFreq + lfo * (kMaxFreq - kMinFreq);
                    const float tanW = std::tan(3.14159265f * fc / static_cast<float>(sampleRate_));
                    const float a = (tanW - 1.0f) / (tanW + 1.0f);

                    float wet = data[i] + kFeedback * feedbackState;
                    for (auto& s : stages)
                        wet = s.process(wet, a);
                    feedbackState = wet;

                    data[i] = data[i] * (1.0f - kMix) + wet * kMix;

                    phase += lfoInc;
                    if (phase > kTwoPi)
                        phase -= kTwoPi;
                }

                lastOutput_[ch] = feedbackState;
            }

            lfoPhase_ = std::fmod(startPhase + static_cast<float>(numSamples) * lfoInc, kTwoPi);
        }

    private:
        static constexpr int   kNumStages = 4;
        static constexpr float kMinFreq   = 200.0f;
        static constexpr float kMaxFreq   = 1800.0f;
        static constexpr float kFeedback  = 0.45f;
        static constexpr float kMix       = 0.5f;
        static constexpr float kTwoPi     = 6.28318530718f;

        std::atomic<bool>  enabled_ { true };
        std::atomic<float> rateHz_  { 0.8f };

        double sampleRate_ = 0.0;
        float  lfoPhase_   = 0.0f;
        AllpassState states_[kNumChannels][kNumStages];
        float lastOutput_[kNumChannels] = { 0.0f, 0.0f };
    };
}
