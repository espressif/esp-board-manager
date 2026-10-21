/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
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
 * @brief  Read a board-local IO through an extra function named <device>_io_get.
 *
 * @param[in]   handle  Active Board Manager device handle, borrowed for this call.
 * @param[in]   pin     Zero-based device pin index.
 * @param[out]  level   Physical pin level, 0 or 1.
 * @return
 *       - ESP_OK  on success, or a driver error.
 */
typedef esp_err_t (*bmgr_test_io_get_fn_t)(void *handle, int pin, int *level);

/**
 * @brief  Write a board-local IO through an extra function named <device>_io_set.
 *
 * @param[in]  handle  Active Board Manager device handle, borrowed for this call.
 * @param[in]  pin     Zero-based device pin index.
 * @param[in]  level   Physical output level, 0 or 1.
 * @return
 *       - ESP_OK  on success, or a driver error.
 */
typedef esp_err_t (*bmgr_test_io_set_fn_t)(void *handle, int pin, int level);

/** @brief Register the interactive IO command; return ESP_OK or a console error. */
esp_err_t register_io_command(void);

/** @brief Remove the interactive IO command. */
void unregister_io_command(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
