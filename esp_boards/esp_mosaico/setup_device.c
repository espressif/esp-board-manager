/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/* Panel and touch factories for the ESP-Mosaico.
 *
 * dev_display_lcd's SPI path calls lcd_panel_factory_entry_t to build the
 * concrete panel; without this translation unit the board does not link.
 * dev_lcd_touch's I2C path calls lcd_touch_factory_entry_t the same way.
 *
 * Panel: CO5300 AMOLED on a QSPI bus, driven by espressif/esp_lcd_co5300
 * (named in board_devices.yaml). The device schema has no field for the
 * driver's co5300_vendor_config_t, so the YAML leaves vendor_config NULL and
 * this factory supplies one — it must, because use_qspi_interface lives there
 * and nothing else tells the driver the bus is quad. The YAML's io_spi_config
 * (dc_gpio_num -1, lcd_cmd_bits 32, quad_mode) already matches the driver's own
 * CO5300_PANEL_IO_QSPI_CONFIG macro, so the two halves agree.
 *
 * init_cmds is left NULL ON PURPOSE: esp_lcd_co5300 then runs its own
 * vendor_specific_init_default table. No init sequence is invented here.
 *
 * KNOWN GAP, not a guess: that default table addresses a 466x466 window at an
 * x offset of 6 (CASET 0x0006..0x01DD, RASET 0x0000..0x01D1) — the geometry of
 * the common 466x466 CO5300 module, not this board's 480x480 panel. No
 * 480x480 CO5300 sequence exists in the driver component, in esp-bsp, or in
 * any Espressif source reachable from here, so none is written. If a real
 * board shows a shifted or clipped image, the fix is a vendor table from the
 * panel supplier (passed through init_cmds) and/or esp_lcd_panel_set_gap() by
 * the application — it is NOT something to derive by trial here.
 *
 * Touch: CST9220, driven by waveshare/esp_lcd_touch_cst9217 (named in
 * board_devices.yaml), whose one entry point covers the CST9217/CST9220 pair.
 * Its own probe runs, so no touch register table is needed.
 */

#include <string.h>
#include "esp_board_device.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#if __has_include(<esp_lcd_co5300.h>)
#define HAS_CO5300  1
#include "esp_lcd_co5300.h"
#endif  /* __has_include(<esp_lcd_co5300.h>) */
#if __has_include(<esp_lcd_touch_cst9217.h>)
#define HAS_CST9217  1
#include "esp_lcd_touch_cst9217.h"
#endif  /* __has_include(<esp_lcd_touch_cst9217.h>) */
#include "esp_log.h"

static const char *TAG = "ESP_MOSAICO_SETUP_DEVICE";

#if defined(HAS_CO5300)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    static const co5300_vendor_config_t vendor_config = {
        .init_cmds = NULL,  /* use the driver's default sequence */
        .init_cmds_size = 0,
        .flags = {
            .use_qspi_interface = 1,
        },
    };

    esp_lcd_panel_dev_config_t panel_dev_cfg = {0};
    memcpy(&panel_dev_cfg, panel_dev_config, sizeof(esp_lcd_panel_dev_config_t));
    panel_dev_cfg.vendor_config = (void *)&vendor_config;

    esp_err_t ret = esp_lcd_new_panel_co5300(io, &panel_dev_cfg, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create CO5300 panel: %s", esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}
#endif  /* defined(HAS_CO5300) */

#if defined(HAS_CST9217)
__attribute__((weak)) esp_err_t lcd_touch_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *touch_dev_config, esp_lcd_touch_handle_t *ret_touch)
{
    esp_lcd_touch_config_t touch_cfg = {0};
    memcpy(&touch_cfg, touch_dev_config, sizeof(esp_lcd_touch_config_t));

    esp_err_t ret = esp_lcd_touch_new_i2c_cst9217(io, &touch_cfg, ret_touch);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create CST9217/CST9220 touch driver: %s", esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}
#endif  /* defined(HAS_CST9217) */
