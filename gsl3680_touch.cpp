#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_gsl3680.h"
#include "gsl3680_touch.h"
#include <Arduino.h>

#define CONFIG_LCD_HRES 800
#define CONFIG_LCD_VRES 1280

static const char *TAG = "gsl3680";

esp_lcd_touch_handle_t tp;
esp_lcd_panel_io_handle_t tp_io_handle;
static bool s_i2c_ready = false;

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

    // Bring up the legacy I2C driver so we stay compatible with esp_lcd's current IO layer
    i2c_driver_delete(I2C_NUM_0);  // ignore errors; just ensures a clean slate

    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = (gpio_num_t)_sda,
        .scl_io_num = (gpio_num_t)_scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = 400000,
        },
        .clk_flags = 0,
    };

    esp_err_t err = i2c_param_config(I2C_NUM_0, &cfg);
    if (err != ESP_OK) {
        Serial.printf("[gsl3680] ERROR: i2c_param_config failed: %s\n", esp_err_to_name(err));
        return false;
    }

    err = i2c_driver_install(I2C_NUM_0, cfg.mode, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        Serial.printf("[gsl3680] ERROR: i2c_driver_install failed: %s\n", esp_err_to_name(err));
        return false;
    }
    s_i2c_ready = true;
    Serial.println("[gsl3680] legacy I2C driver ready");

    // Quick probe to confirm the device ACKs on the expected address
    uint8_t ping_reg = 0;
    err = i2c_master_write_to_device(I2C_NUM_0, ESP_LCD_TOUCH_IO_I2C_GSL3680_ADDRESS, &ping_reg, 1, 50 / portTICK_PERIOD_MS);
    if (err != ESP_OK) {
        Serial.printf("[gsl3680] ERROR: device did not ACK at 0x%02X: %s\n", ESP_LCD_TOUCH_IO_I2C_GSL3680_ADDRESS, esp_err_to_name(err));
        return false;
    }
    Serial.println("[gsl3680] I2C ping OK");

    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG();
    ESP_LOGI(TAG, "Initialize touch IO (I2C)");
    if (!s_i2c_ready) {
        Serial.println("[gsl3680] ERROR: I2C bus not ready");
        return false;
    }
    err = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &tp_io_config, &tp_io_handle);
    if (err != ESP_OK) {
        Serial.printf("[gsl3680] ERROR: new_panel_io_i2c failed: %s\n", esp_err_to_name(err));
        return false;
    }

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = CONFIG_LCD_HRES,
        .y_max = CONFIG_LCD_VRES,
        .rst_gpio_num = (gpio_num_t)_rst,
        // Force polling mode for now by ignoring the INT line.
        .int_gpio_num = GPIO_NUM_NC,
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

    Serial.printf("[gsl3680] INT pin configured as %d; polling %s\n", (int)tp_cfg.int_gpio_num,
                  tp_cfg.int_gpio_num == GPIO_NUM_NC ? "ENABLED" : "disabled");

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
