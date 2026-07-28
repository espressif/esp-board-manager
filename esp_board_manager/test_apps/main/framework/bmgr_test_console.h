/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_err.h"
#include "bmgr_test_context.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

esp_err_t bmgr_test_console_start(bmgr_test_context_t *ctx);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
