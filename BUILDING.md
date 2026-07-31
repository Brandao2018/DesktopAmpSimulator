# Building Desktop Amp Simulator

## Prerequisites (all platforms)

- **CMake 3.16+**
- **Qt 5.12+ or Qt 6** (Widgets module) — CMake autodetects, preferring Qt6
- A C++17 compiler
- Internet access on first configure (CMake downloads JUCE and Catch2 automatically)

---

## Windows (MSVC 2019+)

1. Install [Visual Studio 2019+](https://visualstudio.microsoft.com/) with the
   *Desktop development with C++* workload.
2. Install Qt via the [Qt Online Installer](https://www.qt.io/download-qt-installer)
   (e.g. Qt 6.7 MSVC 64-bit). Note the install path, e.g. `C:\Qt\6.7.2\msvc2019_64`.
3. Build:

```powershell
git clone https://github.com/YOURUSERNAME/DesktopAmpSimulator.git
cd DesktopAmpSimulator

cmake -B build -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_PREFIX_PATH="C:\Qt\6.7.2\msvc2019_64"
cmake --build build --config Release

.\build\bin\Release\DesktopAmpSimulator.exe
```

### ASIO support (optional, recommended for low latency)

The Steinberg ASIO SDK is free but **not redistributable**, so it is not
bundled or used in CI. To enable ASIO:

1. Download the SDK from [steinberg.net](https://www.steinberg.net/developers/)
2. Unzip anywhere, e.g. `C:\SDKs\asiosdk`
3. Add `-DASIO_SDK_DIR=C:/SDKs/asiosdk` to the configure command.

Without ASIO the app uses WASAPI/DirectSound, which works but has higher latency.

---

## Linux (Ubuntu 20.04+)

```bash
# Toolchain + Qt + audio/X11 development headers
sudo apt update
sudo apt install -y build-essential cmake git \
    qt6-base-dev \
    libasound2-dev libjack-jackd2-dev \
    libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
    libxinerama-dev libxrandr-dev libxrender-dev \
    libfreetype6-dev libfontconfig1-dev libgl1-mesa-dev libcurl4-openssl-dev

# On Ubuntu 20.04 (no qt6-base-dev), use Qt5 instead:
#   sudo apt install -y qtbase5-dev

git clone https://github.com/YOURUSERNAME/DesktopAmpSimulator.git
cd DesktopAmpSimulator
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

./build/bin/DesktopAmpSimulator
```

**JACK (optional):** install `jackd2` and start a JACK server for the lowest
latency; JUCE will offer JACK devices in the device selector when running.

---

## macOS (Xcode 12+, Intel or Apple Silicon)

```bash
xcode-select --install          # command-line tools, if not already present
brew install cmake qt

git clone https://github.com/YOURUSERNAME/DesktopAmpSimulator.git
cd DesktopAmpSimulator
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --parallel

open build/bin/DesktopAmpSimulator.app
```

Notes:
- Deployment target: Qt6 requires macOS 10.14+ (Qt6.5+ requires 11.0). For the
  broadest reach, build against Qt 5.15 with
  `-DCMAKE_OSX_DEPLOYMENT_TARGET=10.13`.
- On first launch, grant microphone/input access when macOS asks — the app
  needs it to read your audio interface.

---

## Running the tests

```bash
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The unit tests are headless (no audio device, Qt, or JUCE required at runtime).

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| CMake can't find Qt | Pass `-DCMAKE_PREFIX_PATH=<qt-install-dir>` |
| First configure is slow | Normal — CMake is cloning JUCE and Catch2 |
| No sound on Linux | Check the device selector; try the ALSA `default` device, or run a JACK server |
| High latency on Windows | Build with the ASIO SDK and use your interface's ASIO driver |
| "No audio device available" | Connect/power your interface, then press Start again (hot-plug is detected) |
