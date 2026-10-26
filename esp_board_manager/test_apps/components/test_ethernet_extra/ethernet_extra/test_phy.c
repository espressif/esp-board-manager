/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * Compile-only custom PHY adapter selected by ethernet_extra/test_phy.py.
 */

#include <stddef.h>
#include "esp_err.h"
#include "dev_ethernet.h"
#include "bmgr_test_eth_phy.h"

static esp_err_t test_phy_post_init(esp_eth_handle_t eth_handle, const dev_ethernet_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (config->vendor_config == NULL && config->vendor_config_size == 0) {
        return ESP_OK;
    }
    if (config->vendor_config == NULL || config->vendor_config_size != sizeof(bmgr_test_eth_phy_config_t)) {
        return ESP_ERR_INVALID_ARG;
    }
    return bmgr_test_eth_phy_configure(eth_handle, config->vendor_config);
}

ESP_BOARD_ETHERNET_PHY_REGISTER(bmgr_test_phy, bmgr_test_eth_phy_new, test_phy_post_init);
