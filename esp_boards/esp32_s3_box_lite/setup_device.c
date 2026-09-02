/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include "esp_board_device.h"
#include "esp_codec_dev.h"
#include "esp_log.h"

#if __has_include(<esp_lcd_panel_st7789.h>)
#define HAS_ST7789  1
#include "esp_lcd_panel_st7789.h"
#endif  /* __has_include(<esp_lcd_panel_st7789.h>) */

static const char *TAG = "BOX_LITE_SETUP_DEVICE";

#if defined(HAS_ST7789)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    esp_lcd_panel_dev_config_t panel_dev_cfg = {0};
    memcpy(&panel_dev_cfg, panel_dev_config, sizeof(esp_lcd_panel_dev_config_t));
    int ret = esp_lcd_new_panel_st7789(io, &panel_dev_cfg, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "New ili9341 panel failed");
        return ret;
    }
    return ESP_OK;
}
#endif  /* defined(HAS_ST7789) */
