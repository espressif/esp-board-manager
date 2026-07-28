/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "bmgr_test_names.h"
#include "bmgr_test_registry.h"

#ifdef CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT
#include "test_dev_lcd_auto.h"
#include "test_dev_lcd_lvgl.h"
#endif  /* CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)  (sizeof(array) / sizeof((array)[0]))
#endif  /* ARRAY_SIZE */

#ifdef CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT
static const char *TAG = "BMGR_CASE_LCD";

static esp_err_t run_lcd_auto_color(bmgr_test_context_t *ctx, int argc, char **argv, test_dev_lcd_auto_color_t color)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    return test_dev_lcd_auto_show_color(color);
}

static esp_err_t run_lcd_auto_pattern(bmgr_test_context_t *ctx, int argc, char **argv, test_dev_lcd_auto_pattern_t pattern)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    return test_dev_lcd_auto_show_pattern(pattern);
}

static esp_err_t run_lcd_color_red(bmgr_test_context_t *ctx, int argc, char **argv)
{
    return run_lcd_auto_color(ctx, argc, argv, TEST_DEV_LCD_AUTO_COLOR_RED);
}

static esp_err_t run_lcd_color_green(bmgr_test_context_t *ctx, int argc, char **argv)
{
    return run_lcd_auto_color(ctx, argc, argv, TEST_DEV_LCD_AUTO_COLOR_GREEN);
}

static esp_err_t run_lcd_color_blue(bmgr_test_context_t *ctx, int argc, char **argv)
{
    return run_lcd_auto_color(ctx, argc, argv, TEST_DEV_LCD_AUTO_COLOR_BLUE);
}

static esp_err_t run_lcd_color_white(bmgr_test_context_t *ctx, int argc, char **argv)
{
    return run_lcd_auto_color(ctx, argc, argv, TEST_DEV_LCD_AUTO_COLOR_WHITE);
}

static esp_err_t run_lcd_color_black(bmgr_test_context_t *ctx, int argc, char **argv)
{
    return run_lcd_auto_color(ctx, argc, argv, TEST_DEV_LCD_AUTO_COLOR_BLACK);
}

static esp_err_t run_lcd_pattern_checker(bmgr_test_context_t *ctx, int argc, char **argv)
{
    return run_lcd_auto_pattern(ctx, argc, argv, TEST_DEV_LCD_AUTO_PATTERN_CHECKER);
}

static esp_err_t run_lcd_pattern_arrow(bmgr_test_context_t *ctx, int argc, char **argv)
{
    return run_lcd_auto_pattern(ctx, argc, argv, TEST_DEV_LCD_AUTO_PATTERN_ARROW);
}

static esp_err_t run_lcd_lvgl(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    esp_err_t clear_ret = test_dev_lcd_auto_clear();
    if (clear_ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to clear LCD automation overlay: %s", esp_err_to_name(clear_ret));
    }

    esp_err_t ret = test_dev_lcd_lvgl_init();
    if (ret != ESP_OK && !(lv_disp_get_default() != NULL && lv_scr_act() != NULL)) {
        return ret;
    }

    esp_err_t touch_ret = test_dev_lcd_touch_init();
    bool touch_inited = touch_ret == ESP_OK;
    if (touch_ret != ESP_OK) {
        ESP_LOGW(TAG, "LCD Touch initialization failed, test continued: %s", esp_err_to_name(touch_ret));
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    esp_err_t show_ret = test_dev_lcd_lvgl_show_menu();

    if (touch_inited) {
        esp_err_t touch_deinit_ret = test_dev_lcd_touch_deinit();
        if (touch_deinit_ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to deinitialize LCD touch: %s", esp_err_to_name(touch_deinit_ret));
        }
    }

    esp_err_t lvgl_deinit_ret = test_dev_lcd_lvgl_deinit();
    if (lvgl_deinit_ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to deinitialize LCD LVGL: %s", esp_err_to_name(lvgl_deinit_ret));
    }

    return show_ret != ESP_OK ? show_ret : ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT */

void bmgr_register_lcd_cases(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT
    const bmgr_test_case_t lcd_cases[] = {
        {
            .name = "lcd.color.red",
            .group = "lcd",
            .help = "Fill the LCD with solid red",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_DISPLAY_LCD),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_LCD_BRIGHTNESS),
            },
            .run = run_lcd_color_red,
            .flags = BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_DISPLAY,
        },
        {
            .name = "lcd.color.green",
            .group = "lcd",
            .help = "Fill the LCD with solid green",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_DISPLAY_LCD),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_LCD_BRIGHTNESS),
            },
            .run = run_lcd_color_green,
            .flags = BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_DISPLAY,
        },
        {
            .name = "lcd.color.blue",
            .group = "lcd",
            .help = "Fill the LCD with solid blue",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_DISPLAY_LCD),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_LCD_BRIGHTNESS),
            },
            .run = run_lcd_color_blue,
            .flags = BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_DISPLAY,
        },
        {
            .name = "lcd.color.white",
            .group = "lcd",
            .help = "Fill the LCD with solid white",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_DISPLAY_LCD),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_LCD_BRIGHTNESS),
            },
            .run = run_lcd_color_white,
            .flags = BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_DISPLAY,
        },
        {
            .name = "lcd.color.black",
            .group = "lcd",
            .help = "Fill the LCD with solid black",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_DISPLAY_LCD),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_LCD_BRIGHTNESS),
            },
            .run = run_lcd_color_black,
            .flags = BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_DISPLAY,
        },
        {
            .name = "lcd.pattern.checker",
            .group = "lcd",
            .help = "Draw a checkerboard pattern on the LCD",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_DISPLAY_LCD),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_LCD_BRIGHTNESS),
            },
            .run = run_lcd_pattern_checker,
            .flags = BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_DISPLAY,
        },
        {
            .name = "lcd.pattern.arrow",
            .group = "lcd",
            .help = "Draw an arrow pattern on the LCD",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_DISPLAY_LCD),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_LCD_BRIGHTNESS),
            },
            .run = run_lcd_pattern_arrow,
            .flags = BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_DISPLAY,
        },
        {
            .name = "lcd.lvgl",
            .group = "lcd",
            .help = "Run LCD LVGL manual test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_DISPLAY_LCD),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_LCD_TOUCH),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_LCD_BRIGHTNESS),
            },
            .run = run_lcd_lvgl,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_DISPLAY,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(lcd_cases, ARRAY_SIZE(lcd_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT */
}
