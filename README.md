# Esp_display_dashboard

[![Build Arduino Sketch](https://github.com/ThereptileII/Esp_display_dashboard/actions/workflows/arduino_build.yml/badge.svg)](https://github.com/ThereptileII/Esp_display_dashboard/actions/workflows/arduino_build.yml)

## Runtime architecture highlights

- LVGL logical canvas: `1280x800` (landscape)
- Panel native canvas: `800x1280` (portrait)
- Flush path uses partial invalidated areas with 90° CCW rotation into panel coordinates.
- Display/touch orientation contract is centralized in `orientation_config.h`.
- Runtime/build logging policy is centralized in `logging_policy.h`.

## Board timing profiles

JD9365 timing is selectable via a single compile-time profile in `esp_lcd_jd9365.h`:
- `JD9365_TIMING_PROFILE_DEFAULT_60HZ`
- `JD9365_TIMING_PROFILE_COMPAT_STABLE`

Set `JD9365_TIMING_PROFILE` to choose the active profile.

## Font asset maintenance

See `FONT_ASSET_PIPELINE.md` for the required workflow and checklist when regenerating Orbitron/LVGL assets.
