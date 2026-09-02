/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#if __has_include(<esp_lcd_st7735.h>)
#define HAS_ST7735  1
#include "esp_log.h"
#include "esp_lcd_st7735.h"
#endif  /* __has_include(<esp_lcd_st7735.h>) */

#if defined(HAS_ST7735)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_panel_dev_config_t *panel_dev_config,
                                                          esp_lcd_panel_handle_t *ret_panel)
{
    static const char *TAG = "M5_ATOM_S3R_LCD";
    const int gap_x = 2;
    const int gap_y = 3;

    esp_err_t ret = esp_lcd_new_panel_st7735(io, panel_dev_config, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ST7735 panel: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_lcd_panel_set_gap(*ret_panel, gap_x, gap_y);
    if (ret == ESP_OK) {
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to set ST7735 panel gap: %s", esp_err_to_name(ret));
    esp_err_t cleanup_ret = esp_lcd_panel_del(*ret_panel);
    if (cleanup_ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to delete ST7735 panel during cleanup: %s",
                 esp_err_to_name(cleanup_ret));
    }
    *ret_panel = NULL;
    return ret;
}
#endif  /* defined(HAS_ST7735) */
