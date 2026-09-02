/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct lp5562_t *lp5562_handle_t;

/**
 * @brief  Create and configure an LP5562 controller.
 *
 * @param[in]   bus         Existing I2C master bus.
 * @param[in]   address     Seven-bit LP5562 I2C address.
 * @param[out]  ret_handle  Returned LP5562 handle.
 *
 * @return
 *       - ESP_OK               Controller created.
 *       - ESP_ERR_INVALID_ARG  Invalid bus, address, or output handle.
 *       - ESP_ERR_NO_MEM       Insufficient memory.
 *       - Other                error codes from the I2C driver.
 */
esp_err_t lp5562_new(i2c_master_bus_handle_t bus, uint8_t address, lp5562_handle_t *ret_handle);

/**
 * @brief  Set the white LED output brightness.
 *
 * @param[in]  handle   LP5562 handle.
 * @param[in]  percent  Brightness in the inclusive range 0 to 100.
 *
 * @return
 *       - ESP_OK               Brightness updated.
 *       - ESP_ERR_INVALID_ARG  Invalid handle or brightness value.
 *       - Other                error codes from the I2C driver.
 */
esp_err_t lp5562_set_brightness(lp5562_handle_t handle, uint8_t percent);

/**
 * @brief  Delete an LP5562 controller and release its I2C device handle.
 *
 * @param[in]  handle  LP5562 handle.
 *
 * @return
 *       - ESP_OK               Controller deleted.
 *       - ESP_ERR_INVALID_ARG  Invalid handle.
 *       - Other                error codes from the I2C driver.
 */
esp_err_t lp5562_del(lp5562_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
