#include "ui.h"
#include "fonts.h"
#include "debug_config.h"
#include <cstdio>

// ---------- Font selection (no external fonts required) ----------
#if defined(USE_ORBITRON) || defined(USE_ORBITRON_FONTS)
static const lv_font_t* FONT_SM = &orbitron_20_700;
static const lv_font_t* FONT_MD = &orbitron_32_800;
static const lv_font_t* FONT_LG = &orbitron_48_900;
#else
// Safe defaults: use LVGL's default font for all sizes (always available)
static const lv_font_t* FONT_SM = LV_FONT_DEFAULT;
static const lv_font_t* FONT_MD = LV_FONT_DEFAULT;
static const lv_font_t* FONT_LG = LV_FONT_DEFAULT;
#endif

// ---- Colors (Design_guidlines) ----
static const lv_color_t COL_BG_MAIN    = lv_color_hex(0x101319);
static const lv_color_t COL_BG_CARD    = lv_color_hex(0x171B23);
static const lv_color_t COL_CYAN       = lv_color_hex(0x1FCEF9);
static const lv_color_t COL_GREEN      = lv_color_hex(0x11D411);
static const lv_color_t COL_ORANGE     = lv_color_hex(0xF99E1F);
static const lv_color_t COL_WHITE      = lv_color_hex(0xFFFFFF);
static const lv_color_t COL_MUTED_TEXT = lv_color_hex(0x8493A8);
static const lv_color_t COL_GRAPH_GRID = lv_color_hex(0x202633);

// ---- Styles ----
static lv_style_t st_screen, st_navbar, st_card, st_caption, st_value_big, st_value_medium, st_label;
static lv_style_t st_chip, st_chip_checked, st_chip_ghost, st_chart_bg, st_tabstrip;

static lv_coord_t g_screen_w = 0;
static lv_coord_t g_screen_h = 0;

// ---- Roots ----
lv_obj_t* ui_root = nullptr;
static lv_obj_t* cont_navbar = nullptr;
static lv_obj_t* cont_grid   = nullptr;
static lv_obj_t* card_rpm = nullptr;
static lv_obj_t* cont_rpm_detail = nullptr;
static lv_obj_t* btn_resolutions[3] = {nullptr, nullptr, nullptr};
static lv_obj_t* btn_back = nullptr;
static lv_obj_t* chart_rpm = nullptr;
static lv_chart_series_t* chart_series_rpm = nullptr;
static lv_obj_t* lbl_axis_x = nullptr;
static lv_obj_t* lbl_axis_y = nullptr;
static lv_obj_t* lbl_stat_current = nullptr;
static lv_obj_t* lbl_stat_avg = nullptr;
static lv_obj_t* lbl_stat_max = nullptr;
static lv_obj_t* lbl_stat_min = nullptr;

// ---- Forward declarations ----
static void show_rpm_detail(bool show);
static void set_rpm_resolution(int idx);
static lv_obj_t* make_chip_button(lv_obj_t* parent,
                                 const char* text,
                                 lv_event_cb_t cb,
                                 void* user_data,
                                 lv_style_t* base_style = &st_chip,
                                 lv_style_t* checked_style = &st_chip_checked,
                                 bool checkable = true);
static lv_obj_t* make_detail_stat_tile(lv_obj_t* parent,
                                       const char* caption,
                                       const char* value,
                                       lv_color_t value_color,
                                       lv_obj_t** out_value_label = nullptr);

// ---- Page state (single page) ----
static int s_current_page = 0; // 0 .. ui_page_count()-1

// Utility: make a titled metric card
static lv_obj_t* make_metric_card(lv_obj_t* parent, const char* title, const char* value, lv_color_t accent, bool tall_value = false)
{
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_add_style(card, &st_card, 0);

    // Title
    lv_obj_t* lbl_title = lv_label_create(card);
    lv_obj_add_style(lbl_title, &st_caption, 0);
    lv_label_set_text(lbl_title, title);
    lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lbl_title, LV_PCT(100));
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 12, 8);

    // Value
    lv_obj_t* lbl_value = lv_label_create(card);
    lv_obj_add_style(lbl_value, tall_value ? &st_value_big : &st_value_medium, 0);
    lv_label_set_text(lbl_value, value);
    lv_label_set_long_mode(lbl_value, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lbl_value, LV_PCT(100));
    lv_obj_align(lbl_value, LV_ALIGN_LEFT_MID, 12, 8);

    // Accent divider
    lv_obj_t* divider = lv_obj_create(card);
    lv_obj_remove_style_all(divider);
    lv_obj_set_style_bg_color(divider, accent, 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_80, 0);
    lv_obj_set_size(divider, LV_PCT(100), 3);
    lv_obj_align(divider, LV_ALIGN_BOTTOM_MID, 0, -6);

    return card;
}

static void apply_styles()
{
    const lv_coord_t min_side = (g_screen_w < g_screen_h) ? g_screen_w : g_screen_h;
    const lv_coord_t outer_pad = (min_side <= 320) ? 8 : ((min_side <= 480) ? 12 : 24);
    const lv_coord_t card_pad = (min_side <= 320) ? 10 : ((min_side <= 480) ? 12 : 16);

    lv_disp_t* disp = lv_disp_get_default();
    lv_disp_set_theme(disp, nullptr);

    lv_style_init(&st_screen);
    lv_style_set_bg_color(&st_screen, COL_BG_MAIN);
    lv_style_set_bg_opa(&st_screen, LV_OPA_COVER);
    lv_style_set_pad_all(&st_screen, outer_pad);

    lv_style_init(&st_navbar);
    lv_style_set_bg_color(&st_navbar, COL_BG_MAIN);
    lv_style_set_bg_opa(&st_navbar, LV_OPA_TRANSP);
    lv_style_set_pad_left(&st_navbar, 0);
    lv_style_set_pad_right(&st_navbar, 0);
    lv_style_set_pad_top(&st_navbar, 0);
    lv_style_set_pad_bottom(&st_navbar, 12);
    lv_style_set_pad_column(&st_navbar, 12);

    lv_style_init(&st_tabstrip);
    lv_style_set_bg_color(&st_tabstrip, COL_BG_CARD);
    lv_style_set_bg_opa(&st_tabstrip, LV_OPA_COVER);
    lv_style_set_radius(&st_tabstrip, 12);
    lv_style_set_pad_all(&st_tabstrip, 8);
    lv_style_set_pad_column(&st_tabstrip, 8);

    lv_style_init(&st_card);
    lv_style_set_bg_color(&st_card, COL_BG_CARD);
    lv_style_set_bg_opa(&st_card, LV_OPA_COVER);
    lv_style_set_radius(&st_card, 16);
    lv_style_set_outline_color(&st_card, COL_GRAPH_GRID);
    lv_style_set_outline_opa(&st_card, LV_OPA_40);
    lv_style_set_outline_width(&st_card, 1);
    lv_style_set_pad_left(&st_card, card_pad);
    lv_style_set_pad_right(&st_card, card_pad);
    lv_style_set_pad_top(&st_card, card_pad - 2);
    lv_style_set_pad_bottom(&st_card, card_pad);

    lv_style_init(&st_caption);
    lv_style_set_text_color(&st_caption, COL_MUTED_TEXT);
    lv_style_set_text_font(&st_caption, FONT_SM);

    lv_style_init(&st_value_big);
    lv_style_set_text_color(&st_value_big, COL_WHITE);
    lv_style_set_text_font(&st_value_big, FONT_LG);

    lv_style_init(&st_value_medium);
    lv_style_set_text_color(&st_value_medium, COL_WHITE);
    lv_style_set_text_font(&st_value_medium, FONT_MD);

    lv_style_init(&st_label);
    lv_style_set_text_color(&st_label, COL_WHITE);
    lv_style_set_text_font(&st_label, FONT_SM);

    lv_style_init(&st_chip);
    lv_style_set_bg_color(&st_chip, COL_BG_CARD);
    lv_style_set_bg_opa(&st_chip, LV_OPA_COVER);
    lv_style_set_radius(&st_chip, 12);
    lv_style_set_pad_left(&st_chip, 14);
    lv_style_set_pad_right(&st_chip, 14);
    lv_style_set_pad_top(&st_chip, 10);
    lv_style_set_pad_bottom(&st_chip, 10);
    lv_style_set_outline_width(&st_chip, 1);
    lv_style_set_outline_color(&st_chip, COL_GRAPH_GRID);
    lv_style_set_text_color(&st_chip, COL_WHITE);
    lv_style_set_text_font(&st_chip, FONT_SM);

    lv_style_init(&st_chip_ghost);
    lv_style_set_bg_opa(&st_chip_ghost, LV_OPA_TRANSP);
    lv_style_set_radius(&st_chip_ghost, 14);
    lv_style_set_pad_left(&st_chip_ghost, 14);
    lv_style_set_pad_right(&st_chip_ghost, 14);
    lv_style_set_pad_top(&st_chip_ghost, 12);
    lv_style_set_pad_bottom(&st_chip_ghost, 12);
    lv_style_set_outline_width(&st_chip_ghost, 2);
    lv_style_set_outline_color(&st_chip_ghost, lv_color_hex(0x23303A));
    lv_style_set_text_color(&st_chip_ghost, COL_WHITE);
    lv_style_set_text_font(&st_chip_ghost, FONT_SM);

    lv_style_init(&st_chip_checked);
    lv_style_set_bg_color(&st_chip_checked, COL_CYAN);
    lv_style_set_bg_opa(&st_chip_checked, LV_OPA_COVER);
    lv_style_set_text_color(&st_chip_checked, lv_color_hex(0x0B0F14));

    lv_style_init(&st_chart_bg);
    lv_style_set_bg_color(&st_chart_bg, COL_BG_CARD);
    lv_style_set_bg_opa(&st_chart_bg, LV_OPA_COVER);
    lv_style_set_radius(&st_chart_bg, 20);
    lv_style_set_outline_color(&st_chart_bg, COL_GRAPH_GRID);
    lv_style_set_outline_opa(&st_chart_bg, LV_OPA_40);
    lv_style_set_outline_width(&st_chart_bg, 1);
}

static void build_navbar(lv_obj_t* parent, lv_coord_t w)
{
    LV_UNUSED(w);
    cont_navbar = lv_obj_create(parent);
    lv_obj_remove_style_all(cont_navbar);
    lv_obj_add_style(cont_navbar, &st_navbar, 0);
    lv_obj_set_width(cont_navbar, LV_PCT(100));
    lv_obj_set_height(cont_navbar, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(cont_navbar, 56, 0);
    lv_obj_set_layout(cont_navbar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont_navbar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont_navbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(cont_navbar, 8, 0);

    lv_obj_t* lbl_title = lv_label_create(cont_navbar);
    lv_obj_add_style(lbl_title, &st_value_medium, 0);
    lv_label_set_text(lbl_title, "Marine Overview");
    lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lbl_title, LV_PCT(100));

    lv_obj_t* tabs = lv_obj_create(cont_navbar);
    lv_obj_remove_style_all(tabs);
    lv_obj_add_style(tabs, &st_tabstrip, 0);
    lv_obj_set_width(tabs, LV_PCT(100));
    lv_obj_set_layout(tabs, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tabs, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(tabs, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    auto add_tab = [tabs](const char* label, bool checked) {
        lv_obj_t* tab = make_chip_button(tabs, label, [](lv_event_t* e) {
            LV_UNUSED(e);
        }, nullptr, &st_chip, &st_chip_checked, true);
        if (checked) {
            lv_obj_add_state(tab, LV_STATE_CHECKED);
        }
        return tab;
    };

    add_tab("Overview", true);
    add_tab("Speed", false);
    add_tab("Voltage", false);
}

static void build_grid(lv_obj_t* parent, lv_coord_t w, lv_coord_t h)
{
    LV_UNUSED(w);
    LV_UNUSED(h);

    cont_grid = lv_obj_create(parent);
    lv_obj_remove_style_all(cont_grid);
    lv_obj_set_style_bg_opa(cont_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_size(cont_grid, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_grow(cont_grid, 1);

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
    const lv_coord_t min_side = (g_screen_w < g_screen_h) ? g_screen_w : g_screen_h;
    const lv_coord_t grid_gap = (min_side <= 320) ? 8 : ((min_side <= 480) ? 10 : 16);
    lv_obj_set_style_pad_row(cont_grid, grid_gap, 0);
    lv_obj_set_style_pad_column(cont_grid, grid_gap, 0);
    lv_obj_set_style_pad_all(cont_grid, 0, 0);

    lv_obj_t* c_speed = make_metric_card(cont_grid, "SPEED", "7.4 kts", COL_CYAN, true);
    lv_obj_t* c_rpm   = make_metric_card(cont_grid, "ENGINE RPM", "1350", COL_ORANGE, true);
    card_rpm = c_rpm;
    lv_obj_t* c_batt  = make_metric_card(cont_grid, "REMAINING POWER", "78%", COL_GREEN, true);
    lv_obj_t* c_wind  = make_metric_card(cont_grid, "STW", "14.2 kts", COL_CYAN);
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
    lv_obj_add_style(badge, &st_caption, 0);
    lv_label_set_text(badge, "NO ALARMS");
    lv_obj_set_style_text_color(badge, COL_GREEN, 0);
    lv_obj_align(badge, LV_ALIGN_BOTTOM_RIGHT, -12, -12);

    // Page-level touch handling for detail navigation
    lv_obj_add_flag(cont_grid, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cont_grid, [](lv_event_t* e) {
        lv_indev_t* indev = lv_indev_get_act();
        if (!indev || !card_rpm) return;

        lv_point_t p;
        lv_indev_get_point(indev, &p);

        lv_area_t rpm_area;
        lv_obj_get_coords(card_rpm, &rpm_area);
        if (p.x >= rpm_area.x1 && p.x <= rpm_area.x2 && p.y >= rpm_area.y1 && p.y <= rpm_area.y2) {
            show_rpm_detail(true);
        }
    }, LV_EVENT_CLICKED, nullptr);
}

static void show_rpm_detail(bool show)
{
    if (!cont_rpm_detail) return;
    if (show) {
        lv_obj_clear_flag(cont_rpm_detail, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(cont_rpm_detail, LV_OPA_COVER, 0);
        lv_obj_move_foreground(cont_rpm_detail);
    } else {
        lv_obj_add_flag(cont_rpm_detail, LV_OBJ_FLAG_HIDDEN);
    }
}

static void set_rpm_resolution(int idx)
{
    for (int i = 0; i < 3; ++i) {
        if (!btn_resolutions[i]) continue;
        lv_obj_clear_state(btn_resolutions[i], LV_STATE_CHECKED);
        if (i == idx) {
            lv_obj_add_state(btn_resolutions[i], LV_STATE_CHECKED);
        }
    }

    static const lv_coord_t data_6h[]  = {1320, 1340, 1360, 1380, 1420, 1460, 1480, 1500, 1470, 1440};
    static const lv_coord_t data_12h[] = {1280, 1300, 1310, 1330, 1345, 1360, 1390, 1420, 1450, 1490, 1520, 1500};
    static const lv_coord_t data_24h[] = {1200, 1220, 1250, 1270, 1300, 1320, 1350, 1365, 1380, 1400, 1420, 1430, 1440, 1460, 1475, 1490};

    const lv_coord_t* data = data_6h;
    uint16_t cnt = sizeof(data_6h) / sizeof(data_6h[0]);
    const char* time_scale = "Time (6h)";
    switch (idx) {
    case 1:
        data = data_12h;
        cnt = sizeof(data_12h) / sizeof(data_12h[0]);
        time_scale = "Time (12h)";
        break;
    case 2:
        data = data_24h;
        cnt = sizeof(data_24h) / sizeof(data_24h[0]);
        time_scale = "Time (24h)";
        break;
    default:
        break;
    }

    lv_chart_set_point_count(chart_rpm, cnt);

    int sum = 0;
    lv_coord_t min_v = data[0];
    lv_coord_t max_v = data[0];
    for (uint16_t i = 0; i < cnt; ++i) {
        lv_chart_set_value_by_id(chart_rpm, chart_series_rpm, i, data[i]);
        sum += data[i];
        if (data[i] < min_v) min_v = data[i];
        if (data[i] > max_v) max_v = data[i];
    }
    lv_chart_refresh(chart_rpm);

    if (lbl_axis_x) {
        lv_label_set_text(lbl_axis_x, time_scale);
    }

    const lv_coord_t current_v = data[cnt - 1];
    const lv_coord_t avg_v = (lv_coord_t)(sum / (int)cnt);
    char buf[24];

    if (lbl_stat_current) {
        std::snprintf(buf, sizeof(buf), "%d rpm", (int)current_v);
        lv_label_set_text(lbl_stat_current, buf);
    }
    if (lbl_stat_avg) {
        std::snprintf(buf, sizeof(buf), "%d rpm", (int)avg_v);
        lv_label_set_text(lbl_stat_avg, buf);
    }
    if (lbl_stat_max) {
        std::snprintf(buf, sizeof(buf), "%d rpm", (int)max_v);
        lv_label_set_text(lbl_stat_max, buf);
    }
    if (lbl_stat_min) {
        std::snprintf(buf, sizeof(buf), "%d rpm", (int)min_v);
        lv_label_set_text(lbl_stat_min, buf);
    }
}

static lv_obj_t* make_chip_button(lv_obj_t* parent,
                                 const char* text,
                                 lv_event_cb_t cb,
                                 void* user_data,
                                 lv_style_t* base_style,
                                 lv_style_t* checked_style,
                                 bool checkable)
{
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);
    if (base_style) lv_obj_add_style(btn, base_style, 0);
    if (checkable && checked_style) lv_obj_add_style(btn, checked_style, LV_STATE_CHECKED);
    if (checkable) lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);

    lv_obj_t* lbl = lv_label_create(btn);
    lv_obj_add_style(lbl, &st_label, 0);
    lv_label_set_text(lbl, text);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lbl, LV_SIZE_CONTENT);
    lv_obj_center(lbl);
    return btn;
}

static lv_obj_t* make_detail_stat_tile(lv_obj_t* parent,
                                       const char* caption,
                                       const char* value,
                                       lv_color_t value_color,
                                       lv_obj_t** out_value_label)
{
    lv_obj_t* tile = lv_obj_create(parent);
    lv_obj_remove_style_all(tile);
    lv_obj_add_style(tile, &st_card, 0);
    lv_obj_set_height(tile, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_top(tile, 8, 0);
    lv_obj_set_style_pad_bottom(tile, 10, 0);

    lv_obj_t* lbl_caption = lv_label_create(tile);
    lv_obj_add_style(lbl_caption, &st_caption, 0);
    lv_label_set_text(lbl_caption, caption);
    lv_label_set_long_mode(lbl_caption, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lbl_caption, LV_PCT(100));
    lv_obj_align(lbl_caption, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* lbl_value = lv_label_create(tile);
    lv_obj_add_style(lbl_value, &st_value_medium, 0);
    lv_obj_set_style_text_color(lbl_value, value_color, 0);
    lv_label_set_text(lbl_value, value);
    lv_label_set_long_mode(lbl_value, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lbl_value, LV_PCT(100));
    lv_obj_align(lbl_value, LV_ALIGN_TOP_LEFT, 0, 20);

    if (out_value_label) *out_value_label = lbl_value;
    return tile;
}

static void build_rpm_detail(lv_obj_t* parent, lv_coord_t w, lv_coord_t h)
{
    const lv_coord_t min_side = (w < h) ? w : h;
    const lv_coord_t detail_pad = (min_side <= 320) ? 6 : 10;
    const lv_coord_t section_gap = (min_side <= 320) ? 6 : 10;
    const lv_coord_t chart_inner_pad = (min_side <= 320) ? 10 : 18;
    const uint8_t axis_label_space = (min_side <= 320) ? 28 : 42;
    const bool portrait = (w < h);

    cont_rpm_detail = lv_obj_create(parent);
    lv_obj_remove_style_all(cont_rpm_detail);
    lv_obj_add_style(cont_rpm_detail, &st_screen, 0);
    lv_obj_set_size(cont_rpm_detail, w, h);
    lv_obj_center(cont_rpm_detail);
    lv_obj_set_layout(cont_rpm_detail, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont_rpm_detail, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont_rpm_detail, detail_pad, 0);
    lv_obj_set_style_pad_row(cont_rpm_detail, section_gap, 0);
    lv_obj_add_flag(cont_rpm_detail, LV_OBJ_FLAG_HIDDEN);

    // Top component bar: back + H1 + options (single row)
    lv_obj_t* header = lv_obj_create(cont_rpm_detail);
    lv_obj_remove_style_all(header);
    lv_obj_add_style(header, &st_card, 0);
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_height(header, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(header, 6, 0);
    lv_obj_set_style_radius(header, 12, 0);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(header, 8, 0);

    btn_back = make_chip_button(header, "← Back", [](lv_event_t* e) {
        LV_UNUSED(e);
        show_rpm_detail(false);
    }, nullptr, &st_chip_ghost, nullptr, false);
    lv_obj_set_style_pad_left(btn_back, 10, 0);
    lv_obj_set_style_pad_right(btn_back, 10, 0);
    lv_obj_set_style_pad_top(btn_back, 6, 0);
    lv_obj_set_style_pad_bottom(btn_back, 6, 0);

    lv_obj_t* lbl_title = lv_label_create(header);
    lv_obj_add_style(lbl_title, &st_value_medium, 0);
    lv_label_set_text(lbl_title, "RPM Monitor");
    lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_CLIP);
    lv_obj_set_flex_grow(lbl_title, 1);

    lv_obj_t* res_group = lv_obj_create(header);
    lv_obj_remove_style_all(res_group);
    lv_obj_set_layout(res_group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(res_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(res_group, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(res_group, 6, 0);

    btn_resolutions[0] = make_chip_button(res_group, "6h", [](lv_event_t* e) { set_rpm_resolution(0); }, nullptr);
    btn_resolutions[1] = make_chip_button(res_group, "12h", [](lv_event_t* e) { set_rpm_resolution(1); }, nullptr);
    btn_resolutions[2] = make_chip_button(res_group, "24h", [](lv_event_t* e) { set_rpm_resolution(2); }, nullptr);

    for (int i = 0; i < 3; ++i) {
        if (!btn_resolutions[i]) continue;
        lv_obj_set_style_pad_left(btn_resolutions[i], 10, 0);
        lv_obj_set_style_pad_right(btn_resolutions[i], 10, 0);
        lv_obj_set_style_pad_top(btn_resolutions[i], 6, 0);
        lv_obj_set_style_pad_bottom(btn_resolutions[i], 6, 0);
    }

    // Component row: four stat tiles under header
    lv_obj_t* stats_row = lv_obj_create(cont_rpm_detail);
    lv_obj_remove_style_all(stats_row);
    lv_obj_set_width(stats_row, LV_PCT(100));
    lv_obj_set_height(stats_row, LV_SIZE_CONTENT);
    lv_obj_set_layout(stats_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(stats_row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(stats_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(stats_row, section_gap, 0);
    lv_obj_set_style_pad_column(stats_row, section_gap, 0);

    const lv_coord_t stat_w = portrait ? LV_PCT(48) : LV_PCT(24);
    lv_obj_t* st_cur = make_detail_stat_tile(stats_row, "CURRENT", "1350 rpm", COL_GREEN, &lbl_stat_current);
    lv_obj_t* st_avg = make_detail_stat_tile(stats_row, "AVERAGE", "1362 rpm", COL_WHITE, &lbl_stat_avg);
    lv_obj_t* st_max = make_detail_stat_tile(stats_row, "MAXIMUM", "1520 rpm", COL_CYAN, &lbl_stat_max);
    lv_obj_t* st_min = make_detail_stat_tile(stats_row, "MINIMUM", "1200 rpm", COL_ORANGE, &lbl_stat_min);
    lv_obj_set_width(st_cur, stat_w);
    lv_obj_set_width(st_avg, stat_w);
    lv_obj_set_width(st_max, stat_w);
    lv_obj_set_width(st_min, stat_w);

    // Graph component
    lv_obj_t* chart_card = lv_obj_create(cont_rpm_detail);
    lv_obj_remove_style_all(chart_card);
    lv_obj_add_style(chart_card, &st_chart_bg, 0);
    lv_obj_set_width(chart_card, LV_PCT(100));
    lv_obj_set_flex_grow(chart_card, 1);
    lv_obj_set_style_pad_all(chart_card, detail_pad, 0);
    lv_obj_set_layout(chart_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(chart_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(chart_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(chart_card, 4, 0);

    lv_obj_t* lbl_chart_title = lv_label_create(chart_card);
    lv_obj_add_style(lbl_chart_title, &st_label, 0);
    lv_label_set_text(lbl_chart_title, "RPM OVER TIME (6h)");
    lv_obj_set_width(lbl_chart_title, LV_PCT(100));
    lv_label_set_long_mode(lbl_chart_title, LV_LABEL_LONG_CLIP);

    chart_rpm = lv_chart_create(chart_card);
    lv_obj_set_width(chart_rpm, LV_PCT(100));
    lv_obj_set_flex_grow(chart_rpm, 1);
    lv_chart_set_type(chart_rpm, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart_rpm, 10);
    lv_chart_set_div_line_count(chart_rpm, 4, 6);
    lv_chart_set_range(chart_rpm, LV_CHART_AXIS_PRIMARY_Y, 1200, 1600);
    lv_obj_set_style_bg_color(chart_rpm, COL_BG_CARD, 0);
    lv_obj_set_style_bg_opa(chart_rpm, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(chart_rpm, 0, 0);
    lv_obj_set_style_pad_all(chart_rpm, chart_inner_pad, 0);
    lv_obj_set_style_line_width(chart_rpm, 3, LV_PART_ITEMS);
    lv_obj_set_style_line_color(chart_rpm, COL_GREEN, LV_PART_ITEMS);
    lv_obj_set_style_size(chart_rpm, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_chart_set_update_mode(chart_rpm, LV_CHART_UPDATE_MODE_SHIFT);
    lv_obj_set_style_line_color(chart_rpm, COL_GRAPH_GRID, LV_PART_MAIN);
    lv_obj_set_style_line_opa(chart_rpm, LV_OPA_30, LV_PART_MAIN);
    lv_chart_set_axis_tick(chart_rpm, LV_CHART_AXIS_PRIMARY_X, 8, 4, 6, 1, true, axis_label_space);
    lv_chart_set_axis_tick(chart_rpm, LV_CHART_AXIS_PRIMARY_Y, 8, 4, 5, 1, true, axis_label_space);
    lv_obj_set_style_text_font(chart_rpm, FONT_SM, 0);
    lv_obj_set_style_text_color(chart_rpm, COL_MUTED_TEXT, 0);

    lbl_axis_y = lv_label_create(chart_card);
    lv_obj_add_style(lbl_axis_y, &st_label, 0);
    lv_obj_set_style_text_color(lbl_axis_y, COL_MUTED_TEXT, 0);
    lv_label_set_text(lbl_axis_y, "RPM");
    lv_obj_align(lbl_axis_y, LV_ALIGN_TOP_LEFT, 4, 20);

    lbl_axis_x = lv_label_create(chart_card);
    lv_obj_add_style(lbl_axis_x, &st_label, 0);
    lv_obj_set_style_text_color(lbl_axis_x, COL_MUTED_TEXT, 0);
    lv_label_set_text(lbl_axis_x, "Time (6h)");
    lv_obj_align(lbl_axis_x, LV_ALIGN_BOTTOM_RIGHT, -4, -4);

    chart_series_rpm = lv_chart_add_series(chart_rpm, COL_GREEN, LV_CHART_AXIS_PRIMARY_Y);
    set_rpm_resolution(0);
}

void ui_init(void)
{
    if (ui_root) return;

    lv_disp_t* disp = lv_disp_get_default();
    lv_coord_t w = lv_disp_get_hor_res(disp);
    lv_coord_t h = lv_disp_get_ver_res(disp);

    g_screen_w = w;
    g_screen_h = h;
    apply_styles();

    ui_root = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(ui_root);
    lv_obj_add_style(ui_root, &st_screen, 0);
    lv_obj_set_size(ui_root, w, h);
    lv_obj_center(ui_root);
    lv_obj_set_layout(ui_root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(ui_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    build_navbar(ui_root, w);
    build_grid(ui_root, w, h);
    build_rpm_detail(ui_root, w, h);
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
