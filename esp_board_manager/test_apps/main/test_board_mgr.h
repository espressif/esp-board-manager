/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

esp_err_t test_board_mgr_audio_embed_playback(void);
esp_err_t test_board_mgr_audio_partition_record_playback(void);
esp_err_t test_board_mgr_audio_fatfs_playback(void);
esp_err_t test_board_mgr_audio_fatfs_record_playback(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
