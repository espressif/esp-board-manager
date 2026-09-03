/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "dev_custom.h"
#include "esp_board_periph.h"
#include "gen_board_device_custom.h"
#if __has_include(<bmi270_api.h>)
#define HAS_BMI270  1
#include "bmi270_api.h"
#endif  /* __has_include(<bmi270_api.h>) */
#if __has_include(<i2c_bus.h>)
#define HAS_I2C_BUS  1
#include "i2c_bus.h"
#endif  /* __has_include(<i2c_bus.h>) */

#if defined(HAS_BMI270) && defined(HAS_I2C_BUS)

#define BMI270_SENSOR_I2C_SPEED_HZ  (400000U)

typedef struct {
    bmi270_handle_t  sensor;
    const char      *peripheral_name;
    gpio_num_t       sdo_gpio_num;
    bool             peripheral_ref_held;
    bool             sdo_gpio_configured;
} bmi270_sensor_context_t;

static const char *TAG = "SENSAIRSHUTTLE_BMI270";
static bmi270_sensor_context_t s_bmi270_sensor;

static int bmi270_sensor_init(void *config, int config_size, void **device_handle)
{
    if (config == NULL || device_handle == NULL ||
        config_size != sizeof(dev_custom_bmi270_sensor_config_t)) {
        return ESP_ERR_INVALID_ARG;
    }
    *device_handle = NULL;

    const dev_custom_bmi270_sensor_config_t *cfg = config;
    if (cfg->peripheral_name == NULL || cfg->i2c_addr != BMI270_I2C_ADDRESS) {
        ESP_LOGE(TAG, "BMI270 sensor requires I2C address 0x%02x, configured 0x%02x",
                 BMI270_I2C_ADDRESS, (unsigned int)cfg->i2c_addr);
        return ESP_ERR_INVALID_ARG;
    }
    if (!GPIO_IS_VALID_OUTPUT_GPIO(cfg->sdo_gpio_num)) {
        ESP_LOGE(TAG, "BMI270 SDO GPIO %d is not output capable", cfg->sdo_gpio_num);
        return ESP_ERR_INVALID_ARG;
    }
    if (s_bmi270_sensor.sensor != NULL || s_bmi270_sensor.peripheral_ref_held ||
        s_bmi270_sensor.sdo_gpio_configured) {
        ESP_LOGE(TAG, "BMI270 sensor is already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_master_bus_handle_t i2c_master = NULL;
    i2c_master_bus_config_t *i2c_cfg = NULL;
    i2c_bus_handle_t bridge_i2c = NULL;
    bmi270_handle_t sensor = NULL;
    bool peripheral_ref_held = false;
    bool sdo_gpio_configured = false;

    const gpio_config_t sdo_config = {
        .pin_bit_mask = 1ULL << cfg->sdo_gpio_num,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&sdo_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure BMI270 SDO GPIO%d: %s", cfg->sdo_gpio_num,
                 esp_err_to_name(ret));
        return ret;
    }
    sdo_gpio_configured = true;

    ret = gpio_set_level(cfg->sdo_gpio_num, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to select BMI270 I2C address 0x%02x: %s", BMI270_I2C_ADDRESS,
                 esp_err_to_name(ret));
        goto cleanup;
    }

    ret = esp_board_periph_ref_handle(cfg->peripheral_name, (void **)&i2c_master);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reference I2C peripheral %s: %s", cfg->peripheral_name,
                 esp_err_to_name(ret));
        goto cleanup;
    }
    peripheral_ref_held = true;

    if (i2c_master == NULL) {
        ret = ESP_ERR_INVALID_STATE;
        ESP_LOGE(TAG, "I2C peripheral %s returned a NULL handle", cfg->peripheral_name);
        goto cleanup;
    }

    ret = esp_board_periph_get_config(cfg->peripheral_name, (void **)&i2c_cfg);
    if (ret != ESP_OK || i2c_cfg == NULL) {
        ESP_LOGE(TAG, "Failed to get I2C peripheral config %s: %s", cfg->peripheral_name,
                 esp_err_to_name(ret));
        goto cleanup;
    }

    const i2c_config_t legacy_i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = i2c_cfg->sda_io_num,
        .scl_io_num = i2c_cfg->scl_io_num,
        .sda_pullup_en = i2c_cfg->flags.enable_internal_pullup,
        .scl_pullup_en = i2c_cfg->flags.enable_internal_pullup,
        .master.clk_speed = BMI270_SENSOR_I2C_SPEED_HZ,
    };
    bridge_i2c = i2c_bus_create(i2c_cfg->i2c_port, &legacy_i2c_cfg);
    if (bridge_i2c == NULL) {
        ret = ESP_FAIL;
        ESP_LOGE(TAG, "Failed to create i2c_bus compatibility handle");
        goto cleanup;
    }

    ret = bmi270_sensor_create(bridge_i2c, &sensor, bmi270_config_file,
                               BMI2_GYRO_CROSS_SENS_ENABLE | BMI2_CRT_RTOSK_ENABLE);
    if (ret != ESP_OK || sensor == NULL) {
        if (ret == ESP_OK) {
            ret = ESP_FAIL;
        }
        ESP_LOGE(TAG, "Failed to create BMI270 sensor: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    /* The i2c_bus compatibility object wraps the BMGR-owned native bus. */
    s_bmi270_sensor.sensor = sensor;
    s_bmi270_sensor.peripheral_name = cfg->peripheral_name;
    s_bmi270_sensor.sdo_gpio_num = cfg->sdo_gpio_num;
    s_bmi270_sensor.peripheral_ref_held = true;
    s_bmi270_sensor.sdo_gpio_configured = true;
    *device_handle = sensor;
    return ESP_OK;

cleanup:
    if (sensor != NULL) {
        esp_err_t del_ret = bmi270_sensor_del(&sensor);
        if (del_ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to delete partially initialized BMI270 sensor: %s",
                     esp_err_to_name(del_ret));
        }
    }
    /* Never call i2c_bus_delete(): it would delete the BMGR-owned native bus. */
    if (peripheral_ref_held) {
        esp_err_t unref_ret = esp_board_periph_unref_handle(cfg->peripheral_name);
        if (unref_ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to release I2C peripheral %s: %s", cfg->peripheral_name,
                     esp_err_to_name(unref_ret));
        }
    }
    if (sdo_gpio_configured) {
        esp_err_t reset_ret = gpio_reset_pin(cfg->sdo_gpio_num);
        if (reset_ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to reset BMI270 SDO GPIO%d: %s", cfg->sdo_gpio_num,
                     esp_err_to_name(reset_ret));
        }
    }
    return ret;
}

static int bmi270_sensor_deinit(void *device_handle)
{
    if (device_handle == NULL ||
        (s_bmi270_sensor.sensor == NULL && !s_bmi270_sensor.peripheral_ref_held &&
         !s_bmi270_sensor.sdo_gpio_configured)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_bmi270_sensor.sensor != NULL) {
        if (device_handle != s_bmi270_sensor.sensor) {
            ESP_LOGE(TAG, "BMI270 sensor handle does not match the active board device");
            return ESP_ERR_INVALID_ARG;
        }

        bmi270_handle_t sensor = s_bmi270_sensor.sensor;
        esp_err_t ret = bmi270_sensor_del(&sensor);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete BMI270 sensor: %s", esp_err_to_name(ret));
            return ret;
        }
        s_bmi270_sensor.sensor = NULL;
    }

    if (s_bmi270_sensor.peripheral_ref_held) {
        esp_err_t ret = esp_board_periph_unref_handle(s_bmi270_sensor.peripheral_name);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to release I2C peripheral %s: %s", s_bmi270_sensor.peripheral_name,
                     esp_err_to_name(ret));
            return ret;
        }
        s_bmi270_sensor.peripheral_ref_held = false;
    }

    if (s_bmi270_sensor.sdo_gpio_configured) {
        esp_err_t ret = gpio_reset_pin(s_bmi270_sensor.sdo_gpio_num);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to reset BMI270 SDO GPIO%d: %s", s_bmi270_sensor.sdo_gpio_num,
                     esp_err_to_name(ret));
            return ret;
        }
        s_bmi270_sensor.sdo_gpio_configured = false;
    }

    s_bmi270_sensor.peripheral_name = NULL;
    return ESP_OK;
}

CUSTOM_DEVICE_IMPLEMENT(bmi270_sensor, bmi270_sensor_init, bmi270_sensor_deinit);
#endif  /* defined(HAS_BMI270) && defined(HAS_I2C_BUS) */
