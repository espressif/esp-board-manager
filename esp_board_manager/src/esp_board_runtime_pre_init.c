/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_err.h"
#include "esp_board_manager.h"

/**
 * Default lives in its own translation unit. A one-line weak definition in
 * esp_board_runtime_state.c is inlined into prepare() at -Os, so a board
 * strong definition would never run.
 */
__attribute__((weak)) esp_err_t esp_board_runtime_pre_init(void)
{
    return ESP_OK;
}
