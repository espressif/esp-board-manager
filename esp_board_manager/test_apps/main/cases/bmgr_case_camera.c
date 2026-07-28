/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <inttypes.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "bmgr_test_json.h"
#include "bmgr_test_names.h"
#include "bmgr_test_options.h"
#include "bmgr_test_registry.h"

#ifdef CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT
#include "test_dev_camera_auto.h"
#include "test_dev_camera.h"
#endif  /* CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT */

#if defined(CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT) && defined(CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT)
#include "test_dev_camera_lcd.h"
#endif  /* defined(CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT) && defined(CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT) */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)  (sizeof(array) / sizeof((array)[0]))
#endif  /* ARRAY_SIZE */

#ifdef CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT
static void emit_camera_capture_summary(const test_dev_camera_auto_summary_t *summary)
{
    if (summary == NULL) {
        return;
    }

    if (bmgr_test_options_get().json) {
        bmgr_test_json_metric_begin("camera.capture", "camera.summary");
        bmgr_test_json_metric_u32("width", summary->width);
        bmgr_test_json_metric_u32("height", summary->height);
        bmgr_test_json_metric_u32("pixelformat", summary->pixelformat);
        bmgr_test_json_metric_u32("bytesused", summary->bytesused);
        bmgr_test_json_metric_u32("checksum", summary->checksum);
        bmgr_test_json_metric_float("mean_luma", summary->mean_luma);
        bmgr_test_json_metric_end();
        return;
    }

    printf("[SUMMARY] camera.capture width=%" PRIu32
           " height=%" PRIu32
           " pixelformat=0x%08" PRIx32
           " bytesused=%" PRIu32
           " checksum=%" PRIu32
           " mean_luma=%.6f\n",
           summary->width,
           summary->height,
           summary->pixelformat,
           summary->bytesused,
           summary->checksum,
           (double)summary->mean_luma);
}

static esp_err_t run_camera_capture(bmgr_test_context_t *ctx, int argc, char **argv)
{
    test_dev_camera_auto_summary_t summary = {0};
    esp_err_t ret = ESP_OK;

    (void)ctx;
    (void)argc;
    (void)argv;

    if (!bmgr_test_options_get().summary) {
        return test_dev_camera();
    }

    ret = test_dev_camera_auto_capture(&summary);
    if (ret != ESP_OK) {
        return ret;
    }
    emit_camera_capture_summary(&summary);
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT */

#if defined(CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT) && defined(CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT)
static esp_err_t run_camera_lcd(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    return test_dev_camera_lcd();
}
#endif  /* defined(CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT) && defined(CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT) */

void bmgr_register_camera_cases(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT
    const bmgr_test_case_t camera_cases[] = {
        {
            .name = "camera.capture",
            .group = "camera",
            .help = "Run camera capture test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_CAMERA),
                /* Needed so /sdcard exists when saving the captured JPEG; optional so
                 * boards without an FS_FAT device (or without a card inserted) still
                 * run the capture itself. */
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_FS_FAT),
            },
            .run = run_camera_capture,
            .flags = BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
#ifdef CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT
        {
            .name = "camera.lcd",
            .group = "camera",
            .help = "Preview camera frames directly on LCD without LVGL",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_CAMERA),
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_DISPLAY_LCD),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_LCD_BRIGHTNESS),
            },
            .run = run_camera_lcd,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD | BMGR_TEST_CASE_FLAG_NEEDS_DISPLAY,
        },
#endif  /* CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT */
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(camera_cases, ARRAY_SIZE(camera_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT */
}
