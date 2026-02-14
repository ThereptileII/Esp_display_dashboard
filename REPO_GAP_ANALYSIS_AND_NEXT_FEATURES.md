# Repository Gap Analysis & Recommended Next Features

This document summarizes what appears to be **missing** in the current repository and proposes a practical, prioritized feature roadmap.

## How this analysis was derived

Reviewed repository markdown and key implementation files:
- `README.md`
- `DEVELOPMENT_IMPROVEMENT_ROADMAP.md`
- `HARDWARE_JC8012P4A1C_I_W_Y.md`
- `PERFORMANCE_PITFALLS.md`
- `FONT_ASSET_PIPELINE.md`
- `Design_guidlines.md`
- `ui.cpp`
- `.github/workflows/arduino_build.yml`

---

## 1) High-impact missing items (priority: P0)

### 1.1 Real data pipeline is missing
**Observation:** UI values are currently static literals (`"7.4 kts"`, `"1350"`, `"78%"`) and the RPM chart uses hardcoded demo arrays.

**Why this matters:** The dashboard cannot yet function as an operational display without a telemetry ingestion/update loop.

**Develop next:**
- Add a data model (`struct DashboardState`) and central store.
- Add periodic UI update functions (`ui_update_speed`, `ui_update_rpm`, etc.).
- Add a transport layer for real inputs with a **hard requirement: TJA1050 CAN transceiver + standard NMEA2000 PGNs**.

**Suggested acceptance criteria:**
- At least 10 Hz updates for critical metrics without frame pacing regressions.
- Demonstrated stale-data behavior (timeouts/invalid markers).

### 1.1.a Transport decision now required (based on latest requirement)
**Requirement update:** All incoming data will be delivered over a **TJA1050 CAN bus interface module** using **standard NMEA2000 messages**.

**Develop next:**
- Choose processor ownership for CAN + NMEA2000 stack (ESP32-P4 direct vs ESP32-C6 bridge).
- Define a PGN mapping matrix from NMEA2000 fields to dashboard metrics (RPM, STW, SOC, Voltage, Heading, XTE, ETA, etc.).
- Add message validation, timeout/stale policy, and parser error counters.
- Expose bus health diagnostics in a dedicated diagnostics view.

**Suggested acceptance criteria:**
- Stable decode of required PGNs under sustained bus traffic.
- Deterministic stale-data handling when PGN streams stop.
- Metrics update latency within target budget for helm display use.

### 1.2 Multi-page navigation is not implemented
**Observation:** Tabs exist visually, but callbacks are placeholders; compatibility shims report only one page.

**Why this matters:** UX suggests multiple sections (Overview / Speed / Voltage), but behavior does not match.

**Develop next:**
- Implement page router/state machine.
- Bind each tab to a real page/container.
- Add transition policy (instant vs animated), with low-overhead defaults.

**Suggested acceptance criteria:**
- All tabs switch content deterministically.
- `ui_page_count()` reflects actual page count and `slide_to_page()` is functional.

### 1.3 No error/health UX for sensor or comms faults
**Observation:** There is no visible fault-state model for disconnected sensors, comms loss, or out-of-range values.

**Why this matters:** Marine/embedded dashboards need robust degraded-mode behavior.

**Develop next:**
- Add health state per metric source (OK/WARN/ERROR/STALE).
- Add global banner or per-card warning badges.
- Add watchdog timestamps and source heartbeat handling.

**Suggested acceptance criteria:**
- Simulated source dropouts are shown within bounded latency (e.g., 2s).
- Recovery path visibly clears warning states.

---

## 2) Important engineering gaps (priority: P1)

### 2.1 No automated tests for UI logic and mapping transforms
**Observation:** Build workflow compiles sketch, but no unit tests for coordinate mapping, orientation, or widget state logic are visible.

**Develop next:**
- Add host-side tests for transform functions and mapping math.
- Add data formatting tests for value-to-string rendering.
- Add smoke tests for page switching and RPM detail open/close logic where possible.

### 2.2 No reproducible performance benchmark harness
**Observation:** Docs discuss performance and pitfalls, but there is no committed benchmark script/log format for continuous tracking.

**Develop next:**
- Add compile-time benchmark mode recording flush timing (p50/p95/p99, max).
- Save machine-readable logs (CSV or JSON) for regression detection.
- Add baseline thresholds in CI artifacts or documented manual benchmark process.

### 2.3 Touch gesture model appears basic/incomplete
**Observation:** Repo contains gesture/touch integration files, but visible UI behavior emphasizes taps and lacks clear navigation gestures and long-press semantics.

**Develop next:**
- Define explicit gesture contract (tap, long-press, swipe-back, drag).
- Add debouncing/noise handling and accidental-touch rejection.
- Add gesture telemetry counters for field tuning.

### 2.4 Configuration management for board variants remains shallow
**Observation:** Docs mention timing profiles and board revision concerns, but systematic per-board configuration packaging appears limited.

**Develop next:**
- Add named board profiles (pinmap + timing + orientation + touch transform).
- Add compile-time profile selector with strict validation.
- Add compatibility matrix table in docs.

---

## 3) Productization/documentation gaps (priority: P2)

### 3.1 Missing complete onboarding/quickstart
**Observation:** README highlights architecture but lacks a full “start from zero” flow (toolchain setup, dependency versions, flash/monitor, troubleshooting).

**Develop next:**
- Add Quickstart section with exact commands.
- Add known-good toolchain versions.
- Add first-boot checklist and common failure signatures.

### 3.2 Design spec is descriptive, not enforceable
**Observation:** `Design_guidlines.md` includes rich style intent, but no explicit design tokens header/schema to enforce consistency.

**Develop next:**
- Introduce tokenized style constants (spacing, radii, typography scale).
- Optionally generate style header from a single source (JSON/YAML).
- Add UI review checklist to PR template.

### 3.3 Missing architecture diagrams and module boundaries
**Observation:** Useful narrative docs exist, but there is no canonical component diagram showing data flow, UI, touch, panel, and optional C6 coprocessor boundary.

**Develop next:**
- Add one runtime diagram and one build/dependency diagram.
- Document ownership boundaries for each module.

### 3.4 No release checklist/versioning policy
**Observation:** No release process doc is obvious (tagging, changelog, test gates, performance gate).

**Develop next:**
- Add `RELEASE_CHECKLIST.md` and semantic version strategy.
- Add “required checks before release” list.

---

## 4) Security/reliability hardening opportunities (priority: P2)

- Add brownout/boot-loop diagnostics and safe-mode boot path.
- Add configuration sanity checks at startup (panel timing, orientation, touch map bounds).
- Add persistent crash marker / last reset reason display for maintenance.
- If networking is introduced, define auth/key storage + OTA rollback policy early.

---

## 5) Proposed development sequence (recommended)

1. **Data model + live telemetry adapter** (P0)
2. **True multi-page navigation + tab behavior** (P0)
3. **Health/fault UX + stale data handling** (P0)
4. **Benchmark harness + measurable performance baselines** (P1)
5. **Test coverage for transforms, formatting, and UI state transitions** (P1)
6. **Board profile system + compatibility matrix** (P1)
7. **Quickstart/release docs + architecture diagrams** (P2)

---

## 6) Candidate feature backlog (concrete items)

- Live metric bindings (speed/RPM/voltage/SOC).
- Alarm center (thresholds + acknowledgement workflow).
- Trend pages (history windows beyond RPM; battery and speed trends).
- Config page (units, brightness, theme contrast, refresh policy).
- Runtime diagnostics page (FPS, flush timing, memory, touch event rate).
- If ESP32-C6 is used, restrict it to a strict CAN/NMEA2000 bridge role with versioned schema toward the UI processor.
- OTA update workflow and recovery screen.

---

## 7) Definition of done for “next milestone”

For implementation details on bus ingestion + SD persistence for graphs, see `NMEA2000_SD_LOGGING_PLAN.md`.


A practical next milestone can be considered complete when:
- Dashboard values are fed by a live input source (or simulator) and update continuously.
- Tabs switch between at least 3 real pages.
- Faults/stale data are visible and recover correctly.
- Performance benchmark logs are captured and compared to a documented baseline.
- README contains complete setup/flash/run instructions for a new contributor.
