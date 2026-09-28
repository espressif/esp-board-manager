/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "host_idf_stubs.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

esp_err_t esp_board_runtime_pre_init(void);
esp_err_t esp_board_manager_prepare(void);
esp_err_t esp_board_manager_init(void);
esp_err_t esp_board_manager_deinit(void);
bool esp_board_manager_check_name(const char *name);
esp_err_t esp_board_manager_get_periph_handle(const char *periph_name, void **periph_handle);
esp_err_t esp_board_manager_get_device_handle(const char *dev_name, void **device_handle);
esp_err_t esp_board_manager_get_device_config(const char *dev_name, void **config);
esp_err_t esp_board_manager_get_periph_config(const char *periph_name, void **config);
esp_err_t esp_board_manager_get_board_info(esp_board_info_t *board_info);
esp_err_t esp_board_manager_register_device_handle(esp_board_device_handle_t *reg_handle)
    __attribute__((deprecated("Use esp_board_device_set_ops for existing devices")));
esp_err_t esp_board_manager_init_device_by_name(const char *dev_name);
esp_err_t esp_board_manager_deinit_device_by_name(const char *dev_name);
esp_err_t esp_board_manager_print(void);
esp_err_t esp_board_manager_print_board_info(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
