/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include "esp_check.h"
#include "esp_err.h"
#include "esp_eth.h"
#include "esp_log.h"
#include "esp_eth_phy_yt8531.h"
#include "dev_ethernet.h"

static const char *TAG = "DEV_ETHERNET_YT8531";

/**
 * @brief  Apply optional board RGMII delays through the YT8531 driver.
 *
 *         The PHY driver handles auto-negotiation during installation. This callback
 *         applies configured RX/TX delays before the application starts it.
 *
 * @param[in]  eth_handle  Installed Ethernet driver handle.
 * @param[in]  config      Device configuration used to select interface-specific settings.
 *
 * @return
 *       - ESP_OK  Success.
 *       - Other   PHY delay configuration failure.
 */
static esp_err_t dev_ethernet_phy_yt8531_post_init(esp_eth_handle_t eth_handle, const dev_ethernet_config_t *config)
{
    if (eth_handle == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (config->vendor_config == NULL && config->vendor_config_size == 0) {
        return ESP_OK;
    }
    if (config->vendor_config == NULL ||
        config->vendor_config_size != sizeof(yt8531_rgmii_clk_delay_config_t) ||
        config->sub_type == NULL ||
        strcmp(config->sub_type, ESP_BOARD_DEVICE_ETHERNET_SUB_TYPE_EMAC_RGMII) != 0) {
        return ESP_ERR_INVALID_ARG;
    }

    yt8531_rgmii_clk_delay_config_t delay = *(const yt8531_rgmii_clk_delay_config_t *)config->vendor_config;
    ESP_RETURN_ON_ERROR(esp_eth_ioctl(eth_handle, (esp_eth_io_cmd_t)YT8531_ETH_CMD_S_RGMII_CLK_DELAY, &delay), TAG,
                        "set RGMII PHY clock delays failed");

    ESP_LOGI(TAG, "RGMII PHY delays configured: rxc_dly_en=%d rx=%u tx=%u tx_fe=%u",
             delay.rxc_dly_en, delay.rx_delay_sel, delay.tx_delay_sel, delay.tx_delay_sel_fe);
    return ESP_OK;
}

ESP_BOARD_ETHERNET_PHY_REGISTER(yt8531, esp_eth_phy_new_yt8531, dev_ethernet_phy_yt8531_post_init);
