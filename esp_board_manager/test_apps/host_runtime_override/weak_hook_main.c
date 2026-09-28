/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 *
 * Verifies the library weak esp_board_runtime_pre_init() returns ESP_OK
 * when no strong definition is linked.
 */

#include <stdio.h>
#include <stdint.h>
#include "esp_board_runtime_state.h"
#include "esp_board_manager.h"
#include "host_idf_stubs.h"

static uint8_t s_dev_cfg = 0x11;

static esp_err_t periph_init(void *cfg, int cfg_size, void **handle)
{
    (void)cfg;
    (void)cfg_size;
    *handle = &s_dev_cfg;
    return ESP_OK;
}

static esp_err_t periph_deinit(void *handle)
{
    (void)handle;
    return ESP_OK;
}

const esp_board_info_t g_esp_board_info = {
    .name         = "weak_hook_board",
    .chip         = "esp32s3",
    .version      = "1.0",
    .description  = "weak pre_init smoke",
    .manufacturer = "test",
};

ESP_BOARD_TABLE_CONST esp_board_device_desc_t g_esp_board_devices[] = {
    {
        .next     = NULL,
        .name     = "audio_dac",
        .type     = "audio_codec",
        .cfg      = &s_dev_cfg,
        .cfg_size = sizeof(s_dev_cfg),
    },
};

ESP_BOARD_TABLE_CONST esp_board_periph_desc_t g_esp_board_peripherals[] = {
    {
        .next     = NULL,
        .name     = "i2c_master",
        .type     = "i2c",
        .role     = ESP_BOARD_PERIPH_ROLE_MASTER,
        .cfg      = &s_dev_cfg,
        .cfg_size = sizeof(s_dev_cfg),
    },
};

esp_board_device_handle_t g_esp_board_device_handles[] = {
    {
        .next = NULL,
        .name = "audio_dac",
        .type = "audio_codec",
    },
};

esp_board_periph_entry_t g_esp_board_periph_handles[] = {
    {
        .next   = NULL,
        .type   = "i2c",
        .role   = ESP_BOARD_PERIPH_ROLE_MASTER,
        .init   = periph_init,
        .deinit = periph_deinit,
    },
};

int main(void)
{
    esp_board_runtime_test_reset();
    if (esp_board_runtime_pre_init() != ESP_OK) {
        fprintf(stderr, "FAIL: weak pre_init did not return ESP_OK\n");
        return 1;
    }
    if (esp_board_runtime_prepare() != ESP_OK) {
        fprintf(stderr, "FAIL: prepare with weak hook\n");
        return 1;
    }
    if (esp_board_runtime_get_prepare_state() != ESP_BOARD_RUNTIME_PREPARED) {
        fprintf(stderr, "FAIL: expected PREPARED after weak prepare\n");
        return 1;
    }
    if (esp_board_runtime_prepare() != ESP_OK) {
        fprintf(stderr, "FAIL: weak prepare not idempotent\n");
        return 1;
    }
    printf("PASS: weak esp_board_runtime_pre_init returns ESP_OK\n");
    return 0;
}
