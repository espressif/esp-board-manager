/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <math.h>
#include <stdio.h>
#include "esp_clk_tree.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "dev_audio_codec.h"
#include "esp_board_manager.h"
#include "periph_i2s.h"
#include "bmgr_test_names.h"
#include "test_dev_audio_codec.h"

extern const uint8_t test_wav_start[] asm("_binary_test_wav_start");
extern const uint8_t test_wav_end[] asm("_binary_test_wav_end");

static const char *TAG = "BMGR_AUDIO_EMBED";

// Partition-based recording variables
static const esp_partition_t *record_partition = NULL;
static size_t record_partition_offset = 0;
static size_t record_partition_used = 0;

// Store recording configuration for consistent playback
static audio_config_t recorded_audio_config = {0};

// Clean up recording resources
static void cleanup_recording_resources(void)
{
    if (record_partition) {
        // Erase the partition to clean up
        esp_partition_erase_range(record_partition, 0, record_partition->size);
        record_partition = NULL;
        record_partition_offset = 0;
        record_partition_used = 0;
    }
    memset(&recorded_audio_config, 0, sizeof(recorded_audio_config));
}

// Task for playing embedded WAV file
static esp_err_t embedded_wav_playback(void)
{
    ESP_LOGI(TAG, "Starting embedded WAV file playback...");
    dev_audio_codec_handles_t *dac_handles = NULL;
    uint8_t *playback_buffer = NULL;
    esp_err_t ret = ESP_OK;

    // Calculate embedded file size， -1 make the size is correctly
    size_t embedded_file_size = test_wav_end - test_wav_start - 1;
    ESP_LOGI(TAG, "Embedded WAV file size: %d bytes", embedded_file_size);

    // Create a temporary file pointer to read the embedded data
    // We'll use a memory stream approach
    const uint8_t *current_pos = test_wav_start;

    // Read WAV header from embedded data
    if (embedded_file_size < 44) {
        ESP_LOGE(TAG, "Embedded file too small to contain WAV header");
        ret = ESP_ERR_INVALID_SIZE;
        goto cleanup_embedded_playback;
    }

    // Parse WAV header manually
    uint32_t sample_rate = *(uint32_t *)(current_pos + 24);
    uint16_t channels = *(uint16_t *)(current_pos + 22);
    uint16_t bits_per_sample = *(uint16_t *)(current_pos + 34);

    ESP_LOGI(TAG, "WAV file info: %" PRIu32 " Hz, %d channels, %d bits", sample_rate, channels, bits_per_sample);

    // Configure DAC
    audio_config_t dac_config = {
        .sample_rate = sample_rate,
        .channels = channels,
        .bits_per_sample = bits_per_sample,
        .duration_seconds = 10,
    };

    ret = configure_codec(BMGR_TEST_NAME_AUDIO_DAC, &dac_config, true, &dac_handles);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure DAC");
        goto cleanup_embedded_playback;
    }

    // Skip WAV header (44 bytes) and play audio data
    current_pos += 44;
    size_t audio_data_size = embedded_file_size - 44;

    const size_t buffer_size = 5 * 1024;
    playback_buffer = malloc(buffer_size);
    if (playback_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate playback buffer");
        ret = ESP_ERR_NO_MEM;
        goto cleanup_embedded_playback;
    }
    size_t remaining_data = audio_data_size;

    while (remaining_data > 0) {
        size_t bytes_to_write = (remaining_data > buffer_size) ? buffer_size : remaining_data;
        memcpy(playback_buffer, current_pos, bytes_to_write);

        ret = esp_codec_dev_write(dac_handles->codec_dev, playback_buffer, bytes_to_write);
        if (ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Failed to write to DAC");
            break;
        }

        current_pos += bytes_to_write;
        remaining_data -= bytes_to_write;
    }

    ESP_LOGI(TAG, "Embedded WAV file playback completed");
    free(playback_buffer);
    playback_buffer = NULL;
    close_codec(dac_handles);
    return ret;

cleanup_embedded_playback:
    if (playback_buffer) {
        free(playback_buffer);
    }
    close_codec(dac_handles);
    return ret;
}

// Task for recording audio to partition
static esp_err_t partition_recording_task(void)
{
    ESP_LOGI(TAG, "Starting partition-based recording...");
    dev_audio_codec_handles_t *adc_handles = NULL;
    uint8_t *recording_buffer = NULL;
    esp_err_t ret = ESP_FAIL;

    // Find the record partition
    record_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "record");
    if (!record_partition) {
        ESP_LOGE(TAG, "Failed to find record partition");
        goto cleanup_partition_recording;
    }

    ESP_LOGI(TAG, "Found record partition: size=%" PRIu32 " bytes", record_partition->size);

    // Erase the partition before recording
    ret = esp_partition_erase_range(record_partition, 0, record_partition->size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase record partition");
        goto cleanup_partition_recording;
    }

    record_partition_offset = 0;
    record_partition_used = 0;

    dev_audio_codec_config_t *adc_cfg = NULL;
    ret = esp_board_manager_get_device_config(BMGR_TEST_NAME_AUDIO_ADC, (void **)&adc_cfg);
    if (ret != ESP_OK || adc_cfg == NULL) {
        ESP_LOGE(TAG, "Failed to get audio_adc device config");
        ret = ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
        goto cleanup_partition_recording;
    }

    periph_i2s_config_t *i2s_rx_cfg = NULL;
#ifdef CONFIG_ESP_BOARD_PERIPH_I2S_SUPPORT
    if (adc_cfg->i2s_cfg.name != NULL) {
        ret = esp_board_manager_get_periph_config(adc_cfg->i2s_cfg.name, (void **)&i2s_rx_cfg);
        if (ret != ESP_OK || i2s_rx_cfg == NULL) {
            ESP_LOGE(TAG, "Failed to get I2S RX config for %s", adc_cfg->i2s_cfg.name);
            ret = ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
            goto cleanup_partition_recording;
        }
    }
#endif  /* CONFIG_ESP_BOARD_PERIPH_I2S_SUPPORT */

    // Configure ADC for recording
    audio_config_t adc_config = {.duration_seconds = 3};
    if (i2s_rx_cfg != NULL) {
        ret = audio_config_from_i2s(i2s_rx_cfg, &adc_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to resolve I2S recording format: %s", esp_err_to_name(ret));
            goto cleanup_partition_recording;
        }
    } else {
        ESP_LOGI(TAG, "No I2S RX config for audio ADC, using 16 kHz mono defaults");
        adc_config.sample_rate = 16000;
        adc_config.channels = 1;
        adc_config.bits_per_sample = 16;
    }

    // Save configuration for playback
    recorded_audio_config = adc_config;
    ESP_LOGI(TAG, "Recording config: %" PRIu32 " Hz, %d channels, %d bits",
             adc_config.sample_rate, adc_config.channels, adc_config.bits_per_sample);

    ret = configure_codec(BMGR_TEST_NAME_AUDIO_ADC, &adc_config, false, &adc_handles);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC");
        goto cleanup_partition_recording;
    }

    const size_t buffer_size = 4096;
    recording_buffer = malloc(buffer_size);
    // recording_buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (recording_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate recording buffer");
        ret = ESP_ERR_NO_MEM;
        goto cleanup_partition_recording;
    }

    ESP_LOGI(TAG, "Recording %u seconds of audio to partition...", (unsigned)adc_config.duration_seconds);
    uint32_t record_duration_ms = adc_config.duration_seconds * 1000;
    uint32_t start_time = esp_timer_get_time() / 1000;
    record_partition_used = 0;

    while ((esp_timer_get_time() / 1000) - start_time < record_duration_ms) {
        ret = esp_codec_dev_read(adc_handles->codec_dev, recording_buffer, buffer_size);
        if (ret == ESP_CODEC_DEV_OK) {
            if (record_partition_offset + record_partition_used + buffer_size <= record_partition->size) {
                ret = esp_partition_write(record_partition, record_partition_offset + record_partition_used, recording_buffer, buffer_size);
                if (ret == ESP_OK) {
                    record_partition_used += buffer_size;
                } else {
                    ESP_LOGE(TAG, "Failed to write to partition");
                    break;
                }
            } else {
                ESP_LOGW(TAG, "Partition full, stopping recording");
                break;
            }
        } else {
            ESP_LOGE(TAG, "Failed to read audio data from ADC");
            ret = ESP_FAIL;
            break;
        }
    }

    ESP_LOGI(TAG, "Partition recording completed. Recorded %d bytes", record_partition_used);
    free(recording_buffer);
    recording_buffer = NULL;
    close_codec(adc_handles);
    return ret;

cleanup_partition_recording:
    if (recording_buffer) {
        free(recording_buffer);
    }
    close_codec(adc_handles);
    return ret;
}

// Task for playing back recorded audio from memory
static esp_err_t play_recorded_audio_task(void)
{
    ESP_LOGI(TAG, "Playing back recorded audio from partition...");
    dev_audio_codec_handles_t *dac_handles = NULL;
    uint8_t *playback_buffer = NULL;
    esp_err_t ret = ESP_OK;
    if (record_partition == NULL || record_partition_used == 0) {
        ESP_LOGE(TAG, "No recorded audio data available");
        ret = ESP_ERR_INVALID_STATE;
        goto cleanup_play_recorded;
    }

    // Get the same I2S configuration used for recording to ensure consistency
    dev_audio_codec_config_t *dac_cfg = NULL;
    ret = esp_board_manager_get_device_config(BMGR_TEST_NAME_AUDIO_DAC, (void **)&dac_cfg);
    if (ret != ESP_OK || dac_cfg == NULL) {
        ESP_LOGE(TAG, "Failed to get audio_dac device config");
        ret = ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
        goto cleanup_play_recorded;
    }

    // Configure DAC for playback of recorded audio - use same config as recording
    audio_config_t dac_config = {
        .sample_rate = recorded_audio_config.sample_rate,
        .channels = recorded_audio_config.channels,
        .bits_per_sample = recorded_audio_config.bits_per_sample,
        .duration_seconds = recorded_audio_config.duration_seconds,
    };

    ret = configure_codec(BMGR_TEST_NAME_AUDIO_DAC, &dac_config, true, &dac_handles);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure DAC for recorded audio playback");
        goto cleanup_play_recorded;
    }

    const size_t buffer_size = 4096;
    playback_buffer = malloc(buffer_size);
    if (playback_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate playback buffer");
        ret = ESP_ERR_NO_MEM;
        goto cleanup_play_recorded;
    }

    ESP_LOGI(TAG, "Playing back %d bytes of recorded audio...", record_partition_used);
    size_t remaining_data = record_partition_used;
    size_t read_offset = 0;

    while (remaining_data > 0) {
        size_t bytes_to_read = (remaining_data > buffer_size) ? buffer_size : remaining_data;

        // Read data from partition
        ret = esp_partition_read(record_partition, record_partition_offset + read_offset, playback_buffer, bytes_to_read);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read from partition");
            ret = ESP_FAIL;
            break;
        }

        ret = esp_codec_dev_write(dac_handles->codec_dev, playback_buffer, bytes_to_read);
        if (ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Failed to write recorded audio to DAC");
            ret = ESP_FAIL;
            break;
        }

        read_offset += bytes_to_read;
        remaining_data -= bytes_to_read;
    }

    ESP_LOGI(TAG, "Recorded audio playback completed");
    free(playback_buffer);
    playback_buffer = NULL;
    close_codec(dac_handles);
    return ret;

cleanup_play_recorded:
    if (playback_buffer) {
        free(playback_buffer);
    }
    close_codec(dac_handles);
    return ret;
}

esp_err_t test_board_mgr_audio_embed_playback(void)
{
    ESP_LOGI(TAG, "Starting audio playback only test (using embedded file)...");
    esp_err_t ret = embedded_wav_playback();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Audio playback only test completed");
    }
    return ret;
}

static esp_err_t audio_partition_record_playback(void)
{
    ESP_LOGI(TAG, "Starting partition audio record and playback test...");

    // Clean up any previous recording state
    cleanup_recording_resources();

    ESP_LOGI(TAG, "Step 1: Recording audio to partition...");
    esp_err_t ret = partition_recording_task();
    if (ret != ESP_OK) {
        goto cleanup;
    }

    ESP_LOGI(TAG, "Step 2: Playing back recorded audio...");
    ret = play_recorded_audio_task();
    if (ret != ESP_OK) {
        goto cleanup;
    }

cleanup:
    cleanup_recording_resources();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Partition audio record and playback test completed");
    }
    return ret;
}

// Main function that combines both steps
esp_err_t test_board_mgr_audio_partition_record_playback(void)
{
    return audio_partition_record_playback();
}
