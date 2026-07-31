#pragma once

// Global constants shared between the audio engine and the UI.

namespace ampsim
{
    // Meter range shown in the UI (dBFS).
    constexpr float kMeterFloorDb   = -120.0f;  // treated as -inf
    constexpr float kMeterMinDb     = -60.0f;   // bottom of the visible scale
    constexpr float kMeterMaxDb     = 6.0f;     // top of the visible scale

    // Meter ballistics.
    constexpr float kMeterSmoothingMs = 10.0f;  // RMS smoothing window
    constexpr float kPeakHoldSeconds  = 1.5f;   // peak-hold indicator decay

    // UI refresh rate.
    constexpr int   kUiRefreshHz      = 30;
    constexpr int   kUiRefreshMs      = 1000 / kUiRefreshHz;

    // Supported buffer sizes offered in the UI.
    constexpr int   kBufferSizes[]    = { 64, 128, 256, 512 };

    // Channel layout for Phase 1: stereo in, stereo out.
    constexpr int   kNumChannels      = 2;
}
