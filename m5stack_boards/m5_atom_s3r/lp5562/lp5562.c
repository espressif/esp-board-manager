/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#include "lp5562.h"

#define LP5562_I2C_SPEED_HZ         (400000U)
#define LP5562_REG_ENABLE           (0x00U)
#define LP5562_REG_CONFIG           (0x08U)
#define LP5562_REG_WHITE_PWM        (0x0EU)
#define LP5562_REG_ENGINE_SELECT    (0x70U)
#define LP5562_ENABLE_CHIP          (0x40U)
#define LP5562_ENABLE_INTERNAL_CLK  (0x41U)
#define LP5562_I2C_CONTROLLED       (0x00U)
#define LP5562_TIMEOUT_MS           (100)

struct lp5562_t {
    i2c_master_dev_handle_t  i2c_dev;
};

static esp_err_t lp5562_write_reg(lp5562_handle_t handle, uint8_t reg, uint8_t value)
{
    const uint8_t command[] = {reg, value};
    return i2c_master_transmit(handle->i2c_dev, command, sizeof(command), LP5562_TIMEOUT_MS);
}

esp_err_t lp5562_new(i2c_master_bus_handle_t bus, uint8_t address, lp5562_handle_t *ret_handle)
{
    if (bus == NULL || ret_handle == NULL || address > 0x7FU) {
        return ESP_ERR_INVALID_ARG;
    }
    *ret_handle = NULL;

    lp5562_handle_t handle = calloc(1, sizeof(*handle));
    if (handle == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = LP5562_I2C_SPEED_HZ,
    };
    esp_err_t ret = i2c_master_bus_add_device(bus, &device_config, &handle->i2c_dev);
    if (ret != ESP_OK) {
        free(handle);
        return ret;
    }

    ret = lp5562_write_reg(handle, LP5562_REG_ENABLE, LP5562_ENABLE_CHIP);
    if (ret == ESP_OK) {
        ret = lp5562_write_reg(handle, LP5562_REG_CONFIG, LP5562_ENABLE_INTERNAL_CLK);
    }
    if (ret == ESP_OK) {
        ret = lp5562_write_reg(handle, LP5562_REG_ENGINE_SELECT, LP5562_I2C_CONTROLLED);
    }
    if (ret != ESP_OK) {
        esp_err_t cleanup_ret = i2c_master_bus_rm_device(handle->i2c_dev);
        if (cleanup_ret == ESP_OK) {
            free(handle);
        }
        return ret;
    }

    *ret_handle = handle;
    return ESP_OK;
}

esp_err_t lp5562_set_brightness(lp5562_handle_t handle, uint8_t percent)
{
    if (handle == NULL || percent > 100U) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t pwm = ((uint16_t)percent * UINT8_MAX) / 100U;
    return lp5562_write_reg(handle, LP5562_REG_WHITE_PWM, pwm);
}

esp_err_t lp5562_del(lp5562_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = i2c_master_bus_rm_device(handle->i2c_dev);
    if (ret == ESP_OK) {
        free(handle);
    }
    return ret;
}
