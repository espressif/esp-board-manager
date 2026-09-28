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

esp_err_t esp_board_periph_init(const char *name);
esp_err_t esp_board_periph_deinit(const char *name);
esp_err_t esp_board_periph_init_all(void);
esp_err_t esp_board_periph_deinit_all(void);
esp_err_t esp_board_periph_get_handle(const char *name, void **periph_handle);
esp_err_t esp_board_periph_get_config(const char *name, void **config);
esp_err_t esp_board_periph_show(const char *name);
esp_err_t esp_board_periph_override_config(const char *name, const void *config, uint16_t config_size);
esp_err_t esp_board_periph_restore_config(const char *name);
esp_err_t esp_board_periph_set_ops(const char *name, esp_board_periph_init_func init,
                                   esp_board_periph_deinit_func deinit);
esp_err_t esp_board_periph_restore_ops(const char *name);
esp_err_t esp_board_periph_init_custom(const char *name, esp_board_periph_init_func init,
                                       esp_board_periph_deinit_func deinit)
    __attribute__((deprecated("Use esp_board_periph_set_ops or the board pre-init hook")));

#ifdef __cplusplus
}
#endif  /* __cplusplus */
