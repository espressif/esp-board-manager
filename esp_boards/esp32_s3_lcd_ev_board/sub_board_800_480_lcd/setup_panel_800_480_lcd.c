/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 *
 * Strong override applied via -a amend for the 800x480 LCD sub-board
 * (ST7262 RGB panel + GT1151 capacitive touch; silkscreen SUB3).
 *
 * The base board's setup_device.c provides weak factory entries for FT5x06
 * touch and GC9503 RGB panel. For this sub-board:
 *
 *   - display_lcd uses sub_type=rgb, which calls esp_lcd_new_rgb_panel()
 *     directly from dev_display_lcd_sub_rgb.c and never invokes
 *     lcd_panel_factory_entry_t, so no panel factory override is needed.
 *   - lcd_touch switches to GT1151; we provide a strong
 *     lcd_touch_factory_entry_t that overrides the base's weak FT5x06 entry.
 */

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#if __has_include(<esp_lcd_touch_gt1151.h>)
#define HAS_GT1151  1
#include "esp_lcd_touch_gt1151.h"
#endif  /* __has_include(<esp_lcd_touch_gt1151.h>) */

static const char *TAG = "S3_LCD_EV_BOARD_800_480_LCD_SETUP";

#if defined(HAS_GT1151)
esp_err_t lcd_touch_factory_entry_t(const esp_lcd_panel_io_handle_t io,
                                    const esp_lcd_touch_config_t *config,
                                    esp_lcd_touch_handle_t *tp)
{
    esp_err_t ret = esp_lcd_touch_new_i2c_gt1151(io, config, tp);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create GT1151 touch: %s", esp_err_to_name(ret));
    }
    return ret;
}
#endif  /* defined(HAS_GT1151) */
