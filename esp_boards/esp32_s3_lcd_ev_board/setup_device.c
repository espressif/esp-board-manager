/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_board_manager_includes.h"
#if __has_include(<esp_io_expander_tca9554.h>)
#define HAS_TCA9554  1
#include "esp_io_expander_tca9554.h"
#endif  /* __has_include(<esp_io_expander_tca9554.h>) */
#if __has_include(<esp_lcd_gc9503.h>)
#define HAS_GC9503  1
#include "esp_lcd_gc9503.h"
#endif  /* __has_include(<esp_lcd_gc9503.h>) */
#if __has_include(<esp_lcd_touch_ft5x06.h>)
#define HAS_FT5X06  1
#include "esp_lcd_touch_ft5x06.h"
#endif  /* __has_include(<esp_lcd_touch_ft5x06.h>) */

static const char *TAG = "S3_LCD_EV_BOARD_SETUP";

#define GC9503_VSYNC_GPIO  GPIO_NUM_3

#if defined(HAS_TCA9554)
__attribute__((weak)) esp_err_t io_expander_factory_entry_t(i2c_master_bus_handle_t i2c_handle,
                                                            const uint16_t dev_addr,
                                                            esp_io_expander_handle_t *handle_ret)
{
    esp_err_t ret = esp_io_expander_new_i2c_tca9554(i2c_handle, dev_addr, handle_ret);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create TCA9554 IO expander: %s", esp_err_to_name(ret));
    }
    return ret;
}
#endif  /* defined(HAS_TCA9554) */

#if defined(HAS_GC9503)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_panel_dev_config_t *panel_dev_config,
                                                          esp_lcd_panel_handle_t *ret_panel)
{
    ESP_RETURN_ON_FALSE(panel_dev_config, ESP_ERR_INVALID_ARG, TAG, "panel_dev_config is NULL");

    dev_display_lcd_config_t *lcd_cfg = NULL;
    ESP_RETURN_ON_ERROR(esp_board_device_get_config("display_lcd", (void **)&lcd_cfg), TAG, "Get LCD config failed");
    ESP_RETURN_ON_FALSE(lcd_cfg, ESP_ERR_INVALID_STATE, TAG, "display_lcd config is NULL");
    ESP_RETURN_ON_FALSE(strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_RGB_3WIRE_SPI) == 0, ESP_ERR_INVALID_STATE,
                        TAG, "display_lcd is not configured as rgb_3wire_spi");

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << GC9503_VSYNC_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = true,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "Configure GC9503 VSYNC GPIO failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(GC9503_VSYNC_GPIO, 1), TAG, "Set GC9503 VSYNC GPIO failed");

    esp_lcd_panel_dev_config_t panel_cfg = {0};
    memcpy(&panel_cfg, panel_dev_config, sizeof(panel_cfg));

    gc9503_vendor_config_t vendor_config = {
        .rgb_config = &lcd_cfg->sub_cfg.rgb_3wire_spi.rgb_panel_config,
        .flags = {
            .mirror_by_cmd = 0,
            .auto_del_panel_io = lcd_cfg->sub_cfg.rgb_3wire_spi.auto_del_panel_io,
        },
    };
    panel_cfg.vendor_config = &vendor_config;

    esp_err_t ret = esp_lcd_new_panel_gc9503(io, &panel_cfg, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create GC9503 panel: %s", esp_err_to_name(ret));
    }
    return ret;
}
#endif  /* defined(HAS_GC9503) */

#if defined(HAS_FT5X06)
__attribute__((weak)) esp_err_t lcd_touch_factory_entry_t(const esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_touch_config_t *config,
                                                          esp_lcd_touch_handle_t *tp)
{
    esp_err_t ret = esp_lcd_touch_new_i2c_ft5x06(io, config, tp);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create FT5x06 touch: %s", esp_err_to_name(ret));
    }
    return ret;
}
#endif  /* defined(HAS_FT5X06) */
