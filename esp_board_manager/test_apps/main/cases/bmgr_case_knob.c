/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "bmgr_test_names.h"
#include "bmgr_test_registry.h"
#include "sdkconfig.h"

#ifdef CONFIG_ESP_BOARD_DEV_KNOB_SUPPORT
#include "test_dev_knob.h"
#endif  /* CONFIG_ESP_BOARD_DEV_KNOB_SUPPORT */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)  (sizeof(array) / sizeof((array)[0]))
#endif  /* ARRAY_SIZE */

#ifdef CONFIG_ESP_BOARD_DEV_KNOB_SUPPORT
static esp_err_t run_knob_read(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    return test_dev_knob();
}
#endif  /* CONFIG_ESP_BOARD_DEV_KNOB_SUPPORT */

void bmgr_register_knob_cases(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_KNOB_SUPPORT
    const bmgr_test_case_t knob_cases[] = {
        {
            .name = "knob.read",
            .group = "knob",
            .help = "Read GPIO quadrature knob events and count",
            .resources =
                {
                    BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_KNOB),
                },
            .run = run_knob_read,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(knob_cases, ARRAY_SIZE(knob_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_KNOB_SUPPORT */
}
