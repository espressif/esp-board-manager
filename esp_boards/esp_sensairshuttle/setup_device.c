/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#if __has_include("esp_lcd_ili9341.h")
#define HAS_ILI9341 1
#include "esp_lcd_ili9341.h"
#endif
#if __has_include("esp_lcd_touch_cst816s.h")
#define HAS_CST816S 1
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_cst816s.h"
#endif
#include "esp_log.h"

#if defined(HAS_ILI9341)
// ST7789P3 panel P183B001-V4-CTP requires this board-specific sequence.
static const ili9341_lcd_init_cmd_t vendor_specific_init[] = {
    {0x11, NULL, 0, 120},                                                                         // Sleep Out
    {0x36, (uint8_t []){0x00}, 1, 0},                                                             // Memory Data Access Control
    {0x3A, (uint8_t []){0x05}, 1, 0},                                                             // Interface Pixel Format (16-bit)
    {0xB2, (uint8_t []){0x0C, 0x0C, 0x00, 0x33, 0x33}, 5, 0},                                     // Porch Setting
    {0xB7, (uint8_t []){0x05}, 1, 0},                                                             // Gate Control
    {0xBB, (uint8_t []){0x21}, 1, 0},                                                             // VCOM Setting
    {0xC0, (uint8_t []){0x2C}, 1, 0},                                                             // LCM Control
    {0xC2, (uint8_t []){0x01}, 1, 0},                                                             // VDV and VRH Command Enable
    {0xC3, (uint8_t []){0x15}, 1, 0},                                                             // VRH Set
    {0xC6, (uint8_t []){0x0F}, 1, 0},                                                             // Frame Rate Control
    {0xD0, (uint8_t []){0xA7}, 1, 0},                                                             // Power Control 1
    {0xD0, (uint8_t []){0xA4, 0xA1}, 2, 0},                                                       // Power Control 1
    {0xD6, (uint8_t []){0xA1}, 1, 0},                                                             // Gate output GND in sleep mode
    {
        0xE0, (uint8_t [])
        {
            0xF0, 0x05, 0x0E, 0x08, 0x0A, 0x17, 0x39, 0x54,
            0x4E, 0x37, 0x12, 0x12, 0x31, 0x37
        }, 14, 0
    },                              // Positive Gamma Control
    {
        0xE1, (uint8_t [])
        {
            0xF0, 0x10, 0x14, 0x0D, 0x0B, 0x05, 0x39, 0x44,
            0x4D, 0x38, 0x14, 0x14, 0x2E, 0x35
        }, 14, 0
    },                              // Negative Gamma Control
    {0xE4, (uint8_t []){0x23, 0x00, 0x00}, 3, 0},                                                 // Gate position control
    {0x21, NULL, 0, 0},                                                                           // Display Inversion On
    {0x29, NULL, 0, 0},                                                                           // Display On
    {0x2C, NULL, 0, 0},                                                                           // Memory Write
};

// Supply board-specific initialization commands to the ILI9341 driver.
static const ili9341_vendor_config_t vendor_config = {
    .init_cmds = vendor_specific_init,
    .init_cmds_size = sizeof(vendor_specific_init) / sizeof(vendor_specific_init[0]),
};

__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    esp_lcd_panel_dev_config_t panel_dev_cfg = {0};
    memcpy(&panel_dev_cfg, panel_dev_config, sizeof(esp_lcd_panel_dev_config_t));

    panel_dev_cfg.vendor_config = (void *)&vendor_config;
    int ret = esp_lcd_new_panel_ili9341(io, &panel_dev_cfg, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE("lcd_panel_factory_entry_t", "New ili9341 panel failed");
        return ret;
    }
    return ESP_OK;
}

#endif  /* defined(HAS_ILI9341) */

#if defined(HAS_CST816S)
__attribute__((weak)) esp_err_t lcd_touch_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *touch_dev_config, esp_lcd_touch_handle_t *ret_touch)
{
    esp_lcd_touch_config_t touch_cfg = {0};
    memcpy(&touch_cfg, touch_dev_config, sizeof(esp_lcd_touch_config_t));
    if (touch_cfg.int_gpio_num != GPIO_NUM_NC) {
        ESP_LOGW("lcd_touch_factory_entry_t", "Touch interrupt supported!");
        touch_cfg.interrupt_callback = NULL;
    }
    esp_err_t ret = esp_lcd_touch_new_i2c_cst816s(io, &touch_cfg, ret_touch);
    if (ret != ESP_OK) {
        ESP_LOGE("lcd_touch_factory_entry_t", "Failed to create cst816s touch driver: %s", esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}

#endif  /* defined(HAS_CST816S) */
