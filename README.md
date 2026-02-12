# wvfrm

`wvfrm` is a JUCE-based VST3 waveform visualizer for Windows.
It is an audio FX plugin (stereo in/stereo out) that passes audio through unchanged and focuses on real-time visual analysis.

## Current Status

- VST3 plugin builds and loads in modern hosts (including Ableton Live).
- Unit tests pass for core DSP utilities.
- Loop visualization mode is implemented.

## Core Features

- Resizable editor UI.
- Time window modes:
  - Tempo-synced (`1/64` to `4/1`).
  - Milliseconds (`10 ms` to `5000 ms`).
- Channel views:
  - `L/R split`, `Left`, `Right`, `Mono`, `Mid`, `Side`.
- Color modes:
  - Flat theme mode.
  - 3-band color mode (low/mid/high energy mapping).
- Theme presets:
  - `minimeters_3band`, `rekordbox_inspired`, `classic_amber`, `ice_blue`.
- Visual controls:
  - Theme intensity, visual gain, smoothing, loop toggle.
- Host automation via APVTS parameters.

## Build (Windows)

Prerequisites:

- CMake >= 3.22
- Visual Studio 2022 Build Tools with C++ workload (`MSVC`, `MSBuild`, Windows SDK)

Configure and build:

```powershell
cmake -S . -B build-vs2022 -G "Visual Studio 17 2022" -A x64
cmake --build build-vs2022 --config Release --target wvfrm_VST3
```

Run tests:

```powershell
cmake --build build-vs2022 --config Release --target wvfrm_tests
ctest --test-dir build-vs2022 -C Release --output-on-failure
```

## Plugin Output

Built plugin bundle:

- `build-vs2022/wvfrm_artefacts/Release/VST3/wvfrm.vst3`

With `COPY_PLUGIN_AFTER_BUILD`, CMake also installs to:

- `C:/Program Files/Common Files/VST3/wvfrm.vst3`

## Plugin Metadata

Edit metadata in `CMakeLists.txt` inside `juce_add_plugin(...)`:

- `COMPANY_NAME`
- `PRODUCT_NAME`
- `PLUGIN_MANUFACTURER_CODE`
- `PLUGIN_CODE`

## Project Layout

- `src/PluginProcessor.*` - audio processor, host timing state, APVTS state I/O
- `src/PluginEditor.*` - UI controls and attachments
- `src/ui/WaveformView.*` - waveform rendering and loop drawing
- `src/ui/ThemeEngine.*` - theme and color logic
- `src/dsp/*` - ring buffer, timing resolver, 3-band analyzer, channel view helpers
- `tests/*` - unit tests for time resolver, band analyzer, and channel math

## Notes

- The plugin is intentionally passthrough: no audio coloration or dynamics processing.
- If a host plugin cache becomes stale, force a full rescan after replacing the VST3 bundle.
