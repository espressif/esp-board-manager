/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "dev_lcd_touch.h"
#include "esp_board_manager.h"
#include "bmgr_test_names.h"
#include "test_dev_lcd_touch.h"

typedef struct {
    esp_err_t                   read_result;
    esp_err_t                   getter_result;
    uint8_t                     count;
    esp_lcd_touch_point_data_t  points[2];
} touch_frame_t;

#define POINT(id_, x_, y_)  {                               \
    .track_id = (id_), .x = (x_), .y = (y_), .strength = 1  \
}
#define TWO_POINTS  {                            \
    .count = 2, .points = { POINT(3, 100, 200),  \
                            POINT(7, 300, 400)   \
    }                                            \
}
#define ONE_POINT  {                              \
    .count = 1, .points = { POINT(7, 300, 400) }  \
}

static const touch_frame_t complete[] = {
    {0},
    TWO_POINTS,
    {.count = 2, .points = {POINT(7, 310, 405), POINT(3, 100, 200)}},
    {.count = 2, .points = {POINT(3, 100, 200), POINT(7, 310, 405)}},
    {.count = 1, .points = {POINT(7, 310, 405)}},
    {.read_result = ESP_ERR_NOT_FINISHED},
    {.getter_result = ESP_ERR_NOT_FINISHED},
    {.count = 1, .points = {POINT(7, 320, 410)}},
    {0},
};
static const touch_frame_t no_operation[]       = {{0}};
static const touch_frame_t missing_multitouch[] = {ONE_POINT, {0}};
static const touch_frame_t missing_partial[]    = {TWO_POINTS, {0}, ONE_POINT, {0}};
static const touch_frame_t missing_release[]    = {TWO_POINTS, ONE_POINT};
static const touch_frame_t no_retained_id[]     = {
    TWO_POINTS,
    {.count = 1, .points = {POINT(9, 500, 600)}},
    {0},
};
static const touch_frame_t duplicate_ids[] = {
    {.count = 2, .points = {POINT(3, 100, 200), POINT(3, 300, 400)}},
};
static const touch_frame_t invalid_count[] = {{.count = CONFIG_ESP_LCD_TOUCH_MAX_POINTS + 1}};
static const touch_frame_t read_error[]    = {{.read_result = 0x4321}};
static const touch_frame_t getter_error[]  = {{.getter_result = 0x4322}};

static const touch_frame_t *frames;
static size_t frame_count;
static unsigned read_calls, getter_calls, successful_getters, delay_calls, timer_calls, handle_calls;
static unsigned observer_calls, observer_failure_call;
static const unsigned observer_cookie = 0x1234;
static int64_t now_us;
static esp_lcd_touch_t native_touch;
static dev_lcd_touch_handles_t handles = {.touch_handle = &native_touch};

const char *esp_err_to_name(esp_err_t error)
{
    (void)error;
    return "host error";
}

int64_t esp_timer_get_time(void)
{
    assert(++timer_calls < 10000);
    return now_us;
}

void vTaskDelay(TickType_t ticks)
{
    assert(ticks == pdMS_TO_TICKS(20));
    delay_calls++;
    now_us += (int64_t)ticks * 1000;
}

static esp_err_t native_track_id(esp_lcd_touch_handle_t touch, uint8_t *ids, uint8_t count)
{
    (void)touch;
    (void)ids;
    (void)count;
    return ESP_OK;
}

esp_err_t esp_board_manager_get_device_handle(const char *name, void **out)
{
    assert(!strcmp(name, BMGR_TEST_NAME_LCD_TOUCH));
    handle_calls++;
    *out = &handles;
    return ESP_OK;
}

/* Borrowed handles must never be initialized or released by the test itself. */
esp_err_t esp_board_manager_init_device_by_name(const char *name)
{
    (void)name;
    assert(!"unexpected device init");
    return ESP_FAIL;
}

esp_err_t esp_board_manager_deinit_device_by_name(const char *name)
{
    (void)name;
    assert(!"unexpected device deinit");
    return ESP_FAIL;
}

static const touch_frame_t *current_frame(void)
{
    assert(read_calls > 0 && read_calls < 2000);
    size_t index = read_calls - 1;
    return &frames[index < frame_count ? index : frame_count - 1];
}

esp_err_t esp_lcd_touch_read_data(esp_lcd_touch_handle_t touch)
{
    assert(touch == &native_touch);
    read_calls++;
    return current_frame()->read_result;
}

esp_err_t esp_lcd_touch_get_data(esp_lcd_touch_handle_t touch, esp_lcd_touch_point_data_t *points,
                                 uint8_t *count, uint8_t capacity)
{
    assert(touch == &native_touch);
    const touch_frame_t *frame = current_frame();
    /* Calling the getter after a skipped/failed acquisition is a real defect. */
    assert(frame->read_result == ESP_OK);
    getter_calls++;
    if (frame->getter_result != ESP_OK) {
        return frame->getter_result;
    }
    *count = frame->count;
    if (frame->count > capacity) {
        /* Simulate a malformed count without corrupting the caller's buffer. */
        return ESP_OK;
    }
    successful_getters++;
    memcpy(points, frame->points, frame->count * sizeof(*points));
    return ESP_OK;
}

static esp_err_t observe_frame(const esp_lcd_touch_point_data_t *points, uint8_t count, void *user_ctx)
{
    assert(user_ctx == &observer_cookie);
    const touch_frame_t *frame = current_frame();
    assert(frame->read_result == ESP_OK && frame->getter_result == ESP_OK);
    assert(count == frame->count);
    for (uint8_t i = 0; i < count; i++) {
        assert(points[i].track_id == frame->points[i].track_id);
        assert(points[i].x == frame->points[i].x && points[i].y == frame->points[i].y);
        assert(points[i].strength == frame->points[i].strength);
        for (uint8_t j = 0; j < i; j++) {
            assert(points[i].track_id != points[j].track_id);
        }
    }
    observer_calls++;
    printf("[OBSERVE] read=%u count=%u\n", read_calls, (unsigned)count);
    return observer_calls == observer_failure_call ? 0x4323 : ESP_OK;
}

static void select_frames(const touch_frame_t *selected, size_t count)
{
    frames = selected;
    frame_count = count;
}

#define SELECT(array)  select_frames((array), sizeof(array) / sizeof((array)[0]))

int main(int argc, char **argv)
{
    assert(argc == 2);
    native_touch.get_track_id = native_track_id;
    esp_err_t expected = ESP_ERR_TIMEOUT;
    test_dev_lcd_touch_observer_t observer = observe_frame;
    if (!strcmp(argv[1], "complete") || !strcmp(argv[1], "complete_no_observer")) {
        SELECT(complete);
        expected = ESP_OK;
        if (!strcmp(argv[1], "complete_no_observer")) {
            observer = NULL;
        }
    } else if (!strcmp(argv[1], "observer_error") || !strcmp(argv[1], "observer_release_error")) {
        SELECT(complete);
        observer_failure_call = !strcmp(argv[1], "observer_error") ? 2 : 7;
        expected = 0x4323;
    } else if (!strcmp(argv[1], "no_operation")) {
        SELECT(no_operation);
    } else if (!strcmp(argv[1], "missing_multitouch")) {
        SELECT(missing_multitouch);
    } else if (!strcmp(argv[1], "missing_partial")) {
        SELECT(missing_partial);
    } else if (!strcmp(argv[1], "missing_release")) {
        SELECT(missing_release);
    } else if (!strcmp(argv[1], "no_retained_id")) {
        SELECT(no_retained_id);
    } else if (!strcmp(argv[1], "duplicate_ids")) {
        SELECT(duplicate_ids);
        expected = ESP_ERR_INVALID_RESPONSE;
    } else if (!strcmp(argv[1], "invalid_count")) {
        SELECT(invalid_count);
        expected = ESP_ERR_INVALID_RESPONSE;
    } else if (!strcmp(argv[1], "read_error")) {
        SELECT(read_error);
        expected = 0x4321;
    } else if (!strcmp(argv[1], "getter_error")) {
        SELECT(getter_error);
        expected = 0x4322;
    } else {
        assert(!strcmp(argv[1], "no_native_ids"));
        SELECT(no_operation);
        native_touch.get_track_id = NULL;
        expected = ESP_ERR_NOT_SUPPORTED;
    }

    esp_err_t actual = test_dev_lcd_touch_multitouch(observer, (void *)&observer_cookie);
    fprintf(stderr, "scenario=%s actual=0x%x expected=0x%x elapsed_us=%lld reads=%u getters=%u delays=%u\n",
            argv[1], actual, expected, (long long)now_us, read_calls, getter_calls, delay_calls);
    assert(actual == expected);
    assert(handle_calls == 1);
    if (expected == ESP_ERR_TIMEOUT) {
        assert(now_us >= 30000000 && now_us <= 30020000);
        assert(delay_calls >= 1500);
    } else {
        assert(now_us < 30000000);
    }
    if (!strcmp(argv[1], "complete") || !strcmp(argv[1], "complete_no_observer")) {
        assert(read_calls == sizeof(complete) / sizeof(complete[0]));
        assert(getter_calls == read_calls - 1);
    }
    if (observer == NULL || !strcmp(argv[1], "duplicate_ids") || !strcmp(argv[1], "invalid_count")) {
        assert(observer_calls == 0);
    } else {
        assert(observer_calls == successful_getters);
    }
    if (observer_failure_call != 0) {
        assert(observer_calls == observer_failure_call);
    }
    if (!strcmp(argv[1], "no_native_ids")) {
        assert(read_calls == 0 && getter_calls == 0);
    }
    return 0;
}
