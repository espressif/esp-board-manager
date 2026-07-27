/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_check.h"
#include "sdkconfig.h"
#include "bmgr_test_names.h"
#include "bmgr_test_registry.h"

#ifdef CONFIG_ESP_BOARD_DEV_FS_FAT_SUPPORT
#include "test_dev_fs_fat.h"
#endif  /* CONFIG_ESP_BOARD_DEV_FS_FAT_SUPPORT */

#ifdef CONFIG_ESP_BOARD_DEV_FS_SPIFFS_SUPPORT
#include "test_dev_fs_spiffs.h"
#endif  /* CONFIG_ESP_BOARD_DEV_FS_SPIFFS_SUPPORT */

#ifdef CONFIG_ESP_BOARD_DEV_LITTLEFS_SUPPORT
#include "test_dev_littlefs.h"
#endif  /* CONFIG_ESP_BOARD_DEV_LITTLEFS_SUPPORT */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)  (sizeof(array) / sizeof((array)[0]))
#endif  /* ARRAY_SIZE */

#ifdef CONFIG_ESP_BOARD_DEV_FS_SPIFFS_SUPPORT
static esp_err_t run_fs_spiffs(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_spiffs();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_DEV_FS_SPIFFS_SUPPORT */

#ifdef CONFIG_ESP_BOARD_DEV_FS_FAT_SUPPORT
static esp_err_t run_fs_fat(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_fs_fat_device();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_DEV_FS_FAT_SUPPORT */

#ifdef CONFIG_ESP_BOARD_DEV_LITTLEFS_SUPPORT
static esp_err_t run_fs_littlefs(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_littlefs();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_DEV_LITTLEFS_SUPPORT */

void bmgr_register_fs_cases(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_FS_SPIFFS_SUPPORT
    const bmgr_test_case_t spiffs_cases[] = {
        {
            .name = "fs.spiffs",
            .group = "fs",
            .help = "Run SPIFFS filesystem test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_FS_SPIFFS),
            },
            .run = run_fs_spiffs,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(spiffs_cases, ARRAY_SIZE(spiffs_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_FS_SPIFFS_SUPPORT */

#ifdef CONFIG_ESP_BOARD_DEV_FS_FAT_SUPPORT
    /* fs.fat manages its own init/deinit (with fs_fat -> fs_sdcard fallback),
     * so it does not declare requires. */
    const bmgr_test_case_t fat_cases[] = {
        {
            .name = "fs.fat",
            .group = "fs",
            .help = "Run FAT filesystem test",
            .run = run_fs_fat,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(fat_cases, ARRAY_SIZE(fat_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_FS_FAT_SUPPORT */

#ifdef CONFIG_ESP_BOARD_DEV_LITTLEFS_SUPPORT
    const bmgr_test_case_t littlefs_cases[] = {
        {
            .name = "fs.littlefs",
            .group = "fs",
            .help = "Run LittleFS filesystem test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_LITTLEFS),
            },
            .run = run_fs_littlefs,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(littlefs_cases, ARRAY_SIZE(littlefs_cases)));
#endif  /* CONFIG_ESP_BOARD_DEV_LITTLEFS_SUPPORT */
}
