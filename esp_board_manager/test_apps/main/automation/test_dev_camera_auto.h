/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct {
    uint32_t  width;
    uint32_t  height;
    uint32_t  pixelformat;
    uint32_t  bytesused;
    uint32_t  checksum;
    float     mean_luma;
} test_dev_camera_auto_summary_t;

esp_err_t test_dev_camera_auto_capture(test_dev_camera_auto_summary_t *summary);
const test_dev_camera_auto_summary_t *test_dev_camera_auto_get_last_summary(void);
void test_dev_camera_auto_clear(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
