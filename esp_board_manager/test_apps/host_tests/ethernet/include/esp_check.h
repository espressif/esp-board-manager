/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once
#include "esp_err.h"
#define ESP_RETURN_ON_ERROR(expr, tag, ...)  do {  \
    esp_err_t ret_ = (expr);                       \
    if (ret_ != ESP_OK) {                          \
        (void)(tag);                               \
        return ret_;                               \
    }                                              \
} while (0)
