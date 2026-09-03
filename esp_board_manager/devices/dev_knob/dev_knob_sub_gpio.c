/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdlib.h>
#include "esp_board_entry.h"
#include "esp_log.h"
#include "knob_rtc.h"
#include "dev_knob.h"

static const char *TAG = "DEV_KNOB_GPIO";

static int dev_knob_sub_gpio_init(void *cfg, int cfg_size, void **device_handle)
{
    if (cfg == NULL || device_handle == NULL ||
        cfg_size != sizeof(dev_knob_config_t)) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    const dev_knob_config_t *config = (const dev_knob_config_t *)cfg;
    dev_knob_handles_t *handles = calloc(1, sizeof(dev_knob_handles_t));
    if (handles == NULL) {
        ESP_LOGE(TAG, "Failed to allocate knob handle");
        return -1;
    }

    if (config->use_rtc) {
        handles->knob_handle = iot_knob_create_rtc(&config->knob_config);
    } else {
        handles->knob_handle = iot_knob_create(&config->knob_config);
    }
    if (handles->knob_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create %s knob", config->use_rtc ? "RTC GPIO" : "GPIO");
        free(handles);
        return -1;
    }

    *device_handle = handles;
    ESP_LOGI(TAG, "Initialized knob %s: A=%u B=%u backend=%s",
             config->name, config->knob_config.gpio_encoder_a, config->knob_config.gpio_encoder_b,
             config->use_rtc ? "rtc" : "gpio");
    return 0;
}

static int dev_knob_sub_gpio_deinit(void *device_handle)
{
    if (device_handle == NULL) {
        ESP_LOGE(TAG, "Invalid device handle");
        return -1;
    }

    dev_knob_handles_t *handles = (dev_knob_handles_t *)device_handle;
    if (handles->knob_handle != NULL) {
        esp_err_t ret = iot_knob_delete(handles->knob_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete knob: %s", esp_err_to_name(ret));
            return -1;
        }
        handles->knob_handle = NULL;
    }

    free(handles);
    return 0;
}

ESP_BOARD_SUBTYPE_ENTRY_IMPLEMENT(knob, gpio, dev_knob_sub_gpio_init,
                                  dev_knob_sub_gpio_deinit);
