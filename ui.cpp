#include "ui.h"
#include "fonts.h"
#include "debug_config.h"

// ---------- Font selection (no external fonts required) ----------
#if defined(USE_ORBITRON) || defined(USE_ORBITRON_FONTS)
static const lv_font_t* FONT_SM = &orbitron_20_700;
static const lv_font_t* FONT_LG = &orbitron_48_900;
#else
// Safe defaults: use LVGL's default font for all sizes (always available)
static const lv_font_t* FONT_SM = LV_FONT_DEFAULT;
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
static lv_style_t st_chip, st_chip_checked, st_chip_ghost, st_chart_bg;

// ---- Roots ----
lv_obj_t* ui_root = nullptr;
static lv_obj_t* cont_status = nullptr;
static lv_obj_t* cont_grid   = nullptr;
static lv_obj_t* cont_footer = nullptr;
static lv_obj_t* cont_rpm_detail = nullptr;
static lv_obj_t* btn_resolutions[3] = {nullptr, nullptr, nullptr};
static lv_obj_t* btn_back = nullptr;
static lv_obj_t* chart_rpm = nullptr;
static lv_chart_series_t* chart_series_rpm = nullptr;
static lv_obj_t* lbl_axis_x = nullptr;
static lv_obj_t* lbl_axis_y = nullptr;
static lv_timer_t* hide_timer = nullptr;

// ---- Forward declarations ----
static void show_rpm_detail(bool show);
static void set_rpm_resolution(int idx);

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
    lv_style_set_text_font(&st_label, FONT_SM);

    lv_style_init(&st_chip);
    lv_style_set_bg_color(&st_chip, COL_CARD);
    lv_style_set_bg_opa(&st_chip, LV_OPA_COVER);
    lv_style_set_radius(&st_chip, 12);
    lv_style_set_pad_all(&st_chip, 10);
    lv_style_set_outline_width(&st_chip, 1);
    lv_style_set_outline_color(&st_chip, lv_color_hex(0x1B252D));
    lv_style_set_text_color(&st_chip, COL_TXT);
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
    lv_style_set_text_color(&st_chip_ghost, COL_TXT);
    lv_style_set_text_font(&st_chip_ghost, FONT_SM);

    lv_style_init(&st_chip_checked);
    lv_style_set_bg_color(&st_chip_checked, COL_ORANGE);
    lv_style_set_bg_opa(&st_chip_checked, LV_OPA_COVER);
    lv_style_set_text_color(&st_chip_checked, lv_color_hex(0x0B0F14));

    lv_style_init(&st_chart_bg);
    lv_style_set_bg_color(&st_chart_bg, COL_CARD);
    lv_style_set_bg_opa(&st_chart_bg, LV_OPA_COVER);
    lv_style_set_radius(&st_chart_bg, 16);
    lv_style_set_outline_color(&st_chart_bg, lv_color_hex(0x10161C));
    lv_style_set_outline_width(&st_chart_bg, 1);
    lv_style_set_shadow_width(&st_chart_bg, 14);
    lv_style_set_shadow_opa(&st_chart_bg, LV_OPA_30);
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
    lv_label_set_text(lbl_conn, "CAN: OK  *  GPS: 3D  *  LOG: SD");
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

    // RPM detail navigation
    lv_obj_add_flag(c_rpm, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(c_rpm, [](lv_event_t* e) {
        LV_UNUSED(e);
        show_rpm_detail(true);
    }, LV_EVENT_CLICKED, nullptr);
}

static void hide_timer_cb(lv_timer_t* timer)
{
    if (cont_rpm_detail) {
        lv_obj_add_flag(cont_rpm_detail, LV_OBJ_FLAG_HIDDEN);
    }
    lv_timer_del(timer);
    hide_timer = nullptr;
}

static void show_rpm_detail(bool show)
{
    if (!cont_rpm_detail) return;
    if (show) {
        if (hide_timer) {
            lv_timer_del(hide_timer);
            hide_timer = nullptr;
        }
        lv_obj_clear_flag(cont_rpm_detail, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(cont_rpm_detail, LV_OPA_TRANSP, 0);
        lv_obj_move_foreground(cont_rpm_detail);
        lv_obj_fade_in(cont_rpm_detail, 90, 0);
    } else {
        lv_obj_fade_out(cont_rpm_detail, 70, 0);
        hide_timer = lv_timer_create(hide_timer_cb, 80, nullptr);
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
    for (uint16_t i = 0; i < cnt; ++i) {
        lv_chart_set_value_by_id(chart_rpm, chart_series_rpm, i, data[i]);
    }
    lv_chart_refresh(chart_rpm);

    if (lbl_axis_x) {
        lv_label_set_text(lbl_axis_x, time_scale);
    }
}

static lv_obj_t* make_chip_button(lv_obj_t* parent,
                                 const char* text,
                                 lv_event_cb_t cb,
                                 void* user_data,
                                 lv_style_t* base_style = &st_chip,
                                 lv_style_t* checked_style = &st_chip_checked,
                                 bool checkable = true)
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
    lv_obj_center(lbl);
    return btn;
}

static void build_rpm_detail(lv_obj_t* parent, lv_coord_t w, lv_coord_t h)
{
    cont_rpm_detail = lv_obj_create(parent);
    lv_obj_remove_style_all(cont_rpm_detail);
    lv_obj_add_style(cont_rpm_detail, &st_screen, 0);
    lv_obj_set_size(cont_rpm_detail, w, h);
    lv_obj_center(cont_rpm_detail);
    lv_obj_set_layout(cont_rpm_detail, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont_rpm_detail, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont_rpm_detail, 12, 0);
    lv_obj_set_style_pad_row(cont_rpm_detail, 12, 0);
    lv_obj_add_flag(cont_rpm_detail, LV_OBJ_FLAG_HIDDEN);

    // Header bar
    lv_obj_t* header = lv_obj_create(cont_rpm_detail);
    lv_obj_remove_style_all(header);
    lv_obj_add_style(header, &st_statusbar, 0);
    lv_obj_set_width(header, LV_PCT(100));
    lv_obj_set_height(header, 56);
    lv_obj_set_style_pad_left(header, 12, 0);
    lv_obj_set_style_pad_right(header, 12, 0);
    lv_obj_set_style_pad_top(header, 8, 0);
    lv_obj_set_style_pad_bottom(header, 8, 0);
    lv_obj_set_style_pad_column(header, 10, 0);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    btn_back = make_chip_button(header, "← Back", [](lv_event_t* e) {
        LV_UNUSED(e);
        show_rpm_detail(false);
    }, nullptr, &st_chip_ghost, nullptr, false);

    lv_obj_t* lbl_title = lv_label_create(header);
    lv_obj_add_style(lbl_title, &st_label, 0);
    lv_label_set_text(lbl_title, "RPM Monitor");
    lv_obj_set_flex_grow(lbl_title, 1);

    lv_obj_t* res_group = lv_obj_create(header);
    lv_obj_remove_style_all(res_group);
    lv_obj_set_style_pad_row(res_group, 0, 0);
    lv_obj_set_style_pad_column(res_group, 8, 0);
    lv_obj_set_layout(res_group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(res_group, LV_FLEX_FLOW_ROW);

    btn_resolutions[0] = make_chip_button(res_group, "6h", [](lv_event_t* e) {
        set_rpm_resolution(0);
    }, nullptr);
    btn_resolutions[1] = make_chip_button(res_group, "12h", [](lv_event_t* e) {
        set_rpm_resolution(1);
    }, nullptr);
    btn_resolutions[2] = make_chip_button(res_group, "24h", [](lv_event_t* e) {
        set_rpm_resolution(2);
    }, nullptr);

    // Chart card
    lv_obj_t* chart_card = lv_obj_create(cont_rpm_detail);
    lv_obj_remove_style_all(chart_card);
    lv_obj_add_style(chart_card, &st_chart_bg, 0);
    lv_obj_set_width(chart_card, LV_PCT(100));
    lv_obj_set_flex_grow(chart_card, 1);
    lv_obj_set_style_pad_all(chart_card, 12, 0);

    chart_rpm = lv_chart_create(chart_card);
    lv_obj_set_size(chart_rpm, LV_PCT(100), LV_PCT(100));
    lv_obj_center(chart_rpm);
    lv_chart_set_type(chart_rpm, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart_rpm, 10);
    lv_chart_set_div_line_count(chart_rpm, 5, 7);
    lv_chart_set_range(chart_rpm, LV_CHART_AXIS_PRIMARY_Y, 1200, 1600);
    lv_obj_set_style_bg_color(chart_rpm, COL_CARD, 0);
    lv_obj_set_style_bg_opa(chart_rpm, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chart_rpm, 0, 0);
    lv_obj_set_style_pad_all(chart_rpm, 16, 0);
    lv_obj_set_style_line_width(chart_rpm, 4, LV_PART_ITEMS);
    lv_obj_set_style_line_color(chart_rpm, COL_GREEN, LV_PART_ITEMS);
    lv_obj_set_style_size(chart_rpm, 6, LV_PART_INDICATOR);
    lv_chart_set_update_mode(chart_rpm, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_chart_set_axis_tick(chart_rpm, LV_CHART_AXIS_PRIMARY_X, 8, 4, 6, 1, true, 48);
    lv_chart_set_axis_tick(chart_rpm, LV_CHART_AXIS_PRIMARY_Y, 8, 4, 5, 1, true, 48);
    lv_obj_set_style_text_font(chart_rpm, FONT_SM, 0);
    lv_obj_set_style_text_color(chart_rpm, COL_MUTED, 0);

    lbl_axis_y = lv_label_create(chart_card);
    lv_obj_add_style(lbl_axis_y, &st_label, 0);
    lv_obj_set_style_text_color(lbl_axis_y, COL_MUTED, 0);
    lv_label_set_text(lbl_axis_y, "RPM");
    lv_obj_align(lbl_axis_y, LV_ALIGN_TOP_LEFT, 4, 4);

    lbl_axis_x = lv_label_create(chart_card);
    lv_obj_add_style(lbl_axis_x, &st_label, 0);
    lv_obj_set_style_text_color(lbl_axis_x, COL_MUTED, 0);
    lv_label_set_text(lbl_axis_x, "Time");
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

    apply_styles();

    ui_root = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(ui_root);
    lv_obj_add_style(ui_root, &st_screen, 0);
    lv_obj_set_size(ui_root, w, h);
    lv_obj_center(ui_root);

    build_status_bar(ui_root, w);
    build_grid(ui_root, w, h);
    build_footer(ui_root, w, h);
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
