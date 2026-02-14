# Image-Based Missing Pages & Feature Specification

This document answers: **“What pages are missing, and what should each include?”** based on the supplied reference images and the current implementation.

---

## 0) Current state snapshot (what exists now)

From current `ui.cpp` behavior:
- Main view has a navbar with tabs labeled `Overview`, `Speed`, `Voltage`.
- Only one effective page is implemented (`ui_page_count() == 1`); tab callbacks are placeholders.
- There is one detail overlay currently implemented: **RPM Monitor** with a line chart and `6h/12h/24h` selectors.

Therefore, most referenced UI concepts are still missing as real screens/flows.

---

## 1) Missing top-level pages (from the first two screenshots)

The top nav in references clearly expects four primary tabs:
1. **Overview**
2. **Speed Focus**
3. **Minimal**
4. **Wind Data**

In current implementation, only a partial “Overview-like” dashboard exists; the other three are missing as true pages.

---

## 2) Page-by-page specification (detailed)

## 2.1 Overview Page (enhanced final target)

### Purpose
At-a-glance vessel/electrical status with high-contrast large metrics.

### Layout
- **Top tab strip** centered (pill tabs).
- **Main card region** in a 2-column responsive grid:
  - Left large tiles:
    - RPM (cyan)
    - Battery Voltage (green)
  - Right stacked tiles:
    - Remaining Power (orange, kWh)
    - Power Draw (white, kW)
    - SOC (white/orange, %)

### Required content blocks
- RPM: numeric + optional trend arrow/sparkline.
- Battery Voltage: pack voltage + status color logic.
- Remaining Power: kWh estimate.
- Power Draw: signed value (+ discharge / − regen).
- SOC: percent + severity thresholds.

### Missing features to build
- Live data binding for all five cards.
- Value staleness indicator (e.g., “—” + stale badge).
- Alarm badges on each tile (warn/critical thresholds).
- Unit profile support (metric/imperial where relevant).

---

## 2.2 Speed Focus Page (full-screen speed primary)

### Purpose
Helm-focused speed readability with secondary propulsion state.

### Layout
- **Hero card** occupying top ~65–70%:
  - Label `STW:`
  - Very large `5.6 kts` style value centered.
- **Bottom 4 mini-cards row**:
  1. RPM (cyan)
  2. Gear (orange)
  3. Regen state (orange, text or icon)
  4. SOC (orange/white)

### Required interactions
- Tap hero speed -> open speed trend page (optional quick drill-down).
- Long press on gear card -> gear detail/status source panel.

### Missing features to build
- STW source integration + smoothing filter.
- Low-speed precision mode (e.g., 0.01 kts under 2 kts).
- Gear-state decoding and neutral/reverse color semantics.
- Regen active/inactive indicator with hysteresis.

---

## 2.3 Minimal Page (new)

### Purpose
Ultra-clean low-cognitive-load page for rough conditions/night use.

### Layout (recommended from references + design language)
- Single large central metric (configurable: STW or heading).
- Two compact bottom cards only (SOC + Depth or SOC + RPM).
- Optional tiny status strip: alarms, autopilot mode, GNSS fix.

### Missing features to build
- “Minimal profile” config (which two secondary metrics show).
- Night mode brightness cap + high-contrast palette.
- Gesture shortcuts (swipe left/right to cycle selected primary metric).

---

## 2.4 Wind Data Page (new)

### Purpose
Dedicated wind history/trend visualization.

### Layout (from narrow portrait reference, adapted to 1280x800)
- Header:
  - TWD range labels (e.g., 60° … 120°)
  - Current TWD highlighted in a rounded badge (e.g., `082°`).
- Main chart:
  - Vertical scrolling or time-series trace (gold line on dark background).
  - Time window label (`10 mins`, `30 mins`, etc.).
- Optional side stats rail:
  - TWD, TWS, gust, lulls, variance.

### Missing features to build
- Wind stream ring buffer storage (time-indexed).
- Time-window selector (10m/30m/1h).
- Scale auto/manual toggle.
- Outlier/spike handling and sensor dropout visualization.

---

## 3) Missing detail/secondary pages from references

## 3.1 Voltage Monitor Detail Page (partially present conceptually, not implemented as shown)

### Purpose
Trend analytics for battery pack voltage over selected time ranges.

### Layout (from screenshot)
- Top bar with back button + title (`⚡ Voltage Monitor`).
- Period chips on right (`1h`, `6h`, `24h`).
- Stats row cards:
  - Current
  - Average
  - Maximum
  - Minimum
- Full-width line chart:
  - Y-axis voltage ticks.
  - X-axis time stamps.
  - Smooth green trace.

### Missing features
- Real historical storage with rolling aggregation.
- Period decimation/downsampling for large windows.
- Chart axes, grid, and min/max marker annotation.
- Crosshair tap-to-inspect value/time.

---

## 3.2 RPM Monitor Detail Page (exists but incomplete vs full feature target)

### Already present
- Back button
- `6h/12h/24h` selectors
- Line chart with sample data

### Missing enhancements
- Replace synthetic arrays with real RPM history.
- Summary cards (current/avg/max/min) like voltage page parity.
- Alarm overlays (overspeed events).
- Zoom/pan and marker readout.

---

## 3.3 Autopilot – Heading Control Page (landscape)

### Purpose
Primary steering/autopilot interaction screen.

### Layout (from red/blue variants)
- Top status/action row:
  - Left mode pill: `Steering to Heading`
  - Right red CTA: `Disengage pilot`
- Center-left: compass/heading dial with:
  - Locked heading value in center
  - Command marker and vessel heading marker
  - Colored limit/indicator segments
- Left and right side control buttons:
  - `±1°` and `±10°` heading change controls.
- Right telemetry stack cards:
  - Heading
  - COG
  - SOG
  - Depth
- Bottom progress/track strip.

### Missing features
- Autopilot command API integration.
- Safety interlocks (long-press or confirm for disengage).
- Command acknowledgment and timeout handling.
- Mode-specific color themes (active/standby/alert).

---

## 3.4 Autopilot – Navigation Assist Page (route steering)

### Purpose
Steering to waypoint with lateral error and ETA context.

### Layout (from white navigation mock)
- Header row:
  - Left mode pill: `Steering to Navigation`
  - Center locked heading
  - Right `Disengage pilot`
- KPI row:
  - XTE (left/right)
  - DTWpt
  - TTWpt
  - ETA
- Central map/route preview panel:
  - Boat icon
  - Current path + target path
  - Waypoint label/number
  - Port/starboard scale markers (e.g., `0.3 nm`)
- Action buttons:
  - `Restart XTE`
  - `Advance wpt`
  - `Override`

### Missing features
- Route/waypoint ingestion from nav source.
- XTE computation and signed display.
- Waypoint advance logic with guardrails.
- Simple local map projection/track renderer.

---

## 3.5 Autopilot Confirmation Modal (portrait and landscape variants)

### Purpose
Decision confirmation before heading/nav change.

### Layout
- Modal card over dimmed autopilot page.
- Title prompt (e.g., `Turn to next waypoint?`).
- Two primary decisions:
  - Turn to waypoint (bearing change shown).
  - Maintain heading (current heading shown).
- Critical action button: `Disengage pilot`.

### Missing features
- Blocking modal state machine.
- Rotary/gesture/button focus order and default-safe selection.
- Timeout behavior (auto-cancel/maintain heading).
- Audible/visual alert pulse integration.

---

## 3.6 Instrument Gauge Page (TWS circular dial)

### Purpose
Traditional instrument visualization for wind + heading/depth quick glance.

### Layout
- Left: large circular dial with numeric center value (`TWS 12.5 kts` style).
- Dial overlays:
  - Direction ticks/labels
  - Colored sectors
  - Needles/markers
- Right stacked cards:
  - Heading
  - STW
  - Time & Date
  - Depth

### Missing features
- LVGL custom arc/canvas rendering for dial.
- Needle animation and damping.
- Multi-sensor fusion for wind/heading coherence.

---

## 4) Cross-page system features missing (needed for all pages)

## 4.1 Navigation framework
- Real page router with tab binding.
- Back-stack for detail pages/modals.
- Standard transitions with frame-budget guard.

## 4.2 Data architecture
- Unified `DashboardState` model.
- **Hard requirement:** all runtime telemetry enters over a **TJA1050 CAN transceiver** on one processor.
- **Protocol requirement:** decode only **standard NMEA 2000 PGNs** (no proprietary payload as primary source).
- Define a processor ownership decision and freeze it:
  - Option A: CAN ISR + decode on ESP32-P4 (single-firmware simplicity).
  - Option B: CAN + NMEA2000 stack on ESP32-C6, forward normalized metrics to P4 UI.
- Add PGN decode layer -> unified `DashboardState` mapping table (PGN -> field -> unit).
- Add sampling, smoothing, stale detection, and quality flags.

## 4.3 Alarm/status framework
- Per-metric threshold rules.
- Global alarm banner and per-card badges.
- Ack/silence workflow.

## 4.4 Chart/history infrastructure
- Ring buffers per metric.
- Time-window queries (1h/6h/24h, etc.).
- Reusable chart widget styling and interaction.

## 4.5 Theme and mode system
- Normal/day mode.
- Night/dim mode.
- Alert palette override (red emphasis).

## 4.6 Safety-critical interactions
- Confirmations for pilot disengage and heading changes.
- Command result feedback (sent/acked/failed).
- Communication loss behavior (degrade + lockout policy).

---

## 5) Prioritized implementation order (practical)

### Phase 1 (must-have UX completion)
1. Freeze transport architecture: **TJA1050 + NMEA2000** path and processor ownership (P4 vs C6 bridge).
2. Implement NMEA2000 PGN decode-to-state pipeline for all currently displayed metrics.
3. Implement real top-level pages: **Overview, Speed Focus, Minimal, Wind Data**.
4. Implement navigation/router and tab behavior.
5. Wire live data model to all existing cards.

### Phase 2 (analytics + drill-down)
6. Build full **Voltage Monitor** detail page with real history.
7. Upgrade **RPM Monitor** to real data + stat summary cards.
8. Add shared chart/history service.

### Phase 3 (autopilot suite)
9. Build **Heading Control** page.
10. Build **Navigation Assist** page.
11. Add **confirmation modal** workflows and safety interlocks.

### Phase 4 (instrument polish)
12. Build **TWS Gauge** page and reusable dial widget.
13. Add day/night + alert theme profiles.
14. Add alarm center and diagnostics overlays.

---


## 6) NMEA 2000 over TJA1050 implementation notes (new requirement)

Detailed streaming/logging execution plan: `NMEA2000_SD_LOGGING_PLAN.md`.

### Physical/link assumptions
- Use one TJA1050 module connected to one processor CAN controller.
- Ensure 120Ω termination strategy is correct for the actual bus topology.
- Validate CAN bit timing against NMEA2000 network requirements before field testing.

### Software architecture (recommended)
- Layer 1: CAN RX/TX driver (interrupt-safe ring buffer).
- Layer 2: NMEA2000 PGN parser/dispatcher.
- Layer 3: Signal mapping service (`PGN -> normalized metric fields`).
- Layer 4: UI-facing state store (`DashboardState`) with timestamps + quality.
- Layer 5: UI binder pushing updates to cards/charts at bounded frame rate.

### Minimum PGN mapping plan
- Vessel speed/heading PGNs -> STW/SOG/Heading cards.
- Engine PGNs -> RPM/Gear/Regen cards.
- Electrical PGNs -> Voltage/Power/SOC/Remaining energy cards.
- Navigation/autopilot PGNs -> XTE/DTW/TTW/ETA + commanded heading states.

### Error handling requirements
- Mark stale after timeout per metric source.
- Flag invalid/out-of-range values and avoid rendering misleading numbers.
- Surface bus health status (RX rate, parser errors, last good frame age) on diagnostics page.

---

## 7) Concise “missing pages list” checklist

- [ ] Overview (enhanced, production-ready)
- [ ] Speed Focus
- [ ] Minimal
- [ ] Wind Data
- [ ] Voltage Monitor detail
- [ ] RPM Monitor (enhanced)
- [ ] Autopilot Heading Control
- [ ] Autopilot Navigation Assist
- [ ] Autopilot Confirmation Modal flow
- [ ] Instrument Gauge (TWS dial + side metrics)
