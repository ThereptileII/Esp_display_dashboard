1. Base assumptions

LVGL version: 8.x

Target resolution: 1280 x 800 

Orientation: Landscape

Theme: Disable built-in theme and use custom styles

lv_disp_t * disp = lv_disp_get_default();
lv_theme_t * th = lv_theme_default_init(disp,
                                        lv_palette_main(LV_PALETTE_BLUE),
                                        lv_palette_main(LV_PALETTE_RED),
                                        false,  /* light theme? */
                                        LV_FONT_DEFAULT);
lv_disp_set_theme(disp, NULL);   // We will not rely on the theme

2. Color palette (from your images)

Use these exact hex values:

// Backgrounds
#define COL_BG_MAIN    0x101319  // very dark navy
#define COL_BG_CARD    0x171B23  // slightly lighter for cards

// Accent / text colors
#define COL_CYAN       0x1FCEF9  // RPM / STW blue-cyan
#define COL_GREEN      0x11D411  // Voltage & "Voltage Monitor" green
#define COL_ORANGE     0xF99E1F  // Remaining power / SOC orange
#define COL_WHITE      0xFFFFFF
#define COL_MUTED_TEXT 0x8493A8  // small captions like "RPM:", "SOC:"

// Graph line
#define COL_GRAPH_LINE 0x11D411  // same as green
#define COL_GRAPH_GRID 0x202633  // subtle grid lines


In LVGL:

lv_color_t col_bg_main    = lv_color_hex(COL_BG_MAIN);
lv_color_t col_bg_card    = lv_color_hex(COL_BG_CARD);
lv_color_t col_cyan       = lv_color_hex(COL_CYAN);
lv_color_t col_green      = lv_color_hex(COL_GREEN);
lv_color_t col_orange     = lv_color_hex(COL_ORANGE);
lv_color_t col_white      = lv_color_hex(COL_WHITE);
lv_color_t col_muted_text = lv_color_hex(COL_MUTED_TEXT);

3. Fonts

The design uses an “Orbitron-style” techno font with monospaced digits.

3.1. Choose font sizes

You’ll need at least:

orbitron_72 – gigantic numbers (RPM/STW, main graph stats)

orbitron_48 – big numbers on Overview cards

orbitron_32 – medium values (bottom tiles)

orbitron_20 – small values / headings

montserrat_16 (or similar) – captions (“RPM:”, “REMAINING POWER:” etc.)

3.2. Generate fonts for LVGL

Use the LVGL font converter (online or offline):

Font: Orbitron (regular or medium)

Range: at least:

Digits 0-9

Period .

Percent %

Letters used in labels (kts, kW, kWh, RPM, SOC, STW, GEARR, etc.)

Bpp: 4 (gives smooth edges)

Then include them:

#include "orbitron_72.h"
#include "orbitron_48.h"
#include "orbitron_32.h"
#include "orbitron_20.h"
#include "montserrat_16.h"

LV_FONT_DECLARE(orbitron_72);
LV_FONT_DECLARE(orbitron_48);
LV_FONT_DECLARE(orbitron_32);
LV_FONT_DECLARE(orbitron_20);
LV_FONT_DECLARE(montserrat_16);

4. Global screen style

Apply to the main parent of each page.

4.1. Screen background style
static lv_style_t style_screen_bg;

void style_init_screen_bg(void)
{
    lv_style_init(&style_screen_bg);
    lv_style_set_bg_color(&style_screen_bg, col_bg_main);
    lv_style_set_bg_opa(&style_screen_bg, LV_OPA_COVER);
    lv_style_set_pad_all(&style_screen_bg, 24);    // outer margin
}


When creating a page:

lv_obj_t * scr_overview = lv_obj_create(NULL);
lv_obj_add_style(scr_overview, &style_screen_bg, 0);

5. Core reusable components
5.1. “Card” container (the dark rounded boxes)

All metric blocks and chart areas are these “cards”.

static lv_style_t style_card;

void style_init_card(void)
{
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, col_bg_card);
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_radius(&style_card, 20);
    lv_style_set_border_width(&style_card, 0);

    // Soft glow-like shadow around card
    lv_style_set_shadow_width(&style_card, 25);
    lv_style_set_shadow_opa(&style_card, LV_OPA_20);
    lv_style_set_shadow_color(&style_card, lv_color_hex(0x000000));
    lv_style_set_shadow_spread(&style_card, 0);
    lv_style_set_shadow_ofs_x(&style_card, 0);
    lv_style_set_shadow_ofs_y(&style_card, 8);

    // Inner padding for content
    lv_style_set_pad_left(&style_card, 32);
    lv_style_set_pad_right(&style_card, 32);
    lv_style_set_pad_top(&style_card, 24);
    lv_style_set_pad_bottom(&style_card, 24);
}


To create a card:

lv_obj_t * card = lv_obj_create(parent);
lv_obj_add_style(card, &style_card, 0);

5.2. Caption labels (“RPM:”, “BATTERY VOLTAGE:”)

Small, uppercase, muted.

static lv_style_t style_caption;

void style_init_caption(void)
{
    lv_style_init(&style_caption);
    lv_style_set_text_color(&style_caption, col_muted_text);
    lv_style_set_text_font(&style_caption, &montserrat_16);
    lv_style_set_text_letter_space(&style_caption, 2);
}


Create:

lv_obj_t * cap = lv_label_create(card);
lv_obj_add_style(cap, &style_caption, 0);
lv_label_set_text(cap, "RPM:");
lv_obj_align(cap, LV_ALIGN_TOP_LEFT, 0, 0);

5.3. Big numeric labels with neon glow

We’ll make three variants: cyan, green, orange.
All share same base style; only color changes.

static lv_style_t style_value_big_base;
static lv_style_t style_value_big_cyan;
static lv_style_t style_value_big_green;
static lv_style_t style_value_big_orange;

void style_init_value_big(void)
{
    // base
    lv_style_init(&style_value_big_base);
    lv_style_set_text_font(&style_value_big_base, &orbitron_72);
    lv_style_set_text_letter_space(&style_value_big_base, 4);
    lv_style_set_text_line_space(&style_value_big_base, 4);
    lv_style_set_text_align(&style_value_big_base, LV_TEXT_ALIGN_LEFT);

    // neon-like shadow
    lv_style_set_text_shadow_width(&style_value_big_base, 18);
    lv_style_set_text_shadow_ofs_x(&style_value_big_base, 0);
    lv_style_set_text_shadow_ofs_y(&style_value_big_base, 0);
    lv_style_set_text_shadow_opa(&style_value_big_base, LV_OPA_60);

    // cyan
    lv_style_init(&style_value_big_cyan);
    lv_style_copy(&style_value_big_cyan, &style_value_big_base);
    lv_style_set_text_color(&style_value_big_cyan, col_cyan);
    lv_style_set_text_shadow_color(&style_value_big_cyan, col_cyan);

    // green
    lv_style_init(&style_value_big_green);
    lv_style_copy(&style_value_big_green, &style_value_big_base);
    lv_style_set_text_color(&style_value_big_green, col_green);
    lv_style_set_text_shadow_color(&style_value_big_green, col_green);

    // orange
    lv_style_init(&style_value_big_orange);
    lv_style_copy(&style_value_big_orange, &style_value_big_base);
    lv_style_set_text_color(&style_value_big_orange, col_orange);
    lv_style_set_text_shadow_color(&style_value_big_orange, col_orange);
}


Usage:

lv_obj_t * rpm_val = lv_label_create(card_rpm);
lv_obj_add_style(rpm_val, &style_value_big_cyan, 0);
lv_label_set_text(rpm_val, "600");        // text only numbers
lv_obj_align(rpm_val, LV_ALIGN_LEFT_MID, 0, 10);

5.4. Medium numeric labels

For the smaller tiles (like SOC 95%, STW bottom bar):

static lv_style_t style_value_med_cyan;
static lv_style_t style_value_med_orange;
static lv_style_t style_value_med_white;

void style_init_value_medium(void)
{
    // base
    lv_style_t base;
    lv_style_init(&base);
    lv_style_set_text_font(&base, &orbitron_32);
    lv_style_set_text_letter_space(&base, 3);
    lv_style_set_text_shadow_width(&base, 12);
    lv_style_set_text_shadow_ofs_x(&base, 0);
    lv_style_set_text_shadow_ofs_y(&base, 0);
    lv_style_set_text_shadow_opa(&base, LV_OPA_60);

    lv_style_init(&style_value_med_cyan);
    lv_style_copy(&style_value_med_cyan, &base);
    lv_style_set_text_color(&style_value_med_cyan, col_cyan);
    lv_style_set_text_shadow_color(&style_value_med_cyan, col_cyan);

    lv_style_init(&style_value_med_orange);
    lv_style_copy(&style_value_med_orange, &base);
    lv_style_set_text_color(&style_value_med_orange, col_orange);
    lv_style_set_text_shadow_color(&style_value_med_orange, col_orange);

    lv_style_init(&style_value_med_white);
    lv_style_copy(&style_value_med_white, &base);
    lv_style_set_text_color(&style_value_med_white, col_white);
    lv_style_set_text_shadow_color(&style_value_med_white, col_white);
}

6. Top navigation bar (Overview / Speed Focus / Minimal / Wind Data)

Create a horizontal bar at the very top.

6.1. Container
static lv_style_t style_navbar;

void style_init_navbar(void)
{
    lv_style_init(&style_navbar);
    lv_style_set_bg_opa(&style_navbar, LV_OPA_TRANSP);
    lv_style_set_pad_left(&style_navbar, 24);
    lv_style_set_pad_right(&style_navbar, 24);
    lv_style_set_pad_top(&style_navbar, 12);
    lv_style_set_pad_bottom(&style_navbar, 12);
    lv_style_set_layout(&style_navbar, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&style_navbar, LV_FLEX_FLOW_ROW);
    lv_style_set_flex_main_place(&style_navbar, LV_FLEX_ALIGN_CENTER);
    lv_style_set_flex_track_place(&style_navbar, LV_FLEX_ALIGN_CENTER);
    lv_style_set_flex_cross_place(&style_navbar, LV_FLEX_ALIGN_CENTER);
    lv_style_set_pad_column(&style_navbar, 12);
}


Usage:

lv_obj_t * navbar = lv_obj_create(scr_overview);
lv_obj_add_style(navbar, &style_navbar, 0);
lv_obj_set_width(navbar, LV_PCT(100));
lv_obj_align(navbar, LV_ALIGN_TOP_MID, 0, 0);

6.2. Tab button styles

Active tab = cyan pill; inactive = dark pill.

static lv_style_t style_tab_base;
static lv_style_t style_tab_active;
static lv_style_t style_tab_inactive;

void style_init_tabs(void)
{
    // base
    lv_style_init(&style_tab_base);
    lv_style_set_radius(&style_tab_base, 16);
    lv_style_set_pad_left(&style_tab_base, 24);
    lv_style_set_pad_right(&style_tab_base, 24);
    lv_style_set_pad_top(&style_tab_base, 6);
    lv_style_set_pad_bottom(&style_tab_base, 6);
    lv_style_set_border_width(&style_tab_base, 1);
    lv_style_set_border_color(&style_tab_base, lv_color_hex(0x303845));
    lv_style_set_bg_color(&style_tab_base, col_bg_card);
    lv_style_set_bg_opa(&style_tab_base, LV_OPA_COVER);
    lv_style_set_text_font(&style_tab_base, &montserrat_16);
    lv_style_set_text_letter_space(&style_tab_base, 2);

    // inactive
    lv_style_init(&style_tab_inactive);
    lv_style_copy(&style_tab_inactive, &style_tab_base);
    lv_style_set_text_color(&style_tab_inactive, col_white);

    // active
    lv_style_init(&style_tab_active);
    lv_style_copy(&style_tab_active, &style_tab_base);
    lv_style_set_bg_color(&style_tab_active, col_cyan);
    lv_style_set_text_color(&style_tab_active, col_bg_main);
}


Create a tab:

lv_obj_t * btn_overview = lv_btn_create(navbar);
lv_obj_add_style(btn_overview, &style_tab_base, 0);
lv_obj_add_style(btn_overview, &style_tab_active, LV_STATE_CHECKED); // active when checked

lv_obj_t * lbl_overview = lv_label_create(btn_overview);
lv_label_set_text(lbl_overview, "Overview");

// Use CHECKED state to indicate active page
lv_obj_add_state(btn_overview, LV_STATE_CHECKED);


For other tabs, use style_tab_inactive for default.

7. Layout with LVGL Grid – Overview page

The Overview screen is a 2x2 grid of cards under the navbar.

7.1. Create a content container
lv_obj_t * cont = lv_obj_create(scr_overview);
lv_obj_add_style(cont, &style_screen_bg, 0);  // same bg as screen
lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);
lv_obj_set_style_pad_top(cont, 72, 0);    // leave space for navbar

7.2. Set grid layout
static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

lv_obj_set_layout(cont, LV_LAYOUT_GRID);
lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
lv_obj_set_style_pad_row(cont, 24, 0);
lv_obj_set_style_pad_column(cont, 24, 0);
lv_obj_set_style_pad_all(cont, 24, 0);

7.3. Create the four cards

RPM

Remaining power

Battery voltage

Power draw + SOC (vertical stack inside the card)

Example RPM card (top-left):

lv_obj_t * card_rpm = lv_obj_create(cont);
lv_obj_add_style(card_rpm, &style_card, 0);
lv_obj_set_grid_cell(card_rpm,
                     LV_GRID_ALIGN_STRETCH, 0, 1,
                     LV_GRID_ALIGN_STRETCH, 0, 1);

// Caption
lv_obj_t * cap_rpm = lv_label_create(card_rpm);
lv_obj_add_style(cap_rpm, &style_caption, 0);
lv_label_set_text(cap_rpm, "RPM:");
lv_obj_align(cap_rpm, LV_ALIGN_TOP_LEFT, 0, 0);

// Value
lv_obj_t * val_rpm = lv_label_create(card_rpm);
lv_obj_add_style(val_rpm, &style_value_big_cyan, 0);
lv_label_set_text(val_rpm, "600");
lv_obj_align(val_rpm, LV_ALIGN_LEFT_MID, 0, 10);


Remaining power card (top-right, orange):

lv_obj_t * card_rem = lv_obj_create(cont);
lv_obj_add_style(card_rem, &style_card, 0);
lv_obj_set_grid_cell(card_rem,
                     LV_GRID_ALIGN_STRETCH, 1, 1,
                     LV_GRID_ALIGN_STRETCH, 0, 1);

lv_obj_t * cap_rem = lv_label_create(card_rem);
lv_obj_add_style(cap_rem, &style_caption, 0);
lv_label_set_text(cap_rem, "REMAINING POWER:");

lv_obj_t * val_rem = lv_label_create(card_rem);
lv_obj_add_style(val_rem, &style_value_big_orange, 0);
lv_label_set_text(val_rem, "23.2");           // digits only
lv_obj_align(val_rem, LV_ALIGN_LEFT_MID, 0, 10);

// Unit label "kWh" to the right
lv_obj_t * unit_rem = lv_label_create(card_rem);
lv_obj_add_style(unit_rem, &style_caption, 0);
lv_style_set_text_color(&style_caption, col_orange);  // optional stronger
lv_label_set_text(unit_rem, "kWh");
lv_obj_align_to(unit_rem, val_rem, LV_ALIGN_OUT_RIGHT_MID, 16, 6);


Battery voltage (bottom-left, green):

lv_obj_t * card_volt = lv_obj_create(cont);
lv_obj_add_style(card_volt, &style_card, 0);
lv_obj_set_grid_cell(card_volt,
                     LV_GRID_ALIGN_STRETCH, 0, 1,
                     LV_GRID_ALIGN_STRETCH, 1, 1);

lv_obj_t * cap_volt = lv_label_create(card_volt);
lv_obj_add_style(cap_volt, &style_caption, 0);
lv_label_set_text(cap_volt, "BATTERY VOLTAGE:");

lv_obj_t * val_volt = lv_label_create(card_volt);
lv_obj_add_style(val_volt, &style_value_big_green, 0);
lv_label_set_text(val_volt, "380");
lv_obj_align(val_volt, LV_ALIGN_LEFT_MID, 0, 10);

// small "V" near baseline
lv_obj_t * unit_volt = lv_label_create(card_volt);
lv_obj_add_style(unit_volt, &style_caption, 0);
lv_label_set_text(unit_volt, "V");
lv_obj_align_to(unit_volt, val_volt, LV_ALIGN_OUT_RIGHT_BOTTOM, 8, 0);


Bottom-right card contains two metric blocks stacked: POWER DRAW and SOC. For a novice, simplest is to create two child containers inside that card with flex-column.

8. Speed Focus page

Layout:

Huge STW number centered.

Bottom row: 4 equal cards: RPM, GEAR, REGEN, SOC.

8.1. Parent containers
lv_obj_t * scr_speed = lv_obj_create(NULL);
lv_obj_add_style(scr_speed, &style_screen_bg, 0);

// Reuse navbar at top, same as before (tabs)

lv_obj_t * cont_speed = lv_obj_create(scr_speed);
lv_obj_add_style(cont_speed, &style_screen_bg, 0);
lv_obj_set_size(cont_speed, LV_PCT(100), LV_PCT(100));
lv_obj_align(cont_speed, LV_ALIGN_BOTTOM_MID, 0, 0);
lv_obj_set_style_pad_top(cont_speed, 72, 0);

8.2. Big STW readout

Create a large, full-width card for the center area (no strong edges in screenshot, but to match shadows use style_card).

lv_obj_t * card_stw = lv_obj_create(cont_speed);
lv_obj_add_style(card_stw, &style_card, 0);
lv_obj_set_size(card_stw, LV_PCT(100), 360);
lv_obj_align(card_stw, LV_ALIGN_TOP_MID, 0, 32);

// caption
lv_obj_t * cap_stw = lv_label_create(card_stw);
lv_obj_add_style(cap_stw, &style_caption, 0);
lv_label_set_text(cap_stw, "STW:");
lv_obj_align(cap_stw, LV_ALIGN_TOP_MID, 0, 0);

// big value (white)
static lv_style_t style_value_huge;
lv_style_init(&style_value_huge);
lv_style_copy(&style_value_huge, &style_value_big_base);
lv_style_set_text_font(&style_value_huge, &orbitron_72);
lv_style_set_text_color(&style_value_huge, col_white);
lv_style_set_text_shadow_color(&style_value_huge, col_white);

lv_obj_t * val_stw = lv_label_create(card_stw);
lv_obj_add_style(val_stw, &style_value_huge, 0);
lv_label_set_text(val_stw, "5.6");
lv_obj_align(val_stw, LV_ALIGN_CENTER, -80, 20);

// "kts" to the right, medium value style
lv_obj_t * unit_stw = lv_label_create(card_stw);
lv_obj_add_style(unit_stw, &style_value_med_white, 0);
lv_label_set_text(unit_stw, "kts");
lv_obj_align_to(unit_stw, val_stw, LV_ALIGN_OUT_RIGHT_BOTTOM, 40, -10);

8.3. Bottom cards row

Use flex layout:

lv_obj_t * cont_bottom = lv_obj_create(cont_speed);
lv_obj_add_style(cont_bottom, &style_screen_bg, 0);
lv_obj_set_size(cont_bottom, LV_PCT(100), 180);
lv_obj_align(cont_bottom, LV_ALIGN_BOTTOM_MID, 0, -24);

lv_obj_set_layout(cont_bottom, LV_LAYOUT_FLEX);
lv_obj_set_flex_flow(cont_bottom, LV_FLEX_FLOW_ROW);
lv_obj_set_style_pad_column(cont_bottom, 24, 0);
lv_obj_set_style_pad_left(cont_bottom, 24, 0);
lv_obj_set_style_pad_right(cont_bottom, 24, 0);


Create 4 equal-width cards:

for(int i = 0; i < 4; i++) {
    lv_obj_t * card = lv_obj_create(cont_bottom);
    lv_obj_add_style(card, &style_card, 0);
    lv_obj_set_flex_grow(card, 1);
}


Inside each, create caption + value using medium styles (cyan RPM, orange GEAR and SOC, etc.).

9. Voltage Monitor page (graph)

Layout:

Top: back button + title

Under title: 4 summary cards (CURRENT, AVERAGE, MAXIMUM, MINIMUM)

Main area: chart card with line graph

9.1. Back button and title
// Back container
lv_obj_t * bar_top = lv_obj_create(scr_graph);
lv_obj_add_style(bar_top, &style_screen_bg, 0);
lv_obj_set_size(bar_top, LV_PCT(100), 64);
lv_obj_align(bar_top, LV_ALIGN_TOP_MID, 0, 0);
lv_obj_set_layout(bar_top, LV_LAYOUT_FLEX);
lv_obj_set_flex_flow(bar_top, LV_FLEX_FLOW_ROW);
lv_obj_set_style_pad_left(bar_top, 16, 0);
lv_obj_set_style_pad_right(bar_top, 16, 0);
lv_obj_set_style_pad_column(bar_top, 16, 0);

// Back button (rounded)
lv_obj_t * btn_back = lv_btn_create(bar_top);
lv_obj_add_style(btn_back, &style_tab_base, 0);
lv_obj_set_style_bg_color(btn_back, col_bg_card, 0);
lv_obj_set_style_bg_opa(btn_back, LV_OPA_COVER, 0);

lv_obj_t * lbl_back = lv_label_create(btn_back);
lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " Back");

// Title
lv_obj_t * lbl_title = lv_label_create(bar_top);
lv_obj_add_style(lbl_title, &style_value_med_green, 0);
lv_label_set_text(lbl_title, LV_SYMBOL_CHARGE " Voltage Monitor");
lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);


(Use a green style similar to style_value_med_green but smaller font if needed.)

9.2. Top statistics row (4 cards)

Create another container under top bar with flex row and 4 style_card tiles. Each tile uses:

Caption: “CURRENT”, “AVERAGE”, etc. (muted text)

Value: green or cyan medium value

Small “V” unit to the right

Structurally it’s identical to other cards.

9.3. Chart card

Create a big card:

lv_obj_t * card_chart = lv_obj_create(scr_graph);
lv_obj_add_style(card_chart, &style_card, 0);
lv_obj_set_size(card_chart, LV_PCT(100), LV_PCT(100));
lv_obj_align(card_chart, LV_ALIGN_BOTTOM_MID, 0, -24);
lv_obj_set_style_pad_top(card_chart, 40, 0);  // caption area


Caption:

lv_obj_t * cap_chart = lv_label_create(card_chart);
lv_obj_add_style(cap_chart, &style_caption, 0);
lv_label_set_text(cap_chart, "VOLTAGE OVER TIME (1H)");
lv_obj_align(cap_chart, LV_ALIGN_TOP_LEFT, 0, 0);

9.4. LVGL chart configuration
lv_obj_t * chart = lv_chart_create(card_chart);
lv_obj_set_size(chart, LV_PCT(100), LV_PCT(100));
lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -8);

// Match background
lv_obj_set_style_bg_color(chart, col_bg_card, 0);
lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
lv_obj_set_style_border_width(chart, 0, 0);

// Grid lines
lv_obj_set_style_line_color(chart, lv_color_hex(COL_GRAPH_GRID), LV_PART_MAIN);
lv_chart_set_div_line_count(chart, 4, 6);
lv_obj_set_style_line_opa(chart, LV_OPA_30, LV_PART_MAIN);

// Axis text style (small muted)
lv_obj_set_style_text_color(chart, col_muted_text, 0);
lv_obj_set_style_text_font(chart, &montserrat_16, 0);

// Line series
lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

lv_chart_series_t * ser = lv_chart_add_series(chart, lv_color_hex(COL_GRAPH_LINE), LV_CHART_AXIS_PRIMARY_Y);

// Smooth curve
lv_chart_set_point_count(chart, 64);
lv_chart_set_zoom_x(chart, 200);  // zoom in a bit if desired


Then feed data:

for(int i = 0; i < 64; i++) {
    lv_chart_set_next_value(chart, ser, my_voltage_values[i]);  // 0-4095 scaled
}


To mimic the smooth line, use many sample points and not dots:

lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT); // hide point markers

10. Putting it together for a novice

Create a file styles.c / styles.h

Implement all style_init_* functions above.

Call them once after LVGL init:

void ui_style_init(void)
{
    style_init_screen_bg();
    style_init_card();
    style_init_caption();
    style_init_value_big();
    style_init_value_medium();
    style_init_navbar();
    style_init_tabs();
    // etc.
}


In ui.c, after ui_style_init(), create functions:

void ui_overview_create(void);

void ui_speed_focus_create(void);

void ui_voltage_monitor_create(void);

Each function:

Creates a screen (lv_obj_create(NULL)).

Adds background style.

Creates navbar with tabs.

Builds its own layout (grid/flex per section above).

Tab logic (simple)

When a tab button is pressed, call the appropriate ui_*_create() and lv_scr_load(scr_xxx);

Use LV_STATE_CHECKED on the active tab to apply the cyan style.

Data updating

For each numeric label, keep a lv_obj_t * handle in global variables.

When data changes (RPM, SOC, etc.), format strings and call:

static char rpm_buf[8];
lv_snprintf(rpm_buf, sizeof(rpm_buf), "%d", rpm_value);
lv_label_set_text(rpm_label, rpm_buf);


This preserves the style and glow.

If you follow these steps and copy the style values, you’ll get a UI extremely close to your screenshots:

Same dark navy background

Same neon cyan / orange / green big numerals with glow

Same rounded “card” layout

Same top nav tabs

Same chart look for the voltage graph.
