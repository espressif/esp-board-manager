/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <string.h>
#include "esp_log.h"
#include "esp_io_expander.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#if __has_include(<esp_io_expander_aw9523.h>)
#define HAS_AW9523  1
#include "esp_io_expander_aw9523.h"
#endif  /* __has_include(<esp_io_expander_aw9523.h>) */
#if __has_include(<esp_lcd_ili9341.h>)
#define HAS_ILI9341  1
#include "esp_lcd_ili9341.h"
#endif  /* __has_include(<esp_lcd_ili9341.h>) */
#if __has_include(<esp_lcd_touch_ft5x06.h>)
#define HAS_FT5X06  1
#include "esp_lcd_touch_ft5x06.h"
#endif  /* __has_include(<esp_lcd_touch_ft5x06.h>) */

static const char *TAG = "M5STACK_CORES3_SETUP_DEVICE";

#if defined(HAS_AW9523)
__attribute__((weak)) esp_err_t io_expander_factory_entry_t(i2c_master_bus_handle_t i2c_handle, const uint16_t dev_addr, esp_io_expander_handle_t *handle_ret)
{
    return esp_io_expander_new_i2c_aw9523(i2c_handle, dev_addr, handle_ret);
}
#endif  /* defined(HAS_AW9523) */

#if defined(HAS_ILI9341)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    esp_lcd_panel_dev_config_t panel_dev_cfg = {0};
    memcpy(&panel_dev_cfg, panel_dev_config, sizeof(esp_lcd_panel_dev_config_t));
    int ret = esp_lcd_new_panel_ili9341(io, &panel_dev_cfg, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "New ili9341 panel failed");
    }
    return ret;
}
#endif  /* defined(HAS_ILI9341) */

#if defined(HAS_FT5X06)
__attribute__((weak)) esp_err_t lcd_touch_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *touch_dev_config, esp_lcd_touch_handle_t *ret_touch)
{
    esp_err_t ret = esp_lcd_touch_new_i2c_ft5x06(io, touch_dev_config, ret_touch);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ft5x06 touch driver: %s", esp_err_to_name(ret));
    }
    return ret;
}
#endif  /* defined(HAS_FT5X06) */
