/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include "esp_eth.h"
#include "esp_eth_mac_esp.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "dev_ethernet_private.h"
#include "esp_board_entry.h"

#if CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RMII_SUPPORT

static const char *TAG = "DEV_ETHERNET_EMAC_RMII";

static int dev_ethernet_sub_emac_rmii_init(void *cfg, int cfg_size, void **device_handle)
{
    if (cfg == NULL || device_handle == NULL || cfg_size != sizeof(dev_ethernet_config_t)) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }
    *device_handle = NULL;

    const dev_ethernet_config_t *config = (const dev_ethernet_config_t *)cfg;
    if (config->sub_type == NULL || strcmp(config->sub_type, ESP_BOARD_DEVICE_ETHERNET_SUB_TYPE_EMAC_RMII) != 0) {
        ESP_LOGE(TAG, "Invalid RMII subtype");
        return -1;
    }

#if CONFIG_ETH_USE_ESP32_EMAC
    const dev_ethernet_emac_config_t *rmii = &config->sub_cfg.emac_rmii;
    eth_esp32_emac_config_t emac_config = {
        .smi_gpio = rmii->smi_gpio,
        .interface = EMAC_DATA_INTERFACE_RMII,
        .clock_config = {
            .rmii = rmii->clock_config.rmii,
        },
        .dma_burst_len = rmii->dma_burst_len,
        .intr_priority = rmii->intr_priority,
#if SOC_EMAC_USE_MULTI_IO_MUX || SOC_EMAC_MII_USE_GPIO_MATRIX
        .emac_dataif_gpio = {
            .rmii = rmii->dataif_gpio.rmii,
        },
#endif  /* SOC_EMAC_USE_MULTI_IO_MUX || SOC_EMAC_MII_USE_GPIO_MATRIX */
#if !SOC_EMAC_RMII_CLK_OUT_INTERNAL_LOOPBACK
        .clock_config_out_in = rmii->clock_config_out_in,
#endif  /* !SOC_EMAC_RMII_CLK_OUT_INTERNAL_LOOPBACK */
        .mdc_freq_hz = rmii->mdc_freq_hz,
    };

    dev_ethernet_handles_t *handles = calloc(1, sizeof(dev_ethernet_handles_t));
    if (handles == NULL) {
        ESP_LOGE(TAG, "Failed to allocate Ethernet handle");
        return -1;
    }

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&emac_config, &config->mac_config);
    if (mac == NULL) {
        ESP_LOGE(TAG, "Failed to create RMII MAC instance");
        return dev_ethernet_emac_rollback(handles, NULL, NULL, device_handle);
    }
    esp_eth_phy_t *phy = dev_ethernet_phy_new(config);
    if (phy == NULL) {
        ESP_LOGE(TAG, "Failed to create PHY instance for chip %s", config->chip ? config->chip : "(generic)");
        return dev_ethernet_emac_rollback(handles, mac, NULL, device_handle);
    }

    esp_eth_config_t eth_config = {
        .mac = mac,
        .phy = phy,
        .check_link_period_ms = config->check_link_period_ms,
    };
    esp_err_t ret = esp_eth_driver_install(&eth_config, &handles->eth_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install RMII Ethernet driver: %s", esp_err_to_name(ret));
        handles->eth_handle = NULL;
        return dev_ethernet_emac_rollback(handles, mac, phy, device_handle);
    }
    ret = dev_ethernet_phy_post_init(handles->eth_handle, config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PHY post-init failed: %s", esp_err_to_name(ret));
        return dev_ethernet_emac_rollback(handles, mac, phy, device_handle);
    }

    /* Driver start and network-stack attachment belong to the application. */
    *device_handle = handles;
    ESP_LOGI(TAG, "Initialized Ethernet %s: chip=%s phy_addr=%ld",
             config->name ? config->name : "(unnamed)", config->chip ? config->chip : "(generic)",
             (long)config->phy_config.phy_addr);
    return 0;
#else
    ESP_LOGE(TAG, "ESP32 EMAC support is disabled (CONFIG_ETH_USE_ESP32_EMAC)");
    return -1;
#endif  /* CONFIG_ETH_USE_ESP32_EMAC */
}

static int dev_ethernet_sub_emac_rmii_deinit(void *device_handle)
{
#if CONFIG_ETH_USE_ESP32_EMAC
    return dev_ethernet_emac_delete(device_handle);
#else
    (void)device_handle;
    ESP_LOGE(TAG, "ESP32 EMAC support is disabled (CONFIG_ETH_USE_ESP32_EMAC)");
    return -1;
#endif  /* CONFIG_ETH_USE_ESP32_EMAC */
}

ESP_BOARD_SUBTYPE_ENTRY_IMPLEMENT(ethernet, emac_rmii, dev_ethernet_sub_emac_rmii_init, dev_ethernet_sub_emac_rmii_deinit);

#endif  /* CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RMII_SUPPORT */
