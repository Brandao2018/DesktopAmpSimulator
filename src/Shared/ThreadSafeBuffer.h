#pragma once

#include <atomic>

#include "Constants.h"

// Lock-free exchange of meter data between the real-time audio thread
// (writer) and the Qt UI thread (reader).
//
// The audio thread stores values with memory_order_release; the UI thread
// loads with memory_order_acquire. No locks, no allocation — safe for use
// inside the audio callback.

namespace ampsim
{
    struct ChannelMeter
    {
        std::atomic<float> peakDb { kMeterFloorDb };
        std::atomic<float> rmsDb  { kMeterFloorDb };

        void store(float peak, float rms) noexcept
        {
            peakDb.store(peak, std::memory_order_release);
            rmsDb.store(rms, std::memory_order_release);
        }

        float loadPeak() const noexcept { return peakDb.load(std::memory_order_acquire); }
        float loadRms()  const noexcept { return rmsDb.load(std::memory_order_acquire); }

        void reset() noexcept { store(kMeterFloorDb, kMeterFloorDb); }
    };

    struct StereoMeter
    {
        ChannelMeter left;
        ChannelMeter right;

        void reset() noexcept
        {
            left.reset();
            right.reset();
        }
    };

    // All values the UI needs from the audio engine, in one lock-free block.
    struct MeterExchange
    {
        StereoMeter input;
        StereoMeter output;
        std::atomic<double> sampleRate  { 0.0 };
        std::atomic<int>    bufferSize  { 0 };
        std::atomic<float>  latencyMs   { 0.0f };
        std::atomic<bool>   running     { false };

        void reset() noexcept
        {
            input.reset();
            output.reset();
        }
    };
}
