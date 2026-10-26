/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "dev_button.h"
#include "esp_board_device.h"
#include "test_dev_button.h"

#define BUTTON_TEST_DURATION_MS  10000

typedef struct {
    const char           *name;
    dev_button_handles_t *handles;
    uint32_t              registered[CONFIG_ADC_BUTTON_MAX_BUTTON_PER_CHANNEL];
    bool                  acquired;
} button_test_target_t;

static const char *TAG = "TEST_BUTTON";
static const button_event_t s_events[] = {
    BUTTON_PRESS_DOWN, BUTTON_PRESS_UP, BUTTON_SINGLE_CLICK, BUTTON_DOUBLE_CLICK,
    BUTTON_LONG_PRESS_START, BUTTON_LONG_PRESS_HOLD, BUTTON_LONG_PRESS_UP,
    BUTTON_PRESS_REPEAT, BUTTON_PRESS_REPEAT_DONE, BUTTON_PRESS_END,
};

static void button_test_event(void *handle, void *user_data)
{
    /* Board device names have static lifetime, including after this test exits. */
    ESP_LOGI(TAG, "%s (%p): %s", (const char *)user_data, handle,
             iot_button_get_event_str(iot_button_get_event(handle)));
}

typedef struct {
    button_test_target_t *targets;
    size_t                count;
    SemaphoreHandle_t     done;
    bool                  cleanup;
    bool                  completed;
    esp_err_t             result;
} button_test_dispatch_t;

/* button 4.x dispatches on ESP_TIMER_TASK. Run mutations on that same task:
 * any in-flight scan has returned, and the next scan cannot overlap them.
 * Keep the ten-second monitoring wait on the console task. */
static void button_test_update_callbacks(void *arg)
{
    button_test_dispatch_t *dispatch = arg;
    button_test_target_t *targets = dispatch->targets;
    size_t count = dispatch->count;
    esp_err_t ret = dispatch->result;

    if (!dispatch->cleanup) {
        for (size_t i = 0; i < count; i++) {
            button_test_target_t *target = &targets[i];
            for (size_t button = 0; button < target->handles->num_buttons; button++) {
                for (size_t event = 0; event < sizeof(s_events) / sizeof(s_events[0]); event++) {
                    ret = iot_button_register_cb(target->handles->button_handles[button], s_events[event],
                                                 NULL, button_test_event, (void *)target->name);
                    /* Some driver errors are returned after inserting the callback. */
                    if (iot_button_count_event_cb(target->handles->button_handles[button], s_events[event]) != 0) {
                        target->registered[button] |= UINT32_C(1) << event;
                    }
                    if (ret != ESP_OK) {
                        goto done;
                    }
                }
            }
        }
    } else {
        for (size_t i = count; i > 0; i--) {
            button_test_target_t *target = &targets[i - 1];
            if (target->handles != NULL) {
                for (size_t button = 0; button < sizeof(target->registered) / sizeof(target->registered[0]); button++) {
                    for (size_t event = 0; event < sizeof(s_events) / sizeof(s_events[0]); event++) {
                        if (target->registered[button] & (UINT32_C(1) << event)) {
                            esp_err_t remove_ret = iot_button_unregister_cb(target->handles->button_handles[button], s_events[event], NULL);
                            if (remove_ret != ESP_OK) {
                                ESP_LOGE(TAG, "Failed to remove test callback on %s: %s", target->name, esp_err_to_name(remove_ret));
                                if (ret == ESP_OK) {
                                    ret = remove_ret;
                                }
                            }
                        }
                    }
                }
            }
            if (target->acquired) {
                esp_err_t release_ret = esp_board_device_deinit(target->name);
                if (release_ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to release %s: %s", target->name, esp_err_to_name(release_ret));
                    if (ret == ESP_OK) {
                        ret = release_ret;
                    }
                }
            }
        }
    }

done:
    dispatch->result = ret;
    dispatch->completed = true;
    xSemaphoreGive(dispatch->done);
}

static esp_err_t button_test_dispatch(esp_timer_handle_t timer, button_test_dispatch_t *dispatch)
{
    dispatch->completed = false;
    esp_err_t ret = esp_timer_start_once(timer, 1);
    if (ret != ESP_OK) {
        return ret;
    }
    while (xSemaphoreTake(dispatch->done, portMAX_DELAY) != pdTRUE) {
        /* Do not let stack-owned dispatch state expire before completion. */
    }
    return dispatch->result;
}

esp_err_t test_dev_button_named(const char *selected_name)
{
    const bool all = selected_name == NULL || strcmp(selected_name, "all") == 0;
    const char *name = NULL;
    void *cursor = NULL;
    size_t count = 0;
    esp_err_t ret;

    while ((ret = esp_board_device_iterate_name_by_type("button", &cursor, &name)) == ESP_OK) {
        if (all || strcmp(selected_name, name) == 0) {
            count++;
        }
    }
    if (ret != ESP_BOARD_ERR_DEVICE_NOT_FOUND) {
        return ret;
    }
    if (count == 0) {
        ESP_LOGE(TAG, "No matching button: %s", all ? "all" : selected_name);
        return ESP_ERR_NOT_FOUND;
    }
    button_test_target_t *targets = calloc(count, sizeof(*targets));
    if (targets == NULL) {
        return ESP_ERR_NO_MEM;
    }

    button_test_dispatch_t dispatch = {
        .targets = targets,
        .count = count,
        .done = xSemaphoreCreateBinary(),
    };
    if (dispatch.done == NULL) {
        free(targets);
        return ESP_ERR_NO_MEM;
    }
    const esp_timer_create_args_t timer_args = {
        .callback = button_test_update_callbacks,
        .arg = &dispatch,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "button_test",
    };
    esp_timer_handle_t timer = NULL;
    ret = esp_timer_create(&timer_args, &timer);
    if (ret != ESP_OK) {
        vSemaphoreDelete(dispatch.done);
        free(targets);
        return ret;
    }

    cursor = NULL;
    size_t index = 0;
    while ((ret = esp_board_device_iterate_name_by_type("button", &cursor, &name)) == ESP_OK) {
        if (!all && strcmp(selected_name, name) != 0) {
            continue;
        }
        if (index >= count) {
            ret = ESP_ERR_INVALID_STATE;
            goto cleanup;
        }
        button_test_target_t *target = &targets[index++];
        target->name = name;
        ret = esp_board_device_init(name);
        if (ret != ESP_OK) {
            goto cleanup;
        }
        target->acquired = true;
        ret = esp_board_device_get_handle(name, (void **)&target->handles);
        if (ret != ESP_OK) {
            goto cleanup;
        }
        if (target->handles == NULL || target->handles->num_buttons == 0 ||
            target->handles->num_buttons > sizeof(target->handles->button_handles) / sizeof(target->handles->button_handles[0])) {
            ret = ESP_ERR_INVALID_STATE;
            goto cleanup;
        }
        for (size_t button = 0; button < target->handles->num_buttons; button++) {
            if (target->handles->button_handles[button] == NULL) {
                ret = ESP_ERR_INVALID_STATE;
                goto cleanup;
            }
            for (size_t event = 0; event < sizeof(s_events) / sizeof(s_events[0]); event++) {
                /* The button API cannot remove one callback by function pointer.
                 * Refuse occupied events instead of removing another consumer. */
                if (iot_button_count_event_cb(target->handles->button_handles[button], s_events[event]) != 0) {
                    ESP_LOGE(TAG, "%s already has event callbacks; release its other consumer first", name);
                    ret = ESP_ERR_INVALID_STATE;
                    goto cleanup;
                }
            }
        }
        ESP_LOGI(TAG, "Monitoring %s (%u buttons)", name, (unsigned)target->handles->num_buttons);
    }
    if (ret != ESP_BOARD_ERR_DEVICE_NOT_FOUND || index != count) {
        if (ret == ESP_BOARD_ERR_DEVICE_NOT_FOUND) {
            ret = ESP_ERR_INVALID_STATE;
        }
        goto cleanup;
    }

    dispatch.result = ESP_OK;
    ret = button_test_dispatch(timer, &dispatch);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    ESP_LOGI(TAG, "Press any monitored button during the next 10 seconds");
    vTaskDelay(pdMS_TO_TICKS(BUTTON_TEST_DURATION_MS));
    ESP_LOGI(TAG, "Button monitoring finished; confirm the observed events manually");
    ret = ESP_OK;

cleanup:
    dispatch.cleanup = true;
    dispatch.result = ret;
    ret = button_test_dispatch(timer, &dispatch);
    if (!dispatch.completed) {
        /* A dispatch failure must not fall back to racing the live button scan. */
        ESP_LOGE(TAG, "Could not schedule button cleanup; device references retained: %s", esp_err_to_name(ret));
    }
    esp_err_t delete_ret = esp_timer_delete(timer);
    if (delete_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to delete button test timer: %s", esp_err_to_name(delete_ret));
        if (ret == ESP_OK) {
            ret = delete_ret;
        }
    }
    vSemaphoreDelete(dispatch.done);
    free(targets);
    return ret;
}

void test_dev_button(void)
{
    esp_err_t ret = test_dev_button_named(NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Button test failed: %s", esp_err_to_name(ret));
    }
}
