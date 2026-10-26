/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/**
 * Host-side ESP-IDF error and logging substitutes. BMGR types and helpers
 * come from the production include/ and private_inc/ directories.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int32_t esp_err_t;

#define ESP_OK                 0
#define ESP_FAIL               -1
#define ESP_ERR_NO_MEM         0x101
#define ESP_ERR_INVALID_ARG    0x102
#define ESP_ERR_INVALID_STATE  0x103
#define ESP_ERR_INVALID_SIZE   0x104
#define ESP_ERR_NOT_FOUND      0x105
#define ESP_ERR_NOT_SUPPORTED  0x106

static inline const char *esp_err_to_name(esp_err_t err)
{
    switch (err) {
        case ESP_OK:
            return "ESP_OK";
        case ESP_FAIL:
            return "ESP_FAIL";
        case ESP_ERR_INVALID_STATE:
            return "ESP_ERR_INVALID_STATE";
        case ESP_ERR_NOT_FOUND:
            return "ESP_ERR_NOT_FOUND";
        default:
            return "ESP_ERR_UNKNOWN";
    }
}

#define ESP_LOGE(tag, fmt, ...)  fprintf(stderr, "E (%s) " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...)  fprintf(stderr, "W (%s) " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...)  ((void)0)
#define ESP_LOGD(tag, fmt, ...)  ((void)0)
