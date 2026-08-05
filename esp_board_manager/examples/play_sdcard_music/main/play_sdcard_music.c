/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_board_manager_includes.h"
#include "sd_file_cache.h"
#include "wav_header.h"

static const char *TAG = "BMGR_PLAY_SDCARD_MUSIC";

#define DEFAULT_PLAY_URL  "/sdcard/test.wav"
#define DEFAULT_PLAY_VOL  60

void app_main(void)
{
    ESP_LOGI(TAG, "Playing music from %s", DEFAULT_PLAY_URL);
    esp_err_t ret = ESP_OK;
    dev_audio_codec_handles_t *dac_handle = NULL;
    FILE *fp = NULL;
    uint8_t *playback_buffer = NULL;
    sd_file_cache_t file_cache = { 0 };
    wav_info_t wav_info = { 0 };

    // Initialize audio DAC device
    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio DAC device");
        goto cleanup;
    }

    // Initialize SD card filesystem
    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_FS_SDCARD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SD card filesystem");
        goto cleanup;
    }

    // Get DAC device handle
    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_AUDIO_DAC, (void **)&dac_handle);
    if (ret != ESP_OK || dac_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get DAC device handle");
        goto cleanup;
    }

    // Open WAV file
    fp = fopen(DEFAULT_PLAY_URL, "rb");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open file %s", DEFAULT_PLAY_URL);
        goto cleanup;
    }
    ret = sd_file_cache_attach(fp, &file_cache, SD_FILE_CACHE_DEFAULT_SIZE,
        SD_FILE_CACHE_MIN_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Using default stdio buffer: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SD file cache enabled: %u bytes", (unsigned)file_cache.size);
    }

    // Read WAV header
    ret = read_wav_info(fp, &wav_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WAV header");
        goto cleanup;
    }

    ESP_LOGI(TAG, "Play WAV file info: %" PRIu32 " Hz, %" PRIu16 " channels, %" PRIu16 " bits",
        wav_info.sample_rate, wav_info.channels, wav_info.bits_per_sample);

    // Configure codec device with file's audio format
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = wav_info.sample_rate,
        .channel = wav_info.channels,
        .bits_per_sample = wav_info.bits_per_sample,
    };
    ret = esp_codec_dev_open(dac_handle->codec_dev, &fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to open codec device");
        goto cleanup;
    }

    // Set output volume
    ret = esp_codec_dev_set_out_vol(dac_handle->codec_dev, DEFAULT_PLAY_VOL);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to set DAC volume");
        // Continue playback anyway
    }

    // Allocate playback buffer
    const size_t buffer_size = 1 * 1024;
    const size_t transfer_size = buffer_size - (buffer_size % wav_info.block_align);
    if (transfer_size == 0) {
        ESP_LOGE(TAG, "WAV block alignment exceeds playback buffer size");
        goto cleanup;
    }
    playback_buffer = malloc(buffer_size);
    if (playback_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate playback buffer");
        goto cleanup;
    }

    // Stream audio data from file to DAC
    size_t bytes_remaining = wav_info.data_size;
    while (bytes_remaining > 0) {
        size_t bytes_to_read = bytes_remaining > transfer_size ? transfer_size : bytes_remaining;
        size_t bytes_read = fread(playback_buffer, 1, bytes_to_read, fp);
        if (bytes_read != bytes_to_read) {
            ESP_LOGE(TAG, "Failed to read WAV data");
            break;
        }
        ret = esp_codec_dev_write(dac_handle->codec_dev, playback_buffer, bytes_read);
        if (ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Failed to write to DAC");
            break;
        }
        bytes_remaining -= bytes_read;
    }
    ESP_LOGI(TAG, "Play WAV file completed.");
    free(playback_buffer);
    playback_buffer = NULL;
    ret = sd_file_cache_close(&fp, &file_cache);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to close WAV file");
        goto cleanup;
    }

    // Deinitialize audio DAC device and SD card filesystem
    ret = esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize audio DAC device");
        goto cleanup;
    }
    ret = esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_FS_SDCARD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize SD card filesystem");
        goto cleanup;
    }
    return;
cleanup:
    if (playback_buffer) {
        free(playback_buffer);
    }
    if (fp) {
        sd_file_cache_close(&fp, &file_cache);
    }
}
