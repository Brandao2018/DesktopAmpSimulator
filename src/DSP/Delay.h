#pragma once

#include <algorithm>
#include <atomic>
#include <vector>

#include "DelayLine.h"
#include "Shared/Constants.h"

// A DD-3-style digital delay/echo: single feedback tap, Time/Feedback/Mix
// knobs. Feedback is clamped well under 1.0 so it always decays.

namespace ampsim
{
    class Delay
    {
    public:
        // UI thread (or tests).
        void setEnabled(bool on)       { enabled_.store(on, std::memory_order_release); }
        void setTimeMs(float ms)       { timeMs_.store(ms, std::memory_order_release); }
        void setFeedbackPct(float pct) { feedbackPct_.store(pct, std::memory_order_release); }
        void setMixPct(float pct)      { mixPct_.store(pct, std::memory_order_release); }

        bool isEnabled() const         { return enabled_.load(std::memory_order_acquire); }

        // Audio thread.
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            bufferSize_ = static_cast<size_t>(kMaxDelaySeconds * sampleRate) + 8;

            for (auto& buf : buffer_)
                buf.assign(bufferSize_, 0.0f);

            writePos_ = 0;
        }

        void process(float* const* channels, int numChannels, int numSamples) noexcept
        {
            if (!enabled_.load(std::memory_order_acquire) || sampleRate_ <= 0.0)
                return;

            const float timeMs = std::clamp(timeMs_.load(std::memory_order_acquire),
                                            kMinDelayMs, kMaxDelaySeconds * 1000.0f);
            const float feedback = std::clamp(feedbackPct_.load(std::memory_order_acquire), 0.0f, 90.0f) / 100.0f;
            const float mix = std::clamp(mixPct_.load(std::memory_order_acquire), 0.0f, 100.0f) / 100.0f;
            const float delaySamples = timeMs * 0.001f * static_cast<float>(sampleRate_);

            const int maxCh = numChannels < kNumChannels ? numChannels : kNumChannels;
            for (int ch = 0; ch < maxCh; ++ch)
            {
                auto& buf = buffer_[ch];
                float* data = channels[ch];

                for (int i = 0; i < numSamples; ++i)
                {
                    const size_t wp = (writePos_ + static_cast<size_t>(i)) % bufferSize_;
                    const float delayed = readInterpolated(buf.data(), bufferSize_,
                                                            static_cast<float>(wp) - delaySamples);
                    const float input = data[i];

                    buf[wp] = input + delayed * feedback;
                    data[i] = input * (1.0f - mix) + delayed * mix;
                }
            }

            writePos_ = (writePos_ + static_cast<size_t>(numSamples)) % bufferSize_;
        }

    private:
        static constexpr float kMinDelaySeconds = 0.05f;
        static constexpr float kMinDelayMs      = kMinDelaySeconds * 1000.0f;
        static constexpr float kMaxDelaySeconds = 0.9f;

        std::atomic<bool>  enabled_     { true };
        std::atomic<float> timeMs_      { 350.0f };
        std::atomic<float> feedbackPct_ { 35.0f };
        std::atomic<float> mixPct_      { 30.0f };

        double sampleRate_ = 0.0;
        size_t bufferSize_ = 0;
        size_t writePos_   = 0;
        std::vector<float> buffer_[kNumChannels];
    };
}
