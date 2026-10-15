/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define EXTRA_FUNC_IMPLEMENT(name, extra_func)  /* host stub */

static inline int esp_board_extra_func_get(const char *name, void **func)
{
    (void)name;
    (void)func;
    return -1;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
