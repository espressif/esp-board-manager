/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <inttypes.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "bmgr_test_names.h"
#include "bmgr_test_registry.h"

#ifdef CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT
#include "bmgr_test_json.h"
#include "bmgr_test_options.h"
#include "esp_log.h"
#include "test_board_mgr.h"
#include "test_dev_audio_auto.h"
#ifdef CONFIG_ESP_BOARD_DEV_GPIO_CTRL_SUPPORT
#include "test_dev_pwr_ctrl.h"
#endif  /* CONFIG_ESP_BOARD_DEV_GPIO_CTRL_SUPPORT */
#endif  /* CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)  (sizeof(array) / sizeof((array)[0]))
#endif  /* ARRAY_SIZE */

#ifdef CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT
static const char *TAG = "BMGR_CASE_AUDIO";

static esp_err_t enable_audio_power_if_supported(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_GPIO_CTRL_SUPPORT
    return test_dev_pwr_audio_ctrl(true);
#else
    return ESP_OK;
#endif  /* CONFIG_ESP_BOARD_DEV_GPIO_CTRL_SUPPORT */
}

static void emit_audio_record_summary(const test_dev_audio_auto_summary_t *summary)
{
    if (summary == NULL) {
        return;
    }

    if (!bmgr_test_options_get().summary) {
        return;
    }

    if (bmgr_test_options_get().json) {
        bmgr_test_json_metric_begin("audio.record", "mic.summary");
        bmgr_test_json_metric_u32("sample_rate", summary->sample_rate);
        bmgr_test_json_metric_u32("channels", summary->channels);
        bmgr_test_json_metric_u32("bits_per_sample", summary->bits_per_sample);
        bmgr_test_json_metric_u32("bytes", summary->bytes);
        bmgr_test_json_metric_float("rms", summary->rms);
        bmgr_test_json_metric_float("peak", summary->peak);
        bmgr_test_json_metric_float("zero_cross_hz", summary->zero_cross_hz);
        bmgr_test_json_metric_end();
        return;
    }

    printf("[SUMMARY] audio.record sample_rate=%" PRIu32
           " channels=%u bits_per_sample=%u bytes=%" PRIu32
           " rms=%.6f peak=%.6f zero_cross_hz=%.6f\n",
           summary->sample_rate,
           (unsigned)summary->channels,
           (unsigned)summary->bits_per_sample,
           summary->bytes,
           (double)summary->rms,
           (double)summary->peak,
           (double)summary->zero_cross_hz);
}

static esp_err_t run_speaker_tone_1khz(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    esp_err_t ret = enable_audio_power_if_supported();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable audio power: %s", esp_err_to_name(ret));
        return ret;
    }

    return test_dev_audio_auto_play_1khz_wav(60);
}

static esp_err_t run_audio_record(bmgr_test_context_t *ctx, int argc, char **argv)
{
    test_dev_audio_auto_summary_t summary = {0};
    esp_err_t ret = ESP_OK;

    (void)ctx;
    (void)argc;
    (void)argv;

    ret = enable_audio_power_if_supported();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable audio power: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = test_dev_audio_auto_record(3000, 16000, &summary);
    if (ret != ESP_OK) {
        return ret;
    }

    emit_audio_record_summary(&summary);
    return ESP_OK;
}

static esp_err_t run_audio_embed_playback(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    esp_err_t ret = enable_audio_power_if_supported();
    if (ret != ESP_OK) {
        return ret;
    }

    return test_board_mgr_audio_embed_playback();
}

static esp_err_t run_audio_partition_record_playback(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    esp_err_t ret = enable_audio_power_if_supported();
    if (ret != ESP_OK) {
        return ret;
    }

    return test_board_mgr_audio_partition_record_playback();
}

#ifdef CONFIG_ESP_BOARD_DEV_FS_FAT_SUPPORT
static esp_err_t run_audio_fatfs_playback(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    esp_err_t ret = enable_audio_power_if_supported();
    if (ret != ESP_OK) {
        return ret;
    }

    return test_board_mgr_audio_fatfs_playback();
}

static esp_err_t run_audio_fatfs_record_playback(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    esp_err_t ret = enable_audio_power_if_supported();
    if (ret != ESP_OK) {
        return ret;
    }

    return test_board_mgr_audio_fatfs_record_playback();
}
#endif  /* CONFIG_ESP_BOARD_DEV_FS_FAT_SUPPORT */
#endif  /* CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT */

void bmgr_register_audio_cases(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT
    const bmgr_test_case_t audio_cases[] = {
        {
            .name = "speaker.tone.1khz",
            .group = "audio",
            .help = "Play the embedded 1 kHz WAV automation tone",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_AUDIO_DAC),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_AUDIO_POWER),
            },
            .run = run_speaker_tone_1khz,
            .flags = BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_AUDIO,
        },
        {
            .name = "audio.record",
            .group = "audio",
            .help = "Record about 3 seconds of audio and emit optional summary metrics",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_AUDIO_ADC),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_AUDIO_POWER),
            },
            .run = run_audio_record,
            .flags = BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_AUDIO,
        },
        {
            .name = "audio.playback.embed",
            .group = "audio",
            .help = "Run embedded audio playback manual test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_AUDIO_DAC),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_AUDIO_POWER),
            },
            .run = run_audio_embed_playback,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_AUDIO,
        },
        {
            .name = "audio.record_playback.partition",
            .group = "audio",
            .help = "Record audio to a partition and play it back",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_AUDIO_DAC),
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_AUDIO_ADC),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_AUDIO_POWER),
            },
            .run = run_audio_partition_record_playback,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_AUDIO,
        },
#ifdef CONFIG_ESP_BOARD_DEV_FS_FAT_SUPPORT
        {
            .name = "audio.playback.fatfs",
            .group = "audio",
            .help = "Run FATFS audio playback manual test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_AUDIO_DAC),
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_FS_FAT),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_AUDIO_POWER),
            },
            .run = run_audio_fatfs_playback,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_AUDIO,
        },
        {
            .name = "audio.record_playback.fatfs",
            .group = "audio",
            .help = "Record audio to FATFS and play it back",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_AUDIO_DAC),
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_AUDIO_ADC),
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_FS_FAT),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_AUDIO_POWER),
            },
            .run = run_audio_fatfs_record_playback,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_AUDIO,
        },
#endif  /* CONFIG_ESP_BOARD_DEV_FS_FAT_SUPPORT */
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(audio_cases, ARRAY_SIZE(audio_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT */
}
