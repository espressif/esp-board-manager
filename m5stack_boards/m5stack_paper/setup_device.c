/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/* Touch factory for the M5Paper's GT911.
 *
 * This board declares an lcd_touch device but NO display_lcd — its IT8951E
 * e-paper controller has no bmgr device type (see board_devices.yaml). A touch
 * device alone still requires this translation unit: dev_lcd_touch's i2c path
 * calls lcd_touch_factory_entry_t, so without it the board fails to link with
 * `undefined reference to lcd_touch_factory_entry_t` — the same trap as a
 * display board missing its panel factory, minus the display.
 */

#include <string.h>
#include "esp_board_device.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#if __has_include(<esp_lcd_touch_gt911.h>)
#define HAS_GT911  1
#include "esp_lcd_touch_gt911.h"
#endif  /* __has_include(<esp_lcd_touch_gt911.h>) */
#include "esp_log.h"

#if defined(HAS_GT911)
__attribute__((weak)) esp_err_t lcd_touch_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *touch_dev_config, esp_lcd_touch_handle_t *ret_touch)
{
    esp_lcd_touch_config_t touch_cfg = {0};
    memcpy(&touch_cfg, touch_dev_config, sizeof(esp_lcd_touch_config_t));

    esp_err_t ret = esp_lcd_touch_new_i2c_gt911(io, &touch_cfg, ret_touch);
    if (ret != ESP_OK) {
        ESP_LOGE("lcd_touch_factory_entry_t", "Failed to create GT911 touch driver: %s", esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}
#endif  /* defined(HAS_GT911) */
