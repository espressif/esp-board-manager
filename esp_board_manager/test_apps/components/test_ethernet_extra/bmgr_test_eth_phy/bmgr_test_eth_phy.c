/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 */

#include <stddef.h>
#include "esp_err.h"
#include "esp_eth_phy.h"
#include "bmgr_test_eth_phy.h"

esp_eth_phy_t *bmgr_test_eth_phy_new(const eth_phy_config_t *config)
{
    return esp_eth_phy_new_generic(config);
}

esp_err_t bmgr_test_eth_phy_configure(esp_eth_handle_t eth_handle, const bmgr_test_eth_phy_config_t *config)
{
    if (eth_handle == NULL || config == NULL || config->settle_time_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_ERR_NOT_SUPPORTED;
}
