/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Run the standalone knob device test.
 *
 * @return
 *       - ESP_OK  The knob handle and native APIs were exercised.
 *       - Others  The knob device or an invoked native API failed.
 */
esp_err_t test_dev_knob(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
