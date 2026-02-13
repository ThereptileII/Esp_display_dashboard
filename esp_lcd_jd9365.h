/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_mipi_dsi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int cmd;
    const void *data;
    size_t data_bytes;
    unsigned int delay_ms;
} jd9365_lcd_init_cmd_t;

typedef struct {
    const jd9365_lcd_init_cmd_t *init_cmds;
    uint16_t init_cmds_size;
    struct {
        esp_lcd_dsi_bus_handle_t dsi_bus;
        const esp_lcd_dpi_panel_config_t *dpi_config;
        uint8_t lane_num;
    } mipi_config;
} jd9365_vendor_config_t;

esp_err_t esp_lcd_new_panel_jd9365(const esp_lcd_panel_io_handle_t io,
                                   const esp_lcd_panel_dev_config_t *panel_dev_config,
                                   esp_lcd_panel_handle_t *ret_panel);

#define JD9365_PANEL_BUS_DSI_2CH_CONFIG()                \
    {                                                    \
        .bus_id = 0,                                     \
        .num_data_lanes = 2,                             \
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,     \
        .lane_bit_rate_mbps = 1500,                      \
    }

#define JD9365_PANEL_IO_DBI_CONFIG()  \
    {                                 \
        .virtual_channel = 0,         \
        .lcd_cmd_bits = 8,            \
        .lcd_param_bits = 8,          \
    }

#define JD9365_TIMING_PROFILE_DEFAULT_60HZ 0
#define JD9365_TIMING_PROFILE_COMPAT_STABLE 1

#ifndef JD9365_TIMING_PROFILE
#define JD9365_TIMING_PROFILE JD9365_TIMING_PROFILE_DEFAULT_60HZ
#endif

#define JD9365_800_1280_PANEL_60HZ_DPI_CONFIG(px_format) \
    {                                                     \
        .virtual_channel = 0,                             \
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,      \
        .dpi_clock_freq_mhz = 60,                         \
        .pixel_format = px_format,                        \
        .num_fbs = 1,                                     \
        .video_timing = {                                 \
            .h_size = 800,                                \
            .v_size = 1280,                               \
            .hsync_pulse_width = 20,                      \
            .hsync_back_porch = 20,                       \
            .hsync_front_porch = 40,                      \
            .vsync_pulse_width = 4,                       \
            .vsync_back_porch = 8,                        \
            .vsync_front_porch = 20,                      \
        },                                                \
        .flags = {                                        \
            .use_dma2d = true,                            \
        },                                                \
    }

#define JD9365_800_1280_PANEL_COMPAT_STABLE_DPI_CONFIG(px_format) \
    {                                                              \
        .virtual_channel = 0,                                      \
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,               \
        .dpi_clock_freq_mhz = 52,                                  \
        .pixel_format = px_format,                                 \
        .num_fbs = 1,                                              \
        .video_timing = {                                          \
            .h_size = 800,                                         \
            .v_size = 1280,                                        \
            .hsync_pulse_width = 24,                               \
            .hsync_back_porch = 32,                                \
            .hsync_front_porch = 56,                               \
            .vsync_pulse_width = 6,                                \
            .vsync_back_porch = 14,                                \
            .vsync_front_porch = 26,                               \
        },                                                         \
        .flags = {                                                 \
            .use_dma2d = true,                                     \
        },                                                         \
    }

#if JD9365_TIMING_PROFILE == JD9365_TIMING_PROFILE_COMPAT_STABLE
#define JD9365_ACTIVE_DPI_CONFIG(px_format) JD9365_800_1280_PANEL_COMPAT_STABLE_DPI_CONFIG(px_format)
#else
#define JD9365_ACTIVE_DPI_CONFIG(px_format) JD9365_800_1280_PANEL_60HZ_DPI_CONFIG(px_format)
#endif

#endif

#ifdef __cplusplus
}
#endif
