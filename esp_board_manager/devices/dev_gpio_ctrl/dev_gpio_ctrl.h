/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>
#include "periph_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  GPIO control device configuration structure
 */
typedef struct {
    const char *name;           /*!< GPIO control device name */
    const char *type;           /*!< Device type: "gpio_ctrl" */
    const char *gpio_name;      /*!< GPIO peripheral name */
    uint8_t     active_level;   /*!< Level when the control function is active (0 or 1) */
    uint8_t     default_level;  /*!< Level when inactive (0 or 1); deinit does not write it */
    int8_t      init_level;     /*!< Initial output level: -1 uses active_level, 0 or 1 selects that level */
} dev_gpio_ctrl_config_t;

/**
 * @brief  Initialize GPIO control device
 *
 *         Write init_level, or active_level when init_level is -1. GPIO peripheral
 *         initialization may have already applied a different peripheral default.
 *         Regenerate existing BMGR configurations after adding this field. Handwritten
 *         C configurations must set init_level explicitly.
 *
 * @param[in]   cfg            Pointer to device configuration
 * @param[in]   cfg_size       Size of configuration structure
 * @param[out]  device_handle  Pointer to store periph_gpio_handle_t handle
 *
 * @return
 *       - ESP_OK                On success
 *       - ESP_ERR_INVALID_ARG   If a level is outside its allowed range
 *       - ESP_ERR_INVALID_SIZE  If cfg_size is smaller than dev_gpio_ctrl_config_t
 *       - Others                On invalid arguments, peripheral lookup, or GPIO failure
 */
int dev_gpio_ctrl_init(void *cfg, int cfg_size, void **device_handle);

/**
 * @brief  Deinitialize GPIO control device
 *
 * @param[in]  device_handle  Device handle
 *
 * @return
 *       - 0               On success
 *       - Negative_value  On failure
 */
int dev_gpio_ctrl_deinit(void *device_handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
