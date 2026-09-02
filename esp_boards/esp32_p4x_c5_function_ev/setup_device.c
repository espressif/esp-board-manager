/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/* Panel + touch factories for the ESP32-P4X-C5-Function-EV-Board.
 *
 * dev_display_lcd's DSI path calls lcd_dsi_panel_factory_entry_t and
 * dev_lcd_touch's I2C path calls lcd_touch_factory_entry_t; without this
 * translation unit the board does not link.
 *
 * COPIED VERBATIM (apart from TAG and this comment) from esp-board-manager
 * esp_boards/esp32_p4_function_ev_board/setup_device.c. This board differs from
 * that one only in the companion radio module (ESP32-C5-MINI-1) and two of its
 * control pins; the same 7-inch 1024x600 unit sits on the same LCD adapter
 * board, and this board's display_lcd and lcd_touch device blocks are
 * byte-identical to upstream's, down to the DSI timings, the reset GPIO and the
 * GT911 addresses. Nothing is adapted and nothing is invented.
 *
 * No vendor init table: esp_lcd_ek79007 carries its own init sequence, and
 * esp_lcd_touch_gt911 its own probe, exactly as upstream relies on.
 *
 * PANEL IDENTITY: board_devices.yaml declares ek79007 and this factory follows
 * it. The user guide's Related Documents also lists an EK73217BCGA datasheet
 * beside the EK79007AD one without saying which the shipped panel carries;
 * there is no esp_lcd_ek73217 component in the registry or in esp-bsp, so
 * EK73217BCGA is not a driver identity anything could be built against. If a
 * real board is ever found not to init, that is the ambiguity to revisit.
 */

#include <string.h>
#include "esp_log.h"
#include "esp_board_manager_includes.h"
#if __has_include(<esp_lcd_ek79007.h>)
#define HAS_EK79007  1
#include "esp_lcd_ek79007.h"
#endif  /* __has_include(<esp_lcd_ek79007.h>) */
#if __has_include(<esp_lcd_touch_gt911.h>)
#define HAS_GT911  1
#include "esp_lcd_touch_gt911.h"
#endif  /* __has_include(<esp_lcd_touch_gt911.h>) */

static const char *TAG = "P4X_C5_FUNCTION_EV_SETUP_DEVICE";

#if defined(HAS_EK79007)
__attribute__((weak)) esp_err_t lcd_dsi_panel_factory_entry_t(esp_lcd_dsi_bus_handle_t dsi_handle, dev_display_lcd_config_t *lcd_cfg, dev_display_lcd_handles_t *lcd_handles)
{
    ek79007_vendor_config_t vendor_config = {
        .mipi_config = {
            .dsi_bus = dsi_handle,
            .dpi_config = &lcd_cfg->sub_cfg.dsi.dpi_config,
        },
    };

    esp_lcd_panel_dev_config_t lcd_dev_config = {
        .reset_gpio_num = lcd_cfg->sub_cfg.dsi.reset_gpio_num,
        .rgb_ele_order = lcd_cfg->rgb_ele_order,
        .bits_per_pixel = lcd_cfg->bits_per_pixel,
        .data_endian = lcd_cfg->data_endian,
        .flags = {
            .reset_active_high = lcd_cfg->sub_cfg.dsi.reset_active_high,
        },
        .vendor_config = &vendor_config,
    };

    esp_err_t ret = esp_lcd_new_panel_ek79007(lcd_handles->io_handle, &lcd_dev_config, &lcd_handles->panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ek79007 panel: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    return ESP_OK;
}
#endif  /* defined(HAS_EK79007) */
#if defined(HAS_GT911)
__attribute__((weak)) esp_err_t lcd_touch_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *touch_dev_config, esp_lcd_touch_handle_t *ret_touch)
{
    esp_err_t ret = esp_lcd_touch_new_i2c_gt911(io, touch_dev_config, ret_touch);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create gt911 touch driver: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}
#endif  /* defined(HAS_GT911) */
