/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "host_idf_stubs.h"
#include "esp_board_extra_func_entry.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

esp_err_t esp_board_device_init(const char *name);
esp_err_t esp_board_device_deinit(const char *name);
esp_err_t esp_board_device_init_all(void);
esp_err_t esp_board_device_deinit_all(void);
esp_err_t esp_board_device_get_handle(const char *name, void **device_handle);
esp_err_t esp_board_device_get_config(const char *name, void **config);
esp_err_t esp_board_device_show(const char *name);
esp_err_t esp_board_device_override_config(const char *name, const void *config, uint16_t config_size);
esp_err_t esp_board_device_restore_config(const char *name);
esp_err_t esp_board_device_set_ops(const char *name, esp_board_device_init_func init,
                                   esp_board_device_deinit_func deinit);
esp_err_t esp_board_device_restore_ops(const char *name);
esp_err_t esp_board_device_power_ctrl(const char *name, bool power_on);
esp_err_t esp_board_device_callback_register(const char *name, void *callback, void *user_data);
esp_err_t esp_board_device_get_i2c_effective_addr(const char *device_name, uint16_t *addr);
const esp_board_device_handle_t *esp_board_device_find_by_handle(void *device_handle);
void esp_board_device_restore_all_configs(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
