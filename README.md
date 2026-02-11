# wvfrm

Resizable VST3 waveform visualizer built with JUCE.

## Build (Windows)

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The VST3 binary is produced by the `wvfrm` target.
