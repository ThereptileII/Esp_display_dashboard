// gestures.cpp
#include "gestures.h"
#include "ui.h"
#include <lvgl.h>


static lv_obj_t* s_capture = nullptr;
static lv_point_t s_down = {0,0};

static void on_input_event(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_indev_t* indev = lv_indev_get_act();
  if (!indev) return;

  lv_point_t p; lv_indev_get_point(indev, &p);

  switch (code) {
    case LV_EVENT_PRESSED:
      s_down = p;
      break;
    case LV_EVENT_RELEASED: {
      int dx = p.x - s_down.x;
      int dy = p.y - s_down.y;
      if (LV_ABS(dx) > 80 && LV_ABS(dx) > LV_ABS(dy) + 20) {
        int cur = ui_get_current_page();
        if (dx < 0) slide_to_page(cur + 1);  // left → next
        else        slide_to_page(cur - 1);  // right → prev
      }
      break;
    }
    default: break;
  }
}

void gestures_attach_to_root() {
  lv_obj_t* root = lv_scr_act();
  if (!root) return;

  if (!s_capture) {
    s_capture = lv_obj_create(root);
    lv_obj_remove_style_all(s_capture);
    lv_obj_set_size(s_capture, lv_obj_get_width(root), lv_obj_get_height(root));
    lv_obj_set_pos(s_capture, 0, 0);
    lv_obj_add_flag(s_capture, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_capture, on_input_event, LV_EVENT_ALL, nullptr);
    lv_obj_move_foreground(s_capture);
    lv_obj_set_style_bg_opa(s_capture, LV_OPA_TRANSP, 0);
  } else {
    lv_obj_set_parent(s_capture, root);
    lv_obj_set_size(s_capture, lv_obj_get_width(root), lv_obj_get_height(root));
    lv_obj_move_foreground(s_capture);
  }
}
