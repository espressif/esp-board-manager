/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "dev_custom.h"
#include "esp_board_periph.h"
#include "gen_board_device_custom.h"

static const char *TAG = "S31_KORVO_CAM";

typedef struct {
    const char *i2c_name;
} camera_sensor_bringup_handle_t;

static int camera_sensor_bringup_init(void *config, int config_size,
                                      void **device_handle)
{
    if (config == NULL || device_handle == NULL ||
        config_size != sizeof(dev_custom_camera_sensor_bringup_config_t)) {
        ESP_LOGE(TAG, "Invalid camera sensor bring-up config");
        return ESP_ERR_INVALID_ARG;
    }

    const dev_custom_camera_sensor_bringup_config_t *cfg = config;
    if (cfg->i2c_name == NULL) {
        ESP_LOGE(TAG, "Camera SCCB I2C peripheral is not configured");
        return ESP_ERR_INVALID_ARG;
    }

    if (cfg->xclk_stable_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(cfg->xclk_stable_ms));
    }

    void *periph_handle = NULL;
    esp_err_t ret = esp_board_periph_ref_handle(cfg->i2c_name, &periph_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SCCB I2C peripheral %s: %s", cfg->i2c_name, esp_err_to_name(ret));
        return ret;
    }

    i2c_master_bus_handle_t i2c_handle = periph_handle;
    const i2c_device_config_t sensor_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = cfg->sccb_addr,
        .scl_speed_hz = cfg->sccb_freq_hz,
    };
    i2c_master_dev_handle_t sensor_handle = NULL;
    ret = i2c_master_bus_add_device(i2c_handle, &sensor_config, &sensor_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add OV3660 SCCB device: %s", esp_err_to_name(ret));
        goto unref_i2c;
    }

    const uint8_t soft_reset[] = {
        (uint8_t)(cfg->reset_reg >> 8),
        (uint8_t)cfg->reset_reg,
        cfg->reset_value,
    };
    ret = i2c_master_transmit(sensor_handle, soft_reset, sizeof(soft_reset),
                              cfg->timeout_ms);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset OV3660 over SCCB: %s", esp_err_to_name(ret));
    }

    esp_err_t remove_ret = i2c_master_bus_rm_device(sensor_handle);
    if (ret == ESP_OK && remove_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to remove OV3660 SCCB device: %s", esp_err_to_name(remove_ret));
        ret = remove_ret;
    }
    if (ret != ESP_OK) {
        goto unref_i2c;
    }

    if (cfg->reset_delay_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(cfg->reset_delay_ms));
    }

    camera_sensor_bringup_handle_t *handle = calloc(1, sizeof(*handle));
    if (handle == NULL) {
        ESP_LOGE(TAG, "Failed to allocate camera sensor bring-up handle");
        ret = ESP_ERR_NO_MEM;
        goto unref_i2c;
    }

    handle->i2c_name = cfg->i2c_name;
    *device_handle = handle;
    ESP_LOGI(TAG, "OV3660 XCLK settled and SCCB soft reset completed");
    return ESP_OK;

unref_i2c:
    esp_err_t unref_ret = esp_board_periph_unref_handle(cfg->i2c_name);
    if (unref_ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to release SCCB I2C peripheral %s: %s", cfg->i2c_name, esp_err_to_name(unref_ret));
    }
    return ret;
}

static int camera_sensor_bringup_deinit(void *device_handle)
{
    if (device_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    camera_sensor_bringup_handle_t *handle = device_handle;
    esp_err_t ret = esp_board_periph_unref_handle(handle->i2c_name);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to release SCCB I2C peripheral %s: %s", handle->i2c_name, esp_err_to_name(ret));
    }
    free(handle);
    return ret;
}

CUSTOM_DEVICE_IMPLEMENT(camera_sensor_bringup, camera_sensor_bringup_init, camera_sensor_bringup_deinit);
