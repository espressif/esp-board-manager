# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

"""Replay native touch snapshots through the real interactive test and case."""

import re
import shutil
import subprocess

import pytest


HEADERS = {
    'esp_err.h': '''#pragma once
#include <assert.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_NO_MEM 0x101
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_INVALID_SIZE 0x104
#define ESP_ERR_NOT_FOUND 0x105
#define ESP_ERR_NOT_SUPPORTED 0x106
#define ESP_ERR_TIMEOUT 0x107
#define ESP_ERR_INVALID_RESPONSE 0x108
#define ESP_ERR_NOT_FINISHED 0x10c
#define ESP_ERROR_CHECK(expression) assert((expression) == ESP_OK)
const char *esp_err_to_name(esp_err_t error);
''',
    'esp_log.h': '''#pragma once
#include <stdio.h>
#define ESP_LOGI(tag, ...) do { (void)(tag); printf(__VA_ARGS__); puts(""); } while (0)
#define ESP_LOGE ESP_LOGI
#define ESP_LOGW ESP_LOGI
#define ESP_LOGD ESP_LOGI
''',
    'esp_check.h': '''#pragma once
#include "esp_err.h"
#define ESP_RETURN_ON_FALSE(condition, error, tag, ...) do { if (!(condition)) return (error); } while (0)
#define ESP_RETURN_ON_ERROR(expression, tag, ...) do { esp_err_t check_ret = (expression); if (check_ret != ESP_OK) return check_ret; } while (0)
''',
    'esp_bit_defs.h': '#pragma once\n#define BIT(bit) (1U << (bit))\n',
    'esp_timer.h': '#pragma once\n#include <stdint.h>\nint64_t esp_timer_get_time(void);\n',
    'freertos/FreeRTOS.h': '''#pragma once
#include <stdint.h>
typedef unsigned TickType_t;
#define pdMS_TO_TICKS(ms) (ms)
''',
    'freertos/task.h': '#pragma once\n#include "freertos/FreeRTOS.h"\nvoid vTaskDelay(TickType_t ticks);\n',
    'esp_lcd_panel_io.h': '''#pragma once
typedef void *esp_lcd_panel_io_handle_t;
typedef struct { int unused; } esp_lcd_panel_io_i2c_config_t;
typedef struct { int unused; } esp_lcd_panel_io_spi_config_t;
''',
    'esp_lcd_touch.h': '''#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "sdkconfig.h"
typedef struct esp_lcd_touch_s esp_lcd_touch_t;
typedef esp_lcd_touch_t *esp_lcd_touch_handle_t;
typedef struct { uint8_t track_id; uint16_t x, y, strength; } esp_lcd_touch_point_data_t;
typedef struct { int unused; } esp_lcd_touch_config_t;
struct esp_lcd_touch_s {
    esp_err_t (*get_track_id)(esp_lcd_touch_handle_t, uint8_t *, uint8_t);
};
esp_err_t esp_lcd_touch_read_data(esp_lcd_touch_handle_t touch);
esp_err_t esp_lcd_touch_get_data(esp_lcd_touch_handle_t touch, esp_lcd_touch_point_data_t *points,
                               uint8_t *count, uint8_t capacity);
''',
}


def _build(tmp_path, bmgr_root, host_board_linker_flags, sources, harness, enabled=True, display=False):
    cc = shutil.which('cc')
    if cc is None:
        pytest.skip('host C compiler is unavailable')
    headers = dict(HEADERS)
    headers['sdkconfig.h'] = (
        '#pragma once\n#define CONFIG_ESP_LCD_TOUCH_MAX_POINTS 5\n'
        + ('#define CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT 1\n' if enabled else '')
        + ('#define CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT 1\n' if display else '')
    )
    for name, content in headers.items():
        path = tmp_path / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding='utf-8')
    main = tmp_path / 'harness.c'
    main.write_text(harness, encoding='utf-8')
    binary = tmp_path / 'touch'
    includes = (
        bmgr_root / 'include', bmgr_root / 'private_inc',
        bmgr_root / 'devices/dev_lcd_touch', bmgr_root / 'test_apps/main',
        bmgr_root / 'test_apps/main/framework',
    )
    command = [cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-I', str(tmp_path)]
    for directory in includes:
        command.extend(['-I', str(directory)])
    command.extend([*host_board_linker_flags, *map(str, sources), str(main), '-o', str(binary)])
    result = subprocess.run(command, capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result.stdout + result.stderr
    return binary


@pytest.fixture(scope='module')
def touch_binary(tmp_path_factory, bmgr_root, host_board_linker_flags):
    host_main = bmgr_root / 'test_apps/host_tests/lcd_touch/host_main.c'
    return _build(
        tmp_path_factory.mktemp('touch_multitouch'), bmgr_root, host_board_linker_flags,
        [bmgr_root / 'test_apps/main/test_dev_lcd_touch.c'], host_main.read_text(encoding='utf-8'),
    )


def _run_result(binary, scenario):
    result = subprocess.run([str(binary), scenario], capture_output=True, text=True, timeout=5)
    assert result.returncode == 0, result.stdout + result.stderr
    return result


def _run(binary, scenario):
    result = _run_result(binary, scenario)
    events = re.findall(r'\[TOUCH\] (DOWN|MOVE|UP) id=(\d+) x=(\d+) y=(\d+)', result.stdout)
    return [(event, int(track_id), int(x), int(y)) for event, track_id, x, y in events]


def test_multitouch_reorder_partial_release_and_skipped_frames(touch_binary):
    assert _run(touch_binary, 'complete') == [
        ('DOWN', 3, 100, 200), ('DOWN', 7, 300, 400),
        ('MOVE', 7, 310, 405), ('UP', 3, 100, 200),
        ('MOVE', 7, 320, 410), ('UP', 7, 320, 410),
    ]


def test_observer_receives_every_valid_frame_including_full_release(touch_binary):
    result = _run_result(touch_binary, 'complete')
    assert re.findall(r'\[OBSERVE\] read=(\d+) count=(\d+)', result.stdout) == [
        ('1', '0'), ('2', '2'), ('3', '2'), ('4', '2'), ('5', '1'), ('8', '1'), ('9', '0'),
    ]


def test_null_observer_keeps_serial_state_machine(touch_binary):
    assert _run(touch_binary, 'complete_no_observer') == _run(touch_binary, 'complete')


@pytest.mark.parametrize('scenario', ['observer_error', 'observer_release_error'])
def test_observer_error_is_returned_even_on_successful_release(touch_binary, scenario):
    _run(touch_binary, scenario)


@pytest.mark.parametrize('scenario', [
    'no_operation', 'missing_multitouch', 'missing_partial', 'missing_release', 'no_retained_id',
])
def test_incomplete_gesture_times_out(touch_binary, scenario):
    _run(touch_binary, scenario)


@pytest.mark.parametrize('scenario', [
    'duplicate_ids', 'invalid_count', 'read_error', 'getter_error', 'no_native_ids',
])
def test_driver_and_identity_failures(touch_binary, scenario):
    _run(touch_binary, scenario)


@pytest.mark.parametrize('enabled', [False, True])
@pytest.mark.parametrize('display', [False, True])
def test_touch_case_resource_registration_and_busy_context(
    tmp_path, bmgr_root, host_board_linker_flags, enabled, display,
):
    binary = _build(
        tmp_path, bmgr_root, host_board_linker_flags,
        [bmgr_root / 'test_apps/main/cases/bmgr_case_touch.c'], r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "bmgr_test_names.h"
#include "bmgr_test_registry.h"
#include "test_dev_lcd_touch.h"
#include "test_dev_lcd_touch_view.h"
extern void bmgr_register_touch_cases(void);
static bmgr_test_case_t registered_case;
struct test_dev_lcd_touch_view { int cookie; };
static test_dev_lcd_touch_view_t view = {.cookie = 0x1234};
static bool display_present, expect_observer;
static int registered, test_calls, name_calls, start_calls, update_calls, finish_calls, stop_calls;
static esp_err_t start_result, test_result, update_result, completed_result, stop_result;
enum { START = 1, TEST, UPDATE, FINISH, STOP };
static int sequence[8];
static size_t sequence_count;
const char *esp_err_to_name(esp_err_t error) { (void)error; return "error"; }
bool esp_board_manager_check_name(const char *name)
{
    assert(!strcmp(name, BMGR_TEST_NAME_DISPLAY_LCD));
    name_calls++;
    return display_present;
}
esp_err_t test_dev_lcd_touch_multitouch(test_dev_lcd_touch_observer_t observer, void *user_ctx)
{
    test_calls++;
    sequence[sequence_count++] = TEST;
    if (expect_observer) {
        assert(observer == test_dev_lcd_touch_view_update && user_ctx == &view);
        const esp_lcd_touch_point_data_t points[] = {
            {.track_id = 3, .x = 100, .y = 200, .strength = 1},
            {.track_id = 7, .x = 300, .y = 400, .strength = 1},
        };
        completed_result = observer(points, 2, user_ctx);
        if (completed_result != ESP_OK) {
            return completed_result;
        }
    } else {
        assert(observer == NULL && user_ctx == NULL);
    }
    completed_result = test_result;
    return completed_result;
}
esp_err_t test_dev_lcd_touch_view_start(test_dev_lcd_touch_view_t **out)
{
    assert(out != NULL);
    start_calls++;
    sequence[sequence_count++] = START;
    if (start_result == ESP_OK) {
        *out = &view;
    }
    return start_result;
}
esp_err_t test_dev_lcd_touch_view_update(const esp_lcd_touch_point_data_t *points, uint8_t count, void *user_ctx)
{
    assert(user_ctx == &view && count == 2);
    assert(points[0].track_id == 3 && points[0].x == 100 && points[0].y == 200);
    assert(points[1].track_id == 7 && points[1].x == 300 && points[1].y == 400);
    update_calls++;
    sequence[sequence_count++] = UPDATE;
    return update_result;
}
void test_dev_lcd_touch_view_finish(test_dev_lcd_touch_view_t *actual, esp_err_t result)
{
    assert(actual == &view && result == completed_result);
    finish_calls++;
    sequence[sequence_count++] = FINISH;
}
esp_err_t test_dev_lcd_touch_view_stop(test_dev_lcd_touch_view_t *actual)
{
    assert(actual == &view);
    stop_calls++;
    sequence[sequence_count++] = STOP;
    return stop_result;
}
void reset_calls(void)
{
    test_calls = name_calls = start_calls = update_calls = finish_calls = stop_calls = 0;
    sequence_count = 0;
    start_result = test_result = update_result = completed_result = stop_result = ESP_OK;
    display_present = expect_observer = false;
}
void assert_view_cleanup(void)
{
    const int expected[] = {START, TEST, UPDATE, FINISH, STOP};
    assert(name_calls == 1 && start_calls == 1 && test_calls == 1 && update_calls == 1);
    assert(finish_calls == 1 && stop_calls == 1);
    assert(sequence_count == sizeof(expected) / sizeof(expected[0]));
    assert(!memcmp(sequence, expected, sizeof(expected)));
}
esp_err_t bmgr_test_registry_add(const bmgr_test_case_t *cases, size_t count)
{
    assert(count == 1);
    registered_case = cases[0];
    registered++;
    return ESP_OK;
}
int main(void)
{
    bmgr_register_touch_cases();
#ifdef CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT
    assert(registered == 1 && registered_case.run != NULL);
    assert(!strcmp(registered_case.name, "touch.multitouch"));
    assert(!strcmp(registered_case.group, "touch"));
    assert(!strcmp(registered_case.resources[0].name, BMGR_TEST_NAME_LCD_TOUCH));
    assert(registered_case.resources[0].policy == BMGR_TEST_RESOURCE_POLICY_REQUIRED);
#ifdef CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT
    assert(!strcmp(registered_case.resources[1].name, BMGR_TEST_NAME_DISPLAY_LCD));
    assert(registered_case.resources[1].policy == BMGR_TEST_RESOURCE_POLICY_OPTIONAL);
    assert(!strcmp(registered_case.resources[2].name, BMGR_TEST_NAME_LCD_BRIGHTNESS));
    assert(registered_case.resources[2].policy == BMGR_TEST_RESOURCE_POLICY_OPTIONAL);
    assert(registered_case.resources[3].name == NULL);
#else
    assert(registered_case.resources[1].name == NULL);
#endif
    assert(registered_case.flags & BMGR_TEST_CASE_FLAG_MANUAL);
    assert(registered_case.flags & BMGR_TEST_CASE_FLAG_NEEDS_BOARD);
    assert(!(registered_case.flags & BMGR_TEST_CASE_FLAG_NEEDS_DISPLAY));
    bmgr_test_context_t context = {.lvgl_inited = true};
    assert(registered_case.run(NULL, 0, NULL) == ESP_ERR_INVALID_ARG && test_calls == 0);
    assert(registered_case.run(&context, 1, NULL) == ESP_ERR_INVALID_ARG && test_calls == 0);
    assert(registered_case.run(&context, 0, NULL) == ESP_ERR_INVALID_STATE && test_calls == 0);
    context.lvgl_inited = false;
    context.lcd_touch_inited = true;
    assert(registered_case.run(&context, 0, NULL) == ESP_ERR_INVALID_STATE && test_calls == 0);
    context.lcd_touch_inited = false;
    assert(name_calls == 0 && start_calls == 0);

    /* An absent optional display must keep the serial-only path available. */
    reset_calls();
    assert(registered_case.run(&context, 0, NULL) == ESP_OK && test_calls == 1);
    assert(start_calls == 0 && update_calls == 0 && finish_calls == 0 && stop_calls == 0);
    assert(sequence_count == 1 && sequence[0] == TEST);
#ifdef CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT
    assert(name_calls == 1);
#else
    assert(name_calls == 0);
#endif
    assert(!context.lvgl_inited && !context.lcd_touch_inited);

    reset_calls();
    test_result = 0x4321;
    assert(registered_case.run(&context, 0, NULL) == test_result && test_calls == 1);
    assert(start_calls == 0 && finish_calls == 0 && stop_calls == 0);

#ifdef CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT
    reset_calls();
    display_present = expect_observer = true;
    assert(registered_case.run(&context, 0, NULL) == ESP_OK);
    assert_view_cleanup();
    assert(!context.lvgl_inited && !context.lcd_touch_inited);

    reset_calls();
    display_present = true;
    start_result = 0x4322;
    assert(registered_case.run(&context, 0, NULL) == start_result);
    assert(name_calls == 1 && start_calls == 1 && test_calls == 0);
    assert(update_calls == 0 && finish_calls == 0 && stop_calls == 0);
    assert(sequence_count == 1 && sequence[0] == START);

    reset_calls();
    display_present = expect_observer = true;
    test_result = 0x4323;
    assert(registered_case.run(&context, 0, NULL) == test_result);
    assert_view_cleanup();

    reset_calls();
    display_present = expect_observer = true;
    test_result = 0x4323;
    stop_result = 0x4324;
    assert(registered_case.run(&context, 0, NULL) == test_result);
    assert_view_cleanup();

    reset_calls();
    display_present = expect_observer = true;
    stop_result = 0x4324;
    assert(registered_case.run(&context, 0, NULL) == stop_result);
    assert_view_cleanup();

    reset_calls();
    display_present = expect_observer = true;
    update_result = 0x4325;
    assert(registered_case.run(&context, 0, NULL) == update_result);
    assert_view_cleanup();
    assert(!context.lvgl_inited && !context.lcd_touch_inited);
#endif
#else
    assert(registered == 0 && test_calls == 0);
#endif
    return 0;
}
''', enabled=enabled, display=display,
    )
    _run(binary, '')
