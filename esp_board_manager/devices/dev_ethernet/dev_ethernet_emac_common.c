/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "esp_err.h"
#include "esp_eth.h"
#include "esp_eth_phy.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "dev_ethernet_private.h"
#include "esp_board_device.h"

#if CONFIG_ETH_USE_ESP32_EMAC

static const char *TAG = "DEV_ETHERNET_EMAC";

static const dev_ethernet_phy_ops_t *find_phy_ops(const char *chip)
{
    if (chip == NULL || chip[0] == '\0') {
        return NULL;
    }

    /* Compare the namespaced key without allocating a temporary string. */
    static const char prefix[] = "ethernet_phy_";
    extern const esp_board_extra_func_desc_t _esp_board_extra_func_array_start;
    extern const esp_board_extra_func_desc_t _esp_board_extra_func_array_end;
    for (const esp_board_extra_func_desc_t *entry = &_esp_board_extra_func_array_start;
         entry != &_esp_board_extra_func_array_end; ++entry) {
        if (strncmp(entry->owner_name, prefix, sizeof(prefix) - 1) == 0 &&
            strcmp(entry->owner_name + sizeof(prefix) - 1, chip) == 0) {
            return (const dev_ethernet_phy_ops_t *)entry->extra_func;
        }
    }
    return NULL;
}

static bool vendor_config_is_valid(const dev_ethernet_config_t *config, const dev_ethernet_phy_ops_t *ops)
{
    if ((config->vendor_config == NULL) != (config->vendor_config_size == 0)) {
        ESP_LOGE(TAG, "PHY vendor configuration pointer and size are inconsistent");
        return false;
    }
    if (config->vendor_config != NULL && (ops == NULL || ops->post_init == NULL)) {
        ESP_LOGE(TAG, "PHY chip %s has no registered vendor configuration handler",
                 config->chip ? config->chip : "(generic)");
        return false;
    }
    return true;
}

esp_eth_phy_t *dev_ethernet_phy_new(const dev_ethernet_config_t *config)
{
    if (config == NULL) {
        return NULL;
    }
    const dev_ethernet_phy_ops_t *ops = find_phy_ops(config->chip);
    if (!vendor_config_is_valid(config, ops)) {
        return NULL;
    }
    if (ops != NULL && ops->create != NULL) {
        return ops->create(&config->phy_config);
    }
    return esp_eth_phy_new_generic(&config->phy_config);
}

esp_err_t dev_ethernet_phy_post_init(esp_eth_handle_t eth_handle, const dev_ethernet_config_t *config)
{
    if (eth_handle == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    const dev_ethernet_phy_ops_t *ops = find_phy_ops(config->chip);
    if (!vendor_config_is_valid(config, ops)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (ops == NULL || ops->post_init == NULL) {
        return ESP_OK;
    }
    return ops->post_init(eth_handle, config);
}

int dev_ethernet_emac_rollback(dev_ethernet_handles_t *handles, esp_eth_mac_t *mac,
                               esp_eth_phy_t *phy, void **device_handle)
{
    if (handles != NULL && handles->eth_handle != NULL) {
        esp_err_t ret = esp_eth_driver_uninstall(handles->eth_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to uninstall Ethernet driver during rollback: %s", esp_err_to_name(ret));
            *device_handle = handles;
            return -1;
        }
        handles->eth_handle = NULL;
    }
    if (phy != NULL) {
        phy->del(phy);
    }
    if (mac != NULL) {
        mac->del(mac);
    }
    free(handles);
    return -1;
}

int dev_ethernet_emac_delete(void *device_handle)
{
    if (device_handle == NULL) {
        ESP_LOGE(TAG, "Invalid device handle");
        return -1;
    }

    dev_ethernet_handles_t *handles = (dev_ethernet_handles_t *)device_handle;
    if (handles->eth_handle != NULL) {
        esp_eth_mac_t *mac = NULL;
        esp_eth_phy_t *phy = NULL;
        esp_err_t ret = esp_eth_get_mac_instance(handles->eth_handle, &mac);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get Ethernet MAC instance: %s", esp_err_to_name(ret));
            return -1;
        }
        ret = esp_eth_get_phy_instance(handles->eth_handle, &phy);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get Ethernet PHY instance: %s", esp_err_to_name(ret));
            return -1;
        }
        ret = esp_eth_driver_uninstall(handles->eth_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to uninstall Ethernet driver: %s", esp_err_to_name(ret));
            return -1;
        }

        if (mac != NULL) {
            mac->del(mac);
        }
        if (phy != NULL) {
            phy->del(phy);
        }
        handles->eth_handle = NULL;
    }

    free(handles);
    return 0;
}

#endif  /* CONFIG_ETH_USE_ESP32_EMAC */
