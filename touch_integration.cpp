#include "touch_integration.h"
#include <Arduino.h>
#include <lvgl.h>

// Your vendor driver
#include "gsl3680_touch.h"

// Optional pin map header (works with or without)
#if __has_include("pins_config.h")
  #include "pins_config.h"
#endif

// ---- Pin auto-detect (override any of these in your pins_config.h) ----
#if !defined(TP_I2C_SDA)
  #if defined(TOUCH_SDA)
    #define TP_I2C_SDA TOUCH_SDA
  #elif defined(BOARD_TOUCH_SDA)
    #define TP_I2C_SDA BOARD_TOUCH_SDA
  #else
    #define TP_I2C_SDA 18   // fallback; change if needed
  #endif
#endif

#if !defined(TP_I2C_SCL)
  #if defined(TOUCH_SCL)
    #define TP_I2C_SCL TOUCH_SCL
  #elif defined(BOARD_TOUCH_SCL)
    #define TP_I2C_SCL BOARD_TOUCH_SCL
  #else
    #define TP_I2C_SCL 17
  #endif
#endif

#if !defined(TP_RST)
  #if defined(TOUCH_RST)
    #define TP_RST TOUCH_RST
  #elif defined(BOARD_TOUCH_RST)
    #define TP_RST BOARD_TOUCH_RST
  #else
    #define TP_RST -1       // many boards don’t wire reset; -1 = unused
  #endif
#endif

#if !defined(TP_INT)
  #if defined(TOUCH_INT)
    #define TP_INT TOUCH_INT
  #elif defined(BOARD_TOUCH_INT)
    #define TP_INT BOARD_TOUCH_INT
  #else
    #define TP_INT -1       // optional IRQ
  #endif
#endif
// ----------------------------------------------------------------------

static bool          s_verbose = true;
static lv_indev_t*   s_indev   = nullptr;
static uint8_t       s_rot     = 1;    // default: swap XY (portrait panel -> LVGL landscape)
static uint16_t      s_w       = 1280; // LVGL logical width
static uint16_t      s_h       = 800;  // LVGL logical height

// Construct with required pins (your header shows this ctor signature)
static gsl3680_touch s_touch(TP_I2C_SDA, TP_I2C_SCL, TP_RST, TP_INT);

void touch_set_verbose(bool v) { s_verbose = v; }

void touch_set_rotation(uint8_t r) {
  s_rot = (r & 3);
  s_touch.set_rotation(s_rot);
  if (s_verbose) Serial.printf("[touch] set_rotation(%u)\n", s_rot);
}

static void touch_read_cb(lv_indev_drv_t* indev, lv_indev_data_t* data) {
  (void)indev;

  uint16_t rx = 0, ry = 0;
  bool pressed = s_touch.getTouch(&rx, &ry);  // vendor API: returns true while touching

  // Clamp to LVGL logical bounds (s_touch already applies rotation internally)
  uint16_t lx = rx, ly = ry;
  if (lx >= s_w) lx = s_w ? (s_w - 1) : 0;
  if (ly >= s_h) ly = s_h ? (s_h - 1) : 0;

  data->state   = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  data->point.x = lx;
  data->point.y = ly;

  if (s_verbose) {
    static uint32_t last = 0;
    uint32_t now = millis();
    if (now - last > 150) {
      Serial.printf("[touch] raw=(%u,%u) -> lv=(%u,%u) pressed=%d rot=%u\n",
                    rx, ry, lx, ly, (int)pressed, s_rot);
      last = now;
    }
  }
}

bool touch_init_and_register(lv_disp_t* disp) {
  if (!disp) {
    Serial.println("[touch] ERROR: disp is null");
    return false;
  }

  // Determine LVGL logical resolution (your code runs 1280x800 logical)
  s_w = lv_disp_get_hor_res(disp);
  s_h = lv_disp_get_ver_res(disp);
  Serial.printf("[touch] LVGL logical size: %ux%u\n", s_w, s_h);

  // Vendor init
  Serial.printf("[touch] begin(sda=%d scl=%d rst=%d int=%d)\n", TP_I2C_SDA, TP_I2C_SCL, TP_RST, TP_INT);
  s_touch.begin();

  // Start with swapXY (1). If L↔R still inverted, try 2 or 3 in setup().
  touch_set_rotation(1);

  // Register LVGL input device
  lv_indev_drv_t drv;
  lv_indev_drv_init(&drv);
  drv.type    = LV_INDEV_TYPE_POINTER;  // LVGL v8; no indev “mode” API here
  drv.read_cb = touch_read_cb;
  drv.disp    = disp;

  s_indev = lv_indev_drv_register(&drv);
  if (!s_indev) {
    Serial.println("[touch] ERROR: lv_indev_drv_register failed");
    return false;
  }

  Serial.println("[touch] registered OK");
  return true;
}
