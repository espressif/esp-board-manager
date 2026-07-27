/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void bmgr_test_json_metric_begin(const char *case_name, const char *name);
void bmgr_test_json_metric_end(void);
void bmgr_test_json_metric_u32(const char *key, uint32_t value);
void bmgr_test_json_metric_i32(const char *key, int32_t value);
void bmgr_test_json_metric_float(const char *key, float value);
void bmgr_test_json_metric_str(const char *key, const char *value);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
