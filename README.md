# Desktop Amp Simulator

![Build](https://github.com/Brandao2018/DesktopAmpSimulator/actions/workflows/build.yml/badge.svg)

A **cross-platform desktop guitar amp simulator** for home studio musicians and
hobbyists. Windows, Linux, and macOS (Intel + Apple Silicon). Standalone app
today; VST3/AU plugin support planned.

> **Status: Phase 1 (v0.1.0-alpha)** — audio I/O foundation. The app opens
> your audio interface, passes guitar input straight to the output, and shows
> real-time level meters and measured latency. No amp models yet — this
> milestone proves the low-latency pipeline on all three platforms.

## Features (Phase 1)

- Real-time audio pass-through via JUCE (ASIO*/WASAPI on Windows, ALSA/JACK on Linux, Core Audio on macOS)
- Stereo input/output peak + RMS meters (dB scale, peak hold, 30 Hz refresh)
- Audio device selector with hot-plug detection
- Buffer size selection (64 / 128 / 256 / 512 samples)
- Sample rate, buffer size, and roundtrip latency readout
- Lock-free audio↔UI communication (no locks on the audio thread)
- Dark-themed Qt interface

\* ASIO requires the free Steinberg ASIO SDK at build time — see [BUILDING.md](BUILDING.md).

## Quick Start

```bash
git clone https://github.com/Brandao2018/DesktopAmpSimulator.git
cd DesktopAmpSimulator
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

JUCE is downloaded automatically by CMake. Qt 5.12+ or Qt 6 must be installed —
per-OS details in [BUILDING.md](BUILDING.md).

## Roadmap

| Phase | Weeks | Deliverable |
|-------|-------|-------------|
| **1** | 1–4   | ✅ Audio I/O foundation (this release) |
| 2     | 5–8   | Amp models: tube saturation, tone stacks |
| 3     | 9–12  | Cabinet IR convolver, basic effects |
| 4     | 13–16 | Parallel routing, looper |
| 5     | 17–20 | VST3/AU wrappers, installers |
| 6     | 21–24 | Polish, testing, launch |

## Pricing (planned)

- **Free tier** — 2–3 amp models
- **Pro license** — €29.99, unlocks all features
- **Add-on packs** — €3.99–14.99 (artist tones, cabinet IRs, effect packs)

Phase 1 is fully free and open source.

## Documentation

- [BUILDING.md](BUILDING.md) — per-OS build instructions
- [DEVELOPMENT.md](DEVELOPMENT.md) — architecture and thread model
- [CHANGELOG.md](CHANGELOG.md) — version history

## License

[MIT](LICENSE)
