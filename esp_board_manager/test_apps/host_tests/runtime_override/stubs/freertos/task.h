/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/** Host test helper: switch apparent FreeRTOS task identity. */
void host_set_current_task(TaskHandle_t task);

TaskHandle_t xTaskGetCurrentTaskHandle(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
