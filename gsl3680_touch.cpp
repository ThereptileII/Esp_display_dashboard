#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_gsl3680.h"
#include "gsl3680_touch.h"
#include <Arduino.h>
#include <cstring>

#define CONFIG_LCD_HRES 800
#define CONFIG_LCD_VRES 1280

static const char *TAG = "gsl3680";
static bool        s_verbose = false;

static esp_lcd_touch_handle_t    tp            = nullptr;
static esp_lcd_panel_io_handle_t tp_io_handle  = nullptr;
static i2c_master_bus_handle_t   s_i2c_bus     = nullptr;

uint16_t touch_strength[5];
uint8_t touch_cnt = 0;

gsl3680_touch::gsl3680_touch(int8_t sda_pin, int8_t scl_pin, int8_t rst_pin, int8_t int_pin)
{
    _sda = sda_pin;
    _scl = scl_pin;
    _rst = rst_pin;
    _int = int_pin;
}

bool gsl3680_touch::begin()
{
    Serial.printf("[gsl3680] begin(sda=%d scl=%d rst=%d int=%d)\n", _sda, _scl, _rst, _int);

    if (tp) {
        Serial.println("[gsl3680] already initialized; reusing existing driver");
        return true;
    }

    if (_sda < 0 || _scl < 0) {
        Serial.println("[gsl3680] ERROR: SDA/SCL pins not set");
        return false;
    }

    if (!s_i2c_bus) {
        i2c_master_bus_config_t bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = (gpio_num_t)_sda,
            .scl_io_num = (gpio_num_t)_scl,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags = {
                .enable_internal_pullup = true,
            },
        };

        esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_i2c_bus);
        if (err == ESP_ERR_INVALID_STATE) {
            Serial.println("[gsl3680] I2C bus already created; reusing");
            err = i2c_master_get_bus_handle(bus_cfg.i2c_port, &s_i2c_bus);
        }
        if (err != ESP_OK || !s_i2c_bus) {
            Serial.printf("[gsl3680] ERROR: i2c master bus unavailable: %s\n", esp_err_to_name(err));
            return false;
        }
        Serial.println("[gsl3680] master bus ready");
    } else {
        Serial.println("[gsl3680] reusing master bus");
    }

    const gpio_num_t rst_gpio = (_rst >= 0) ? (gpio_num_t)_rst : GPIO_NUM_NC;
    const gpio_num_t int_gpio = (_int >= 0) ? (gpio_num_t)_int : GPIO_NUM_NC;

    if (!tp_io_handle) {
        esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG();
        // The Arduino 3.x core ships the ESP-IDF v5 panel IO v2 helper, which expects
        // the I2C device config to carry a sane clock. The default from the macro is
        // zero, so explicitly request 400 kHz to avoid "invalid scl frequency" when
        // esp_lcd_new_panel_io_i2c() calls i2c_master_bus_add_device().
        tp_io_config.scl_speed_hz = 400000;
        ESP_LOGI(TAG, "Initialize touch IO (I2C)");
        esp_err_t err = esp_lcd_new_panel_io_i2c(s_i2c_bus, &tp_io_config, &tp_io_handle);
        if (err != ESP_OK) {
            Serial.printf("[gsl3680] ERROR: new_panel_io_i2c failed: %s\n", esp_err_to_name(err));
            return false;
        }
    } else {
        Serial.println("[gsl3680] reusing touch IO handle");
    }

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = CONFIG_LCD_HRES,
        .y_max = CONFIG_LCD_VRES,
        .rst_gpio_num = rst_gpio,
        // Prefer the dedicated INT line when available so the driver can
        // properly select the I2C address and configure interrupts. Fall back
        // to polling only if the pin is unavailable.
        .int_gpio_num = int_gpio,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    Serial.printf("[gsl3680] RST=%d INT=%d; polling %s\n", (int)tp_cfg.rst_gpio_num, (int)tp_cfg.int_gpio_num,
                  tp_cfg.int_gpio_num == GPIO_NUM_NC ? "ENABLED" : "disabled");

    if (!tp) {
        ESP_LOGI(TAG, "Initialize touch controller gsl3680");
        esp_err_t err = esp_lcd_touch_new_i2c_gsl3680(tp_io_handle, &tp_cfg, &tp);
        if (err != ESP_OK) {
            Serial.printf("[gsl3680] ERROR: touch_new_i2c_gsl3680 failed: %s\n", esp_err_to_name(err));
            return false;
        }
    }

    Serial.println("[gsl3680] init OK");
    return true;
}

void gsl3680_touch::set_verbose(bool v) { s_verbose = v; }

bool gsl3680_touch::getTouch(uint16_t *x, uint16_t *y, uint8_t *count_out, uint16_t *strength_out)
{
    if (!x || !y) {
        Serial.println("[gsl3680] WARN: null coordinate pointers passed to getTouch");
        return false;
    }

    *x = *y = 0;
    touch_cnt = 0;
    memset(touch_strength, 0, sizeof(touch_strength));

    if (!tp) {
        Serial.println("[gsl3680] ERROR: touch handle is null; begin() probably failed");
        *x = *y = 0;
        return false;
    }

    esp_err_t err = esp_lcd_touch_read_data(tp);
    if (err != ESP_OK) {
        static uint32_t last_err_ms = 0;
        uint32_t now = millis();
        if (now - last_err_ms > 500) {
            Serial.printf("[gsl3680] read_data failed: %s\n", esp_err_to_name(err));
            last_err_ms = now;
        }
        *x = *y = 0;
        return false;
    }

    uint16_t x_buf[5]         = {0};
    uint16_t y_buf[5]         = {0};
    uint16_t strength_buf[5]  = {0};
    uint8_t  coord_count      = 0;
    bool     touchpad_pressed = esp_lcd_touch_get_coordinates(tp, x_buf, y_buf, strength_buf, &coord_count, 5);
    touch_cnt                 = coord_count;
    if (touch_cnt > 0) {
        *x               = x_buf[0];
        *y               = y_buf[0];
        touch_strength[0] = strength_buf[0];
    }

    if (s_verbose) {
        Serial.printf("[gsl3680] helper pressed=%d count=%u raw_points=[", (int)touchpad_pressed, touch_cnt);
        for (uint8_t i = 0; i < touch_cnt && i < 5; ++i) {
            Serial.printf("(%u,%u s=%u)%s", x_buf[i], y_buf[i], strength_buf[i],
                          (i + 1 < touch_cnt && i + 1 < 5) ? ", " : "");
        }
        Serial.println("]");

        if (touch_cnt > 0) {
            bool in_range = (x_buf[0] < CONFIG_LCD_HRES) && (y_buf[0] < CONFIG_LCD_VRES);
            Serial.printf("[gsl3680] raw[0] before rotation: (%u,%u) max=(%u,%u)%s\n", x_buf[0], y_buf[0],
                          CONFIG_LCD_HRES - 1, CONFIG_LCD_VRES - 1, in_range ? "" : " OUT_OF_RANGE");
        }
    }

    // If the esp_lcd helper returns no points, fall back to a direct register read so we
    // can still surface events even if the algorithm path fails to populate XY_Coordinate.
    if (!touchpad_pressed && tp_io_handle) {
        uint8_t raw[24] = {0};
        err = esp_lcd_panel_io_rx_param(tp_io_handle, 0x80, raw, sizeof(raw));
        if (err == ESP_OK && raw[0] > 0) {
            touchpad_pressed = true;
            touch_cnt        = raw[0];
            *x = (uint16_t)(((raw[7] & 0x0F) << 8) | raw[6]);
            *y = (uint16_t)((raw[5] << 8) | raw[4]);
            touch_strength[0] = raw[3];
            Serial.printf("[gsl3680] fallback raw count=%u xy=(%u,%u) strength=%u\n",
                          touch_cnt, *x, *y, touch_strength[0]);
        }
    }

    static bool last_pressed = false;
    if (touchpad_pressed || last_pressed != touchpad_pressed) {
        Serial.printf("[gsl3680] polled: pressed=%d count=%u raw=(%u,%u) strength0=%u\n",
                      (int)touchpad_pressed, touch_cnt, *x, *y,
                      touch_cnt ? touch_strength[0] : 0);
        last_pressed = touchpad_pressed;
    }

    if (count_out) *count_out = touch_cnt;
    if (strength_out) *strength_out = touch_cnt ? touch_strength[0] : 0;

    return touchpad_pressed;
}

void gsl3680_touch::set_rotation(uint8_t r) {
    if (!tp) {
        Serial.println("[gsl3680] WARN: set_rotation called before init");
        return;
    }
    switch (r & 3) {
        case 0: // 0°
            esp_lcd_touch_set_swap_xy(tp, false);
            esp_lcd_touch_set_mirror_x(tp, false);
            esp_lcd_touch_set_mirror_y(tp, false);
            break;
        case 1: // 90°
            esp_lcd_touch_set_swap_xy(tp, true);
            esp_lcd_touch_set_mirror_x(tp, true);
            esp_lcd_touch_set_mirror_y(tp, false);
            break;
        case 2: // 180°
            esp_lcd_touch_set_swap_xy(tp, false);
            esp_lcd_touch_set_mirror_x(tp, true);
            esp_lcd_touch_set_mirror_y(tp, true);
            break;
        case 3: // 270°
            esp_lcd_touch_set_swap_xy(tp, true);
            esp_lcd_touch_set_mirror_x(tp, false);
            esp_lcd_touch_set_mirror_y(tp, true);
            break;
    }
}
