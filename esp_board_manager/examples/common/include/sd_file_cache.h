/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stddef.h>
#include <stdio.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SD_FILE_CACHE_DEFAULT_SIZE  (32 * 1024)
#define SD_FILE_CACHE_MIN_SIZE      (4 * 1024)

typedef struct {
    void *buffer;
    size_t  size;
} sd_file_cache_t;

/**
 * @brief  Attach a best-effort, DMA-friendly stdio buffer to a stream.
 *
 *         Call this after fopen() and before the first I/O operation. A failure
 *         leaves the stream usable with its default stdio buffer.
 */
esp_err_t sd_file_cache_attach(FILE *fp, sd_file_cache_t *cache,
    size_t preferred_size, size_t minimum_size);

/**
 * @brief  Close a stream and release a buffer previously attached to it.
 */
esp_err_t sd_file_cache_close(FILE **fp, sd_file_cache_t *cache);

#ifdef __cplusplus
}
#endif
