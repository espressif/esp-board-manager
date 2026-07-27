/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "sdkconfig.h"
#include "bmgr_test_names.h"
#include "bmgr_test_registry.h"

#ifdef CONFIG_ESP_BOARD_DEV_CUSTOM_SUPPORT
#include "test_dev_custom.h"
#endif  /* CONFIG_ESP_BOARD_DEV_CUSTOM_SUPPORT */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)  (sizeof(array) / sizeof((array)[0]))
#endif  /* ARRAY_SIZE */

#ifdef CONFIG_ESP_BOARD_DEV_CUSTOM_SUPPORT
static esp_err_t run_custom_basic(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_dev_custom();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_DEV_CUSTOM_SUPPORT */

void bmgr_register_custom_cases(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_CUSTOM_SUPPORT
    const bmgr_test_case_t custom_cases[] = {
        {
            .name = "custom.basic",
            .group = "custom",
            .help = "Run custom device test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_CUSTOM),
            },
            .run = run_custom_basic,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(custom_cases, ARRAY_SIZE(custom_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_CUSTOM_SUPPORT */
}
