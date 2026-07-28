/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct {
    bool      board_inited;
    bool      debug_board_inited;
    bool      console_ready;
    bool      lvgl_inited;
    bool      lcd_touch_inited;
    bool      audio_power_enabled;
    uint32_t  running_case_count;
} bmgr_test_context_t;

void bmgr_test_context_init(bmgr_test_context_t *ctx);
esp_err_t bmgr_test_board_init(bmgr_test_context_t *ctx, bool debug);
esp_err_t bmgr_test_board_deinit(bmgr_test_context_t *ctx);
esp_err_t bmgr_test_require_board(bmgr_test_context_t *ctx);
void bmgr_test_context_print(const bmgr_test_context_t *ctx);

/**
 * @brief  Check whether a name refers to a device (as opposed to a peripheral)
 *
 * @param[in]  name  Device or peripheral name
 *
 * @return
 *       - true   if the name has a device descriptor
 *       - false  otherwise
 */
bool bmgr_test_name_is_device(const char *name);

/**
 * @brief  Initialize a single device or peripheral by name
 *
 *         Automatically dispatches to the device or peripheral init path based
 *         on the name. Both paths are reference counted, so calling this for a
 *         resource that is already initialized just increments its ref count.
 *
 * @param[in]  name  Device or peripheral name
 *
 * @return
 *       - ESP_OK                                  On success
 *       - ESP_BOARD_ERR_MANAGER_DEVICE_NOT_FOUND  If the name is not present on the board
 *       - Others                                  Error codes from the underlying init
 */
esp_err_t bmgr_test_target_init(const char *name);

/**
 * @brief  Deinitialize a single device or peripheral by name
 *
 * @param[in]  name  Device or peripheral name
 *
 * @return
 *       - ESP_OK                                  On success
 *       - ESP_BOARD_ERR_MANAGER_DEVICE_NOT_FOUND  If the name is not present on the board
 *       - Others                                  Error codes from the underlying deinit
 */
esp_err_t bmgr_test_target_deinit(const char *name);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
