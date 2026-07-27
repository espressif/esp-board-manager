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
    uint32_t  sample_rate;
    uint16_t  channels;
    uint16_t  bits_per_sample;
    uint32_t  bytes;
    float     rms;
    float     peak;
    float     zero_cross_hz;
} test_dev_audio_auto_summary_t;

esp_err_t test_dev_audio_auto_play_1khz_wav(uint8_t volume);
esp_err_t test_dev_audio_auto_record(uint32_t duration_ms, uint32_t sample_rate, test_dev_audio_auto_summary_t *summary);
const test_dev_audio_auto_summary_t *test_dev_audio_auto_get_last_summary(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
