# Dashboard Improvement Roadmap (JC8012P4A1C_I_W_Y)

This document complements `HARDWARE_JC8012P4A1C_I_W_Y.md` by turning key improvement ideas into concrete development paths.

## Scope and assumptions

- Hardware target: ESP32-P4 + JD9365 (MIPI-DSI) + GSL3680 touch.
- Current app architecture: LVGL on P4, software-rotated flush, full-frame refresh.
- Goal: improve smoothness, maintainability, and readiness for board/feature expansion.

---

## 1) Partial refresh + area-based rotation

### Why
Current implementation flushes full frame and rotates full frame every cycle, even for tiny changes.

### Success criteria
- UI updates that change small regions produce significantly lower flush time.
- No visual tearing/artifacts compared with current baseline.

### Development path
1. **Baseline metrics**
   - Add temporary profiling around `my_flush` (time per flush, area size, avg/max).
   - Capture baseline at idle, moderate updates, heavy updates.
2. **Disable forced full refresh**
   - Set `drv.full_refresh = 0` and verify LVGL invalidation behavior.
3. **Implement area-only rotation**
   - Rotate only the invalid area into a staging buffer.
   - Draw only corresponding panel rectangle.
4. **Boundary validation**
   - Test corners, 1-pixel lines, and small widgets for off-by-one issues.
5. **Performance validation**
   - Compare avg/max flush time to baseline.
6. **Cleanup**
   - Keep lightweight optional profiling behind a debug macro.

### Risks
- Coordinate transform bugs at area boundaries.
- Increased code complexity in flush callback.

---

## 2) Runtime debug/logging policy

### Why
Per-flush serial logging can degrade frame pacing and hide true rendering performance.

### Success criteria
- Production builds have negligible logging overhead.
- Debug builds can still enable targeted logs quickly.

### Development path
1. **Define logging levels** (OFF/ERROR/WARN/INFO/TRACE).
2. **Wrap hot-path logs** (`flush`, touch read spam) with level checks/macros.
3. **Create build-time defaults**
   - Debug profile: WARN/INFO.
   - Release profile: ERROR/WARN.
4. **Validate performance delta** with and without logs.

### Risks
- Missing diagnostics in field if defaults are too strict.

---

## 3) Unified orientation contract (display + touch)

### Why
Display rotation and touch mapping are currently controlled in multiple places.

### Success criteria
- One canonical orientation config drives both display and touch transforms.
- Rotating hardware/UI requires editing one config block.

### Development path
1. **Create orientation header**
   - Central constants for logical orientation and transform policy.
2. **Refactor display flush** to consume shared constants.
3. **Refactor touch mapping** to consume same constants.
4. **Add corner test procedure**
   - Tap 4 corners + center; verify expected coordinates.
5. **Document known-good profiles**
   - Portrait-native, landscape-left, landscape-right.

### Risks
- Hidden legacy assumptions in existing touch rotation sequence.

---

## 4) Buffer/memory strategy hardening

### Why
Current design relies heavily on PSRAM and large buffers; future UI growth can destabilize allocations.

### Success criteria
- Stable startup memory margins across normal and stress scenarios.
- Predictable allocation behavior after UI/theme/font changes.

### Development path
1. **Map all large allocations** (size + memory caps + lifetime).
2. **Define allocation policy**
   - PSRAM for large draw buffers.
   - DMA/internal only where transfer path requires it.
3. **Add startup memory report**
   - Print free + largest block for relevant heaps.
4. **Stress test scenarios**
   - Font-heavy screens, chart-heavy screens, rapid page switches.
5. **Fallback behavior**
   - If preferred allocation fails, degrade gracefully (smaller buffers/feature flags).

### Risks
- DMA capability constraints may differ across SDK revisions.

---

## 5) Panel timing profiles by board revision

### Why
Panel lots/revisions can require slightly different timing for stable behavior.

### Success criteria
- Timing can be selected by named profile without code surgery.
- Known profiles are reproducible and documented.

### Development path
1. **Extract timing config** into profile structs/macros.
2. **Create at least 2 profiles**
   - `default_60hz`
   - `compat_stable` (conservative porches/clock if needed)
3. **Select profile via single compile-time flag**.
4. **Validate visual stability**
   - Boot-to-UI reliability, no intermittent blanking/flicker.
5. **Document profile selection guidance** in hardware docs.

### Risks
- Over-tuning to one panel batch can regress another.

---

## 6) ESP32-C6 integration track (optional, feature-driven)

### Why
Board includes ESP32-C6; currently unused by app. Can offload connectivity/background functions.

### Success criteria
- Clear P4↔C6 interface with versioned messages.
- C6 features can evolve without destabilizing LVGL timing on P4.

### Development path
1. **Define scope**
   - Decide first C6 feature (e.g., wireless telemetry bridge).
2. **Pick transport + framing**
   - UART/SPI + framed protocol with CRC and version field.
3. **Define message schema**
   - Commands/events/telemetry with forward-compatible fields.
4. **Implement stub layer on P4**
   - HAL + mock backend first.
5. **Implement minimal C6 firmware PoC**.
6. **Add integration tests**
   - Link resilience, reconnect, timeout, malformed packet handling.
7. **Gate feature rollout**
   - Keep dashboard operational if C6 link is absent.

### Risks
- Scope creep into “full distributed system” before protocol is stable.

---

## 7) Font and UI asset pipeline hygiene

### Why
Font configuration must stay aligned with `lv_conf.h`; drift causes rendering issues.

### Success criteria
- Repeatable font generation/update process.
- No runtime surprises after font updates.

### Development path
1. **Document font source + converter settings** (ranges, bpp, compression).
2. **Create asset checklist** for any font/UI update PR.
3. **Add visual regression routine**
   - Verify key screens and edge glyphs.
4. **Optional CI check**
   - Lint that required font symbols are declared.

### Risks
- Inconsistent glyph ranges if multiple contributors regenerate fonts differently.

---

## Suggested execution order

1. Partial refresh + area rotation (highest performance impact)
2. Logging policy (quick win)
3. Unified orientation contract (reduce bugs)
4. Buffer strategy hardening (stability)
5. Panel timing profiles (portability)
6. Font pipeline hygiene (maintainability)
7. ESP32-C6 integration (only when feature-driven)

## Definition of done (project-level)

- Smooth, stable UI on target hardware under normal and stress conditions.
- Orientation and touch remain correct across selected mounting profiles.
- Memory behavior is predictable with documented fallback behavior.
- Optional C6 usage has a clean boundary and does not compromise P4 UI determinism.
