/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_board_find_utils.h"
#include "esp_board_manager.h"
#include "esp_board_periph.h"
#include "bmgr_test_context.h"

/* gen_board_debug_init.h is only generated when the board config is produced
 * with `idf.py bmgr -b <board> --debug`. The build matrix and CI generate
 * without --debug, so the debug init path must stay optional to keep the test
 * app buildable for every board. */
#if defined(__has_include)
#if __has_include("gen_board_debug_init.h")
#include "gen_board_debug_init.h"
#define BMGR_TEST_HAS_DEBUG_INIT  1
#endif  /* __has_include("gen_board_debug_init.h") */
#endif  /* defined(__has_include) */
#ifndef BMGR_TEST_HAS_DEBUG_INIT
#define BMGR_TEST_HAS_DEBUG_INIT  0
#endif  /* BMGR_TEST_HAS_DEBUG_INIT */

static const char *TAG = "BMGR_TEST_CTX";

void bmgr_test_context_init(bmgr_test_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
}

esp_err_t bmgr_test_board_init(bmgr_test_context_t *ctx, bool debug)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (ctx->board_inited) {
        ESP_LOGW(TAG, "Board manager is already initialized");
        return ESP_OK;
    }

    esp_err_t ret;
    if (debug) {
#if BMGR_TEST_HAS_DEBUG_INIT
        ret = bmgr_debug_board_init();
#else
        ESP_LOGE(TAG, "Debug init code was not generated. Re-run: idf.py bmgr -b <board> --debug");
        return ESP_ERR_NOT_SUPPORTED;
#endif  /* BMGR_TEST_HAS_DEBUG_INIT */
    } else {
        ret = esp_board_manager_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Board manager initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ctx->board_inited = true;
    ctx->debug_board_inited = debug;
    ESP_LOGI(TAG, "Board manager initialized via %s path", debug ? "debug" : "normal");
    return ESP_OK;
}

esp_err_t bmgr_test_board_deinit(bmgr_test_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!ctx->board_inited) {
        ESP_LOGW(TAG, "Board manager is not initialized");
        return ESP_OK;
    }

    esp_err_t ret;
    if (ctx->debug_board_inited) {
#if BMGR_TEST_HAS_DEBUG_INIT
        ret = bmgr_debug_board_deinit();
#else
        ret = ESP_ERR_NOT_SUPPORTED;
#endif  /* BMGR_TEST_HAS_DEBUG_INIT */
    } else {
        ret = esp_board_manager_deinit();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Board manager deinitialization failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ctx->board_inited = false;
    ctx->debug_board_inited = false;
    ctx->lvgl_inited = false;
    ctx->lcd_touch_inited = false;
    ctx->audio_power_enabled = false;
    ESP_LOGI(TAG, "Board manager deinitialized");
    return ESP_OK;
}

esp_err_t bmgr_test_require_board(bmgr_test_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!ctx->board_inited) {
        ESP_LOGE(TAG, "Board manager is not initialized. Run: bmgr init --debug");
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

bool bmgr_test_name_is_device(const char *name)
{
    return esp_board_find_device_desc(name) != NULL;
}

esp_err_t bmgr_test_target_init(const char *name)
{
    if (name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!esp_board_manager_check_name(name)) {
        ESP_LOGE(TAG, "No device or peripheral named '%s'", name);
        return ESP_BOARD_ERR_MANAGER_DEVICE_NOT_FOUND;
    }

    esp_err_t ret;
    if (bmgr_test_name_is_device(name)) {
        ret = esp_board_manager_init_device_by_name(name);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Device '%s' initialized", name);
        }
    } else {
        ret = esp_board_periph_init(name);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Peripheral '%s' initialized", name);
        }
    }
    return ret;
}

esp_err_t bmgr_test_target_deinit(const char *name)
{
    if (name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!esp_board_manager_check_name(name)) {
        ESP_LOGE(TAG, "No device or peripheral named '%s'", name);
        return ESP_BOARD_ERR_MANAGER_DEVICE_NOT_FOUND;
    }

    esp_err_t ret;
    if (bmgr_test_name_is_device(name)) {
        ret = esp_board_manager_deinit_device_by_name(name);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Device '%s' deinitialized", name);
        }
    } else {
        ret = esp_board_periph_deinit(name);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Peripheral '%s' deinitialized", name);
        }
    }
    return ret;
}

void bmgr_test_context_print(const bmgr_test_context_t *ctx)
{
    if (ctx == NULL) {
        printf("context: NULL\n");
        return;
    }
    printf("board_inited=%s\n", ctx->board_inited ? "true" : "false");
    printf("debug_board_inited=%s\n", ctx->debug_board_inited ? "true" : "false");
    printf("console_ready=%s\n", ctx->console_ready ? "true" : "false");
    printf("lvgl_inited=%s\n", ctx->lvgl_inited ? "true" : "false");
    printf("lcd_touch_inited=%s\n", ctx->lcd_touch_inited ? "true" : "false");
    printf("audio_power_enabled=%s\n", ctx->audio_power_enabled ? "true" : "false");
    printf("running_case_count=%" PRIu32 "\n", ctx->running_case_count);
}
