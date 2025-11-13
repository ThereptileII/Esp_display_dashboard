#pragma once

// LVGL logical is landscape 1280x800; panel is portrait 800x1280.
// We rotate in the flush (no LVGL or esp_lcd transforms).
#define DBG_STRIPE_LINES      120      // wider bands -> fewer transfers
#define DBG_LOG_FLUSH         1
#define DBG_LOG_FLUSH_N       12

// Rotation direction (select exactly one)
#define ROTATE_CCW            1
#define ROTATE_CW             0

// Use custom Orbitron fonts (LVGL v8 font .c files must be in your sketch folder)
#define USE_ORBITRON          1
