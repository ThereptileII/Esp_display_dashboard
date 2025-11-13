#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Root of the app UI (main screen container) */
extern lv_obj_t* ui_root;

/** Build the whole UI (landscape dashboard). Safe to call once. */
void ui_init(void);

/** Page navigation API kept for compatibility (single-page impl) */
void slide_to_page(int idx);

/** --- Compatibility shims for older code --- */
void ui_build_page1(void);     // old code calls this in setup()
int  ui_get_current_page(void); // gestures.cpp expects this
int  ui_page_count(void);       // optional: total pages (1)

#ifdef __cplusplus
} // extern "C"
#endif
