# Display Performance Pitfalls (JC8012P4A1C_I_W_Y)

This note captures **what not to do** when optimizing refresh performance for this hardware profile, based on constraints in `HARDWARE_JC8012P4A1C_I_W_Y.md`.

## Hardware constraints you must respect
- ESP32-P4 driving JD9365 over MIPI-DSI.
- Panel native resolution: `800x1280` portrait.
- App logical LVGL space: `1280x800` landscape with software rotation in flush.
- RGB565 (2 bytes/pixel), so bandwidth/memory copies are expensive at full-frame sizes.

## Pitfalls (avoid these)

1. **Do not issue one panel draw call per row for touch-triggered updates**
   - Many tiny `esp_lcd_panel_draw_bitmap(...)` calls can destroy throughput and add scheduling overhead.
   - If retries are involved, row-by-row behavior can multiply latency into seconds.

2. **Do not add millisecond delays in the flush hot path unless absolutely required**
   - `delay(1..2)` inside repeated draw retries quickly balloons total frame latency.
   - Prefer minimal yields (`delay(0)`) and fewer retries.

3. **Do not flush full frame for small invalidations**
   - With a 1280x800 logical canvas, full-frame rotate+send is unnecessary for small touch/UI deltas.
   - Keep `drv.full_refresh = 0` and rotate only invalid areas.

4. **Do not depend on non-compact buffer slices for area updates**
   - Passing pointers into a larger stride buffer can force per-row draws.
   - For best speed, rotate into a compact linear area buffer and send once.

5. **Do not leave verbose serial logging enabled in flush/touch loops**
   - Logging can mask rendering performance and induce jitter.
   - Gate logs with runtime levels and keep production levels low.

## Fast-path approach (recommended)
- Rotate invalid area into a compact linear scratch buffer.
- `msync` that compact range once.
- Push one panel area draw per LVGL flush call.
- Keep retries bounded and non-blocking.

