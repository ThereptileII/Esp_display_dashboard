#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_gsl3680.h"
#include "gsl3680_touch.h"
#include <Arduino.h>

#define CONFIG_LCD_HRES 800
#define CONFIG_LCD_VRES 1280

static const char *TAG = "gsl3680";

esp_lcd_touch_handle_t tp;
esp_lcd_panel_io_handle_t tp_io_handle;
static i2c_master_bus_handle_t s_i2c_bus = nullptr;

uint16_t touch_strength[1];
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

    if (_sda < 0 || _scl < 0) {
        Serial.println("[gsl3680] ERROR: SDA/SCL pins not set");
        return false;
    }

    if (s_i2c_bus) {
        i2c_del_master_bus(s_i2c_bus);
        s_i2c_bus = nullptr;
    }

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
    if (err != ESP_OK) {
        Serial.printf("[gsl3680] ERROR: i2c_new_master_bus failed: %s\n", esp_err_to_name(err));
        return false;
    }
    Serial.println("[gsl3680] master bus ready");

    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG();
    ESP_LOGI(TAG, "Initialize touch IO (I2C)");
    err = esp_lcd_new_panel_io_i2c(s_i2c_bus, &tp_io_config, &tp_io_handle);
    if (err != ESP_OK) {
        Serial.printf("[gsl3680] ERROR: new_panel_io_i2c failed: %s\n", esp_err_to_name(err));
        return false;
    }

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = CONFIG_LCD_HRES,
        .y_max = CONFIG_LCD_VRES,
        .rst_gpio_num = (gpio_num_t)_rst,
        .int_gpio_num = (gpio_num_t)_int,
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

    ESP_LOGI(TAG, "Initialize touch controller gsl3680");
    err = esp_lcd_touch_new_i2c_gsl3680(tp_io_handle, &tp_cfg, &tp);
    if (err != ESP_OK) {
        Serial.printf("[gsl3680] ERROR: touch_new_i2c_gsl3680 failed: %s\n", esp_err_to_name(err));
        return false;
    }

    Serial.println("[gsl3680] init OK");
    return true;
}

bool gsl3680_touch::getTouch(uint16_t *x, uint16_t *y, uint8_t *count_out, uint16_t *strength_out)
{
    if (!x || !y) {
        Serial.println("[gsl3680] WARN: null coordinate pointers passed to getTouch");
        return false;
    }

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

    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, x, y, touch_strength, &touch_cnt, 1);

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
