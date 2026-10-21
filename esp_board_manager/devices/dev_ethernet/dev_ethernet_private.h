/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "dev_ethernet.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Create a registered or generic PHY and validate the vendor configuration contract.
 */
esp_eth_phy_t *dev_ethernet_phy_new(const dev_ethernet_config_t *config);

/**
 * @brief  Run registered PHY post-init operations on an installed Ethernet driver.
 */
esp_err_t dev_ethernet_phy_post_init(esp_eth_handle_t eth_handle, const dev_ethernet_config_t *config);

/**
 * @brief  Release partially initialized resources and return -1.
 *
 *         If driver uninstallation fails, return the live wrapper through
 *         device_handle so Board Manager can prevent reuse and retry cleanup.
 */
int dev_ethernet_emac_rollback(dev_ethernet_handles_t *handles, esp_eth_mac_t *mac,
                               esp_eth_phy_t *phy, void **device_handle);

/**
 * @brief  Release an installed driver and its MAC, PHY and wrapper.
 *
 * @return
 *       - 0   All resources released.
 *       - -1  Resources remain owned by device_handle.
 */
int dev_ethernet_emac_delete(void *device_handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
