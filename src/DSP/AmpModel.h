#pragma once

#include <atomic>
#include <cmath>

#include "Biquad.h"
#include "Shared/Constants.h"
#include "Shared/MeterProcessor.h"

// "Valve One" — a switchable amp model. Signal chain per channel:
//
//     drive gain -> tube-style saturation (peak-normalized)
//                -> tone stack (bass shelf / mid peak / treble shelf,
//                               corner frequencies set by the voicing)
//
// The saturation stage is peak-normalized: a full-scale (+-1) input maps
// back to a full-scale output at any Drive setting, so loud passages don't
// jump in level as Drive is turned up. Quieter signals still pick up
// coloration/compression from the tanh curve even at minimum Drive — that
// is the intended "always-on tube" character, not a transparent bypass
// (use the ACTIVE/BYPASS switch for a true bypass).
//
// Voicing gives each amp brand a distinct character along two axes: the
// tone stack's corner frequencies (where the classic EQ "scoop"/"boost"
// sits) and the saturation curve's asymmetry/pre-gain (how hard and how
// "even-harmonic-rich" it breaks up). It is not a circuit-accurate model of
// any real amplifier — just a deliberately different-sounding preset.
//
// Parameters are atomics: the UI thread writes, the audio thread reads at
// block boundaries. Coefficients are recomputed on the audio thread only
// when a parameter actually changed (cheap, allocation-free).

namespace ampsim
{
    enum class AmpVoicing { Fender, Marshall, Mesa, Orange };
    constexpr int kNumAmpVoicings = 4;

    struct AmpVoicingSpec
    {
        float bassFreq;
        float midFreq;
        float midQ;
        float trebleFreq;
        float asymmetry;   // bias in the saturation curve; 0 = symmetric
        float preGain;     // internal multiplier on Drive (amp "hotness")
    };

    // Fender: bright and scooped, soft symmetric breakup, extended treble.
    // Marshall: midrange-forward British crunch, tighter low end.
    // Mesa: high-gain and tight, asymmetric clipping for aggressive harmonics.
    // Orange: thick, warm low-mid, smoother compression, darker top end.
    inline constexpr AmpVoicingSpec kAmpVoicingSpecs[kNumAmpVoicings] = {
        /* Fender   */ { 100.0f, 500.0f, 0.9f, 4000.0f, 0.00f, 0.85f },
        /* Marshall */ { 110.0f, 800.0f, 0.8f, 3000.0f, 0.00f, 1.30f },
        /* Mesa     */ {  80.0f, 650.0f, 0.7f, 3500.0f, 0.35f, 1.60f },
        /* Orange   */ { 150.0f, 750.0f, 0.6f, 2600.0f, 0.15f, 1.15f },
    };

    class AmpModel
    {
    public:
        // UI thread (or tests).
        void setEnabled(bool on)         { enabled_.store(on, std::memory_order_release); }
        void setDriveDb(float db)        { driveDb_.store(db, std::memory_order_release); }
        void setBassDb(float db)         { bassDb_.store(db, std::memory_order_release); }
        void setMidDb(float db)          { midDb_.store(db, std::memory_order_release); }
        void setTrebleDb(float db)       { trebleDb_.store(db, std::memory_order_release); }
        void setVoicing(AmpVoicing v)    { voicing_.store(v, std::memory_order_release); }

        bool       isEnabled() const     { return enabled_.load(std::memory_order_acquire); }
        float      getDriveDb() const    { return driveDb_.load(std::memory_order_acquire); }
        AmpVoicing getVoicing() const    { return voicing_.load(std::memory_order_acquire); }

        // Audio thread.
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            for (auto& channel : states_)
                for (auto& filter : channel)
                    filter.reset();
            lastBass_ = lastMid_ = lastTreble_ = -1000.0f;   // force recompute
            lastVoicing_ = static_cast<AmpVoicing>(-1);      // force recompute
        }

        void process(float* const* channels, int numChannels, int numSamples) noexcept
        {
            if (!enabled_.load(std::memory_order_acquire) || sampleRate_ <= 0.0)
                return;

            updateCoefficients();

            const auto& spec = kAmpVoicingSpecs[static_cast<int>(voicing_.load(std::memory_order_acquire))];
            const float drive = dbToGain(driveDb_.load(std::memory_order_acquire)) * spec.preGain;
            const float k = spec.asymmetry;
            const float peak = std::tanh(std::max(drive, 1.0e-4f) + k) - std::tanh(k);
            const float makeup = 1.0f / std::max(peak, 1.0e-4f);

            const int maxCh = numChannels < kNumChannels ? numChannels : kNumChannels;
            for (int ch = 0; ch < maxCh; ++ch)
            {
                float* data = channels[ch];
                auto& [bass, mid, treble] = states_[ch];

                for (int i = 0; i < numSamples; ++i)
                {
                    // Tube-style saturation, level-compensated so drive at
                    // minimum stays transparent. A nonzero `k` biases the
                    // curve asymmetrically (more even-harmonic, "amp-like"
                    // breakup) instead of a plain symmetric tanh.
                    float x = (std::tanh(drive * data[i] + k) - std::tanh(k)) * makeup;

                    // Tone stack.
                    x = bass.process(x, bassCoeffs_);
                    x = mid.process(x, midCoeffs_);
                    x = treble.process(x, trebleCoeffs_);

                    data[i] = x;
                }
            }
        }

        // Default (Fender) voicing frequencies, kept for tests exercising
        // generic tone-stack behavior against a realistic frequency.
        static constexpr float kBassFreq   = kAmpVoicingSpecs[0].bassFreq;
        static constexpr float kMidFreq    = kAmpVoicingSpecs[0].midFreq;
        static constexpr float kMidQ       = kAmpVoicingSpecs[0].midQ;
        static constexpr float kTrebleFreq = kAmpVoicingSpecs[0].trebleFreq;

    private:
        void updateCoefficients() noexcept
        {
            const AmpVoicing voicing = voicing_.load(std::memory_order_acquire);
            const float bass = bassDb_.load(std::memory_order_acquire);
            const float mid = midDb_.load(std::memory_order_acquire);
            const float treble = trebleDb_.load(std::memory_order_acquire);
            const bool voicingChanged = voicing != lastVoicing_;
            const auto& spec = kAmpVoicingSpecs[static_cast<int>(voicing)];

            if (voicingChanged || bass != lastBass_)
            {
                bassCoeffs_ = makeLowShelf(sampleRate_, spec.bassFreq, bass);
                lastBass_ = bass;
            }
            if (voicingChanged || mid != lastMid_)
            {
                midCoeffs_ = makePeak(sampleRate_, spec.midFreq, mid, spec.midQ);
                lastMid_ = mid;
            }
            if (voicingChanged || treble != lastTreble_)
            {
                trebleCoeffs_ = makeHighShelf(sampleRate_, spec.trebleFreq, treble);
                lastTreble_ = treble;
            }
            lastVoicing_ = voicing;
        }

        std::atomic<bool>       enabled_  { true };
        std::atomic<float>      driveDb_  { 12.0f };
        std::atomic<float>      bassDb_   { 0.0f };
        std::atomic<float>      midDb_    { 0.0f };
        std::atomic<float>      trebleDb_ { 0.0f };
        std::atomic<AmpVoicing> voicing_  { AmpVoicing::Fender };

        double sampleRate_ = 0.0;
        BiquadCoeffs bassCoeffs_, midCoeffs_, trebleCoeffs_;
        BiquadState states_[kNumChannels][3];
        float lastBass_ = -1000.0f, lastMid_ = -1000.0f, lastTreble_ = -1000.0f;
        AmpVoicing lastVoicing_ = AmpVoicing::Fender;
    };
}
