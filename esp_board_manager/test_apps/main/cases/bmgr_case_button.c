/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "sdkconfig.h"
#include "esp_board_manager_defs.h"
#include "bmgr_test_names.h"
#include "bmgr_test_registry.h"

#ifdef CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT
#include "test_dev_button.h"
#endif  /* CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)  (sizeof(array) / sizeof((array)[0]))
#endif  /* ARRAY_SIZE */

/* Resolve the primary button device name for the configured sub-type. */
#ifdef CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT
#if defined(CONFIG_ESP_BOARD_DEV_BUTTON_SUB_GPIO_SUPPORT)
#define BMGR_TEST_BUTTON_REQUIRE  BMGR_TEST_NAME_GPIO_BUTTON
#elif defined(CONFIG_ESP_BOARD_DEV_BUTTON_SUB_ADC_MULTI_SUPPORT)
#define BMGR_TEST_BUTTON_REQUIRE  ESP_BOARD_DEVICE_NAME_ADC_BUTTON_GROUP
#elif defined(CONFIG_ESP_BOARD_DEV_BUTTON_SUB_ADC_SINGLE_SUPPORT)
#define BMGR_TEST_BUTTON_REQUIRE  ESP_BOARD_DEVICE_NAME_ADC_BUTTON_0
#elif defined(CONFIG_ESP_BOARD_DEV_BUTTON_SUB_CUSTOM_SUPPORT)
#define BMGR_TEST_BUTTON_REQUIRE  BMGR_TEST_NAME_CUSTOM_BUTTON
#endif  /* defined(CONFIG_ESP_BOARD_DEV_BUTTON_SUB_GPIO_SUPPORT) */
#endif  /* CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT */

#ifdef CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT
static esp_err_t run_button_read(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_dev_button();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT */

void bmgr_register_button_cases(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT
    const bmgr_test_case_t button_cases[] = {
        {
            .name = "button.read",
            .group = "button",
            .help = "Run button read test",
#ifdef BMGR_TEST_BUTTON_REQUIRE
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_BUTTON_REQUIRE),
            },
#endif  /* BMGR_TEST_BUTTON_REQUIRE */
            .run = run_button_read,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(button_cases, ARRAY_SIZE(button_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT */
}
