/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

esp_err_t periph_i2c_set_effective_addr_internal(const char *i2c_name, const char *device_name,
                                                 uint16_t addr);
esp_err_t periph_i2c_get_effective_addr_internal(const char *device_name, uint16_t *addr);
esp_err_t periph_i2c_clear_effective_addr_internal(const char *device_name);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
