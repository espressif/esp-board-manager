/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "sdkconfig.h"
#include "bmgr_test_names.h"
#include "bmgr_test_registry.h"

#ifdef CONFIG_ESP_BOARD_DEV_GPIO_EXPANDER_SUPPORT
#include "test_dev_gpio_expander.h"
#endif  /* CONFIG_ESP_BOARD_DEV_GPIO_EXPANDER_SUPPORT */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)  (sizeof(array) / sizeof((array)[0]))
#endif  /* ARRAY_SIZE */

#ifdef CONFIG_ESP_BOARD_DEV_GPIO_EXPANDER_SUPPORT
static esp_err_t run_gpio_expander_io(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_dev_gpio_expander();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_DEV_GPIO_EXPANDER_SUPPORT */

void bmgr_register_gpio_expander_cases(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_GPIO_EXPANDER_SUPPORT
    const bmgr_test_case_t gpio_expander_cases[] = {
        {
            .name = "gpio_expander.io",
            .group = "gpio_expander",
            .help = "Run GPIO expander test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_GPIO_EXPANDER),
            },
            .run = run_gpio_expander_io,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(gpio_expander_cases, ARRAY_SIZE(gpio_expander_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_GPIO_EXPANDER_SUPPORT */
}
