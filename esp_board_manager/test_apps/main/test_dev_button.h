/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Test dev button functionality
 */
void test_dev_button(void);

/**
 * @brief  Monitor button events for one named button device or all button devices.
 *
 * @param[in]  name  Device name, or NULL/"all" to enumerate every button device.
 *
 * @return
 *       - ESP_OK             Monitoring finished; events require manual confirmation.
 *       - ESP_ERR_NOT_FOUND  No matching button device exists.
 *       - Others             Allocation, initialization, or cleanup failed.
 */
esp_err_t test_dev_button_named(const char *name);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
