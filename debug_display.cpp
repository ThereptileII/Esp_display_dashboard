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

static const char* TAG = "debug_display";

// ---------- LVGL plumbing ----------
static lv_disp_t* s_disp          = nullptr;
static lv_disp_draw_buf_t s_draw;
static lv_color_t* s_buf1         = nullptr;
static lv_color_t* s_buf2         = nullptr;

// Bounce buffer (DMA-capable internal RAM) used by our rotated flush
static uint16_t* s_bounce         = nullptr;
static int       s_stripe_lines   = 120;      // LV rows per sub-flush

// LVGL logical framebuffer (landscape). Panel is portrait 800x1280.
static constexpr int LOGICAL_W    = 1280;     // LV canvas width
static constexpr int LOGICAL_H    = 800;      // LV canvas height
static constexpr int PANEL_W      = 800;      // JD9365 native width (portrait)
static constexpr int PANEL_H      = 1280;     // JD9365 native height (portrait)

static void my_flush(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p);

// LV→Panel with 90° rotation: Xp = 799 - y,  Yp = x
static inline void map_lv_to_panel(int x_lv, int y_lv, int& x_p, int& y_p) {
  x_p = (PANEL_W - 1) - y_lv;
  y_p = x_lv;
}

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

  // 5 horizontal bands (logical), appear as 5 vertical color bars on panel.
  static const uint16_t colors[5] = { 0xF800, 0x07E0, 0x001F, 0xFFE0, 0xFFFF };

  // Row buffer in PSRAM (logical width)
  static uint16_t* line = (uint16_t*)heap_caps_malloc(LOGICAL_W * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
  if (!line) return -2;

  // Column buffer in DMA-capable internal RAM (panel height)
  static uint16_t* col = (uint16_t*)heap_caps_malloc(PANEL_H * sizeof(uint16_t),
                                                     MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!col) return -3;

  for (int b = 0; b < 5; ++b) {
    for (int y = 0; y < 40; ++y) {
      for (int x = 0; x < LOGICAL_W; ++x) line[x] = colors[b];

      // LV row → panel column
      const int y_lv = b * 40 + y;
      int x1p, y1p; map_lv_to_panel(0, y_lv, x1p, y1p);
      const int X1 = x1p, X2 = x1p + 1, Y1 = 0, Y2 = PANEL_H;

      // Build a 1×1280 column from the LV row
      for (int yp = 0; yp < PANEL_H; ++yp) col[yp] = line[yp];
      msync_c2m(col, PANEL_H * sizeof(uint16_t));
      esp_err_t e = draw_bitmap_retry(X1, Y1, X2, Y2, col);
      if (e != ESP_OK) {
        Serial.printf("[panel] sanity bar draw failed err=%d\n", (int)e);
        return -4;
      }
    }
  }
  Serial.println(F("[panel] sanity bar draw -> 0"));
  return 0;
}

bool dbg_display_init(void) {
  dbg_dump_env();

  // Bring up the JD9365 via your working wrapper (this sets 'panel_handle' globally)
  static jd9365_lcd lcd(/*lcd_rst=*/-1);
  lcd.begin();    // powers panel, sets mirror, backlight on

  if (!panel_handle) {
    Serial.println(F("[FATAL] display init failed: panel_handle is NULL"));
    return false;
  }

  // LVGL init and buffers
  lv_init();

  const int buf_lines = 120; // 1280×120×2 = 307,200 B per buffer
  s_buf1 = (lv_color_t*)heap_caps_malloc(LOGICAL_W * buf_lines * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
  s_buf2 = (lv_color_t*)heap_caps_malloc(LOGICAL_W * buf_lines * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

  s_bounce = (uint16_t*)heap_caps_malloc(PANEL_H * s_stripe_lines * sizeof(uint16_t),
                                         MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!s_buf1 || !s_buf2 || !s_bounce) {
    // Try smaller stripes
    if (s_bounce) { free(s_bounce); s_bounce = nullptr; }
    s_stripe_lines = 60;
    s_bounce = (uint16_t*)heap_caps_malloc(PANEL_H * s_stripe_lines * sizeof(uint16_t),
                                           MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }

  size_t int_free  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  size_t int_big   = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  size_t dma_free  = heap_caps_get_free_size(MALLOC_CAP_DMA);
  size_t dma_big   = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
  size_t psram_free= heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

  if (s_buf1 && s_buf2 && s_bounce) {
    Serial.printf("[alloc] LVGL PSRAM buffers OK: %d bytes each, bounce=%d bytes\n",
                  LOGICAL_W * buf_lines * (int)sizeof(lv_color_t),
                  PANEL_H * s_stripe_lines * (int)sizeof(uint16_t));
  }
  Serial.printf("[mem][post-alloc] INT free=%u big=%u | DMA free=%u big=%u | PSRAM free=%u\n",
                (unsigned)int_free, (unsigned)int_big, (unsigned)dma_free, (unsigned)dma_big, (unsigned)psram_free);

  lv_disp_draw_buf_init(&s_draw, s_buf1, s_buf2, LOGICAL_W * buf_lines);

  static lv_disp_drv_t drv;
  lv_disp_drv_init(&drv);
  drv.hor_res   = LOGICAL_W;
  drv.ver_res   = LOGICAL_H;
  drv.flush_cb  = my_flush;
  drv.draw_buf  = &s_draw;
  s_disp = lv_disp_drv_register(&drv);

  Serial.printf("[display] ready. stripe_lines=%d (LV logical)\n", s_stripe_lines);
  return true;
}

// ============ LVGL flush with 90° rotation ============
static void my_flush(lv_disp_drv_t* drv, const lv_area_t* a, lv_color_t* color_p) {
  (void)drv;
  if (!panel_handle) { lv_disp_flush_ready(drv); return; }

  int x1 = a->x1, y1 = a->y1, x2 = a->x2, y2 = a->y2;
  if (x2 < x1 || y2 < y1) { lv_disp_flush_ready(drv); return; }

  const int dest_w = (y2 - y1 + 1);
  const int dest_h = (x2 - x1 + 1);

  // LV rows → panel columns across full height
  const int rows_total = (y2 - y1 + 1);
  const int rows_step  = s_stripe_lines > 0 ? s_stripe_lines : rows_total;

  static uint32_t flush_count = 0;

  for (int row0 = 0; row0 < rows_total; row0 += rows_step) {
    const int rows = (row0 + rows_step <= rows_total) ? rows_step : (rows_total - row0);

    // Panel X for this sub-rect
    int xs = (PANEL_W - 1) - (y1 + row0 + rows - 1);
    int xe = (PANEL_W - 1) - (y1 + row0) + 1;     // exclusive
    const int Y1 = x1;
    const int Y2 = x2 + 1;                        // exclusive

    // Build bounce [rows × dest_h] in panel row-major order
    const int inner_w = rows;
    const int inner_h = dest_h;

    // Fill bounce: LV (x_lv, y_lv) → panel (xp, yp)
    uint16_t* dst = s_bounce;
    for (int j = 0; j < inner_h; ++j) {
      for (int i = 0; i < inner_w; ++i) {
        const int y_lv = y1 + row0 + i;
        const int x_lv = x1 + j;
        const lv_color_t px = *(color_p + (y_lv - y1) * (x2 - x1 + 1) + (x_lv - x1));
        *dst++ = px.full; // RGB565
      }
    }

    msync_c2m(s_bounce, inner_w * inner_h * sizeof(uint16_t));
    Serial.printf("[flush] #%u LV a=(%d,%d)-(%d,%d) w=%d h=%d -> PANEL rect x=[%d..%d] y=[%d..%d] (dw=%d dh=%d)\n",
                  flush_count++,
                  a->x1, a->y1, a->x2, a->y2, (a->x2 - a->x1 + 1), (a->y2 - a->y1 + 1),
                  xs, xe - 1, Y1, Y2 - 1, (xe - xs), (Y2 - Y1));

    (void) draw_bitmap_retry(xs, Y1, xe, Y2, s_bounce);
  }

  lv_disp_flush_ready(drv);
}
