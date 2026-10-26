/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "esp_eth.h"
#include "esp_eth_phy.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Driver-owned vendor configuration used by the compile fixture.
 */
typedef struct {
    uint32_t  settle_time_ms;  /*!< Additional settling interval in milliseconds */
} bmgr_test_eth_phy_config_t;

/**
 * @brief  Construct a generic PHY for compile coverage of custom constructors.
 *
 * @param[in]  config  Common PHY creation parameters.
 *
 * @return
 *       - Non-NULL  Created PHY, owned by the caller.
 *       - NULL      Creation failed.
 */
esp_eth_phy_t *bmgr_test_eth_phy_new(const eth_phy_config_t *config);

/**
 * @brief  Validate the fixture payload without performing hardware operations.
 *
 * @param[in]  eth_handle  Installed Ethernet driver handle.
 * @param[in]  config      Driver-owned vendor configuration.
 *
 * @return
 *       - ESP_ERR_INVALID_ARG    Invalid handle or configuration.
 *       - ESP_ERR_NOT_SUPPORTED  Compile-only fixture; no physical chip is configured.
 */
esp_err_t bmgr_test_eth_phy_configure(esp_eth_handle_t eth_handle, const bmgr_test_eth_phy_config_t *config);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
