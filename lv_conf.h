/**
 * Minimal lv_conf.h for LVGL v8.4
 * - RGB565 (LV_COLOR_DEPTH=16)
 * - Logical resolution: 1280x800 (your flush maps it to panel portrait 800x1280)
 * - Custom fonts: Orbitron (16/20/32/48), BPP=4, LARGE tables enabled
 * - UTF-8 text; no compression; no subpixel; no theme overriding
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   LVGL HAL SETTINGS
 *====================*/

/* Use your app's tick (Arduino loop calls lv_timer_handler), so default is fine. */
#define LV_TICK_CUSTOM              0

/*===========================
   DISPLAY RESOLUTION (LOGICAL)
 *===========================*/
#define LV_HOR_RES_MAX              1280
#define LV_VER_RES_MAX              800

/*=================
   COLOR SETTINGS
 *=================*/
#define LV_COLOR_DEPTH              16
#define LV_COLOR_16_SWAP            0
#define LV_COLOR_SCREEN_TRANSP      0

/*=================
   MISC SETTINGS
 *=================*/
#define LV_MEM_CUSTOM               0
#define LV_USE_ASSERT_NULL          0
#define LV_USE_ASSERT_MALLOC        0
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/*=================
   LOGGING
 *=================*/
#define LV_USE_LOG                  1
#define LV_LOG_LEVEL                LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF               1
/*==================
 *   FONT USAGE
 *==================*/

/* Built-in font (keep at least one small Montserrat for control tests) */
#define LV_FONT_MONTSERRAT_14       1

/* Declare your custom fonts so LVGL knows them */
#undef  LV_FONT_CUSTOM_DECLARE
#define LV_FONT_CUSTOM_DECLARE            \
  LV_FONT_DECLARE(orbitron_16_600)        \
  LV_FONT_DECLARE(orbitron_20_700)        \
  LV_FONT_DECLARE(orbitron_32_800)        \
  LV_FONT_DECLARE(orbitron_48_900)

/* Keep the global default as Montserrat while debugging so the control label always renders */
#undef  LV_FONT_DEFAULT
#define LV_FONT_DEFAULT              &lv_font_montserrat_14

/* Text format engine + large tables + BPP=4 (your fonts were generated at 4bpp) */
#if !defined(LV_USE_FONT_FMT_TXT)
#  define LV_USE_FONT_FMT_TXT        1
#endif
#undef  LV_FONT_FMT_TXT_LARGE
#define LV_FONT_FMT_TXT_LARGE        1
#undef  LV_FONT_FMT_TXT_BPP
#define LV_FONT_FMT_TXT_BPP          4

/* *** THE IMPORTANT BIT: your Orbitron .c files are compressed *** */
#undef  LV_USE_FONT_COMPRESSED
#define LV_USE_FONT_COMPRESSED       1

/* Subpixel OFF on RGB565 panels */
#undef  LV_USE_FONT_SUBPX
#define LV_USE_FONT_SUBPX            0

/*=================
 *  TEXT SETTINGS
 *=================*/
#define LV_TXT_ENC                   LV_TXT_ENC_UTF8



/*=================
 *  WIDGETS
 *=================*/
#define LV_USE_ARC                   1
#define LV_USE_BAR                   1
#define LV_USE_BTN                   1
#define LV_USE_LABEL                 1
#define LV_USE_CANVAS                1
#define LV_USE_IMG                   1
#define LV_USE_LINE                  1
#define LV_USE_OBJ                   1
#define LV_USE_RECT                  1

/*=================
 * THEMES (disable to avoid overriding your styles)
 *=================*/
#define LV_USE_THEME_DEFAULT         0

/*=================
 * GPU / RENDER
 *=================*/
#define LV_USE_GPU                   0
#define LV_USE_DRAW_SW               1

/*=================
 * FILE SYSTEM (optional)
 *=================*/
#define LV_USE_FS_STDIO              0

/*=================
 * MISC DRIVERS
 *=================*/
#define LV_USE_MONKEY                0
#define LV_USE_GRIDNAV               0

/*=================
 * EXTRAS
 *=================*/
#define LV_USE_PERF_MONITOR          0
#define LV_USE_MEM_MONITOR           0


/*=================
 * FRAMERATE
 *=================*/
#define LV_DISP_DEF_REFR_PERIOD   16  // ~60 FPS


#endif /* LV_CONF_H */
