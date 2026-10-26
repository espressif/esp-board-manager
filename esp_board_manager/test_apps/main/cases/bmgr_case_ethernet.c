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

#ifdef CONFIG_ESP_BOARD_DEV_ETHERNET_SUPPORT
#include "test_dev_ethernet.h"
#endif  /* CONFIG_ESP_BOARD_DEV_ETHERNET_SUPPORT */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)  (sizeof(array) / sizeof((array)[0]))
#endif  /* ARRAY_SIZE */

#ifdef CONFIG_ESP_BOARD_DEV_ETHERNET_SUPPORT
static esp_err_t run_eth_handle(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    return test_dev_ethernet_handle();
}

static esp_err_t run_eth_link(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    return test_dev_ethernet_link();
}

static esp_err_t run_eth_net(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    return test_dev_ethernet_net();
}
#endif  /* CONFIG_ESP_BOARD_DEV_ETHERNET_SUPPORT */

void bmgr_register_ethernet_cases(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_ETHERNET_SUPPORT
    const bmgr_test_case_t ethernet_cases[] = {
        {
            .name = "eth.net",
            .group = "eth",
            .help = "Obtain DHCP address and ping the default gateway",
            .resources =
                {
                    BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_ETHERNET),
                },
            .run = run_eth_net,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_LONG_RUNNING,
        },
        {
            .name = "eth.handle",
            .group = "eth",
            .help = "Check ethernet driver handle, config, and MAC without starting",
            .resources =
                {
                    BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_ETHERNET),
                },
            .run = run_eth_handle,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
        {
            .name = "eth.link",
            .group = "eth",
            .help = "Start ethernet, wait for link, then stop",
            .resources =
                {
                    BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_ETHERNET),
                },
            .run = run_eth_link,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_LONG_RUNNING,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(ethernet_cases, ARRAY_SIZE(ethernet_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_ETHERNET_SUPPORT */
}
