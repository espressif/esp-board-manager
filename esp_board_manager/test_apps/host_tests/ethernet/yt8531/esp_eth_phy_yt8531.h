/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_eth_phy.h"

/* Public declarations needed by the host-side driver stub. */
typedef enum {
    YT8531_ETH_CMD_S_RGMII_CLK_DELAY = ETH_CMD_CUSTOM_PHY_CMDS_OFFSET,
} phy_yt8531_custom_io_cmd_t;

typedef struct {
    bool     rxc_dly_en;
    uint8_t  rx_delay_sel;
    uint8_t  tx_delay_sel;
    uint8_t  tx_delay_sel_fe;
} yt8531_rgmii_clk_delay_config_t;

esp_eth_phy_t *esp_eth_phy_new_yt8531(const eth_phy_config_t *config);
