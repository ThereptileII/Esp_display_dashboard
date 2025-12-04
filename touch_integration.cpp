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

// Portrait dashboards for this board have historically required a 90° swap/invert
// combo (rotation=1 in the working snippet). Let users override via a macro but
// keep the proven mapping by default.
#ifndef TOUCH_DEFAULT_ROTATION
#  define TOUCH_DEFAULT_ROTATION 1
#endif

static bool          s_verbose   = true;
static lv_indev_t*   s_indev     = nullptr;
static uint8_t       s_rot       = TOUCH_DEFAULT_ROTATION;
static uint16_t      s_w         = 800;  // Updated at init from LVGL display
static uint16_t      s_h         = 1280; // Updated at init from LVGL display
static lv_disp_t*    s_disp      = nullptr;
static lv_obj_t*     s_touch_dot = nullptr;

// Construct with required pins (your header shows this ctor signature)
static gsl3680_touch s_touch(TP_I2C_SDA, TP_I2C_SCL, TP_RST, TP_INT);

void touch_set_verbose(bool v) { s_verbose = v; }

void touch_set_rotation(uint8_t r) {
  s_rot = (r & 3);
  s_touch.set_rotation(s_rot);
  if (s_verbose) Serial.printf("[touch] set_rotation(%u)\n", s_rot);
}

static void ensure_touch_indicator() {
  if (!s_disp || s_touch_dot) return;

  lv_obj_t* layer = lv_disp_get_layer_top(s_disp);
  s_touch_dot     = lv_obj_create(layer);
  lv_obj_remove_style_all(s_touch_dot);
  lv_obj_set_size(s_touch_dot, 18, 18);
  lv_obj_set_style_radius(s_touch_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(s_touch_dot, lv_color_hex(0xFF3B30), 0);
  lv_obj_set_style_bg_opa(s_touch_dot, LV_OPA_COVER, 0);
  lv_obj_set_style_outline_color(s_touch_dot, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_outline_width(s_touch_dot, 2, 0);
  lv_obj_add_flag(s_touch_dot, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(s_touch_dot);
}

static void touch_read_cb(lv_indev_drv_t* indev, lv_indev_data_t* data) {
  (void)indev;

  static uint16_t last_x = 0, last_y = 0;

  uint16_t rx = 0, ry = 0;
  bool pressed = s_touch.getTouch(&rx, &ry);  // vendor API: returns true while touching

  // Apply the same transform sequence that worked in the earlier snippet.
  uint16_t lx = rx, ly = ry;

#if TOUCH_SWAP_XY
  {
    uint16_t t = lx;
    lx         = ly;
    ly         = t;
  }
#endif

  // Clamp to LVGL logical bounds (s_touch already applies rotation internally)
  if (lx >= s_w) lx = s_w ? (s_w - 1) : 0;
  if (ly >= s_h) ly = s_h ? (s_h - 1) : 0;

#if TOUCH_INVERT_X
  lx = (s_w > 0) ? (s_w - 1 - lx) : lx;
#endif
#if TOUCH_INVERT_Y
  ly = (s_h > 0) ? (s_h - 1 - ly) : ly;
#endif

  if (pressed) {
    last_x = lx;
    last_y = ly;
    data->state   = LV_INDEV_STATE_PRESSED;
    data->point.x = lx;
    data->point.y = ly;
  } else {
    data->state   = LV_INDEV_STATE_RELEASED;
    data->point.x = last_x;
    data->point.y = last_y;
  }

  ensure_touch_indicator();
  if (s_touch_dot) {
    lv_coord_t dot_w = lv_obj_get_width(s_touch_dot);
    lv_coord_t dot_h = lv_obj_get_height(s_touch_dot);
    lv_obj_set_pos(s_touch_dot, lx - dot_w / 2, ly - dot_h / 2);
    if (pressed) {
      lv_obj_clear_flag(s_touch_dot, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(s_touch_dot);
    } else {
      lv_obj_add_flag(s_touch_dot, LV_OBJ_FLAG_HIDDEN);
    }
  }

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

  // Determine LVGL logical resolution (now matching the panel's 800x1280 portrait canvas)
  s_w = lv_disp_get_hor_res(disp);
  s_h = lv_disp_get_ver_res(disp);
  Serial.printf("[touch] LVGL logical size: %ux%u\n", s_w, s_h);

  s_disp = disp;

  // Vendor init
  Serial.printf("[touch] begin(sda=%d scl=%d rst=%d int=%d)\n", TP_I2C_SDA, TP_I2C_SCL, TP_RST, TP_INT);
  s_touch.begin();

  // Start with the orientation that matched the working example (can be overridden).
  touch_set_rotation(TOUCH_DEFAULT_ROTATION);

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
  ensure_touch_indicator();
  return true;
}
