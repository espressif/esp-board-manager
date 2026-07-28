/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"
#include "dev_audio_codec.h"
#include "esp_board_manager.h"
#include "periph_i2s.h"
#include "bmgr_test_names.h"
#include "test_dev_audio_auto.h"
#include "test_dev_audio_codec.h"

#define AUDIO_AUTO_WAV_HEADER_MIN_BYTES  44
#define AUDIO_AUTO_PLAYBACK_CHUNK_BYTES  2048
#define AUDIO_AUTO_RECORD_CHUNK_BYTES    2048
#define AUDIO_AUTO_DEFAULT_SAMPLE_RATE   16000
#define AUDIO_AUTO_DEFAULT_CHANNELS      1
#define AUDIO_AUTO_BITS_PER_SAMPLE       16

extern const uint8_t _binary_16k_16bit_1ch_wav_start[] asm("_binary_16k_16bit_1ch_wav_start");
extern const uint8_t _binary_16k_16bit_1ch_wav_end[] asm("_binary_16k_16bit_1ch_wav_end");

typedef struct {
    uint32_t       sample_rate;
    uint16_t       channels;
    uint16_t       bits_per_sample;
    const uint8_t *data;
    size_t         data_bytes;
} test_dev_audio_auto_wav_info_t;

static const char *TAG = "TEST_DEV_AUDIO_AUTO";
static test_dev_audio_auto_summary_t s_last_summary;

static uint16_t audio_auto_read_le16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t audio_auto_read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static esp_err_t audio_auto_parse_embedded_wav(test_dev_audio_auto_wav_info_t *info)
{
    size_t embedded_size = 0;
    size_t offset = 12;
    bool found_fmt = false;
    bool found_data = false;
    const uint8_t *wav_data = _binary_16k_16bit_1ch_wav_start;

    if (info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(info, 0, sizeof(*info));

    embedded_size = (size_t)(_binary_16k_16bit_1ch_wav_end - _binary_16k_16bit_1ch_wav_start);
    if (embedded_size == 0) {
        return ESP_ERR_INVALID_SIZE;
    }

    /* CMake EMBED_TXTFILES appends a trailing NUL byte. */
    embedded_size--;
    if (embedded_size < AUDIO_AUTO_WAV_HEADER_MIN_BYTES) {
        ESP_LOGE(TAG, "Embedded WAV file is too small: %u bytes", (unsigned)embedded_size);
        return ESP_ERR_INVALID_SIZE;
    }

    if (memcmp(wav_data, "RIFF", 4) != 0 || memcmp(wav_data + 8, "WAVE", 4) != 0) {
        ESP_LOGE(TAG, "Embedded file is not a RIFF/WAVE file");
        return ESP_ERR_INVALID_RESPONSE;
    }

    while (offset + 8 <= embedded_size) {
        const uint8_t *chunk = wav_data + offset;
        uint32_t chunk_size = audio_auto_read_le32(chunk + 4);
        size_t chunk_data_offset = offset + 8;
        size_t next_offset = chunk_data_offset + chunk_size + (chunk_size & 0x1U);

        if (next_offset > embedded_size) {
            ESP_LOGE(TAG, "Invalid WAV chunk size at offset %u", (unsigned)offset);
            return ESP_ERR_INVALID_RESPONSE;
        }

        if (memcmp(chunk, "fmt ", 4) == 0) {
            if (chunk_size < 16) {
                return ESP_ERR_INVALID_RESPONSE;
            }
            if (audio_auto_read_le16(chunk + 8) != 1) {
                ESP_LOGE(TAG, "Only PCM WAV files are supported");
                return ESP_ERR_NOT_SUPPORTED;
            }
            info->channels = audio_auto_read_le16(chunk + 10);
            info->sample_rate = audio_auto_read_le32(chunk + 12);
            info->bits_per_sample = audio_auto_read_le16(chunk + 22);
            found_fmt = true;
        } else if (memcmp(chunk, "data", 4) == 0) {
            info->data = wav_data + chunk_data_offset;
            info->data_bytes = chunk_size;
            found_data = true;
        }

        if (found_fmt && found_data) {
            break;
        }
        offset = next_offset;
    }

    if (!found_fmt || !found_data) {
        ESP_LOGE(TAG, "Failed to locate WAV fmt/data chunks");
        return ESP_ERR_NOT_FOUND;
    }
    if (info->channels == 0 || info->bits_per_sample != AUDIO_AUTO_BITS_PER_SAMPLE) {
        ESP_LOGE(TAG, "Unsupported WAV format: channels=%u bits=%u",
                 (unsigned)info->channels, (unsigned)info->bits_per_sample);
        return ESP_ERR_NOT_SUPPORTED;
    }
    return ESP_OK;
}

static esp_err_t audio_auto_resolve_record_config(uint32_t requested_sample_rate, audio_config_t *config)
{
    dev_audio_codec_config_t *adc_cfg = NULL;
    periph_i2s_config_t *i2s_rx_cfg = NULL;
    esp_err_t ret = ESP_OK;

    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(config, 0, sizeof(*config));

    ret = esp_board_manager_get_device_config(BMGR_TEST_NAME_AUDIO_ADC, (void **)&adc_cfg);
    if (ret != ESP_OK || adc_cfg == NULL) {
        ESP_LOGE(TAG, "Failed to get %s config: %s", BMGR_TEST_NAME_AUDIO_ADC, esp_err_to_name(ret));
        return ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
    }

#ifdef CONFIG_ESP_BOARD_PERIPH_I2S_SUPPORT
    if (adc_cfg->i2s_cfg.name != NULL) {
        ret = esp_board_manager_get_periph_config(adc_cfg->i2s_cfg.name, (void **)&i2s_rx_cfg);
        if (ret == ESP_OK && i2s_rx_cfg != NULL) {
            ret = audio_config_from_i2s(i2s_rx_cfg, config);
            if (ret != ESP_OK) {
                return ret;
            }
        } else {
            ESP_LOGW(TAG, "Failed to get I2S RX config for %s, using defaults: %s",
                     adc_cfg->i2s_cfg.name, esp_err_to_name(ret));
        }
    }
#endif  /* CONFIG_ESP_BOARD_PERIPH_I2S_SUPPORT */

    if (config->sample_rate == 0) {
        config->sample_rate = AUDIO_AUTO_DEFAULT_SAMPLE_RATE;
        config->channels = AUDIO_AUTO_DEFAULT_CHANNELS;
        config->bits_per_sample = AUDIO_AUTO_BITS_PER_SAMPLE;
    }
    if (requested_sample_rate != 0) {
        config->sample_rate = requested_sample_rate;
    }
    config->duration_seconds = 0;
    return ESP_OK;
}

const test_dev_audio_auto_summary_t *test_dev_audio_auto_get_last_summary(void)
{
    return &s_last_summary;
}

esp_err_t test_dev_audio_auto_play_1khz_wav(uint8_t volume)
{
    test_dev_audio_auto_wav_info_t wav_info = {0};
    dev_audio_codec_handles_t *dac_handles = NULL;
    uint8_t *playback_buffer = NULL;
    esp_err_t ret = ESP_OK;
    size_t remaining_bytes = 0;
    const uint8_t *read_ptr = NULL;

    ret = audio_auto_parse_embedded_wav(&wav_info);
    if (ret != ESP_OK) {
        return ret;
    }

    audio_config_t dac_config = {
        .sample_rate = wav_info.sample_rate,
        .channels = wav_info.channels,
        .bits_per_sample = wav_info.bits_per_sample,
        .duration_seconds = 0,
    };

    ret = configure_codec(BMGR_TEST_NAME_AUDIO_DAC, &dac_config, true, &dac_handles);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure %s: %s", BMGR_TEST_NAME_AUDIO_DAC, esp_err_to_name(ret));
        return ret;
    }

    if (dac_handles->codec_dev != NULL) {
        int codec_ret = esp_codec_dev_set_out_vol(dac_handles->codec_dev, volume);
        if (codec_ret != ESP_CODEC_DEV_OK) {
            ESP_LOGW(TAG, "Failed to set output volume to %u, continuing", (unsigned)volume);
        }
    }

    playback_buffer = malloc(AUDIO_AUTO_PLAYBACK_CHUNK_BYTES);
    if (playback_buffer == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    read_ptr = wav_info.data;
    remaining_bytes = wav_info.data_bytes;
    while (remaining_bytes > 0) {
        size_t bytes_to_write = remaining_bytes > AUDIO_AUTO_PLAYBACK_CHUNK_BYTES ?
                                AUDIO_AUTO_PLAYBACK_CHUNK_BYTES : remaining_bytes;
        int codec_ret;

        memcpy(playback_buffer, read_ptr, bytes_to_write);
        codec_ret = esp_codec_dev_write(dac_handles->codec_dev, playback_buffer, bytes_to_write);
        if (codec_ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Failed to write audio playback chunk");
            ret = ESP_FAIL;
            goto cleanup;
        }

        read_ptr += bytes_to_write;
        remaining_bytes -= bytes_to_write;
    }

cleanup:
    if (playback_buffer != NULL) {
        free(playback_buffer);
    }
    if (dac_handles != NULL) {
        esp_err_t close_ret = close_codec(dac_handles);
        if (ret == ESP_OK && close_ret != ESP_OK) {
            ret = close_ret;
        }
    }
    return ret;
}

esp_err_t test_dev_audio_auto_record(uint32_t duration_ms, uint32_t sample_rate, test_dev_audio_auto_summary_t *summary)
{
    audio_config_t adc_config = {0};
    dev_audio_codec_handles_t *adc_handles = NULL;
    uint8_t *recording_buffer = NULL;
    uint64_t sum_squares = 0;
    uint64_t total_sample_count = 0;
    uint64_t analyzed_frames = 0;
    uint32_t peak_abs = 0;
    uint32_t total_bytes = 0;
    uint32_t zero_crossings = 0;
    int64_t start_time_us = 0;
    int16_t previous_first_channel = 0;
    bool have_previous_first_channel = false;
    esp_err_t ret = ESP_OK;

    memset(&s_last_summary, 0, sizeof(s_last_summary));

    if (duration_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    ret = audio_auto_resolve_record_config(sample_rate, &adc_config);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = configure_codec(BMGR_TEST_NAME_AUDIO_ADC, &adc_config, false, &adc_handles);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure %s: %s", BMGR_TEST_NAME_AUDIO_ADC, esp_err_to_name(ret));
        return ret;
    }

    recording_buffer = malloc(AUDIO_AUTO_RECORD_CHUNK_BYTES);
    if (recording_buffer == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    start_time_us = esp_timer_get_time();
    while ((uint32_t)((esp_timer_get_time() - start_time_us) / 1000) < duration_ms) {
        size_t sample_count = 0;
        size_t frame_count = 0;
        int codec_ret = esp_codec_dev_read(adc_handles->codec_dev, recording_buffer, AUDIO_AUTO_RECORD_CHUNK_BYTES);
        const int16_t *samples = NULL;

        if (codec_ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Failed to read audio capture chunk");
            ret = ESP_FAIL;
            goto cleanup;
        }

        total_bytes += AUDIO_AUTO_RECORD_CHUNK_BYTES;
        samples = (const int16_t *)recording_buffer;
        sample_count = AUDIO_AUTO_RECORD_CHUNK_BYTES / sizeof(int16_t);
        frame_count = AUDIO_AUTO_RECORD_CHUNK_BYTES / (sizeof(int16_t) * adc_config.channels);

        for (size_t i = 0; i < sample_count; i++) {
            int32_t sample_value = samples[i];
            uint32_t magnitude = sample_value < 0 ? (uint32_t)(-sample_value) : (uint32_t)sample_value;

            if (magnitude > peak_abs) {
                peak_abs = magnitude;
            }
            sum_squares += (uint64_t)((int64_t)sample_value * (int64_t)sample_value);
        }
        total_sample_count += sample_count;

        for (size_t frame = 0; frame < frame_count; frame++) {
            int16_t current_first_channel = samples[frame * adc_config.channels];

            if (have_previous_first_channel) {
                if ((previous_first_channel < 0 && current_first_channel >= 0) ||
                    (previous_first_channel > 0 && current_first_channel <= 0)) {
                    zero_crossings++;
                }
            } else {
                have_previous_first_channel = true;
            }

            previous_first_channel = current_first_channel;
        }
        analyzed_frames += frame_count;
    }

    s_last_summary.sample_rate = adc_config.sample_rate;
    s_last_summary.channels = adc_config.channels;
    s_last_summary.bits_per_sample = adc_config.bits_per_sample;
    s_last_summary.bytes = total_bytes;
    if (total_sample_count > 0) {
        s_last_summary.rms = (float)(sqrt((double)sum_squares / (double)total_sample_count) / 32768.0);
        s_last_summary.peak = (float)((double)peak_abs / 32768.0);
    }
    if (adc_config.sample_rate > 0 && analyzed_frames > 0) {
        double duration_seconds = (double)analyzed_frames / (double)adc_config.sample_rate;
        if (duration_seconds > 0.0) {
            s_last_summary.zero_cross_hz = (float)((double)zero_crossings / (2.0 * duration_seconds));
        }
    }

    if (summary != NULL) {
        *summary = s_last_summary;
    }

cleanup:
    if (recording_buffer != NULL) {
        free(recording_buffer);
    }
    if (adc_handles != NULL) {
        esp_err_t close_ret = close_codec(adc_handles);
        if (ret == ESP_OK && close_ret != ESP_OK) {
            ret = close_ret;
        }
    }
    return ret;
}
