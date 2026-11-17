/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

esp_err_t test_dev_lcd_lvgl_init(void);

esp_err_t test_dev_lcd_touch_init(void);

/**
 * @brief  Get the first acquisition error from the active LVGL 9 touch input.
 *
 * @return
 *       - ESP_OK           No acquisition error, or no active LVGL 9 touch input.
 *       - ESP_ERR_TIMEOUT  The LVGL port lock could not be acquired.
 *       - Others           The first touch read/get error since initialization.
 */
esp_err_t test_dev_lcd_touch_get_error(void);

esp_err_t test_dev_lcd_lvgl_deinit(void);

esp_err_t test_dev_lcd_touch_deinit(void);

/**
 * @brief  Show test menu interface
 *
 * @return
 */
esp_err_t test_dev_lcd_lvgl_show_menu(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
