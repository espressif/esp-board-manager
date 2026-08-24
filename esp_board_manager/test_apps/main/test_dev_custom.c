/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "dev_custom.h"
#include "esp_board_device.h"
#include "esp_board_manager.h"
#include "gen_board_device_custom.h"  // Include generated custom device config header
#include "bmgr_test_names.h"
#include "test_dev_custom.h"

static const char *TAG = "TEST_DEV_CUSTOM";

esp_err_t test_dev_custom(void)
{
    ESP_LOGI(TAG, "=== Custom Device Test ===");
    /* Get the custom device handle */
    void *custom_handle = NULL;
    esp_err_t ret = esp_board_manager_get_device_handle(BMGR_TEST_NAME_CUSTOM, (void **)&custom_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get my_custom_sensor device handle: %s", esp_err_to_name(ret));
        return ret;
    }
    /* Get the device configuration */
    void *custom_config = NULL;
    ret = esp_board_manager_get_device_config(BMGR_TEST_NAME_CUSTOM, &custom_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get my_custom_sensor device config: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Custom device handle and config obtained successfully");
    ESP_LOGI(TAG, "  Device handle obtained: %p", custom_handle);
    ESP_LOGI(TAG, "Custom device test completed successfully!");
    return ESP_OK;
}
