/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include "driver/ledc.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "periph_ledc.h"

static const char *TAG = "PERIPH_LEDC";

typedef struct ledc_timer_ref {
    ledc_mode_t            speed_mode;
    ledc_timer_t           timer_num;
    ledc_timer_bit_t       duty_resolution;
    uint32_t               freq_hz;
    ledc_clk_cfg_t         clk_cfg;
    uint16_t               ref_count;
    struct ledc_timer_ref *next;
} ledc_timer_ref_t;

/* BMGR peripheral init/deinit is serialized by the board manager. */
static ledc_timer_ref_t *s_ledc_timer_refs;

static ledc_timer_ref_t *find_ledc_timer_ref(ledc_mode_t speed_mode, ledc_timer_t timer_num)
{
    for (ledc_timer_ref_t *entry = s_ledc_timer_refs; entry; entry = entry->next) {
        if (entry->speed_mode == speed_mode && entry->timer_num == timer_num) {
            return entry;
        }
    }
    return NULL;
}

static void remove_ledc_timer_ref(ledc_timer_ref_t *entry)
{
    ledc_timer_ref_t **link = &s_ledc_timer_refs;
    while (*link && *link != entry) {
        link = &(*link)->next;
    }
    if (*link) {
        *link = entry->next;
        free(entry);
    }
}

static esp_err_t deconfigure_ledc_timer(ledc_mode_t speed_mode, ledc_timer_t timer_num)
{
    esp_err_t ret = ledc_timer_pause(speed_mode, timer_num);
    if (ret != ESP_OK) {
        return ret;
    }

    ledc_timer_config_t timer_config = {
        .speed_mode = speed_mode,
        .timer_num = timer_num,
        /* LEDC requires a timer to be paused before it is deconfigured. */
        .deconfigure = true,
    };
    return ledc_timer_config(&timer_config);
}

static esp_err_t release_ledc_timer_ref(periph_ledc_handle_t *handle)
{
    if (!handle->timer_ref_acquired) {
        return ESP_OK;
    }

    ledc_timer_ref_t *entry = find_ledc_timer_ref(handle->speed_mode, handle->timer_sel);
    if (!entry || entry->ref_count == 0) {
        ESP_LOGE(TAG, "No timer reference found for speed mode %d timer %d", handle->speed_mode, handle->timer_sel);
        return ESP_ERR_INVALID_STATE;
    }

    if (entry->ref_count > 1) {
        entry->ref_count--;
        handle->timer_ref_acquired = false;
        return ESP_OK;
    }

    esp_err_t ret = deconfigure_ledc_timer(handle->speed_mode, handle->timer_sel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deconfigure LEDC timer %d: %s", handle->timer_sel, esp_err_to_name(ret));
        return ret;
    }

    remove_ledc_timer_ref(entry);
    handle->timer_ref_acquired = false;
    return ESP_OK;
}

int periph_ledc_init(void *cfg, int cfg_size, void **periph_handle)
{
    if (!cfg || cfg_size < sizeof(periph_ledc_config_t) || !periph_handle) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    periph_ledc_config_t config = {0};
    memcpy(&config, cfg, sizeof(config));

    periph_ledc_handle_t *handle = calloc(1, sizeof(periph_ledc_handle_t));
    if (!handle) {
        ESP_LOGE(TAG, "Failed to allocate handle");
        return -1;
    }
    handle->channel = config.handle.channel;
    handle->speed_mode = config.handle.speed_mode;
    handle->timer_sel = config.timer_sel;

    ledc_timer_ref_t *timer_ref = find_ledc_timer_ref(handle->speed_mode, handle->timer_sel);
    if (timer_ref) {
        if (timer_ref->duty_resolution != config.duty_resolution ||
            timer_ref->freq_hz != config.freq_hz ||
            timer_ref->clk_cfg != LEDC_AUTO_CLK) {
            ESP_LOGE(TAG, "LEDC timer %d is already configured with a different clock configuration", handle->timer_sel);
            free(handle);
            return -1;
        }
        timer_ref->ref_count++;
        handle->timer_ref_acquired = true;
    } else {
        timer_ref = calloc(1, sizeof(ledc_timer_ref_t));
        if (!timer_ref) {
            ESP_LOGE(TAG, "Failed to allocate LEDC timer reference");
            free(handle);
            return -1;
        }

        ledc_timer_config_t ledc_timer = {
            .speed_mode = handle->speed_mode,
            .duty_resolution = config.duty_resolution,
            .timer_num = handle->timer_sel,
            .freq_hz = config.freq_hz,
            .clk_cfg = LEDC_AUTO_CLK,
        };
        esp_err_t err = ledc_timer_config(&ledc_timer);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ledc_timer_config failed: %s", esp_err_to_name(err));
            free(timer_ref);
            free(handle);
            return -1;
        }

        timer_ref->speed_mode = handle->speed_mode;
        timer_ref->timer_num = handle->timer_sel;
        timer_ref->duty_resolution = config.duty_resolution;
        timer_ref->freq_hz = config.freq_hz;
        timer_ref->clk_cfg = LEDC_AUTO_CLK;
        timer_ref->ref_count = 1;
        timer_ref->next = s_ledc_timer_refs;
        s_ledc_timer_refs = timer_ref;
        handle->timer_ref_acquired = true;
    }

    ledc_channel_config_t ledc_channel = {
        .speed_mode = handle->speed_mode,
        .channel = handle->channel,
        .timer_sel = handle->timer_sel,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = config.gpio_num,
        .duty = config.duty,
        .hpoint = 0,
        .flags.output_invert = config.output_invert,
        .sleep_mode = config.sleep_mode,
    };

    esp_err_t err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed: %s", esp_err_to_name(err));
        (void)release_ledc_timer_ref(handle);
        free(handle);
        return -1;
    }

    handle->channel_configured = true;
    *periph_handle = handle;
    ESP_LOGI(TAG, "LEDC initialized: speed_mode=%d, channel=%d, gpio=%d, freq=%" PRIu32 "Hz",
             handle->speed_mode, handle->channel, config.gpio_num, config.freq_hz);
    return 0;
}

int periph_ledc_deinit(void *periph_handle)
{
    if (!periph_handle) {
        ESP_LOGE(TAG, "Invalid handle");
        return -1;
    }

    periph_ledc_handle_t *handle = (periph_ledc_handle_t *)periph_handle;
    if (handle->channel_configured) {
        esp_err_t err = ledc_stop(handle->speed_mode, handle->channel, 0);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ledc_stop failed: %s", esp_err_to_name(err));
            return -1;
        }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
        ledc_channel_config_t channel_config = {
            .speed_mode = handle->speed_mode,
            .channel = handle->channel,
            /* Channel deconfigure releases the pins occupied by this channel. */
            .deconfigure = true,
        };
        err = ledc_channel_config(&channel_config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to deconfigure LEDC channel %d: %s", handle->channel, esp_err_to_name(err));
            return -1;
        }
#endif  /* ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0) */
        handle->channel_configured = false;
    }

    if (release_ledc_timer_ref(handle) != ESP_OK) {
        return -1;
    }

    ESP_LOGI(TAG, "LEDC deinitialized: speed_mode=%d, channel=%d", handle->speed_mode, handle->channel);
    free(periph_handle);
    return 0;
}
