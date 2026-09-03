/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"

#include "dev_custom.h"
#include "esp_board_periph.h"
#include "gen_board_device_custom.h"

#if __has_include(<bmi270.h>) && __has_include(<iot_sensor_hub.h>)
#define HAS_BMI270_SENSOR_HUB  1
#include "bmi270.h"
#include "iot_sensor_hub.h"
#endif  /* __has_include(<bmi270.h>) && __has_include(<iot_sensor_hub.h>) */

#if defined(HAS_BMI270_SENSOR_HUB)

typedef struct {
    sensor_handle_t  sensor;
    const char      *peripheral_name;
    bool             peripheral_ref_held;
} bmi270_sensor_hub_device_t;

static const char *TAG = "M5_ATOM_S3R_BMI270_HUB";
static bmi270_sensor_hub_device_t s_bmi270_sensor_hub;

static int bmi270_sensor_hub_init_device(void *config, int config_size, void **device_handle)
{
    if (config == NULL || device_handle == NULL ||
        config_size != sizeof(dev_custom_bmi270_sensor_hub_config_t)) {
        return ESP_ERR_INVALID_ARG;
    }
    *device_handle = NULL;

    const dev_custom_bmi270_sensor_hub_config_t *cfg = config;
    if (cfg->peripheral_name == NULL || cfg->i2c_addr != BMI270_I2C_ADDRESS_L) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_bmi270_sensor_hub.sensor != NULL || s_bmi270_sensor_hub.peripheral_ref_held) {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_master_bus_handle_t i2c_bus = NULL;
    sensor_handle_t sensor = NULL;
    esp_err_t ret = esp_board_periph_ref_handle(cfg->peripheral_name, (void **)&i2c_bus);
    if (ret != ESP_OK || i2c_bus == NULL) {
        return ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
    }

    sensor_config_t sensor_config = {
        .bus = i2c_bus,
        .addr = cfg->i2c_addr,
        .type = IMU_ID,
        .mode = MODE_POLLING,
        .range = RANGE_DEFAULT,
        .min_delay = 100,
        .intr_pin = -1,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = iot_sensor_create("sensor_hub_bmi270", &sensor_config, &sensor);
    if (ret != ESP_OK || sensor == NULL) {
        if (ret == ESP_OK) {
            ret = ESP_FAIL;
        }
        esp_board_periph_unref_handle(cfg->peripheral_name);
        ESP_LOGE(TAG, "Failed to create BMI270 sensor hub: %s", esp_err_to_name(ret));
        return ret;
    }

    s_bmi270_sensor_hub.sensor = sensor;
    s_bmi270_sensor_hub.peripheral_name = cfg->peripheral_name;
    s_bmi270_sensor_hub.peripheral_ref_held = true;
    *device_handle = sensor;
    return ESP_OK;
}

static int bmi270_sensor_hub_deinit_device(void *device_handle)
{
    if (device_handle == NULL || device_handle != s_bmi270_sensor_hub.sensor) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = iot_sensor_stop(s_bmi270_sensor_hub.sensor);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = iot_sensor_delete(s_bmi270_sensor_hub.sensor);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = esp_board_periph_unref_handle(s_bmi270_sensor_hub.peripheral_name);
    if (ret != ESP_OK) {
        return ret;
    }

    s_bmi270_sensor_hub.sensor = NULL;
    s_bmi270_sensor_hub.peripheral_name = NULL;
    s_bmi270_sensor_hub.peripheral_ref_held = false;
    return ESP_OK;
}

CUSTOM_DEVICE_IMPLEMENT(bmi270_sensor_hub, bmi270_sensor_hub_init_device,
                        bmi270_sensor_hub_deinit_device);
#endif  /* defined(HAS_BMI270_SENSOR_HUB) */
