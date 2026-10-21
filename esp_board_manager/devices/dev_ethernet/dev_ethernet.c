/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_err.h"
#include "esp_log.h"
#include "esp_board_device.h"
#include "esp_board_entry.h"
#include "esp_board_manager_defs.h"
#include "dev_ethernet.h"

static const char *TAG = "DEV_ETHERNET";

int dev_ethernet_init(void *cfg, int cfg_size, void **device_handle)
{
    if (cfg == NULL || device_handle == NULL ||
        cfg_size != sizeof(dev_ethernet_config_t)) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    const dev_ethernet_config_t *config = (const dev_ethernet_config_t *)cfg;
    if (config->sub_type == NULL) {
        ESP_LOGE(TAG, "Invalid ethernet sub_type");
        return -1;
    }

    const esp_board_entry_desc_t *entry_desc = esp_board_entry_find_subtype_desc(
        ESP_BOARD_DEVICE_TYPE_ETHERNET, config->sub_type);
    if (entry_desc == NULL || entry_desc->init_func == NULL) {
        ESP_LOGE(TAG, "Unsupported ethernet sub_type: %s", config->sub_type);
        return -1;
    }

    return entry_desc->init_func(cfg, cfg_size, device_handle);
}

int dev_ethernet_deinit(void *device_handle)
{
    if (device_handle == NULL) {
        ESP_LOGE(TAG, "Invalid device handle");
        return -1;
    }

    dev_ethernet_config_t *config = NULL;
    esp_err_t ret =
        esp_board_device_get_config_by_handle(device_handle, (void **)&config);
    if (ret != ESP_OK || config == NULL || config->sub_type == NULL) {
        ESP_LOGE(TAG, "Failed to get ethernet configuration: %s", esp_err_to_name(ret));
        return -1;
    }

    const esp_board_entry_desc_t *entry_desc = esp_board_entry_find_subtype_desc(
        ESP_BOARD_DEVICE_TYPE_ETHERNET, config->sub_type);
    if (entry_desc == NULL || entry_desc->deinit_func == NULL) {
        ESP_LOGE(TAG, "Unsupported ethernet sub_type: %s", config->sub_type);
        return -1;
    }

    ret = entry_desc->deinit_func(device_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize ethernet sub_type %s", config->sub_type);
    }
    return ret;
}
