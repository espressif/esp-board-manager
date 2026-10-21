/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once
static inline void host_log(const char *tag, const char *fmt, ...)
{
    (void)tag;
    (void)fmt;
}
#define ESP_LOGE(...)  host_log(__VA_ARGS__)
#define ESP_LOGD(...)  host_log(__VA_ARGS__)
#define ESP_LOGI(...)  host_log(__VA_ARGS__)
