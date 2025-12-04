#include "debug_display.h"

#include <string.h>
#include "Arduino.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_heap_caps.h"

// IDF LCD panel headers (declare handle + draw API)
#include <esp_lcd_panel_interface.h>
#include <esp_lcd_panel_ops.h>

// Cache msync (IDF v5.x). If not present, no-op safely.
#if __has_include(<esp_cache.h>)
  #include <esp_cache.h>
  static inline void msync_c2m(const void* p, size_t n) {
    esp_cache_msync((void*)p, n, ESP_CACHE_MSYNC_FLAG_DIR_C2M); // cast away const per API
  }
#else
  static inline void msync_c2m(const void*, size_t) {}
#endif

// Your working JD9365 wrapper (unchanged from the version that worked)
#include "jd9365_lcd.h"

// panel_handle is defined in jd9365_lcd.cpp (global there)
extern "C" {
  extern esp_lcd_panel_handle_t panel_handle;
}

// ---------- LVGL plumbing ----------
static lv_disp_t* s_disp          = nullptr;
static lv_disp_draw_buf_t s_draw;
static lv_color_t* s_buf1         = nullptr;
static lv_color_t* s_buf2         = nullptr;
static lv_color_t* s_rotated_full = nullptr;   // Full-screen rotated frame (panel orientation)

// LVGL logical framebuffer is landscape (1280x800); panel is portrait
// (800x1280). We rotate 90° counter-clockwise in the flush callback.
static constexpr int PANEL_W      = 800;      // JD9365 native width (portrait)
static constexpr int PANEL_H      = 1280;     // JD9365 native height (portrait)
static constexpr int LOGICAL_W    = PANEL_H;  // LV canvas width (landscape)
static constexpr int LOGICAL_H    = PANEL_W;  // LV canvas height (landscape)

static void my_flush(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p);

// Lightweight retry wrapper for draw (throttled)
static esp_err_t draw_bitmap_retry(int x1, int y1, int x2, int y2, const void* data) {
  if (!panel_handle) return ESP_ERR_INVALID_STATE;

  // Try a few times; if panel reports "previous draw not finished", yield briefly.
  for (int attempt = 0; attempt < 12; ++attempt) {
    esp_err_t err = esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2, y2, data);
    if (err == ESP_OK) return ESP_OK;
    if (err == ESP_ERR_INVALID_STATE) {
      // Prior draw still in progress – give DPI driver time to catch up
      // Use a short delay; larger stripes need a bit more time.
      delay(2);     // yields to RTOS; ~2 ms works well with 60–120 line stripes
      continue;
    }
    // Any other error: bubble it up
    return err;
  }
  return ESP_FAIL;
}

// ===== Public helpers =====

lv_disp_t* dbg_lvgl_display(void) { return s_disp; }

void dbg_dump_env(void) {
  Serial.println(F("=== JD9365 Dashboard bring-up ==="));
  Serial.print(F("LVGL v")); Serial.print(LVGL_VERSION_MAJOR); Serial.print('.');
  Serial.print(LVGL_VERSION_MINOR); Serial.print('.'); Serial.println(LVGL_VERSION_PATCH);
  Serial.print(F("LV_COLOR_DEPTH=")); Serial.print(LV_COLOR_DEPTH);
  Serial.print(F(", sizeof(lv_color_t)=")); Serial.println(sizeof(lv_color_t));

  size_t int_free  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  size_t int_big   = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  size_t dma_free  = heap_caps_get_free_size(MALLOC_CAP_DMA);
  size_t dma_big   = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
  size_t psram_free= heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

  Serial.printf("[mem][before-alloc] INT free=%u big=%u | DMA free=%u big=%u | PSRAM free=%u\n",
                (unsigned)int_free, (unsigned)int_big, (unsigned)dma_free, (unsigned)dma_big, (unsigned)psram_free);
}

int dbg_panel_sanity_pattern(void) {
  if (!panel_handle) {
    Serial.println(F("[display] panel_handle is null (not initialized)."));
    return -1;
  }

  // 5 horizontal bands that cover the screen top to bottom.
  static const uint16_t colors[5] = { 0xF800, 0x07E0, 0x001F, 0xFFE0, 0xFFFF };

  const int stripe_h = LOGICAL_H / 5;
  uint16_t* stripe = (uint16_t*)heap_caps_malloc(LOGICAL_W * stripe_h * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!stripe) return -2;

  for (int b = 0; b < 5; ++b) {
    for (int px = 0; px < LOGICAL_W * stripe_h; ++px) {
      stripe[px] = colors[b];
    }
    msync_c2m(stripe, LOGICAL_W * stripe_h * sizeof(uint16_t));

    const int y0 = b * stripe_h;
    const int y1 = (b == 4) ? LOGICAL_H : (y0 + stripe_h);
    esp_err_t e = draw_bitmap_retry(0, y0, LOGICAL_W, y1, stripe);
    if (e != ESP_OK) {
      Serial.printf("[panel] sanity band draw failed err=%d\n", (int)e);
      free(stripe);
      return -3;
    }
  }

  free(stripe);
  Serial.println(F("[panel] sanity band draw -> 0"));
  return 0;
}

bool dbg_display_init(void) {
  dbg_dump_env();

  // Bring up the JD9365 via your working wrapper (this sets 'panel_handle' globally)
  static jd9365_lcd lcd(/*lcd_rst=*/-1);
  lcd.begin();    // powers panel and backlight

  if (!panel_handle) {
    Serial.println(F("[FATAL] display init failed: panel_handle is NULL"));
    return false;
  }

  // LVGL init and buffers
  lv_init();

  const size_t buf_pixels = LOGICAL_W * LOGICAL_H; // Full-screen buffers
  s_buf1 = (lv_color_t*)heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  s_buf2 = (lv_color_t*)heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  // Rotate the full LVGL frame (landscape) into panel orientation (portrait) in one shot
  s_rotated_full = (lv_color_t*)heap_caps_malloc(PANEL_W * PANEL_H * sizeof(lv_color_t),
                                                 MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);

  size_t int_free  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  size_t int_big   = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  size_t dma_free  = heap_caps_get_free_size(MALLOC_CAP_DMA);
  size_t dma_big   = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
  size_t psram_free= heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

  if (s_buf1 && s_buf2 && s_rotated_full) {
    Serial.printf("[alloc] LVGL PSRAM buffers OK: %d bytes each, rotated frame=%d bytes\n",
                  (int)(buf_pixels * sizeof(lv_color_t)),
                  (int)(PANEL_W * PANEL_H * sizeof(lv_color_t)));
  } else {
    Serial.println(F("[alloc][FATAL] LVGL buffers could not be allocated"));
    return false;
  }
  Serial.printf("[mem][post-alloc] INT free=%u big=%u | DMA free=%u big=%u | PSRAM free=%u\n",
                (unsigned)int_free, (unsigned)int_big, (unsigned)dma_free, (unsigned)dma_big, (unsigned)psram_free);

  lv_disp_draw_buf_init(&s_draw, s_buf1, s_buf2, buf_pixels);

  static lv_disp_drv_t drv;
  lv_disp_drv_init(&drv);
  drv.hor_res   = LOGICAL_W;
  drv.ver_res   = LOGICAL_H;
  drv.flush_cb  = my_flush;
  drv.draw_buf  = &s_draw;
  drv.full_refresh = 1; // Always flush a whole frame
  s_disp = lv_disp_drv_register(&drv);

  Serial.println(F("[display] ready. full-frame double buffering enabled"));
  return true;
}

// ============ LVGL flush with 90° CCW rotation (landscape -> portrait) ============
static void my_flush(lv_disp_drv_t* drv, const lv_area_t* a, lv_color_t* color_p) {
  (void)drv;
  if (!panel_handle) { lv_disp_flush_ready(drv); return; }

  const int x1 = a->x1, y1 = a->y1, x2 = a->x2, y2 = a->y2;
  if (x2 < x1 || y2 < y1 || !s_rotated_full) { lv_disp_flush_ready(drv); return; }

  // Expect a full-frame refresh (full_refresh=1). If not, we still handle the area but log once.
  const int src_w = (x2 - x1 + 1);  // LV logical width of the area
  const int src_h = (y2 - y1 + 1);  // LV logical height of the area
  const bool full_frame = (x1 == 0 && y1 == 0 && src_w == LOGICAL_W && src_h == LOGICAL_H);

  // Rotate 90° CCW: (x, y) -> (y, LOGICAL_W - 1 - x)
  const int dest_w = PANEL_W;             // panel width (portrait)
  const int dest_h = PANEL_H;             // panel height (portrait)

  lv_color_t* dst = s_rotated_full;
  for (int sy = 0; sy < src_h; ++sy) {
    const lv_color_t* src_row = color_p + (sy * src_w);
    for (int sx = 0; sx < src_w; ++sx) {
      const int dx = y1 + sy;                 // LV y becomes panel x
      const int dy = LOGICAL_W - 1 - (x1 + sx); // LV x becomes panel y (inverted)
      dst[dy * dest_w + dx] = src_row[sx];
    }
  }

  msync_c2m(s_rotated_full, dest_w * dest_h * sizeof(lv_color_t));

  static uint32_t flush_count = 0;
  Serial.printf("[flush] #%lu LV a=(%d,%d)-(%d,%d) w=%d h=%d -> PANEL full-frame=%s\n",
                static_cast<unsigned long>(flush_count++),
                a->x1, a->y1, a->x2, a->y2, src_w, src_h,
                full_frame ? "yes" : "no");

  (void) draw_bitmap_retry(0, 0, dest_w, dest_h, s_rotated_full);

  lv_disp_flush_ready(drv);
}
