/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>
#include "iot_knob.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define ESP_BOARD_DEVICE_KNOB_SUB_TYPE_GPIO  "gpio"  /*!< GPIO quadrature encoder input */

/**
 * @brief  Knob device handles.
 */
typedef struct {
    knob_handle_t  knob_handle;  /*!< Native knob component handle */
} dev_knob_handles_t;

/**
 * @brief  Knob device configuration.
 *
 *         The GPIO and RTC implementations share @c knob_config_t. When
 *         @c use_rtc is true, both encoder GPIOs must be RTC-capable and the
 *         device uses the knob component RTC GPIO backend.
 */
typedef struct {
    const char    *name;         /*!< Device name */
    const char    *sub_type;     /*!< Input interface, currently "gpio" */
    bool           use_rtc;      /*!< Use RTC GPIO backend instead of regular GPIO */
    knob_config_t  knob_config;  /*!< Native knob component configuration */
} dev_knob_config_t;

/**
 * @brief  Initialize a knob device.
 *
 * @param[in]   cfg            Pointer to dev_knob_config_t.
 * @param[in]   cfg_size       Size of @p cfg.
 * @param[out]  device_handle  Pointer to receive dev_knob_handles_t.
 *
 * @return
 *       - 0   Success.
 *       - -1  Invalid arguments or knob initialization failure.
 */
int dev_knob_init(void *cfg, int cfg_size, void **device_handle);

/**
 * @brief  Deinitialize a knob device.
 *
 * @param[in]  device_handle  Pointer to dev_knob_handles_t.
 *
 * @return
 *       - 0   Success.
 *       - -1  Invalid arguments or knob deinitialization failure.
 */
int dev_knob_deinit(void *device_handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
