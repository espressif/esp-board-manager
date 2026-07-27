/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef enum {
    TEST_DEV_LCD_AUTO_COLOR_RED = 0,
    TEST_DEV_LCD_AUTO_COLOR_GREEN,
    TEST_DEV_LCD_AUTO_COLOR_BLUE,
    TEST_DEV_LCD_AUTO_COLOR_WHITE,
    TEST_DEV_LCD_AUTO_COLOR_BLACK,
    TEST_DEV_LCD_AUTO_COLOR_COUNT,
} test_dev_lcd_auto_color_t;

typedef enum {
    TEST_DEV_LCD_AUTO_PATTERN_CHECKER = 0,
    TEST_DEV_LCD_AUTO_PATTERN_ARROW,
    TEST_DEV_LCD_AUTO_PATTERN_COUNT,
} test_dev_lcd_auto_pattern_t;

esp_err_t test_dev_lcd_auto_show_color(test_dev_lcd_auto_color_t color);

esp_err_t test_dev_lcd_auto_show_pattern(test_dev_lcd_auto_pattern_t pattern);

esp_err_t test_dev_lcd_auto_clear(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
