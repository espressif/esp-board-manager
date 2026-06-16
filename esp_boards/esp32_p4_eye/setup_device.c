/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"

static const char *TAG = "P4_EYE_SETUP_DEVICE";

typedef struct {
    int           cmd;
    const void   *data;
    size_t        data_bytes;
    unsigned int  delay_ms;
} st7789_lcd_init_cmd_t;

static const st7789_lcd_init_cmd_t vendor_specific_init[] = {
    {0x11, (uint8_t[]) {0x00}, 1, 120},
    {0xB2, (uint8_t[]) {0x0C, 0x0C, 0x00, 0x33, 0x33}, 5, 0},
    {0x35, (uint8_t[]) {0x00}, 1, 0},
    {0x36, (uint8_t[]) {0x00}, 1, 0},
    {0x3A, (uint8_t[]) {0x05}, 1, 0},
    {0xB7, (uint8_t[]) {0x35}, 1, 0},
    {0xBB, (uint8_t[]) {0x2D}, 1, 0},
    {0xC0, (uint8_t[]) {0x2C}, 1, 0},
    {0xC2, (uint8_t[]) {0x01}, 1, 0},
    {0xC3, (uint8_t[]) {0x15}, 1, 0},
    {0xC4, (uint8_t[]) {0x20}, 1, 0},
    {0xC6, (uint8_t[]) {0x0F}, 1, 0},
    {0xD0, (uint8_t[]) {0xA4, 0xA1}, 2, 0},
    {0xD6, (uint8_t[]) {0xA1}, 1, 0},
    {0xE0, (uint8_t[]) {0x70, 0x05, 0x0A, 0x0B, 0x0A, 0x27, 0x2F, 0x44, 0x47, 0x37, 0x14, 0x14, 0x29, 0x2F}, 14, 0},
    {0xE1, (uint8_t[]) {0x70, 0x07, 0x0C, 0x08, 0x08, 0x04, 0x2F, 0x33, 0x46, 0x18, 0x15, 0x15, 0x2B, 0x2D}, 14, 0},
    {0x21, (uint8_t[]) {0x00}, 1, 0},
    {0x29, (uint8_t[]) {0x00}, 1, 0},
    {0x2C, (uint8_t[]) {0x00}, 1, 0},
};

esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    esp_err_t ret = esp_lcd_new_panel_st7789(io, panel_dev_config, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ST7789 panel: %s", esp_err_to_name(ret));
        return ret;
    }

    for (size_t i = 0; i < sizeof(vendor_specific_init) / sizeof(vendor_specific_init[0]); i++) {
        ret = esp_lcd_panel_io_tx_param(io, vendor_specific_init[i].cmd,
                                        vendor_specific_init[i].data,
                                        vendor_specific_init[i].data_bytes);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send init cmd 0x%02x: %s", vendor_specific_init[i].cmd, esp_err_to_name(ret));
            return ret;
        }
        if (vendor_specific_init[i].delay_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(vendor_specific_init[i].delay_ms));
        }
    }
    return ESP_OK;
}
