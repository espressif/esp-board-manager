/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/* Panel factory for the M5Stack Tough.
 *
 * dev_display_lcd's SPI path calls lcd_panel_factory_entry_t to build the
 * concrete panel; without this translation unit the board does not link.
 *
 * The ILI9341 panel (2.0-inch 320x240 ILI9342C) comes from
 * espressif/esp_lcd_ili9341, named in board_devices.yaml; its built-in init
 * sequence is used, so no vendor init table is needed.
 */

#include <string.h>
#include "esp_board_device.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#if __has_include(<esp_lcd_ili9341.h>)
#define HAS_ILI9341  1
#include "esp_lcd_ili9341.h"
#endif  /* __has_include(<esp_lcd_ili9341.h>) */
#include "esp_log.h"

#if defined(HAS_ILI9341)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    esp_lcd_panel_dev_config_t panel_dev_cfg = {0};
    memcpy(&panel_dev_cfg, panel_dev_config, sizeof(esp_lcd_panel_dev_config_t));

    esp_err_t ret = esp_lcd_new_panel_ili9341(io, &panel_dev_cfg, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE("lcd_panel_factory_entry_t", "New ILI9341 panel failed");
        return ret;
    }
    return ESP_OK;
}
#endif  /* defined(HAS_ILI9341) */
