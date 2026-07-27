/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include "bmgr_test_options.h"

static bmgr_test_options_t s_options;

void bmgr_test_options_set(const bmgr_test_options_t *options)
{
    if (options == NULL) {
        memset(&s_options, 0, sizeof(s_options));
        return;
    }
    s_options = *options;
}

bmgr_test_options_t bmgr_test_options_get(void)
{
    return s_options;
}
