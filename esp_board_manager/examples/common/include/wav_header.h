/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include <stdio.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    uint32_t  sample_rate;
    uint32_t  data_size;
    uint16_t  channels;
    uint16_t  bits_per_sample;
    uint16_t  block_align;
} wav_info_t;

/**
 * @brief  Read a PCM WAV stream and locate its data chunk.
 *
 *         This function scans RIFF chunks, allowing metadata and format-extension
 *         chunks before the audio data. Only PCM and WAVE_FORMAT_EXTENSIBLE PCM
 *         streams are supported. On success, @p fp points to the first data byte.
 *
 * @param[in]   fp    Pointer to an opened binary read stream.
 * @param[out]  info  Parsed stream information.
 *
 * @return
 *       - ESP_OK               Successfully read and parsed the WAV header.
 *       - ESP_ERR_INVALID_ARG  Invalid argument.
 *       - ESP_FAIL             File read error or unsupported/invalid WAV format.
 */
esp_err_t read_wav_info(FILE *fp, wav_info_t *info);

/**
 * @brief  Write a standard 44-byte PCM WAV header to a file.
 *
 * @param[in]  fp                Pointer to an opened binary write stream.
 * @param[in]  sample_rate       Sample rate in Hz.
 * @param[in]  channels          Number of audio channels.
 * @param[in]  bits_per_sample   Bits per sample.
 * @param[in]  duration_seconds  Duration used to calculate the data size.
 *
 * @return
 *       - ESP_OK    Header written successfully.
 *       - ESP_FAIL  File write error.
 */
esp_err_t write_wav_header(FILE *fp, uint32_t sample_rate, uint16_t channels,
    uint16_t bits_per_sample, uint32_t duration_seconds);

#ifdef __cplusplus
}
#endif /* __cplusplus */
