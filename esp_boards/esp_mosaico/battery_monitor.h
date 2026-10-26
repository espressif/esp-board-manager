/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_err.h"
#include "sdkconfig.h"

#if CONFIG_ESP_BOARD_DEV_CUSTOM_SUPPORT && __has_include("gen_board_device_custom.h") && __has_include("bq27220.h")
#define MOSAICO_HAS_BATTERY_MONITOR  1

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Install board I2C cleanup for the BQ27220 compatibility wrapper.
 *
 * @note  Call after revision selection and before any peripheral is initialized.
 *        I2C initialization and the exposed native bus handle stay unchanged.
 *
 * @return
 *       - ESP_OK  Board I2C operations installed.
 *       - Other   Error returned by esp_board_periph_set_ops().
 */
esp_err_t mosaico_battery_monitor_setup(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#else
#define MOSAICO_HAS_BATTERY_MONITOR  0
#endif  /* CONFIG_ESP_BOARD_DEV_CUSTOM_SUPPORT && __has_include("gen_board_device_custom.h") && __has_include("bq27220.h") */
