#include "ui.h"
#include "fonts.h"
#include "debug_config.h"

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

// ---- Roots ----
lv_obj_t* ui_root = nullptr;
static lv_obj_t* cont_navbar = nullptr;
static lv_obj_t* cont_grid   = nullptr;
static lv_obj_t* cont_rpm_detail = nullptr;
static lv_obj_t* btn_resolutions[3] = {nullptr, nullptr, nullptr};
static lv_obj_t* btn_back = nullptr;
static lv_obj_t* chart_rpm = nullptr;
static lv_chart_series_t* chart_series_rpm = nullptr;
static lv_obj_t* lbl_axis_x = nullptr;
static lv_obj_t* lbl_axis_y = nullptr;

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
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 12, 8);

    // Value
    lv_obj_t* lbl_value = lv_label_create(card);
    lv_obj_add_style(lbl_value, tall_value ? &st_value_big : &st_value_medium, 0);
    lv_label_set_text(lbl_value, value);
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
    lv_disp_t* disp = lv_disp_get_default();
    lv_disp_set_theme(disp, nullptr);

    lv_style_init(&st_screen);
    lv_style_set_bg_color(&st_screen, COL_BG_MAIN);
    lv_style_set_bg_opa(&st_screen, LV_OPA_COVER);
    lv_style_set_pad_all(&st_screen, 24);

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
    lv_style_set_radius(&st_card, 20);
    lv_style_set_outline_color(&st_card, COL_GRAPH_GRID);
    lv_style_set_outline_opa(&st_card, LV_OPA_40);
    lv_style_set_outline_width(&st_card, 1);
    lv_style_set_pad_left(&st_card, 16);
    lv_style_set_pad_right(&st_card, 16);
    lv_style_set_pad_top(&st_card, 12);
    lv_style_set_pad_bottom(&st_card, 18);

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
    cont_navbar = lv_obj_create(parent);
    lv_obj_remove_style_all(cont_navbar);
    lv_obj_add_style(cont_navbar, &st_navbar, 0);
    lv_obj_set_width(cont_navbar, w);
    lv_obj_set_height(cont_navbar, 64);
    lv_obj_align(cont_navbar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_layout(cont_navbar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont_navbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_navbar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* lbl_title = lv_label_create(cont_navbar);
    lv_obj_add_style(lbl_title, &st_value_medium, 0);
    lv_label_set_text(lbl_title, "Marine Overview");

    lv_obj_t* tabs = lv_obj_create(cont_navbar);
    lv_obj_remove_style_all(tabs);
    lv_obj_add_style(tabs, &st_tabstrip, 0);
    lv_obj_set_layout(tabs, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tabs, LV_FLEX_FLOW_ROW);
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
    const lv_coord_t top_h = 64;
    const lv_coord_t bot_h = 0;
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
    lv_obj_set_style_pad_row(cont_grid, 16, 0);
    lv_obj_set_style_pad_column(cont_grid, 16, 0);
    lv_obj_set_style_pad_all(cont_grid, 16, 0);

    lv_obj_t* c_speed = make_metric_card(cont_grid, "SPEED", "7.4 kts", COL_CYAN, true);
    lv_obj_t* c_rpm   = make_metric_card(cont_grid, "ENGINE RPM", "1350", COL_ORANGE, true);
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

    // RPM detail navigation
    lv_obj_add_flag(c_rpm, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(c_rpm, [](lv_event_t* e) {
        LV_UNUSED(e);
        show_rpm_detail(true);
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
    lv_obj_add_style(header, &st_navbar, 0);
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
    lv_obj_set_style_bg_color(chart_rpm, COL_BG_CARD, 0);
    lv_obj_set_style_bg_opa(chart_rpm, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(chart_rpm, 0, 0);
    lv_obj_set_style_pad_all(chart_rpm, 20, 0);
    lv_obj_set_style_line_width(chart_rpm, 4, LV_PART_ITEMS);
    lv_obj_set_style_line_color(chart_rpm, COL_GREEN, LV_PART_ITEMS);
    lv_obj_set_style_size(chart_rpm, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_chart_set_update_mode(chart_rpm, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(chart_rpm, 4, 6);
    lv_obj_set_style_line_color(chart_rpm, COL_GRAPH_GRID, LV_PART_MAIN);
    lv_obj_set_style_line_opa(chart_rpm, LV_OPA_30, LV_PART_MAIN);
    lv_chart_set_axis_tick(chart_rpm, LV_CHART_AXIS_PRIMARY_X, 8, 4, 6, 1, true, 48);
    lv_chart_set_axis_tick(chart_rpm, LV_CHART_AXIS_PRIMARY_Y, 8, 4, 5, 1, true, 48);
    lv_obj_set_style_text_font(chart_rpm, FONT_SM, 0);
    lv_obj_set_style_text_color(chart_rpm, COL_MUTED_TEXT, 0);

    lbl_axis_y = lv_label_create(chart_card);
    lv_obj_add_style(lbl_axis_y, &st_label, 0);
    lv_obj_set_style_text_color(lbl_axis_y, COL_MUTED_TEXT, 0);
    lv_label_set_text(lbl_axis_y, "RPM");
    lv_obj_align(lbl_axis_y, LV_ALIGN_TOP_LEFT, 4, 4);

    lbl_axis_x = lv_label_create(chart_card);
    lv_obj_add_style(lbl_axis_x, &st_label, 0);
    lv_obj_set_style_text_color(lbl_axis_x, COL_MUTED_TEXT, 0);
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
