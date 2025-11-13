#include "ui.h"
#include "fonts.h"

// ---------- Font selection (no external fonts required) ----------
#if defined(USE_ORBITRON_FONTS)
static const lv_font_t* FONT_SM = &orbitron_20_700;
static const lv_font_t* FONT_MD = &orbitron_32_800;
static const lv_font_t* FONT_LG = &orbitron_48_900;
#else
// Safe defaults: use LVGL's default font for all sizes (always available)
static const lv_font_t* FONT_SM = LV_FONT_DEFAULT;
static const lv_font_t* FONT_MD = LV_FONT_DEFAULT;
static const lv_font_t* FONT_LG = LV_FONT_DEFAULT;
#endif

// ---- Colors ----
static const lv_color_t COL_BG     = lv_color_hex(0x0A0F14);
static const lv_color_t COL_CARD   = lv_color_hex(0x121A21);
static const lv_color_t COL_TXT    = lv_color_hex(0xE6F0F2);
static const lv_color_t COL_CYAN   = lv_color_hex(0x00E0FF);
static const lv_color_t COL_ORANGE = lv_color_hex(0xFF9F43);
static const lv_color_t COL_GREEN  = lv_color_hex(0x2ECC71);
static const lv_color_t COL_MUTED  = lv_color_hex(0x7F8C8D);

// ---- Styles ----
static lv_style_t st_screen, st_statusbar, st_footer, st_card, st_title, st_value, st_label;

// ---- Roots ----
lv_obj_t* ui_root = nullptr;
static lv_obj_t* cont_status = nullptr;
static lv_obj_t* cont_grid   = nullptr;
static lv_obj_t* cont_footer = nullptr;

// ---- Page state (single page) ----
static int s_current_page = 0; // 0 .. ui_page_count()-1

// Utility: make a titled metric card
static lv_obj_t* make_metric_card(lv_obj_t* parent, const char* title, const char* value, lv_color_t accent)
{
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_add_style(card, &st_card, 0);

    // Accent bar on top
    lv_obj_t* bar = lv_obj_create(card);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, LV_PCT(100), 4);
    lv_obj_set_style_bg_color(bar, accent, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);

    // Title
    lv_obj_t* lbl_title = lv_label_create(card);
    lv_obj_add_style(lbl_title, &st_title, 0);
    lv_label_set_text(lbl_title, title);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 16, 12);

    // Value
    lv_obj_t* lbl_value = lv_label_create(card);
    lv_obj_add_style(lbl_value, &st_value, 0);
    lv_label_set_text(lbl_value, value);
    lv_obj_align(lbl_value, LV_ALIGN_CENTER, 0, 6);

    return card;
}

static void apply_styles()
{
    lv_style_init(&st_screen);
    lv_style_set_bg_color(&st_screen, COL_BG);
    lv_style_set_bg_opa(&st_screen, LV_OPA_COVER);

    lv_style_init(&st_statusbar);
    lv_style_set_bg_color(&st_statusbar, COL_CARD);
    lv_style_set_bg_opa(&st_statusbar, LV_OPA_COVER);
    lv_style_set_pad_left(&st_statusbar, 16);
    lv_style_set_pad_right(&st_statusbar, 16);
    lv_style_set_pad_top(&st_statusbar, 8);
    lv_style_set_pad_bottom(&st_statusbar, 8);

    lv_style_init(&st_footer);
    lv_style_set_bg_color(&st_footer, COL_CARD);
    lv_style_set_bg_opa(&st_footer, LV_OPA_COVER);
    lv_style_set_pad_left(&st_footer, 16);
    lv_style_set_pad_right(&st_footer, 16);
    lv_style_set_pad_top(&st_footer, 8);
    lv_style_set_pad_bottom(&st_footer, 8);

    lv_style_init(&st_card);
    lv_style_set_bg_color(&st_card, COL_CARD);
    lv_style_set_bg_opa(&st_card, LV_OPA_COVER);
    lv_style_set_radius(&st_card, 16);
    lv_style_set_outline_color(&st_card, lv_color_hex(0x10161C));
    lv_style_set_outline_width(&st_card, 1);
    lv_style_set_shadow_opa(&st_card, LV_OPA_40);
    lv_style_set_shadow_width(&st_card, 16);
    lv_style_set_shadow_color(&st_card, lv_color_hex(0x000000));
    lv_style_set_pad_all(&st_card, 8);

    lv_style_init(&st_title);
    lv_style_set_text_color(&st_title, COL_MUTED);
    lv_style_set_text_font(&st_title, FONT_SM);

    lv_style_init(&st_value);
    lv_style_set_text_color(&st_value, COL_TXT);
    lv_style_set_text_font(&st_value, FONT_LG);

    lv_style_init(&st_label);
    lv_style_set_text_color(&st_label, COL_TXT);
    lv_style_set_text_font(&st_label, FONT_MD);
}

static void build_status_bar(lv_obj_t* parent, lv_coord_t w)
{
    cont_status = lv_obj_create(parent);
    lv_obj_remove_style_all(cont_status);
    lv_obj_add_style(cont_status, &st_statusbar, 0);
    lv_obj_set_width(cont_status, w);
    lv_obj_set_height(cont_status, 56);
    lv_obj_align(cont_status, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* lbl_title = lv_label_create(cont_status);
    lv_obj_add_style(lbl_title, &st_label, 0);
    lv_label_set_text(lbl_title, "Marine Dashboard");
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* lbl_conn = lv_label_create(cont_status);
    lv_obj_add_style(lbl_conn, &st_label, 0);
    lv_label_set_text(lbl_conn, "CAN: OK  •  GPS: 3D  •  LOG: SD");
    lv_obj_align(lbl_conn, LV_ALIGN_RIGHT_MID, 0, 0);
}

static void build_footer(lv_obj_t* parent, lv_coord_t w, lv_coord_t h)
{
    LV_UNUSED(h);
    cont_footer = lv_obj_create(parent);
    lv_obj_remove_style_all(cont_footer);
    lv_obj_add_style(cont_footer, &st_footer, 0);
    lv_obj_set_width(cont_footer, w);
    lv_obj_set_height(cont_footer, 56);
    lv_obj_align(cont_footer, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t* lbl = lv_label_create(cont_footer);
    lv_obj_add_style(lbl, &st_label, 0);
    lv_label_set_text(lbl, "▲ Swipe for details  |  Long-press: settings");
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
}

static void build_grid(lv_obj_t* parent, lv_coord_t w, lv_coord_t h)
{
    const lv_coord_t top_h = 56;
    const lv_coord_t bot_h = 56;
    const lv_coord_t grid_w = w - 24;
    const lv_coord_t grid_h = h - (top_h + bot_h) - 24;

    cont_grid = lv_obj_create(parent);
    lv_obj_remove_style_all(cont_grid);
    lv_obj_add_style(cont_grid, &st_screen, 0);
    lv_obj_set_size(cont_grid, grid_w, grid_h);
    lv_obj_align(cont_grid, LV_ALIGN_CENTER, 0, (top_h - bot_h)/2);

    const bool portrait = (w < h);

    if (portrait) {
        static lv_coord_t cols_portrait[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t rows_portrait[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        lv_obj_set_grid_dsc_array(cont_grid, cols_portrait, rows_portrait);
    } else {
        static lv_coord_t cols_landscape[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t rows_landscape[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        lv_obj_set_grid_dsc_array(cont_grid, cols_landscape, rows_landscape);
    }
    lv_obj_set_style_pad_row(cont_grid, 12, 0);
    lv_obj_set_style_pad_column(cont_grid, 12, 0);
    lv_obj_set_style_pad_all(cont_grid, 12, 0);

    lv_obj_t* c_speed = make_metric_card(cont_grid, "SPEED", "7.4 kn", COL_CYAN);
    lv_obj_t* c_rpm   = make_metric_card(cont_grid, "ENGINE RPM", "1 350", COL_ORANGE);
    lv_obj_t* c_batt  = make_metric_card(cont_grid, "BATTERY", "78 %", COL_GREEN);
    lv_obj_t* c_wind  = make_metric_card(cont_grid, "WIND AWA", "14.2 kn", COL_CYAN);
    lv_obj_t* c_ap    = make_metric_card(cont_grid, "AUTOPILOT", "TRACK", COL_ORANGE);

    if (portrait) {
        lv_obj_set_grid_cell(c_speed, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 0, 1);
        lv_obj_set_grid_cell(c_rpm,   LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
        lv_obj_set_grid_cell(c_batt,  LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
        lv_obj_set_grid_cell(c_wind,  LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
        lv_obj_set_grid_cell(c_ap,    LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
    } else {
        lv_obj_set_grid_cell(c_speed, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 0, 1);
        lv_obj_set_grid_cell(c_rpm,   LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
        lv_obj_set_grid_cell(c_batt,  LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
        lv_obj_set_grid_cell(c_wind,  LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
        lv_obj_set_grid_cell(c_ap,    LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    }

    // Small badge on speed card
    lv_obj_t* badge = lv_label_create(c_speed);
    lv_obj_add_style(badge, &st_title, 0);
    lv_label_set_text(badge, "⚠ NO ALARMS");
    lv_obj_set_style_text_color(badge, COL_GREEN, 0);
    lv_obj_align(badge, LV_ALIGN_BOTTOM_RIGHT, -12, -10);
}

void ui_init(void)
{
    if (ui_root) return;

    lv_disp_t* disp = lv_disp_get_default();
    lv_coord_t w = lv_disp_get_hor_res(disp);
    lv_coord_t h = lv_disp_get_ver_res(disp);

    apply_styles();

    ui_root = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(ui_root);
    lv_obj_add_style(ui_root, &st_screen, 0);
    lv_obj_set_size(ui_root, w, h);
    lv_obj_center(ui_root);

    build_status_bar(ui_root, w);
    build_grid(ui_root, w, h);
    build_footer(ui_root, w, h);
}

/* ------- Compatibility shims ------- */
void ui_build_page1(void) {
    ui_init();
    s_current_page = 0;
}

int ui_get_current_page(void) {
    return s_current_page; // always 0 in single-page layout
}

int ui_page_count(void) {
    return 1;
}

void slide_to_page(int idx) {
    LV_UNUSED(idx);
    // Single-page: nothing to slide. Keep s_current_page = 0.
}
