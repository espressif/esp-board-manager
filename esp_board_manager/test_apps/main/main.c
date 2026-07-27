/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_err.h"
#include "esp_log.h"
#include "bmgr_test_console.h"
#include "bmgr_test_context.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP Board Manager interactive test application");

    static bmgr_test_context_t ctx;
    bmgr_test_context_init(&ctx);

    esp_err_t ret = bmgr_test_console_start(&ctx);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start test console: %s", esp_err_to_name(ret));
    }
}
