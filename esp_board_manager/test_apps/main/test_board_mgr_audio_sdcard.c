/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "dev_audio_codec.h"
#include "esp_board_manager.h"
#include "periph_i2s.h"
#include "bmgr_test_names.h"
#include "test_dev_audio_codec.h"

static volatile bool s_playback_finished;
static volatile bool s_recording_finished;
static volatile bool s_playback_active;
static volatile bool s_recording_active;
static volatile esp_err_t s_playback_result;
static volatile esp_err_t s_recording_result;

static const char *TAG = "BMGR_AUDIO_SDCARD";

#define AUDIO_TASK_STACK_SIZE       4096
#define AUDIO_TASK_WAIT_TIMEOUT_MS  (30 * 1000)

static esp_err_t wait_for_audio_task(volatile bool *finished, const char *task_name)
{
    const TickType_t start_tick = xTaskGetTickCount();
    const TickType_t timeout_ticks = pdMS_TO_TICKS(AUDIO_TASK_WAIT_TIMEOUT_MS);

    while (!*finished) {
        if (xTaskGetTickCount() - start_tick >= timeout_ticks) {
            ESP_LOGE(TAG, "%s task timed out after %d ms", task_name, AUDIO_TASK_WAIT_TIMEOUT_MS);
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    return ESP_OK;
}

// Task for reading WAV file and playing (SD card version)
static void wav_playback_task(void *pvParameters)
{
    const char *wav_file_path = (const char *)pvParameters;
    esp_err_t ret = ESP_FAIL;
    FILE *fp = NULL;
    dev_audio_codec_handles_t *dac_handles = NULL;
    uint8_t *playback_buffer = NULL;

    fp = fopen(wav_file_path, "rb");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open WAV file for playback: %s", wav_file_path);
        goto cleanup_playback;
    }

    // Read WAV header
    uint32_t sample_rate;
    uint16_t channels, bits_per_sample;
    ret = read_wav_header(fp, &sample_rate, &channels, &bits_per_sample);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WAV header");
        goto cleanup_playback;
    }

    ESP_LOGI(TAG, "WAV file info: %" PRIu32 " Hz, %" PRIu16 " channels, %" PRIu16 " bits", sample_rate, channels, bits_per_sample);

    // Configure DAC
    audio_config_t dac_config = {
        .sample_rate = sample_rate,
        .bits_per_sample = bits_per_sample,
        .duration_seconds = 10,
    };

    dev_audio_codec_config_t *dac_cfg = NULL;
    ret = esp_board_manager_get_device_config(BMGR_TEST_NAME_AUDIO_DAC, (void **)&dac_cfg);
    if (ret != ESP_OK || dac_cfg == NULL) {
        ESP_LOGE(TAG, "Failed to get audio_dac device config");
        ret = ret == ESP_OK ? ESP_ERR_INVALID_STATE : ret;
        goto cleanup_playback;
    }

    periph_i2s_config_t *i2s_tx_cfg = NULL;
    ret = esp_board_manager_get_periph_config(dac_cfg->i2s_cfg.name, (void **)&i2s_tx_cfg);
    if (ret != ESP_OK || i2s_tx_cfg == NULL) {
        ESP_LOGE(TAG, "Failed to get I2S TX config for %s", dac_cfg->i2s_cfg.name);
        ret = ret == ESP_OK ? ESP_ERR_INVALID_STATE : ret;
        goto cleanup_playback;
    }
    audio_config_t i2s_config = {0};
    ret = audio_config_from_i2s(i2s_tx_cfg, &i2s_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to resolve I2S playback format: %s", esp_err_to_name(ret));
        goto cleanup_playback;
    }
    dac_config.channels = i2s_config.channels;
    ret = configure_codec(BMGR_TEST_NAME_AUDIO_DAC, &dac_config, true, &dac_handles);
    if (ret != ESP_OK) {
        goto cleanup_playback;
    }

    const size_t buffer_size = 1 * 1024;
    playback_buffer = malloc(buffer_size);
    if (playback_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate playback buffer");
        ret = ESP_ERR_NO_MEM;
        goto cleanup_playback;
    }
    size_t bytes_read;
    while ((bytes_read = fread(playback_buffer, 1, buffer_size, fp)) > 0) {
        ret = esp_codec_dev_write(dac_handles->codec_dev, playback_buffer, bytes_read);
        if (ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Failed to write to DAC");
            ret = ESP_FAIL;
            break;
        }
    }

    if (ferror(fp) != 0 && ret == ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WAV data");
        ret = ESP_FAIL;
    }

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "WAV file playback completed");
    }

cleanup_playback:
    if (playback_buffer) {
        free(playback_buffer);
    }
    close_codec(dac_handles);
    if (fp) {
        fclose(fp);
    }
    s_playback_result = ret;
    s_playback_finished = true;
    s_playback_active = false;
    vTaskDelete(NULL);
}

// Task for reading I2S data and saving (SD card version)
static void i2s_recording_task(void *pvParameters)
{
    const char *output_file_path = (const char *)pvParameters;
    FILE *fp = NULL;
    dev_audio_codec_handles_t *adc_handles = NULL;
    uint8_t *recording_buffer = NULL;
    audio_config_t adc_config = {0};
    size_t total_bytes = 0;
    esp_err_t ret = ESP_FAIL;

    fp = fopen(output_file_path, "wb");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open file for recording: %s", output_file_path);
        goto cleanup_recording;
    }

    dev_audio_codec_config_t *adc_cfg = NULL;
    ret = esp_board_manager_get_device_config(BMGR_TEST_NAME_AUDIO_ADC, (void **)&adc_cfg);
    if (ret != ESP_OK || adc_cfg == NULL) {
        ESP_LOGE(TAG, "Failed to get audio_adc device config");
        ret = ret == ESP_OK ? ESP_ERR_INVALID_STATE : ret;
        goto cleanup_recording;
    }

    periph_i2s_config_t *i2s_rx_cfg = NULL;
    ret = esp_board_manager_get_periph_config(adc_cfg->i2s_cfg.name, (void **)&i2s_rx_cfg);
    if (ret != ESP_OK || i2s_rx_cfg == NULL) {
        ESP_LOGE(TAG, "Failed to get I2S RX config for %s", adc_cfg->i2s_cfg.name);
        ret = ret == ESP_OK ? ESP_ERR_INVALID_STATE : ret;
        goto cleanup_recording;
    }

    // Configure ADC
    adc_config.duration_seconds = 10;
    ret = audio_config_from_i2s(i2s_rx_cfg, &adc_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to resolve I2S recording format: %s", esp_err_to_name(ret));
        goto cleanup_recording;
    }
    ret = configure_codec(BMGR_TEST_NAME_AUDIO_ADC, &adc_config, false, &adc_handles);
    if (ret != ESP_OK) {
        goto cleanup_recording;
    }

    // Write WAV header
    ret = write_wav_header(fp, adc_config.sample_rate, adc_config.channels, adc_config.bits_per_sample, 0);
    if (ret != ESP_OK) {
        goto cleanup_recording;
    }

    const size_t buffer_size = 4096;
    recording_buffer = malloc(buffer_size);
    if (recording_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate recording buffer");
        ret = ESP_ERR_NO_MEM;
        goto cleanup_recording;
    }

    ESP_LOGI(TAG, "Starting I2S recording...");
    uint32_t record_duration_ms = adc_config.duration_seconds * 1000;
    uint32_t start_time = esp_timer_get_time() / 1000;

    while ((esp_timer_get_time() / 1000) - start_time < record_duration_ms) {
        ret = esp_codec_dev_read(adc_handles->codec_dev, recording_buffer, buffer_size);
        if (ret == ESP_CODEC_DEV_OK) {
            size_t bytes_written = fwrite(recording_buffer, 1, buffer_size, fp);
            if (bytes_written != buffer_size) {
                ESP_LOGE(TAG, "Failed to write audio data to file");
                ret = ESP_FAIL;
                break;
            }
            total_bytes += bytes_written;
        } else {
            ESP_LOGE(TAG, "Failed to read audio data from ADC");
            ret = ESP_FAIL;
            break;
        }
    }

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "I2S recording completed. Total bytes recorded: %u", (unsigned)total_bytes);
    }

cleanup_recording:
    if (recording_buffer) {
        free(recording_buffer);
    }
    close_codec(adc_handles);
    if (fp) {
        if (adc_config.sample_rate != 0 && fflush(fp) == 0 && fseek(fp, 0, SEEK_SET) == 0) {
            esp_err_t header_ret = write_wav_header(fp, adc_config.sample_rate, adc_config.channels,
                                                    adc_config.bits_per_sample, (uint32_t)total_bytes);
            if (ret == ESP_OK && header_ret != ESP_OK) {
                ret = header_ret;
            }
        } else if (ret == ESP_OK) {
            ret = ESP_FAIL;
        }
        fclose(fp);
    }
    s_recording_result = ret;
    s_recording_finished = true;
    s_recording_active = false;
    vTaskDelete(NULL);
}

esp_err_t test_board_mgr_audio_fatfs_playback(void)
{
    const char *wav_file_path = "/sdcard/test.wav";

    ESP_LOGI(TAG, "Starting audio playback only (SD card version)...");
    if (s_playback_active) {
        ESP_LOGE(TAG, "WAV playback task is already running");
        return ESP_ERR_INVALID_STATE;
    }

    s_playback_finished = false;
    s_playback_result = ESP_FAIL;
    s_playback_active = true;
    if (xTaskCreate(wav_playback_task, "wav_playback", AUDIO_TASK_STACK_SIZE,
                    (void *)wav_file_path, 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create WAV playback task");
        s_playback_active = false;
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = wait_for_audio_task(&s_playback_finished, "WAV playback");
    if (ret != ESP_OK) {
        return ret;
    }

    if (s_playback_result == ESP_OK) {
        ESP_LOGI(TAG, "Audio playback only completed, Playback from: %s", wav_file_path);
    }
    return s_playback_result;
}

esp_err_t test_board_mgr_audio_fatfs_record_playback(void)
{
    ESP_LOGI(TAG, "Starting audio record and playback (SD card version)...");

    // File paths
    const char *output_file_path = "/sdcard/recording_loopback.wav";

    ESP_LOGI(TAG, ">>>>> Step 1: Recording audio to %s...", output_file_path);
    if (s_recording_active) {
        ESP_LOGE(TAG, "I2S recording task is already running");
        return ESP_ERR_INVALID_STATE;
    }

    s_recording_finished = false;
    s_recording_result = ESP_FAIL;
    s_recording_active = true;
    if (xTaskCreate(i2s_recording_task, "i2s_recording", AUDIO_TASK_STACK_SIZE,
                    (void *)output_file_path, 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create I2S recording task");
        s_recording_active = false;
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = wait_for_audio_task(&s_recording_finished, "I2S recording");
    if (ret != ESP_OK) {
        return ret;
    }
    if (s_recording_result != ESP_OK) {
        return s_recording_result;
    }

    // Step 2: Play back the recorded audio file
    ESP_LOGI(TAG, ">>>>> Step 2: Playing back recorded audio file...");
    if (s_playback_active) {
        ESP_LOGE(TAG, "WAV playback task is already running");
        return ESP_ERR_INVALID_STATE;
    }

    s_playback_finished = false;
    s_playback_result = ESP_FAIL;
    s_playback_active = true;
    if (xTaskCreate(wav_playback_task, "wav_playback_recorded", AUDIO_TASK_STACK_SIZE,
                    (void *)output_file_path, 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create recorded WAV playback task");
        s_playback_active = false;
        return ESP_ERR_NO_MEM;
    }

    ret = wait_for_audio_task(&s_playback_finished, "recorded WAV playback");
    if (ret != ESP_OK) {
        return ret;
    }

    if (s_playback_result == ESP_OK) {
        ESP_LOGI(TAG, "Audio record and playback completed, recording saved to: %s\r\n", output_file_path);
    }
    return s_playback_result;
}
