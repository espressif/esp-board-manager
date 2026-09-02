/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include "driver/i2c_master.h"
#include "esp_board_entry.h"
#include "esp_board_periph.h"
#include "esp_err.h"
#include "esp_log.h"

#include "dev_custom.h"
#include "gen_board_device_custom.h"
#if __has_include(<lp5562.h>)
#define HAS_LP5562  1
#include "lp5562.h"
#endif  /* __has_include(<lp5562.h>) */

#if defined(HAS_LP5562)

static const char *TAG = "M5_ATOM_S3R_LP5562";

static lp5562_handle_t s_lp5562;
static const char *s_peripheral_name;
static bool s_peripheral_ref_held;

static int lp5562_backlight_init(void *config, int config_size, void **device_handle)
{
    if (config == NULL || device_handle == NULL ||
        config_size != sizeof(dev_custom_lp5562_backlight_config_t)) {
        return ESP_ERR_INVALID_ARG;
    }
    *device_handle = NULL;

    const dev_custom_lp5562_backlight_config_t *cfg = config;
    if (cfg->peripheral_name == NULL || cfg->i2c_addr <= 0 ||
        cfg->default_percent < 0 || cfg->default_percent > 100) {
        ESP_LOGE(TAG, "Invalid LP5562 configuration");
        return ESP_ERR_INVALID_ARG;
    }
    if (s_lp5562 != NULL || s_peripheral_ref_held) {
        ESP_LOGE(TAG, "LP5562 backlight is already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_master_bus_handle_t i2c_bus = NULL;
    esp_err_t ret = esp_board_periph_ref_handle(cfg->peripheral_name, (void **)&i2c_bus);
    if (ret != ESP_OK || i2c_bus == NULL) {
        ESP_LOGE(TAG, "Failed to reference I2C peripheral %s: %s", cfg->peripheral_name,
                 esp_err_to_name(ret));
        return ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
    }

    ret = lp5562_new(i2c_bus, (uint8_t)cfg->i2c_addr, &s_lp5562);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LP5562: %s", esp_err_to_name(ret));
        esp_err_t unref_ret = esp_board_periph_unref_handle(cfg->peripheral_name);
        if (unref_ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to release I2C peripheral %s: %s", cfg->peripheral_name,
                     esp_err_to_name(unref_ret));
        }
        return ret;
    }
    ret = lp5562_set_brightness(s_lp5562, (uint8_t)cfg->default_percent);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LP5562 brightness: %s", esp_err_to_name(ret));
        esp_err_t cleanup_ret = lp5562_del(s_lp5562);
        if (cleanup_ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete LP5562 during cleanup: %s",
                     esp_err_to_name(cleanup_ret));
            return cleanup_ret;
        }
        s_lp5562 = NULL;
        esp_err_t unref_ret = esp_board_periph_unref_handle(cfg->peripheral_name);
        if (unref_ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to release I2C peripheral %s: %s", cfg->peripheral_name,
                     esp_err_to_name(unref_ret));
        }
        return ret;
    }

    s_peripheral_name = cfg->peripheral_name;
    s_peripheral_ref_held = true;
    *device_handle = s_lp5562;
    return ESP_OK;
}

static int lp5562_backlight_deinit(void *device_handle)
{
    if (device_handle == NULL || device_handle != s_lp5562 || !s_peripheral_ref_held) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = lp5562_del(s_lp5562);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to delete LP5562: %s", esp_err_to_name(ret));
        return ret;
    }
    s_lp5562 = NULL;

    ret = esp_board_periph_unref_handle(s_peripheral_name);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to release I2C peripheral %s: %s", s_peripheral_name,
                 esp_err_to_name(ret));
        return ret;
    }
    s_peripheral_name = NULL;
    s_peripheral_ref_held = false;
    return ESP_OK;
}

CUSTOM_DEVICE_IMPLEMENT(lp5562_backlight, lp5562_backlight_init, lp5562_backlight_deinit);
#endif  /* defined(HAS_LP5562) */
