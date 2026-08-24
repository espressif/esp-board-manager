/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_board_manager_includes.h"
#include "sd_file_cache.h"
#include "wav_header.h"

static const char *TAG = "BMGR_RECORD_TO_SDCARD";

#define DEFAULT_REC_URL           "/sdcard/test_rec.wav"
#define DEFAULT_SAMPLE_RATE       16000
#define DEFAULT_CHANNELS          1
#define DEFAULT_BITS_PER_SAMPLE   16
#define DEFAULT_DURATION_SECONDS  5
#define DEFAULT_REC_GAIN          30

void app_main(void)
{
    ESP_LOGI(TAG, "Record to %s", DEFAULT_REC_URL);
    esp_err_t ret = ESP_OK;
    dev_audio_codec_handles_t *adc_handle = NULL;
    FILE *fp = NULL;
    sd_file_cache_t file_cache = { 0 };

    // Allocate record buffer
    const size_t buffer_size = 4096;
    uint8_t *recording_buffer = malloc(buffer_size);
    if (recording_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate recording buffer");
        goto cleanup;
    }

    // Initialize audio ADC device
#if CONFIG_ESP_BOARD_LYRAT_MINI_V1_1
    // On the esp32_lyrat_mini_v1_1 development board, ES8311 (DAC) needs to be enabled to power the microphone
    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio DAC device");
        goto cleanup;
    }
#endif /* CONFIG_ESP_BOARD_LYRAT_MINI_V1_1 */
    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_ADC);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio ADC device");
        goto cleanup;
    }

    // Initialize SD card and mount it
    ret = esp_board_manager_init_device_by_name(ESP_BOARD_DEVICE_NAME_FS_SDCARD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card");
        goto cleanup;
    }

    // Get ADC device handle
    ret = esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_AUDIO_ADC, (void **)&adc_handle);
    if (ret != ESP_OK || adc_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get ADC device handle");
        goto cleanup;
    }

    // Open WAV file
    fp = fopen(DEFAULT_REC_URL, "wb");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open file %s", DEFAULT_REC_URL);
        goto cleanup;
    }
    ret = sd_file_cache_attach(fp, &file_cache, SD_FILE_CACHE_DEFAULT_SIZE,
        SD_FILE_CACHE_MIN_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Using default stdio buffer: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SD file cache enabled: %u bytes", (unsigned)file_cache.size);
    }

    // Configure codec device
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = DEFAULT_SAMPLE_RATE,
        .channel = DEFAULT_CHANNELS,
        .bits_per_sample = DEFAULT_BITS_PER_SAMPLE,
    };
    ret = esp_codec_dev_open(adc_handle->codec_dev, &fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to open codec device");
        goto cleanup;
    }

    // Set record gain
    ret = esp_codec_dev_set_in_gain(adc_handle->codec_dev, DEFAULT_REC_GAIN);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to set ADC volume");
        // Continue record anyway
    }

    // Write WAV header
    ret = write_wav_header(fp, fs.sample_rate, fs.channel, fs.bits_per_sample, DEFAULT_DURATION_SECONDS);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    ESP_LOGI(TAG, "Record WAV file info: %" PRIu32 " Hz, %" PRIu16 " channels, %" PRIu16 " bits",
        fs.sample_rate, fs.channel, fs.bits_per_sample);

    // Start recording
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Starting I2S recording...");
    uint32_t total_bytes = 0;
    const uint32_t target_bytes = DEFAULT_SAMPLE_RATE * DEFAULT_CHANNELS * (DEFAULT_BITS_PER_SAMPLE / 8) * DEFAULT_DURATION_SECONDS;
    while (total_bytes < target_bytes) {
        size_t bytes_to_read = target_bytes - total_bytes;
        if (bytes_to_read > buffer_size) {
            bytes_to_read = buffer_size;
        }
        ret = esp_codec_dev_read(adc_handle->codec_dev, recording_buffer, bytes_to_read);
        if (ret == ESP_CODEC_DEV_OK) {
            size_t bytes_written = fwrite(recording_buffer, 1, bytes_to_read, fp);
            if (bytes_written != bytes_to_read) {
                ESP_LOGE(TAG, "Failed to write audio data to file");
                break;
            }
            total_bytes += bytes_written;
        } else {
            ESP_LOGE(TAG, "Failed to read audio data from ADC");
            break;
        }
    }
    ESP_LOGI(TAG, "I2S recording completed. Total bytes recorded: %" PRIu32, total_bytes);
    ESP_LOGI(TAG, "Record example finished.");
    free(recording_buffer);
    recording_buffer = NULL;
    ret = sd_file_cache_close(&fp, &file_cache);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to close recording file");
        goto cleanup;
    }

    // Deinitialize audio ADC device and SD card filesystem
#if CONFIG_ESP_BOARD_LYRAT_MINI_V1_1
    // On the esp32_lyrat_mini_v1_1 development board, ES8311 (DAC) needs to be enabled to power the microphone
    ret = esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_DAC);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize audio DAC device");
        goto cleanup;
    }
#endif /* CONFIG_ESP_BOARD_LYRAT_MINI_V1_1 */
    ret = esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_AUDIO_ADC);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize audio ADC device");
        goto cleanup;
    }
    ret = esp_board_manager_deinit_device_by_name(ESP_BOARD_DEVICE_NAME_FS_SDCARD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize SD card filesystem");
        goto cleanup;
    }
    return;
cleanup:
    if (recording_buffer) {
        free(recording_buffer);
    }
    if (fp) {
        sd_file_cache_close(&fp, &file_cache);
    }
}
