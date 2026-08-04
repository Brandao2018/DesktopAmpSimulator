#pragma once

#include <cmath>
#include <cstddef>

// Shared helper for the delay-line-based effects (PitchShifter, Chorus,
// Delay): fractional-position read with linear interpolation from a
// circular buffer. Framework-free and allocation-free.

namespace ampsim
{
    inline float readInterpolated(const float* buffer, size_t bufferSize, float positionSamples) noexcept
    {
        while (positionSamples < 0.0f)
            positionSamples += static_cast<float>(bufferSize);

        const float floorPos = std::floor(positionSamples);
        const size_t i0 = static_cast<size_t>(floorPos) % bufferSize;
        const size_t i1 = (i0 + 1) % bufferSize;
        const float frac = positionSamples - floorPos;
        return buffer[i0] * (1.0f - frac) + buffer[i1] * frac;
    }
}
