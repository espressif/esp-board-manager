/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_check.h"
#include "sdkconfig.h"
#include "bmgr_test_names.h"
#include "bmgr_test_registry.h"

#ifdef CONFIG_ESP_BOARD_DEV_LEDC_CTRL_SUPPORT
#include "test_dev_ledc.h"
#endif  /* CONFIG_ESP_BOARD_DEV_LEDC_CTRL_SUPPORT */

#ifdef CONFIG_ESP_BOARD_DEV_LED_STRIP_SUPPORT
#include "test_dev_led_strip.h"
#endif  /* CONFIG_ESP_BOARD_DEV_LED_STRIP_SUPPORT */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)  (sizeof(array) / sizeof((array)[0]))
#endif  /* ARRAY_SIZE */

#ifdef CONFIG_ESP_BOARD_DEV_LEDC_CTRL_SUPPORT
static esp_err_t run_led_ledc(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_dev_ledc_ctrl();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_DEV_LEDC_CTRL_SUPPORT */

#ifdef CONFIG_ESP_BOARD_DEV_LED_STRIP_SUPPORT
static esp_err_t run_led_strip(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_dev_led_strip();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_DEV_LED_STRIP_SUPPORT */

void bmgr_register_led_cases(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_LEDC_CTRL_SUPPORT
    const bmgr_test_case_t ledc_cases[] = {
        {
            .name = "led.ledc",
            .group = "led",
            .help = "Run LEDC device test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_LCD_BRIGHTNESS),
            },
            .run = run_led_ledc,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(ledc_cases, ARRAY_SIZE(ledc_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_LEDC_CTRL_SUPPORT */

#ifdef CONFIG_ESP_BOARD_DEV_LED_STRIP_SUPPORT
    const bmgr_test_case_t strip_cases[] = {
        {
            .name = "led.strip",
            .group = "led",
            .help = "Run LED strip device test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_LED_STRIP),
            },
            .run = run_led_strip,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(strip_cases, ARRAY_SIZE(strip_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_LED_STRIP_SUPPORT */
}
