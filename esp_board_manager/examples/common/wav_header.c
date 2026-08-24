#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "wav_header.h"

static const char *TAG = "WAV_HEADER";

#define WAV_FORMAT_PCM         0x0001U
#define WAV_FORMAT_EXTENSIBLE  0xfffeU

#pragma pack(push, 1)
typedef struct {
    char  riff[4];
    uint32_t  file_size;
    char  wave[4];
} wav_riff_header_t;

typedef struct {
    char  id[4];
    uint32_t  size;
} wav_chunk_header_t;

typedef struct {
    uint16_t  audio_format;
    uint16_t  channels;
    uint32_t  sample_rate;
    uint32_t  byte_rate;
    uint16_t  block_align;
    uint16_t  bits_per_sample;
} wav_fmt_payload_t;

typedef struct {
    uint16_t  extension_size;
    uint16_t  valid_bits_per_sample;
    uint32_t  channel_mask;
    uint8_t  sub_format[16];
} wav_extensible_payload_t;

typedef struct {
    char  riff[4];
    uint32_t  file_size;
    char  wave[4];
    char  fmt[4];
    uint32_t  fmt_size;
    uint16_t  audio_format;
    uint16_t  num_channels;
    uint32_t  sample_rate;
    uint32_t  byte_rate;
    uint16_t  block_align;
    uint16_t  bits_per_sample;
    char  data[4];
    uint32_t  data_size;
} wav_header_t;
#pragma pack(pop)

static bool read_exact(FILE *fp, void *buffer, size_t size)
{
    return fread(buffer, 1, size, fp) == size;
}

static bool skip_exact(FILE *fp, uint32_t size)
{
    uint8_t discard[64];

    while (size > 0) {
        size_t read_size = size > sizeof(discard) ? sizeof(discard) : size;
        if (!read_exact(fp, discard, read_size)) {
            return false;
        }
        size -= read_size;
    }
    return true;
}

static bool is_pcm_subformat(const uint8_t sub_format[16])
{
    static const uint8_t pcm_subformat[16] = {
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x10,
        0x00,
        0x80,
        0x00,
        0x00,
        0xaa,
        0x00,
        0x38,
        0x9b,
        0x71,
    };

    return memcmp(sub_format, pcm_subformat, sizeof(pcm_subformat)) == 0;
}

static bool is_valid_pcm_format(const wav_fmt_payload_t *fmt)
{
    uint32_t expected_block_align;
    uint64_t expected_byte_rate;

    if (fmt->channels == 0 || fmt->sample_rate == 0 ||
        fmt->bits_per_sample == 0 || (fmt->bits_per_sample % 8) != 0) {
        return false;
    }

    expected_block_align = fmt->channels * (fmt->bits_per_sample / 8);
    expected_byte_rate = (uint64_t)fmt->sample_rate * expected_block_align;
    return expected_block_align <= UINT16_MAX &&
        fmt->block_align == expected_block_align &&
        expected_byte_rate <= UINT32_MAX &&
        fmt->byte_rate == expected_byte_rate;
}

esp_err_t read_wav_info(FILE *fp, wav_info_t *info)
{
    wav_riff_header_t riff_header;
    wav_fmt_payload_t fmt_payload;
    bool fmt_found = false;
    uint32_t remaining;

    if (fp == NULL || info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!read_exact(fp, &riff_header, sizeof(riff_header))) {
        ESP_LOGE(TAG, "Failed to read RIFF header");
        return ESP_FAIL;
    }
    if (memcmp(riff_header.riff, "RIFF", 4) != 0 ||
        memcmp(riff_header.wave, "WAVE", 4) != 0 ||
        riff_header.file_size < sizeof(riff_header.wave)) {
        ESP_LOGE(TAG, "Invalid RIFF/WAVE header");
        return ESP_FAIL;
    }

    remaining = riff_header.file_size - sizeof(riff_header.wave);
    while (remaining >= sizeof(wav_chunk_header_t)) {
        wav_chunk_header_t chunk_header;
        uint32_t padding;

        if (!read_exact(fp, &chunk_header, sizeof(chunk_header))) {
            ESP_LOGE(TAG, "Failed to read WAV chunk header");
            return ESP_FAIL;
        }
        remaining -= sizeof(chunk_header);
        padding = chunk_header.size & 1U;
        if (chunk_header.size > remaining || padding > remaining - chunk_header.size) {
            ESP_LOGE(TAG, "WAV chunk exceeds RIFF bounds");
            return ESP_FAIL;
        }

        if (memcmp(chunk_header.id, "fmt ", 4) == 0 && !fmt_found) {
            uint32_t extension_size;

            if (chunk_header.size < sizeof(fmt_payload) ||
                !read_exact(fp, &fmt_payload, sizeof(fmt_payload))) {
                ESP_LOGE(TAG, "Invalid WAV fmt chunk");
                return ESP_FAIL;
            }
            extension_size = chunk_header.size - sizeof(fmt_payload);
            if (fmt_payload.audio_format == WAV_FORMAT_EXTENSIBLE) {
                wav_extensible_payload_t extensible_payload;

                if (extension_size < sizeof(extensible_payload) ||
                    !read_exact(fp, &extensible_payload, sizeof(extensible_payload)) ||
                    extensible_payload.extension_size < 22 ||
                    !is_pcm_subformat(extensible_payload.sub_format)) {
                    ESP_LOGE(TAG, "Unsupported WAV extensible format");
                    return ESP_FAIL;
                }
                extension_size -= sizeof(extensible_payload);
            } else if (fmt_payload.audio_format != WAV_FORMAT_PCM) {
                ESP_LOGE(TAG, "Unsupported WAV format: %u", fmt_payload.audio_format);
                return ESP_FAIL;
            }
            if (!is_valid_pcm_format(&fmt_payload) || !skip_exact(fp, extension_size)) {
                ESP_LOGE(TAG, "Invalid WAV fmt data");
                return ESP_FAIL;
            }
            fmt_found = true;
        } else if (memcmp(chunk_header.id, "data", 4) == 0) {
            if (!fmt_found || fmt_payload.block_align == 0 ||
                (chunk_header.size % fmt_payload.block_align) != 0) {
                ESP_LOGE(TAG, "Invalid WAV data chunk");
                return ESP_FAIL;
            }
            info->sample_rate = fmt_payload.sample_rate;
            info->channels = fmt_payload.channels;
            info->bits_per_sample = fmt_payload.bits_per_sample;
            info->block_align = fmt_payload.block_align;
            info->data_size = chunk_header.size;
            ESP_LOGI(TAG, "WAV file: %" PRIu32 " Hz, %" PRIu16 " channels, %" PRIu16 " bits",
                info->sample_rate, info->channels, info->bits_per_sample);
            return ESP_OK;
        } else if (!skip_exact(fp, chunk_header.size)) {
            ESP_LOGE(TAG, "Failed to skip WAV chunk");
            return ESP_FAIL;
        }

        if (padding != 0 && !skip_exact(fp, padding)) {
            ESP_LOGE(TAG, "Failed to skip WAV chunk padding");
            return ESP_FAIL;
        }
        remaining -= chunk_header.size + padding;
    }

    ESP_LOGE(TAG, "WAV data chunk not found");
    return ESP_FAIL;
}

esp_err_t write_wav_header(FILE *fp, uint32_t sample_rate, uint16_t channels,
    uint16_t bits_per_sample, uint32_t duration_seconds)
{
    wav_header_t header = { 0 };
    uint32_t data_size;
    uint32_t file_size;

    memcpy(header.riff, "RIFF", 4);
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    memcpy(header.data, "data", 4);

    data_size = sample_rate * channels * (bits_per_sample / 8) * duration_seconds;
    file_size = data_size + sizeof(wav_header_t) - 8;

    header.file_size = file_size;
    header.fmt_size = 16;
    header.audio_format = WAV_FORMAT_PCM;
    header.num_channels = channels;
    header.sample_rate = sample_rate;
    header.byte_rate = sample_rate * channels * bits_per_sample / 8;
    header.block_align = channels * bits_per_sample / 8;
    header.bits_per_sample = bits_per_sample;
    header.data_size = data_size;

    if (fwrite(&header, sizeof(header), 1, fp) != 1) {
        ESP_LOGE(TAG, "Failed to write WAV header");
        return ESP_FAIL;
    }
    return ESP_OK;
}
