/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "dev_gpio_ctrl.h"
#include "esp_board_device.h"
#include "esp_board_periph.h"

static const char *TAG = "DEV_GPIO_CTRL";

int dev_gpio_ctrl_init(void *cfg, int cfg_size, void **device_handle)
{
    if (!cfg || !device_handle) {
        ESP_LOGE(TAG, "Invalid parameters, cfg: %p, device_handle: %p", cfg, device_handle);
        return -1;
    }
    if (cfg_size < (int)sizeof(dev_gpio_ctrl_config_t)) {
        ESP_LOGE(TAG, "Invalid configuration size: %d", cfg_size);
        return ESP_ERR_INVALID_SIZE;
    }

    const dev_gpio_ctrl_config_t *config = (const dev_gpio_ctrl_config_t *)cfg;
    if (config->active_level > 1 || config->default_level > 1 ||
        config->init_level < -1 || config->init_level > 1) {
        ESP_LOGE(TAG, "Invalid GPIO control levels: active=%d, default=%d, init=%d",
                 config->active_level, config->default_level, config->init_level);
        return ESP_ERR_INVALID_ARG;
    }

    // Get GPIO peripheral handle
    periph_gpio_handle_t *gpio_handle = NULL;
    if (!config->gpio_name || strlen(config->gpio_name) == 0) {
        ESP_LOGE(TAG, "No GPIO peripheral name configured for device: %s", config->name);
        return -1;
    }

    // Get GPIO peripheral config
    periph_gpio_config_t *gpio_config = NULL;
    esp_err_t config_ret = esp_board_periph_get_config(config->gpio_name, (void **)&gpio_config);
    if (config_ret != ESP_OK || gpio_config == NULL) {
        ESP_LOGE(TAG, "Failed to get GPIO peripheral config '%s': %s", config->gpio_name, esp_err_to_name(config_ret));
        return config_ret != ESP_OK ? config_ret : ESP_ERR_INVALID_STATE;
    }

    int ret = esp_board_periph_ref_handle(config->gpio_name, (void *)&gpio_handle);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to get GPIO peripheral handle '%s': %d", config->gpio_name, ret);
        return -1;
    }

    // Use the active level unless an explicit initial level was configured.
    uint8_t initial_level = config->init_level == -1 ? config->active_level : config->init_level;
    esp_err_t err = gpio_set_level(gpio_handle->gpio_num, initial_level);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set initial level for GPIO %d: %s", gpio_handle->gpio_num, esp_err_to_name(err));
        goto cleanup;
    }

    ESP_LOGI(TAG, "Initialized GPIO control device '%s': gpio=%d, set initial level:%d (active_level=%d, default_level=%d, init_level=%d)",
             config->name, gpio_handle->gpio_num, initial_level, config->active_level, config->default_level, config->init_level);

    // Store the GPIO handle for later use
    *device_handle = gpio_handle;
    return 0;

cleanup:
    esp_board_periph_unref_handle(config->gpio_name);
    return err;
}

int dev_gpio_ctrl_deinit(void *device_handle)
{
    if (!device_handle) {
        ESP_LOGE(TAG, "Invalid handle");
        return -1;
    }

    // Cast back to GPIO handle type
    periph_gpio_handle_t *gpio_handle = (periph_gpio_handle_t *)device_handle;
    if (!gpio_handle) {
        ESP_LOGE(TAG, "Invalid GPIO handle");
        return -1;
    }

    dev_gpio_ctrl_config_t *cfg = NULL;
    esp_board_device_get_config_by_handle(device_handle, (void **)&cfg);
    int gpio_num = gpio_handle->gpio_num;
    if (cfg) {
        esp_board_periph_unref_handle(cfg->gpio_name);
    }

    ESP_LOGI(TAG, "GPIO control device gpio %d deinitialized", gpio_num);
    return 0;
}
