# wvfrm Plugin Plan

## Vision

Build a modern, performance-safe VST3 waveform visualizer that can be used during mixing and arrangement for timing, transient, and spectral balance feedback.

## Product Goals

1. Provide a clear waveform readout that remains readable at different project tempos and zoom needs.
2. Support both tempo-relative and absolute-time workflows.
3. Offer informative colorization inspired by 3-band spectral weighting and DJ-style displays.
4. Keep CPU impact low enough for always-on use.

## Scope (Implemented)

- Windows VST3 (JUCE).
- Stereo FX passthrough architecture.
- Resizable UI with persisted editor bounds.
- Time window selection:
  - Sync divisions (`1/64`..`4/1`)
  - Milliseconds (`10`..`5000`)
- Tempo fallback behavior if host tempo is unavailable.
- Channel view modes:
  - `lr_split`, `left`, `right`, `mono`, `mid`, `side`
- Color modes:
  - `flat_theme`
  - `three_band`
- Theme presets and intensity control.
- Loop visualization mode with progressive interval fill.
- Unit tests for timing and DSP helper logic.

## Parameter/API Contract

- `time_mode`
- `time_sync_division`
- `time_ms`
- `channel_view`
- `color_mode`
- `theme_preset`
- `theme_intensity`
- `wave_gain_visual`
- `smoothing`
- `ui_scale`
- `wave_loop`

## Architecture Summary

### Audio Thread

- Capture audio into a ring buffer.
- Keep output bit-transparent passthrough.
- Track host timing data (BPM and PPQ) for UI sync.

### UI Thread

- Resolve current window duration.
- Read recent sample window from ring buffer.
- Render waveform per selected channel mode.
- Apply colorization with theme engine.
- In loop mode, draw progressive cycle fill and overwrite on cycle restart.

## Validation Strategy

1. Unit tests (`ctest`) for deterministic utility behavior.
2. Build validation with `wvfrm_tests` and `wvfrm_VST3` targets.
3. Manual host checks (Ableton/Reaper):
   - load/instantiate
   - automation read/write
   - resize behavior
   - loop visualization sync behavior

## Next Milestones

1. Add loop display size selector (25% / 50% / 100%).
2. Improve sync accuracy for non-playing transport states.
3. Add lightweight performance overlay (fps + draw cost).
4. Add optional anti-aliased path rendering mode.
5. Expand tests around loop phase mapping.
