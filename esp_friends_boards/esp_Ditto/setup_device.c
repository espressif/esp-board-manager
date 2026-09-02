/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

#if __has_include(<esp_lcd_ili9341.h>)
#define HAS_ILI9341  1
#include "esp_lcd_ili9341.h"
#endif  /* __has_include(<esp_lcd_ili9341.h>) */

#if __has_include(<esp_lcd_touch_cst816s.h>)
#define HAS_CST816S  1
#include "esp_lcd_touch_cst816s.h"
#endif  /* __has_include(<esp_lcd_touch_cst816s.h>) */

#if defined(HAS_ILI9341)
static const ili9341_lcd_init_cmd_t s_vendor_init_cmds[] = {
    {0x11, NULL, 0, 120},
    {0x36, (uint8_t[]) {0x00}, 1, 0},
    {0x3A, (uint8_t[]) {0x05}, 1, 0},
    {0xB2, (uint8_t[]) {0x0C, 0x0C, 0x00, 0x33, 0x33}, 5, 0},
    {0xB7, (uint8_t[]) {0x05}, 1, 0},
    {0xBB, (uint8_t[]) {0x21}, 1, 0},
    {0xC0, (uint8_t[]) {0x2C}, 1, 0},
    {0xC2, (uint8_t[]) {0x01}, 1, 0},
    {0xC3, (uint8_t[]) {0x15}, 1, 0},
    {0xC6, (uint8_t[]) {0x0F}, 1, 0},
    {0xD0, (uint8_t[]) {0xA7}, 1, 0},
    {0xD0, (uint8_t[]) {0xA4, 0xA1}, 2, 0},
    {0xD6, (uint8_t[]) {0xA1}, 1, 0},
    {0xE0, (uint8_t[]) {0xF0, 0x05, 0x0E, 0x08, 0x0A, 0x17, 0x39, 0x54,
                        0x4E, 0x37, 0x12, 0x12, 0x31, 0x37}, 14, 0},
    {0xE1, (uint8_t[]) {0xF0, 0x10, 0x14, 0x0D, 0x0B, 0x05, 0x39, 0x44,
                        0x4D, 0x38, 0x14, 0x14, 0x2E, 0x35}, 14, 0},
    {0xE4, (uint8_t[]) {0x23, 0x00, 0x00}, 3, 0},
    {0x21, NULL, 0, 0},
    {0x29, NULL, 0, 0},
    {0x2C, NULL, 0, 0},
};

static const ili9341_vendor_config_t s_vendor_config = {
    .init_cmds      = s_vendor_init_cmds,
    .init_cmds_size = sizeof(s_vendor_init_cmds) / sizeof(s_vendor_init_cmds[0]),
};
#endif  /* defined(HAS_ILI9341) */

#if defined(HAS_ILI9341)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_panel_dev_config_t *panel_dev_config,
                                                          esp_lcd_panel_handle_t *ret_panel)
{
    esp_lcd_panel_dev_config_t panel_dev_cfg = {0};
    memcpy(&panel_dev_cfg, panel_dev_config, sizeof(panel_dev_cfg));
    panel_dev_cfg.vendor_config = (void *)&s_vendor_config;

    esp_err_t ret = esp_lcd_new_panel_ili9341(io, &panel_dev_cfg, ret_panel);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = esp_lcd_panel_set_gap(*ret_panel, 36, 0);
    if (ret != ESP_OK) {
        (void)esp_lcd_panel_del(*ret_panel);
        *ret_panel = NULL;
    }
    return ret;
}
#endif  /* defined(HAS_ILI9341) */

#if defined(HAS_CST816S)
__attribute__((weak)) esp_err_t lcd_touch_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_touch_config_t *touch_config,
                                                          esp_lcd_touch_handle_t *ret_touch)
{
    return esp_lcd_touch_new_i2c_cst816s(io, touch_config, ret_touch);
}
#endif  /* defined(HAS_CST816S) */
