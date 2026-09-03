/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/* Panel factory for the ESP32-LCDKit's ILI9341.
 *
 * dev_display_lcd's SPI path calls lcd_panel_factory_entry_t to build the
 * concrete panel; without this translation unit the board does not link.
 *
 * The ILI9341 driver does NOT ship inside esp_lcd — it is the managed
 * component espressif/esp_lcd_ili9341, already declared under this device's
 * `dependencies:` in board_devices.yaml.
 *
 * No vendor init table: the kit's reference code (esp-iot-solution
 * examples/common_components/boards/esp32-lcdkit) drives the panel with the
 * stock ILI9341 init sequence and per-board orientation flags only — the same
 * flags board_devices.yaml carries. There is no published board-specific
 * register sequence to copy, and inventing one would be a guess.
 *
 * GRAM offset: none. The panel is the controller's full 240x320 frame buffer.
 */

#include <string.h>
#include "esp_board_device.h"
#include "esp_log.h"

#if __has_include(<esp_lcd_ili9341.h>)
#define HAS_ILI9341  1
#include "esp_lcd_ili9341.h"
#endif  /* __has_include(<esp_lcd_ili9341.h>) */

static const char *TAG = "ESP32_LCDKIT_SETUP_DEVICE";

#if defined(HAS_ILI9341)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    esp_lcd_panel_dev_config_t panel_dev_cfg = {0};
    memcpy(&panel_dev_cfg, panel_dev_config, sizeof(esp_lcd_panel_dev_config_t));

    esp_err_t ret = esp_lcd_new_panel_ili9341(io, &panel_dev_cfg, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "New ILI9341 panel failed: %s", esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}
#endif  /* defined(HAS_ILI9341) */
