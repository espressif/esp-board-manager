/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "sdkconfig.h"
#include "esp_board_manager_defs.h"
#include "bmgr_test_registry.h"

#ifdef CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT
#include "test_dev_button.h"
#endif  /* CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)  (sizeof(array) / sizeof((array)[0]))
#endif  /* ARRAY_SIZE */

#ifdef CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT
static esp_err_t run_button_read(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    if (argc < 0 || argc > 1 || (argc == 1 && argv == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    return test_dev_button_named(argc == 1 ? argv[0] : NULL);
}
#endif  /* CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT */

void bmgr_register_button_cases(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT
    const bmgr_test_case_t button_cases[] = {
        {
            .name = "button.read",
            .group = "button",
            .help = "Monitor all buttons, or pass a device name (GPIO/ADC/custom)",
            .run = run_button_read,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(button_cases, ARRAY_SIZE(button_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT */
}
