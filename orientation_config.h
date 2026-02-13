#pragma once

// Canonical display/touch orientation contract for JC8012P4A1C_I_W_Y.
// Logical UI space remains 1280x800 (landscape) while panel native is 800x1280 (portrait).

#define ORIENTATION_LOGICAL_WIDTH   1280
#define ORIENTATION_LOGICAL_HEIGHT  800
#define ORIENTATION_PANEL_WIDTH     800
#define ORIENTATION_PANEL_HEIGHT    1280

// Rotation policy from LVGL logical space to panel space.
// Current hardware profile uses a 90° CCW transform.
#define ORIENTATION_ROTATE_CCW_90   1

// Touch transform policy to align with the same display orientation.
// Apply in this order: optional swap XY, then optional invert X/Y.
#define ORIENTATION_TOUCH_SWAP_XY   1
#define ORIENTATION_TOUCH_INVERT_X  1
#define ORIENTATION_TOUCH_INVERT_Y  0
