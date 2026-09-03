/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/* Panel factory for the M5Stack Station 485.
 *
 * dev_display_lcd's SPI path calls lcd_panel_factory_entry_t to build the
 * concrete panel; without this translation unit the board does not link.
 *
 * The ST7789 panel (1.14-inch 240x135 IPS) ships inside esp_lcd, so no extra
 * component and no vendor init table is needed.
 *
 * NOTE: The 240x135 landscape window is offset inside the controller's 240x320
 * GRAM. dev_display_lcd cannot express a GRAM offset, so the application must
 * call esp_lcd_panel_set_gap(panel, 52, 40) after init; a driver that ignores
 * it renders shifted.
 */

#include <string.h>
#include "esp_board_device.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#if __has_include(<esp_lcd_panel_vendor.h>)
#define HAS_LCD_PANEL_VENDOR  1
#include "esp_lcd_panel_vendor.h"
#endif  /* __has_include(<esp_lcd_panel_vendor.h>) */
#include "esp_log.h"

#if defined(HAS_LCD_PANEL_VENDOR)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    esp_lcd_panel_dev_config_t panel_dev_cfg = {0};
    memcpy(&panel_dev_cfg, panel_dev_config, sizeof(esp_lcd_panel_dev_config_t));

    esp_err_t ret = esp_lcd_new_panel_st7789(io, &panel_dev_cfg, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE("lcd_panel_factory_entry_t", "New ST7789 panel failed");
        return ret;
    }
    return ESP_OK;
}
#endif  /* defined(HAS_LCD_PANEL_VENDOR) */
