/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "dev_knob.h"
#include "esp_board_manager.h"
#include "bmgr_test_names.h"
#include "test_dev_knob.h"

static const char *TAG = "TEST_KNOB";

static void knob_event_handler(void *handle, void *user_data)
{
    (void)user_data;
    knob_handle_t knob_handle = handle;
    ESP_LOGI(TAG, "Knob event=%d count=%d", iot_knob_get_event(knob_handle),
             iot_knob_get_count_value(knob_handle));
}

esp_err_t test_dev_knob(void)
{
    dev_knob_handles_t *handles = NULL;
    esp_err_t ret = esp_board_manager_get_device_handle(BMGR_TEST_NAME_KNOB, (void **)&handles);
    if (ret != ESP_OK || handles == NULL || handles->knob_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get knob device '%s': %s", BMGR_TEST_NAME_KNOB, esp_err_to_name(ret));
        return ret == ESP_OK ? ESP_ERR_INVALID_STATE : ret;
    }

    ret = iot_knob_register_cb(handles->knob_handle, KNOB_LEFT,
                               knob_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register left callback: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = iot_knob_register_cb(handles->knob_handle, KNOB_RIGHT,
                               knob_event_handler, NULL);
    if (ret != ESP_OK) {
        iot_knob_unregister_cb(handles->knob_handle, KNOB_LEFT);
        ESP_LOGE(TAG, "Failed to register right callback: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    ret = iot_knob_clear_count_value(handles->knob_handle);
    if (ret != ESP_OK) {
        iot_knob_unregister_cb(handles->knob_handle, KNOB_RIGHT);
        iot_knob_unregister_cb(handles->knob_handle, KNOB_LEFT);
        ESP_LOGE(TAG, "Failed to clear knob count: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Rotate the knob for 10 seconds");
    vTaskDelay(pdMS_TO_TICKS(10000));
    ESP_LOGI(TAG, "Knob count=%d",
             iot_knob_get_count_value(handles->knob_handle));

    ret = iot_knob_unregister_cb(handles->knob_handle, KNOB_RIGHT);
    if (ret == ESP_OK) {
        ret = iot_knob_unregister_cb(handles->knob_handle, KNOB_LEFT);
    }
    return ret;
}
