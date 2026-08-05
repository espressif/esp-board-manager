/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "esp_heap_caps.h"
#include "soc/soc_caps.h"
#include "sd_file_cache.h"

#define SD_FILE_CACHE_ALIGNMENT  64

static void *allocate_file_cache(size_t *size, size_t minimum_size, uint32_t caps)
{
    size_t candidate_size = *size;

    while (candidate_size >= minimum_size) {
        void *buffer = heap_caps_aligned_alloc(SD_FILE_CACHE_ALIGNMENT, candidate_size, caps);
        if (buffer != NULL) {
            *size = candidate_size;
            return buffer;
        }
        if (candidate_size == minimum_size) {
            break;
        }
        candidate_size /= 2;
        if (candidate_size < minimum_size) {
            candidate_size = minimum_size;
        }
    }
    return NULL;
}

esp_err_t sd_file_cache_attach(FILE *fp, sd_file_cache_t *cache,
    size_t preferred_size, size_t minimum_size)
{
    void *buffer = NULL;
    size_t cache_size;

    if (fp == NULL || cache == NULL || cache->buffer != NULL ||
        minimum_size == 0 || preferred_size < minimum_size) {
        return ESP_ERR_INVALID_ARG;
    }

    cache_size = preferred_size;
#if CONFIG_SPIRAM && SOC_SDMMC_PSRAM_DMA_CAPABLE
    buffer = allocate_file_cache(&cache_size, minimum_size,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT |
            MALLOC_CAP_CACHE_ALIGNED);
#endif
    if (buffer == NULL) {
        cache_size = preferred_size;
        buffer = allocate_file_cache(&cache_size, minimum_size,
            MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_CACHE_ALIGNED);
    }
    if (buffer == NULL) {
        cache_size = preferred_size;
        buffer = allocate_file_cache(&cache_size, minimum_size,
            MALLOC_CAP_8BIT | MALLOC_CAP_CACHE_ALIGNED);
    }
    if (buffer == NULL) {
        return ESP_ERR_NO_MEM;
    }
    if (setvbuf(fp, buffer, _IOFBF, cache_size) != 0) {
        heap_caps_free(buffer);
        return ESP_FAIL;
    }

    cache->buffer = buffer;
    cache->size = cache_size;
    return ESP_OK;
}

esp_err_t sd_file_cache_close(FILE **fp, sd_file_cache_t *cache)
{
    int close_result;

    if (fp == NULL || *fp == NULL || cache == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    close_result = fclose(*fp);
    *fp = NULL;
    heap_caps_free(cache->buffer);
    cache->buffer = NULL;
    cache->size = 0;
    return close_result == 0 ? ESP_OK : ESP_FAIL;
}
