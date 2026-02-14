# NMEA2000 Streaming + SD Card Logging Plan (for Graphs)

This plan defines how to ingest all telemetry from the NMEA2000 network (via TJA1050 CAN interface) and persist time-series data to SD card for graph pages (RPM, Voltage, Wind, Speed, etc.).

---

## 1) Goals

- Ingest NMEA2000 traffic continuously without blocking UI rendering.
- Decode required PGNs into a normalized in-memory state (`DashboardState`).
- Persist graph-relevant samples to SD card with bounded write latency.
- Rebuild chart windows (`1h/6h/24h`) from SD history quickly at page open.
- Survive brownouts/reboots with minimal data loss.

---

## 2) High-level architecture

1. **CAN RX layer** (TJA1050 + controller driver)
   - Interrupt receives CAN frames into lock-free/ring buffer.
2. **NMEA2000 parser layer**
   - Reassembles fast packets, validates frames, extracts PGN + source + data.
3. **Signal mapping layer**
   - Maps PGNs -> normalized signals (RPM, STW, SOC, voltage, heading, etc.).
4. **In-memory time-series cache**
   - Per-signal ring buffers for immediate graph rendering.
5. **SD writer service**
   - Batches samples into append-only segment files.
6. **Chart query service**
   - Reads from RAM first, then SD segments for historical windows.

---

## 3) Processor ownership options

## Option A: all on ESP32-P4 (simpler)
- P4 handles CAN + parsing + UI + SD logging.
- Pros: fewer inter-processor protocols.
- Cons: tighter CPU scheduling during heavy UI redraw.

## Option B: C6 as bus/logger coprocessor (recommended for scale)
- C6 handles CAN + NMEA2000 decode + pre-aggregation.
- C6 logs raw/normalized stream to SD (or forwards to P4 logger).
- P4 receives normalized metric stream for UI only.
- Pros: isolates real-time bus handling from UI jitter.
- Cons: requires robust P4<->C6 schema and versioning.

Decision rule: if UI frame-time jitter appears under CAN load, adopt Option B.

---

## 4) Data model for logging

Each normalized sample should include:
- `t_ms` (monotonic timestamp)
- `utc_s` (optional if GNSS/RTC time available)
- `signal_id` (enum/string key)
- `value` (float or scaled int)
- `quality` (OK / STALE / INVALID / ESTIMATED)
- `source` (PGN + source address)

Example signals:
- propulsion: `rpm`, `gear`, `regen_state`
- vessel: `stw_kts`, `sog_kts`, `heading_true_deg`, `cog_true_deg`
- energy: `pack_voltage_v`, `power_kw`, `soc_pct`, `remaining_kwh`
- navigation/autopilot: `xte_nm`, `dtw_nm`, `ttw_min`, `eta_s`
- wind: `twd_deg`, `tws_kts`

---

## 5) Storage strategy on SD card

## 5.1 File layout

Use date-partitioned append logs:

- `/logs/YYYYMMDD/signals_00.bin`
- `/logs/YYYYMMDD/signals_01.bin`
- `/logs/YYYYMMDD/index.idx`
- `/logs/meta/schema_v1.json`

Rotate segment file when size threshold reached (e.g., 8–32 MB).

## 5.2 Record format

Prefer compact binary records for low write overhead:
- fixed header + payload (e.g., 24–32 bytes)
- CRC per block or per record batch
- little-endian

Optional: separate “debug CSV export” tool offline, not at runtime.

## 5.3 Index format

Maintain sparse index entries every N samples/seconds:
- timestamp -> byte offset in segment
- enables fast seek for `last 1h/6h/24h` queries

---

## 6) Write path design (non-blocking)

- Producer tasks push normalized samples into a bounded queue.
- SD writer task flushes queue in batches (e.g., every 100–250 ms or queue threshold).
- Use preallocated write buffers to avoid heap fragmentation.
- On queue pressure:
  - never drop critical signals (RPM, STW, SOC, voltage, heading)
  - downsample non-critical high-rate signals first.

Required telemetry counters:
- queue depth (current/max)
- dropped sample counts by signal priority
- SD write latency p50/p95/max
- fsync duration and error counts

---

## 7) Read/query path for graphs

When opening a graph page:
1. Load recent window from RAM ring buffer.
2. Backfill missing history from SD using sparse index seek.
3. Downsample to chart pixel width (min/max buckets per x-bin) for speed.
4. Render and progressively refine if needed.

For each graph window (`1h`, `6h`, `24h`):
- define target sample density (e.g., 1–2 points per horizontal pixel).
- use min/max envelope to preserve peaks.

---

## 8) PGN-to-graph mapping plan

Minimum graph-capable mapping:
- **RPM graph**: engine speed PGN -> `rpm`
- **Voltage graph**: DC/battery PGNs -> `pack_voltage_v`
- **Power graph**: electrical power PGNs -> `power_kw`
- **Speed graph**: vessel speed PGNs -> `stw_kts`/`sog_kts`
- **Wind graph**: wind PGNs -> `twd_deg`/`tws_kts`

Add per-signal metadata:
- units, min/max normal ranges, warning thresholds, display precision.

---

## 9) Reliability, safety, and recovery

- Use atomic segment close/rename to avoid corrupted active file markers.
- Store lightweight journal marker for “last committed offset”.
- On boot:
  - validate last segment CRC/index
  - truncate partial tail if needed
  - resume appending safely.
- If SD unavailable:
  - continue RAM-only buffering
  - expose visible “logging offline” status
  - attempt periodic remount/recovery.

---

## 10) Performance budgets (initial targets)

- CAN ingest + decode must not miss frames at expected bus load.
- End-to-end metric latency (bus frame -> UI state): <= 150 ms for primary metrics.
- SD writer average duty should remain bounded (no long blocking bursts).
- Graph query time on page open:
  - 1h window: < 200 ms
  - 6h window: < 500 ms
  - 24h window: < 1000 ms (with downsampling)

---

## 11) Implementation phases

### Phase 1 — foundation
- Implement normalized signal model + in-memory ring buffers.
- Build CAN->PGN->signal decode pipeline.
- Add RAM-backed graph rendering only (no SD yet).

### Phase 2 — SD persistence
- Implement SD writer queue + binary segment format + rotation.
- Add sparse index and boot recovery.
- Add diagnostics counters page.

### Phase 3 — historical graph service
- Implement query engine with index seek + downsampling.
- Wire `1h/6h/24h` selectors for RPM/Voltage/Wind/Speed pages.
- Validate performance targets.

### Phase 4 — hardening
- Add stress tests (high bus rate, SD slowdowns, power interruptions).
- Add optional raw frame logging mode for troubleshooting.
- Freeze schema/versioning and document migration strategy.

---

## 12) Definition of done

- Required NMEA2000 metrics stream continuously into UI.
- SD logs are persisted and survive reboot without data corruption.
- RPM/Voltage/Wind/Speed pages load historical windows from SD.
- Stale/invalid data states are visible and handled safely.
- Diagnostics page shows bus health + logger health counters.
