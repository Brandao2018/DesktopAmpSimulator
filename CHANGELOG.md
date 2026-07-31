# Changelog

All notable changes to this project are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/); versioning follows [SemVer](https://semver.org/).

## [0.1.0-alpha] — 2026-07-31

Phase 1: audio I/O foundation.

### Added
- Cross-platform CMake build (Windows/MSVC, Linux/GCC-Clang, macOS Intel + arm64);
  JUCE fetched automatically, Qt5/Qt6 autodetected
- `AudioEngine` (JUCE): real-time pass-through, stereo peak/RMS metering with
  10 ms smoothing, device-reported roundtrip latency, hot-plug detection,
  graceful device-error handling
- Qt UI: stereo input/output meters (dB scale, peak hold), device selector,
  buffer-size selector (64–512), sample rate / buffer / latency readouts,
  Start/Stop, status line, dark theme, window geometry persistence
- Lock-free audio↔UI communication (atomics only on the audio thread)
- Catch2 unit tests (pass-through transparency, meter math) wired into CTest
- GitHub Actions CI: 4-platform matrix build + test, artifact upload,
  automatic prerelease on `v*` tags
- Documentation: README, BUILDING, DEVELOPMENT

### Known limitations
- Latency is device-reported, not chirp-measured (planned)
- ASIO requires a locally downloaded Steinberg SDK (`-DASIO_SDK_DIR=...`)
- No amp models yet — pass-through only (Phase 2)
