/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/* Panel factory for the ESP32-S3-USB-OTG's 240x240 ST7789.
 *
 * dev_display_lcd's SPI path calls lcd_panel_factory_entry_t to build the
 * concrete panel; without this translation unit the board does not link.
 * The ST7789 driver ships inside esp_lcd, so no extra component dependency
 * and no vendor init table are needed.
 *
 * Source for "no vendor init table": esp-bsp bsp/esp32_s3_usb_otg/
 * esp32_s3_usb_otg.c calls esp_lcd_new_panel_st7789() directly and then only
 * esp_lcd_panel_invert_color(true); swap_xy/mirror stay false, matching
 * board_devices.yaml.
 *
 * GRAM offset: none. The 240x240 window starts at the origin of the
 * controller's 240x320 frame buffer, and esp-bsp calls no
 * esp_lcd_panel_set_gap() for this board.
 */

#include <string.h>
#include "esp_board_device.h"
#include "esp_log.h"

#if __has_include(<esp_lcd_panel_vendor.h>)
#define HAS_ST7789  1
#include "esp_lcd_panel_vendor.h"
#endif  /* __has_include(<esp_lcd_panel_vendor.h>) */

static const char *TAG = "S3_USB_OTG_SETUP_DEVICE";

#if defined(HAS_ST7789)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    esp_lcd_panel_dev_config_t panel_dev_cfg = {0};
    memcpy(&panel_dev_cfg, panel_dev_config, sizeof(esp_lcd_panel_dev_config_t));

    esp_err_t ret = esp_lcd_new_panel_st7789(io, &panel_dev_cfg, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "New ST7789 panel failed: %s", esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}
#endif  /* defined(HAS_ST7789) */
