/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "dev_littlefs.h"
#include "esp_board_device.h"
#include "esp_board_manager.h"
#include "esp_board_manager_defs.h"
#include "sdmmc_cmd.h"
#include "bmgr_test_names.h"

static const char *TAG = "TEST_LITTLEFS";

static bool verify_file_content(const char *file_path, const char *expected_content, const char *test_name)
{
    char read_buf[128] = {0};
    FILE *f = fopen(file_path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to read %s file: %s", test_name, strerror(errno));
        return false;
    }

    size_t bytes_read = fread(read_buf, 1, sizeof(read_buf) - 1, f);
    read_buf[bytes_read] = '\0';
    fclose(f);

    ESP_LOGI(TAG, "Read from %s: '%s'", test_name, read_buf);
    if (strcmp(expected_content, read_buf) == 0) {
        ESP_LOGI(TAG, "%s content verification passed", test_name);
        return true;
    }

    ESP_LOGE(TAG, "%s content verification failed", test_name);
    ESP_LOGE(TAG, "Expected: '%s'", expected_content);
    ESP_LOGE(TAG, "Got: '%s'", read_buf);
    return false;
}

void test_littlefs(void)
{
    const char *test_str = "Hello LittleFS!\n";
    const char *expected_test_content = "This is esp board manager\n";

    dev_littlefs_config_t *cfg = NULL;
    if (esp_board_manager_get_device_config(BMGR_TEST_NAME_LITTLEFS, (void **)&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS configuration");
        return;
    }

    dev_littlefs_handle_t *handle = NULL;
    esp_err_t ret = esp_board_manager_get_device_handle(BMGR_TEST_NAME_LITTLEFS, (void **)&handle);
    if (ret == ESP_OK && handle && handle->card) {
        sdmmc_card_print_info(stdout, handle->card);
    }

    esp_board_device_show(BMGR_TEST_NAME_LITTLEFS);

    ESP_LOGI(TAG, "LittleFS Configuration:");
    ESP_LOGI(TAG, "  Name: %s", cfg->name);
    ESP_LOGI(TAG, "  Sub type: %s", cfg->sub_type);
    ESP_LOGI(TAG, "  Base Path: %s", cfg->vfs_config.base_path);
    ESP_LOGI(TAG, "  Partition Label: %s", cfg->vfs_config.partition_label ? cfg->vfs_config.partition_label : "NULL");

    if (strcmp(cfg->sub_type, ESP_BOARD_DEVICE_LITTLEFS_SUB_TYPE_FLASH) == 0) {
        char test_txt_path[256];
        snprintf(test_txt_path, sizeof(test_txt_path), "%s/test.txt", cfg->vfs_config.base_path);
        ESP_LOGI(TAG, "Testing read of existing file: %s", test_txt_path);
        verify_file_content(test_txt_path, expected_test_content, "test.txt");
    } else {
        ESP_LOGI(TAG, "Skipping preloaded test.txt verification for SD card backend");
    }

    char test_file_path[256];
    snprintf(test_file_path, sizeof(test_file_path), "%s/hello.txt", cfg->vfs_config.base_path);
    FILE *f = fopen(test_file_path, "w");
    if (f) {
        fprintf(f, "%s", test_str);
        fclose(f);
        ESP_LOGI(TAG, "Test file written successfully");
    } else {
        ESP_LOGE(TAG, "Failed to create test file: %s, path: %s", strerror(errno), test_file_path);
        goto cleanup;
    }

    verify_file_content(test_file_path, test_str, "hello.txt");

    ESP_LOGI(TAG, "LittleFS root directory contents:");
    DIR *dir = opendir(cfg->vfs_config.base_path);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            struct stat st;
            char fullpath[300];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", cfg->vfs_config.base_path, entry->d_name);

            if (stat(fullpath, &st) == 0) {
                ESP_LOGI(TAG, "%s - %ld bytes", entry->d_name, st.st_size);
            } else {
                ESP_LOGI(TAG, "%s", entry->d_name);
            }
        }
        closedir(dir);
    } else {
        ESP_LOGE(TAG, "Failed to open directory: %s", strerror(errno));
    }

cleanup:
    ESP_LOGI(TAG, "LittleFS filesystem test complete");
}
