# JC8012P4A1C_I_W_Y Hardware Profile and Dashboard Constraints

This document captures the **hardware-specific context** for this repository so future UI/driver work is done with the right constraints in mind.

## 1) Board / module identity

From the bundled vendor package naming and the in-repo driver stack, this project targets the **JC8012P4A1C_I_W_Y** class of ESP32-P4 display modules using:

- **MCU:** ESP32-P4
- **LCD controller:** JD9365 over **MIPI DSI**
- **Touch controller:** GSL3680 over **I2C**
- **Panel resolution (native):** **800 x 1280** (portrait)

The application intentionally renders LVGL in **1280 x 800** (landscape) and rotates in software during flush.

## 1.1) Dual-processor note (ESP32-P4 + ESP32-C6)

You are correct: this board family is commonly marketed with both **ESP32-P4** and **ESP32-C6** on the PCB.

For this repository specifically, the code path currently uses only the **ESP32-P4 application side** (display + touch + LVGL). There is no project-level logic yet that initializes or exchanges data with an ESP32-C6 co-processor (for example over UART/SPI shared protocol).

Practical implication:

- Today: treat this dashboard as a single-MCU P4 UI firmware.
- Future option: use the C6 for wireless offload (Wi-Fi/BLE/802.15.4 roles), sensor gatewaying, or low-power/background tasks, and feed data/events to the P4 UI task.

## 2) Pin map used by this codebase

The active board pin configuration is declared in `pins_config.h`:

- LCD reset: `GPIO27`
- LCD backlight: `GPIO23`
- Touch I2C SDA: `GPIO7`
- Touch I2C SCL: `GPIO8`
- Touch reset: `GPIO22`
- Touch interrupt: `GPIO21`

If you port to another carrier/module revision, update `pins_config.h` first.

## 3) Display path (what is hardware-specific)

### 3.1 MIPI DSI configuration

The JD9365 driver wrapper configures:

- DSI bus with **2 data lanes**
- lane bitrate: **1500 Mbps**
- DPI clock: **60 MHz**
- RGB565 pixel format (16bpp)
- timing for 800x1280@60 class operation

These values are embedded in `esp_lcd_jd9365.h` macros and consumed by `jd9365_lcd.cpp`.

### 3.2 LVGL logical orientation vs physical panel orientation

Current architecture:

- LVGL logical canvas: **1280x800** (landscape)
- Physical panel: **800x1280** (portrait)
- `debug_display.cpp` rotates flushed LVGL invalid areas **90° CCW** and updates only the mapped panel rectangle.
- Orientation policy is centralized in `orientation_config.h` for both display and touch.


## 4) Touch path (what is hardware-specific)

`touch_integration.cpp` + `gsl3680_touch.cpp` combine two transforms:

1. GSL3680 driver-level rotation (`esp_lcd_touch_set_swap_xy/mirror_x/mirror_y`)
2. App-level optional `TOUCH_SWAP_XY`, `TOUCH_INVERT_X`, `TOUCH_INVERT_Y`

Default app mapping is currently tuned for the dashboard’s rotated landscape presentation:

- `TOUCH_SWAP_XY = 1`
- `TOUCH_INVERT_X = 1`
- `TOUCH_INVERT_Y = 0`

When display orientation changes, touch mapping must be revalidated at the same time.

## 5) Memory and performance constraints

`debug_display.cpp` allocates:

- LVGL draw buffer #1: full frame (1280x800x2) ≈ **2.0 MB**
- LVGL draw buffer #2: full frame (1280x800x2) ≈ **2.0 MB**
- rotated output frame (800x1280x2) ≈ **2.0 MB**

Total framebuffer working set is roughly **6.1 MB**, so PSRAM is effectively required.

Also note:

- Partial invalidation is enabled (`drv.full_refresh = 0`).
- Rotation work now scales with invalid area size.
- Hot-path logging is behind level checks from `logging_policy.h`.

## 6) Font assets and their implications

Custom Orbitron fonts are compiled into C sources (`orbitron_16_600.c`, `orbitron_20_700.c`, `orbitron_32_800.c`, `orbitron_48_900.c`) and declared in `fonts.h`.

`lv_conf.h` is configured for font support details that match these assets:

- `LV_FONT_FMT_TXT_BPP = 4`
- `LV_FONT_FMT_TXT_LARGE = 1`
- `LV_USE_FONT_COMPRESSED = 1`

Changing font generation settings without matching `lv_conf.h` can cause missing/garbled text.

## 7) Recommended improvements (library-aligned)

These are concrete improvements based on how LVGL + esp_lcd are currently used.

### A. Reduce flush cost by avoiding full-frame refresh where possible

- Set `drv.full_refresh = 0` and handle partial areas in `my_flush`.
- Rotate/copy only the invalidated area, not full screen.

Expected gain: lower memory bandwidth, lower CPU, better responsiveness for small updates.

### B. Remove or gate per-frame flush logging

- Current `Serial.printf("[flush] ...")` runs every flush.
- Guard it with a debug macro or compile-time flag.

Expected gain: smoother frame timing and lower jitter.

### C. Unify display + touch rotation into one source of truth

- Keep orientation constants in one header (for both display and touch mapping).
- Avoid double transforms (driver rotation + app-level swap/invert) unless explicitly needed.

Expected gain: easier bring-up when hardware revision/orientation changes.

### D. Prefer explicit buffer capability strategy

- Keep LVGL draw buffers in PSRAM.
- Keep only DMA-critical transfer buffer(s) in DMA-capable memory when needed.

Expected gain: less fragile allocation behavior as UI size/features grow.

### E. Make panel timing/profile selectable by board revision

- Wrap JD9365 timing and lane parameters in named profiles (e.g., default/stable/high-refresh).
- This helps when vendor silently swaps panel lots with slightly different tolerances.

Expected gain: faster field debugging and easier board-variant support.

### F. Add an explicit P4<->C6 interface layer (if dual-chip features are needed)

- Define one transport and schema for inter-chip messaging (for example UART with framed packets, protobuf/nanopb, or CBOR).
- Keep LVGL/UI rendering on P4; treat C6 as a connectivity/service processor.
- Introduce a small HAL boundary now (even if C6 is stubbed) so future wireless integration does not leak into UI code.

Expected gain: clean scaling to wireless/cloud features without destabilizing display/touch timing.

## 8) Bring-up checklist for this hardware

1. Confirm pin map matches your module revision (`pins_config.h`).
2. Verify backlight control works (`GPIO23` in current code).
3. Confirm touch IRQ/reset lines are present; if not, set `TP_RST`/`TP_INT` to `-1` as needed.
4. Validate touch corners after any orientation change.
5. Check PSRAM headroom after LVGL and font allocations.
6. Keep serial debug low during performance tests.
7. Decide early whether ESP32-C6 is in scope for this firmware release; if yes, freeze inter-chip protocol before app growth.

## 9) Notes on external vendor resources

- AliExpress product page access may be region/bot-protected.
- Vendor RAR package lists matching demo/driver assets for JD9365 + GSL3680 + ESP32-P4; the repository already contains those driver families and pin defaults.

If you can provide extracted datasheet text/screenshots from the vendor PDFs, this document can be extended with:

- exact power rail limits,
- backlight electrical constraints,
- touch sampling/report rate limits,
- thermal/environment limits.
