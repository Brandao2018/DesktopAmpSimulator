#pragma once

#include <atomic>
#include <cmath>
#include <vector>

#include "DelayLine.h"
#include "Shared/Constants.h"

// "Whammy" — a dive-bomb/octave pitch shifter in the Tom Morello mould
// (DigiTech Whammy style: a knob sets a fixed semitone shift rather than an
// expression pedal sweep). Classic dual-tap delay-line pitch shifter:
//
//   - Two read taps into a short circular buffer, each advancing at
//     `ratio` samples per sample instead of 1 (ratio = 2^(semitones/12)).
//   - Each tap's delay-into-the-past is `phase * grain`, where `phase`
//     cycles 0..1; the two taps are kept 0.5 apart in phase.
//   - A triangular window (zero at phase 0/1, peak at phase 0.5) crossfades
//     each tap in/out right as it would otherwise jump discontinuously.
//     Because the two windows are a fixed 0.5 apart, w(p) + w(p+0.5) == 1
//     for any phase, so the crossfade is always unity-gain.
//
// At 0 semitones the class takes a fast path and passes audio through
// untouched (still keeping the delay buffer warm), rather than relying on
// the crossfade identity to be exactly transparent.

namespace ampsim
{
    class PitchShifter
    {
    public:
        // UI thread (or tests).
        void setEnabled(bool on)     { enabled_.store(on, std::memory_order_release); }
        void setSemitones(float st)  { semitones_.store(st, std::memory_order_release); }

        bool  isEnabled() const      { return enabled_.load(std::memory_order_acquire); }
        float getSemitones() const   { return semitones_.load(std::memory_order_acquire); }

        // Audio thread.
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            grainSamples_ = static_cast<int>(kGrainSeconds * sampleRate);
            if (grainSamples_ < 8)
                grainSamples_ = 8;
            bufferSize_ = static_cast<size_t>(grainSamples_) + 8;

            for (auto& buf : buffer_)
                buf.assign(bufferSize_, 0.0f);

            writePos_ = 0;
            for (auto& p : phase_)
            {
                p[0] = 0.0f;
                p[1] = 0.5f;
            }
        }

        void process(float* const* channels, int numChannels, int numSamples) noexcept
        {
            if (!enabled_.load(std::memory_order_acquire) || sampleRate_ <= 0.0)
                return;

            const float semitones = semitones_.load(std::memory_order_acquire);
            const bool bypassPitch = std::abs(semitones) < 0.01f;
            const float ratio = std::pow(2.0f, semitones / 12.0f);
            const float phaseInc = bypassPitch ? 0.0f : (ratio - 1.0f) / static_cast<float>(grainSamples_);

            const int maxCh = numChannels < kNumChannels ? numChannels : kNumChannels;
            for (int ch = 0; ch < maxCh; ++ch)
            {
                auto& buf = buffer_[ch];
                float* data = channels[ch];
                float phaseA = phase_[ch][0];
                float phaseB = phase_[ch][1];

                for (int i = 0; i < numSamples; ++i)
                {
                    const size_t wp = (writePos_ + static_cast<size_t>(i)) % bufferSize_;
                    buf[wp] = data[i];

                    if (!bypassPitch)
                    {
                        const float delayA = phaseA * static_cast<float>(grainSamples_);
                        const float delayB = phaseB * static_cast<float>(grainSamples_);
                        const float sampleA = readInterpolated(buf.data(), bufferSize_, static_cast<float>(wp) - delayA);
                        const float sampleB = readInterpolated(buf.data(), bufferSize_, static_cast<float>(wp) - delayB);
                        const float windowA = 1.0f - std::abs(2.0f * phaseA - 1.0f);
                        const float windowB = 1.0f - std::abs(2.0f * phaseB - 1.0f);

                        data[i] = sampleA * windowA + sampleB * windowB;

                        phaseA += phaseInc;
                        if (phaseA >= 1.0f) phaseA -= 1.0f;
                        else if (phaseA < 0.0f) phaseA += 1.0f;

                        phaseB += phaseInc;
                        if (phaseB >= 1.0f) phaseB -= 1.0f;
                        else if (phaseB < 0.0f) phaseB += 1.0f;
                    }
                }

                phase_[ch][0] = phaseA;
                phase_[ch][1] = phaseB;
            }

            writePos_ = (writePos_ + static_cast<size_t>(numSamples)) % bufferSize_;
        }

    private:
        static constexpr float kGrainSeconds = 0.02f;   // ~20 ms crossfade window

        std::atomic<bool>  enabled_   { true };
        std::atomic<float> semitones_ { 0.0f };

        double sampleRate_   = 0.0;
        int    grainSamples_ = 0;
        size_t bufferSize_   = 0;
        size_t writePos_     = 0;
        std::vector<float> buffer_[kNumChannels];
        float phase_[kNumChannels][2] = { { 0.0f, 0.5f }, { 0.0f, 0.5f } };
    };
}
