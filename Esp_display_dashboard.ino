#include "debug_display.h"
#include "ui.h"
#include "touch_integration.h"

static uint32_t s_last_ms = 0;

void setup() {
  Serial.begin(115200);
  delay(100);

  if (!dbg_display_init()) return;  // sets up panel + LVGL + flush
  dbg_dump_env();
  // dbg_panel_sanity_pattern();    // optional once; comment it out after first test

  if (!touch_init_and_register(dbg_lvgl_display())) {
    Serial.println("[touch] WARN: touch init failed");
  }

  // Build UI
  ui_init();        // creates the pages/labels
  ui_build_page1(); // draw first page
}

void loop() {
  // ---- LVGL tick + handler ----
  uint32_t now = millis();
  uint32_t el  = now - s_last_ms;
  if (el) {
    lv_tick_inc(el);   // feed elapsed ms since last call
    s_last_ms = now;
  }

  lv_timer_handler();  // let LVGL do its work
  // Tighten the loop so the display updates as quickly as LVGL schedules it
  // while still yielding to the RTOS.
  delay(0);
}
