#pragma once
#include "lvgl.h"

// Initialize JD9365 panel, LVGL buffers, and flush. Returns true on success.
bool dbg_display_init(void);

// Print environment (LVGL ver, color depth, heap/PSRAM, etc.)
void dbg_dump_env(void);

// Draw a simple “sanity” pattern directly to the panel to verify raw blits
// (returns 0 on success, <0 if panel not ready)
int dbg_panel_sanity_pattern(void);

// Return the LVGL display pointer created by dbg_display_init() (or NULL)
lv_disp_t* dbg_lvgl_display(void);
