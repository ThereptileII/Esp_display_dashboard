JD9365_LVGL_TextDebug_Min — minimal LVGL v8 text rendering and logging harness

Vendor files required in the SAME folder:
- esp_lcd_jd9365.c
- esp_lcd_jd9365.h
- jd9365_lcd.cpp
- jd9365_lcd.h

Board: ESP32 P4 Dev Module
PSRAM: Enabled
Flash: 16MB QIO 80 MHz (typical)
LVGL: v8.x

What it does:
- Initializes JD9365 via vendor wrapper (panel_handle)
- Sets LVGL display default, software rotate to landscape
- Draws a raw RGB565 sanity bar at top
- Creates a single label centered; toggles text/color every second
- Logs LVGL version, color depth, flushes, and label metrics
