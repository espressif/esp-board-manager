/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once
#include <stdint.h>
typedef int esp_err_t;
#define ESP_OK               0
#define ESP_FAIL             -1
#define ESP_ERR_INVALID_ARG  0x102
const char *esp_err_to_name(esp_err_t err);
