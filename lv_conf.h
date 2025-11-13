#ifndef LV_CONF_H
#define LV_CONF_H

/* This configuration is tailored for the ESP JD9365 dashboard sketch.
 * Only the options that differ from the LVGL defaults are listed here
 * to keep the file compact and easy to maintain.
 */

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   0

/*====================
   DISPLAY SETTINGS
 *====================*/
#define LV_HOR_RES_MAX     1280
#define LV_VER_RES_MAX     800
#define LV_COLOR_SCREEN_TRANSP 0

/* Use double buffering as we allocate two draw buffers ourselves. */
#define LV_DRAW_BUF_STRIDE_ALIGN 4

/*====================
   STDLIB WRAPPERS
 *====================*/
#define LV_MEM_CUSTOM      0
#define LV_MEM_SIZE        (48U * 1024U)

/*====================
   FEATURE CONFIGURATION
 *====================*/
#define LV_USE_LOG         1
#define LV_LOG_LEVEL       LV_LOG_LEVEL_WARN

#define LV_USE_FONT_SUBPX  1
#define LV_FONT_DEFAULT    &lv_font_montserrat_20
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_32 1

#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1

#define LV_USE_ANIMATION   1
#define LV_USE_TRANSFORM   1
#define LV_USE_USER_DATA   1

#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MEM  1

/* Enable the widgets that are actually used by the UI (labels, containers, grid) */
#define LV_USE_LABEL       1
#define LV_USE_OBJ         1
#define LV_USE_FLEX        1
#define LV_USE_GRID        1

#endif /* LV_CONF_H */
