/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "dev_lcd_touch.h"
#include "esp_board_manager.h"
#include "bmgr_test_names.h"
#include "test_dev_lcd_touch.h"

#define TOUCH_TEST_DURATION_US  INT64_C(30000000)
#define TOUCH_TEST_POLL_MS      20

static const char *TAG = "TEST_LCD_TOUCH";

static const esp_lcd_touch_point_data_t *find_point(const esp_lcd_touch_point_data_t *points,
                                                    uint8_t count, uint8_t track_id)
{
    for (uint8_t i = 0; i < count; i++) {
        if (points[i].track_id == track_id) {
            return &points[i];
        }
    }
    return NULL;
}

static void log_point(const char *event, const esp_lcd_touch_point_data_t *point)
{
    ESP_LOGI(TAG, "[TOUCH] %s id=%u x=%u y=%u", event,
             (unsigned)point->track_id, (unsigned)point->x, (unsigned)point->y);
}

esp_err_t test_dev_lcd_touch_multitouch(test_dev_lcd_touch_observer_t observer, void *user_ctx)
{
    if (CONFIG_ESP_LCD_TOUCH_MAX_POINTS < 2) {
        ESP_LOGE(TAG, "Set CONFIG_ESP_LCD_TOUCH_MAX_POINTS to at least 2");
        return ESP_ERR_NOT_SUPPORTED;
    }

    dev_lcd_touch_handles_t *handles = NULL;
    esp_err_t ret = esp_board_manager_get_device_handle(BMGR_TEST_NAME_LCD_TOUCH, (void **)&handles);
    if (ret != ESP_OK) {
        return ret;
    }
    if (handles == NULL || handles->touch_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_lcd_touch_handle_t touch = handles->touch_handle;
    if (touch->get_track_id == NULL) {
        ESP_LOGE(TAG, "Touch driver does not provide native contact IDs");
        return ESP_ERR_NOT_SUPPORTED;
    }

    esp_lcd_touch_point_data_t previous[CONFIG_ESP_LCD_TOUCH_MAX_POINTS] = {0};
    uint8_t previous_count = 0;
    bool saw_multitouch = false;
    bool saw_partial_release = false;
    TickType_t poll_ticks = pdMS_TO_TICKS(TOUCH_TEST_POLL_MS);
    if (poll_ticks == 0) {
        poll_ticks = 1;
    }

    ESP_LOGI(TAG, "Within 30 seconds: press two fingers, move them, lift one, then lift the other");
    const int64_t start = esp_timer_get_time();
    while (esp_timer_get_time() - start < TOUCH_TEST_DURATION_US) {
        ret = esp_lcd_touch_read_data(touch);
        if (ret == ESP_ERR_NOT_FINISHED) {
            vTaskDelay(poll_ticks);
            continue;
        }
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Touch acquisition failed: %s", esp_err_to_name(ret));
            return ret;
        }

        esp_lcd_touch_point_data_t current[CONFIG_ESP_LCD_TOUCH_MAX_POINTS] = {0};
        uint8_t current_count = 0;
        ret = esp_lcd_touch_get_data(touch, current, &current_count, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);
        if (ret == ESP_ERR_NOT_FINISHED) {
            vTaskDelay(poll_ticks);
            continue;
        }
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Touch data read failed: %s", esp_err_to_name(ret));
            return ret;
        }
        if (current_count > CONFIG_ESP_LCD_TOUCH_MAX_POINTS) {
            return ESP_ERR_INVALID_RESPONSE;
        }
        for (uint8_t i = 0; i < current_count; i++) {
            for (uint8_t j = 0; j < i; j++) {
                if (current[i].track_id == current[j].track_id) {
                    ESP_LOGE(TAG, "Duplicate contact ID %u in a valid frame", (unsigned)current[i].track_id);
                    return ESP_ERR_INVALID_RESPONSE;
                }
            }
        }

        if (observer != NULL) {
            ret = observer(current, current_count, user_ctx);
            if (ret != ESP_OK) {
                return ret;
            }
        }

        bool retained_contact = false;
        for (uint8_t i = 0; i < previous_count; i++) {
            if (find_point(current, current_count, previous[i].track_id) == NULL) {
                log_point("UP", &previous[i]);
            } else {
                retained_contact = true;
            }
        }
        for (uint8_t i = 0; i < current_count; i++) {
            const esp_lcd_touch_point_data_t *old = find_point(previous, previous_count, current[i].track_id);
            if (old == NULL) {
                log_point("DOWN", &current[i]);
            } else if (old->x != current[i].x || old->y != current[i].y) {
                log_point("MOVE", &current[i]);
            }
        }

        if (current_count >= 2) {
            saw_multitouch = true;
        }
        if (saw_multitouch && current_count > 0 && current_count < previous_count && retained_contact) {
            saw_partial_release = true;
        }
        if (current_count == 0) {
            if (saw_multitouch && saw_partial_release) {
                ESP_LOGI(TAG, "Observed simultaneous contacts, partial release with retained ID, and full release");
                return ESP_OK;
            }
            /* Evidence from separate gestures must not form a false pass. */
            saw_multitouch = false;
            saw_partial_release = false;
        }
        memcpy(previous, current, current_count * sizeof(*current));
        previous_count = current_count;
        vTaskDelay(poll_ticks);
    }

    ESP_LOGE(TAG, "Timed out before a complete multi-touch gesture was observed");
    return ESP_ERR_TIMEOUT;
}
