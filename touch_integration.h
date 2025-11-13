#pragma once
#include <lvgl.h>

/** Verbose serial logging for touch */
void touch_set_verbose(bool v);

/** Forward to driver’s set_rotation (0..3). Start with 1 (swap XY). */
void touch_set_rotation(uint8_t r);

/** Init vendor GSL3680 and register an LVGL pointer indev on the given display. */
bool touch_init_and_register(lv_disp_t* disp);
