/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct {
    bool  summary;
    bool  json;
} bmgr_test_options_t;

void bmgr_test_options_set(const bmgr_test_options_t *options);
bmgr_test_options_t bmgr_test_options_get(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
