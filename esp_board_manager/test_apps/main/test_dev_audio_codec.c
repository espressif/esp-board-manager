/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_board_device.h"
#include "esp_board_manager.h"
#include "esp_board_manager_defs.h"
#include "esp_board_periph.h"
#include "periph_i2s.h"
#include "test_dev_audio_codec.h"

static const char *TAG = "TEST_CODEC_CFG";

esp_err_t audio_config_from_i2s(const periph_i2s_config_t *i2s_cfg, audio_config_t *config)
{
    if (i2s_cfg == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    switch (i2s_cfg->mode) {
        case I2S_COMM_MODE_STD:
            config->sample_rate = i2s_cfg->i2s_cfg.std.clk_cfg.sample_rate_hz;
            config->channels = i2s_cfg->i2s_cfg.std.slot_cfg.slot_mode == I2S_SLOT_MODE_STEREO ? 2 : 1;
            config->bits_per_sample = i2s_cfg->i2s_cfg.std.slot_cfg.data_bit_width;
            return ESP_OK;
#if CONFIG_SOC_I2S_SUPPORTS_TDM
        case I2S_COMM_MODE_TDM: {
            uint32_t slot_mask = i2s_cfg->i2s_cfg.tdm.slot_cfg.slot_mask;
            uint32_t total_slots = i2s_cfg->i2s_cfg.tdm.slot_cfg.total_slot;
            uint32_t mask_slots = 0;

            if (slot_mask == 0) {
                return ESP_ERR_INVALID_ARG;
            }
            mask_slots = 32 - __builtin_clz(slot_mask);
            if (total_slots < mask_slots) {
                total_slots = mask_slots;
            }
            if (total_slots < 2 && i2s_cfg->i2s_cfg.tdm.slot_cfg.ws_width != 1) {
                total_slots = 2;
            }
            if (total_slots > UINT16_MAX) {
                return ESP_ERR_INVALID_SIZE;
            }

            config->sample_rate = i2s_cfg->i2s_cfg.tdm.clk_cfg.sample_rate_hz;
            config->channels = (uint16_t)total_slots;
            config->bits_per_sample = i2s_cfg->i2s_cfg.tdm.slot_cfg.data_bit_width;
            return ESP_OK;
        }
#endif  /* CONFIG_SOC_I2S_SUPPORTS_TDM */
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }
}

esp_err_t initialize_devices(const device_config_t *dev_config)
{
    esp_err_t ret;
    if (dev_config->i2c_periph) {
        ret = esp_board_periph_init(dev_config->i2c_periph);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize I2C peripheral");
            return ret;
        }
    }
    if (dev_config->i2s_periph) {
        ret = esp_board_periph_init(dev_config->i2s_periph);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize I2S peripheral");
            return ret;
        }
    }
    if (dev_config->sdcard_device) {
        ret = esp_board_device_init(dev_config->sdcard_device);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize SD card");
            return ret;
        }
    }
    if (dev_config->codec_device) {
        ret = esp_board_device_init(dev_config->codec_device);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize codec device");
            return ret;
        }
    }
    return ESP_OK;
}

esp_err_t configure_codec(const char *codec_name, const audio_config_t *config, bool is_dac, dev_audio_codec_handles_t **codec_handles)
{
    esp_err_t ret = esp_board_manager_get_device_handle(codec_name, (void **)codec_handles);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get %s handle", codec_name);
        return ret;
    }
    if ((*codec_handles)->codec_dev == NULL) {
        ESP_LOGE(TAG, "Failed to get %s handle", codec_name);
        return ESP_FAIL;
    }

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = config->sample_rate,
        .channel = config->channels,
        .bits_per_sample = config->bits_per_sample,
    };
    ESP_LOGI(TAG, "%s sample rate: %" PRIu32 ", channel: %" PRIu8 ", bits: %" PRIu8,
             is_dac ? "DAC" : "ADC", fs.sample_rate, fs.channel, fs.bits_per_sample);
    // Close the codec device first
    esp_codec_dev_close((*codec_handles)->codec_dev);
    ret = esp_codec_dev_open((*codec_handles)->codec_dev, &fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to open %s codec", codec_name);
        return ret;
    }
    if (is_dac) {
        ret = esp_codec_dev_set_out_vol((*codec_handles)->codec_dev, 60);
        if (ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Failed to set DAC volume");
        }
    } else {
        ret = esp_codec_dev_set_in_gain((*codec_handles)->codec_dev, 30);
        if (ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Failed to set ADC input gain");
        }
    }
    return ESP_OK;
}

esp_err_t close_codec(dev_audio_codec_handles_t *codec_handles)
{
    if (codec_handles == NULL || codec_handles->codec_dev == NULL) {
        return ESP_OK;
    }
    esp_err_t ret = esp_codec_dev_close(codec_handles->codec_dev);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to close codec device");
        return ret;
    }
    return ESP_OK;
}

esp_err_t cleanup_devices(const device_config_t *dev_config)
{
    esp_err_t ret = ESP_OK;
    // Cleanup codec device
    if (dev_config->codec_device) {
        esp_board_device_deinit(dev_config->codec_device);
    }
    if (dev_config->sdcard_device) {
        esp_board_device_deinit(dev_config->sdcard_device);
    }
    if (dev_config->i2c_periph) {
        esp_board_periph_deinit(dev_config->i2c_periph);
    }
    if (dev_config->i2s_periph) {
        esp_board_periph_deinit(dev_config->i2s_periph);
    }
    return ret;
}
