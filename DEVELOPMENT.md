# Development Guide

## Architecture Overview

Phase 1 is a thin, honest audio pipeline with a strict thread boundary:

```
┌─────────────────────────┐         ┌──────────────────────────┐
│   Real-time audio thread │        │       Qt UI thread        │
│   (owned by the driver)  │        │  (main thread, event loop)│
│                          │        │                           │
│  AudioEngine::           │ atomic │  MainWindow (QTimer 30 Hz)│
│  getNextAudioBlock()     │───────▶│   reads MeterExchange     │
│   • measure input        │ loads/ │   updates MeterWidgets,   │
│   • pass-through         │ stores │   labels, device list     │
│   • measure output       │        │                           │
└─────────────────────────┘         └──────────────────────────┘
             ▲                                   │
             │      JUCE MessageManager          │ user actions
             │  (pumped from a Qt timer;         ▼
             │   device hot-plug callbacks)   AudioEngine setters
             └────────── juce::AudioDeviceManager ───────────────
```

### Layers

| Directory | Contents | Frameworks |
|-----------|----------|------------|
| `src/Audio/` | `AudioEngine` — device I/O, real-time callback, latency | JUCE |
| `src/UI/` | `MainWindow`, `MeterWidget` — presentation only | Qt |
| `src/Shared/` | `MeterProcessor` (pure DSP), `ThreadSafeBuffer` (atomics), `Constants`, `Logger` | none |
| `src/Main.cpp` | Process bootstrap: Qt event loop + JUCE message pump | both |

`src/Shared/` deliberately has **no framework dependencies** — the DSP helpers
in `MeterProcessor.h` are plain functions over `float*`, which is what lets
`tests/` exercise the exact code the audio callback runs without opening a
device.

## JUCE + Qt Integration

Two frameworks that both want to own the event loop coexist like this:

1. **Qt owns the process.** `QApplication::exec()` is the real event loop.
2. **JUCE's MessageManager is created on the main thread**
   (`ScopedJuceInitialiser_GUI` in `Main.cpp`) and **pumped from a 10 ms Qt
   timer** via `runDispatchLoopUntil(1)`. This is what delivers
   `AudioDeviceManager` change notifications (device hot-plug, config changes).
3. **The audio callback needs neither loop** — the driver calls it on its own
   real-time thread.

Rules that keep this safe:

- JUCE objects are created, used, and destroyed on the main thread only
  (which *is* the JUCE message thread here).
- Qt widgets never call into JUCE from any other thread.
- `AudioEngine` exposes only atomics + message-thread methods; the UI never
  touches `juce::AudioBuffer` or the device directly.

## Audio Thread Model

`AudioEngine::getNextAudioBlock()` runs at real-time priority and must never:

- lock a mutex (the engine's `errorMutex_` is message-thread-only),
- allocate or free memory,
- call Qt, log, or do file I/O.

What it does per block:

1. Measure input peak/RMS per channel (`MeterProcessor.h`).
2. `processPassThrough()` — a deliberate no-op that marks where Phase 2 amp
   models will slot in.
3. Measure output peak/RMS.
4. Publish everything through `MeterExchange` atomics
   (`memory_order_release` stores; the UI does `acquire` loads).

Meter ballistics: instant attack, ~10 ms one-pole release smoothing, computed
per block. Peak-hold (1.5 s) is handled UI-side in `MeterWidget`.

## Latency Reporting

Phase 1 reports **device-reported roundtrip latency**: input latency + output
latency + one buffer, converted to milliseconds
(`AudioEngine::refreshLatencyFromDevice`). An active chirp-loopback
measurement is planned for a later phase — it requires a physical loopback
cable to produce a meaningful number, which most users don't have connected.

## Error Handling & Hot-plug

- Device errors are stored in a message-thread-guarded string
  (`AudioEngine::getLastError`) and surfaced in the status label; the user can
  simply press Start again.
- `AudioDeviceManager` broadcasts changes → `AudioEngine` sets an atomic flag →
  the UI's 30 Hz timer notices (`consumeDeviceListChanged`) and re-scans the
  device list. Plugging in a USB interface while running updates the dropdown.

## How to Extend (Phase 2 preview)

1. Replace `processPassThrough()` with a processor chain — keep the same
   signature (`float* const*`, channels, samples) so tests stay hardware-free.
2. Parameters flow UI → audio thread through atomics (or a lock-free FIFO once
   parameters become structs), mirroring the meter path in reverse.
3. Keep every new DSP unit in `src/Shared/` or a new `src/DSP/` with no
   framework types in its interface, and add Catch2 tests alongside.

## Conventions

- C++17, warnings-as-errors on project code (JUCE/Qt headers exempt).
- Members `trailingUnderscore_`; constants `kCamelCase` in namespace `ampsim`.
- No hardcoded absolute paths anywhere.
