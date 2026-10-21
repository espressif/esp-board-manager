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

/**
 * @brief  Check that the ethernet driver was installed without starting it.
 *
 *         Looks up the handle and config, logs chip/sub_type/interface, and
 *         reads the MAC address. Does not call `esp_eth_start` or create a netif.
 *
 * @return
 *       - ESP_OK  Handle, config, and MAC lookup succeeded.
 *       - Others  Device lookup or ioctl failed.
 */
esp_err_t test_dev_ethernet_handle(void);

/**
 * @brief  Attach netif, start the driver, wait for link, then stop.
 *
 *         Creates an Ethernet netif and glue, starts the installed driver,
 *         waits for `ETHERNET_EVENT_CONNECTED`, then always stops the driver
 *         and deletes glue/netif so the case can be run again. Does not wait
 *         for DHCP.
 *
 * @return
 *       - ESP_OK           Link came up within the timeout.
 *       - ESP_ERR_TIMEOUT  Link did not come up in time.
 *       - Others           Handle lookup, netif, or start failed.
 */
esp_err_t test_dev_ethernet_link(void);

/**
 * @brief  Start Ethernet, obtain a DHCP lease, and ping the default gateway.
 *
 * @return
 *       - ESP_OK           DHCP and gateway reachability succeeded.
 *       - ESP_ERR_TIMEOUT  Link, DHCP, or ping timed out.
 *       - Others           Initialization or network setup failed.
 */
esp_err_t test_dev_ethernet_net(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
