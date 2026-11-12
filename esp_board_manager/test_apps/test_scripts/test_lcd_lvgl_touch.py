# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

"""Replay reports through the test app's actual LVGL touch read callback."""

import re
import shutil
import subprocess

import pytest


def _callback_source(source):
    state = re.search(r'typedef\s+struct\s*\{[^{}]*\}\s*lcd_touch_state_t\s*;', source)
    callback = re.search(
        r'static\s+void\s+lcd_touch_read_cb\s*\([^;]*?\)\s*\{', source,
    )
    assert state is not None and callback is not None, (
        'The test app has no safe LVGL touch read callback yet'
    )
    depth = 1
    end = callback.end()
    while depth and end < len(source):
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    assert depth == 0, 'Unterminated touch read callback'
    return state.group(), source[callback.start():end]


def _function_source(source, name):
    function = re.search(r'(?:static\s+)?esp_err_t\s+' + name + r'\s*\([^;]*?\)\s*\{', source)
    assert function is not None, 'Missing touch integration function: ' + name
    depth = 1
    end = function.end()
    while depth and end < len(source):
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    assert depth == 0, 'Unterminated function: ' + name
    return source[function.start():end]


HARNESS = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_NOT_FINISHED 0x10c
#define ESP_ERR_INVALID_RESPONSE 0x108
#define CONFIG_ESP_LCD_TOUCH_MAX_POINTS 2
#define TAG "LCD_INIT"
#define ESP_LOGE(tag, ...) do { (void)(tag); printf(__VA_ARGS__); puts(""); } while (0)
#define ESP_LOGW ESP_LOGE
#define ESP_LOGD ESP_LOGE
#define ESP_ERROR_CHECK(expression) assert((expression) == ESP_OK)

typedef struct { int32_t x, y; } lv_point_t;
typedef enum { LV_INDEV_STATE_RELEASED, LV_INDEV_STATE_PRESSED } lv_indev_state_t;
typedef struct { int unused; } lv_indev_t;
typedef struct {
    lv_point_t point;
    lv_indev_state_t state;
    bool continue_reading;
} lv_indev_data_t;
typedef void *esp_lcd_touch_handle_t;
typedef struct { uint8_t track_id; uint16_t x, y, strength; } esp_lcd_touch_point_data_t;

/* REAL_STATE_TYPE */
static lcd_touch_state_t touch_state;
static esp_lcd_touch_handle_t tp = (void *)0x1234;
static esp_err_t next_read_result, next_get_result;
static uint8_t next_count;
static uint16_t next_x, next_y;
static unsigned read_calls, get_calls;

static const char *esp_err_to_name(esp_err_t error)
{
    (void)error;
    return "injected error";
}

static esp_err_t esp_lcd_touch_read_data(esp_lcd_touch_handle_t touch)
{
    assert(touch == tp);
    read_calls++;
    return next_read_result;
}

static esp_err_t esp_lcd_touch_get_data(esp_lcd_touch_handle_t touch,
                                      esp_lcd_touch_point_data_t *points,
                                      uint8_t *count, uint8_t capacity)
{
    assert(touch == tp);
    assert(capacity == CONFIG_ESP_LCD_TOUCH_MAX_POINTS);
    get_calls++;
    *count = next_count;
    points[0] = (esp_lcd_touch_point_data_t) {.x = next_x, .y = next_y, .track_id = 7};
    points[1] = (esp_lcd_touch_point_data_t) {.x = 900, .y = 901, .track_id = 8};
    return next_get_result;
}

/* REAL_READ_CALLBACK */

static void reset(void)
{
    memset(&touch_state, 0, sizeof(touch_state));
    touch_state.state = LV_INDEV_STATE_RELEASED;
    next_read_result = next_get_result = ESP_OK;
    next_count = 0;
    next_x = next_y = 0;
    read_calls = get_calls = 0;
}

static void frame(esp_err_t read_result, esp_err_t get_result,
                  uint8_t count, uint16_t x, uint16_t y)
{
    next_read_result = read_result;
    next_get_result = get_result;
    next_count = count;
    next_x = x;
    next_y = y;
}

static void expect_sample(lv_indev_state_t state, int32_t x, int32_t y)
{
    lv_indev_t input = {0};
    lv_indev_data_t data = {
        .point = {.x = 777, .y = 888},
        .state = state == LV_INDEV_STATE_PRESSED ? LV_INDEV_STATE_RELEASED : LV_INDEV_STATE_PRESSED,
        .continue_reading = true,
    };
    lcd_touch_read_cb(&input, &data);
    assert(data.state == state);
    assert(data.point.x == x && data.point.y == y);
    assert(!data.continue_reading);
}

static void press(void)
{
    frame(ESP_OK, ESP_OK, 1, 10, 20);
    expect_sample(LV_INDEV_STATE_PRESSED, 10, 20);
    assert(touch_state.error == ESP_OK);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    reset();

    if (strcmp(argv[1], "initial_read_not_finished") == 0) {
        frame(ESP_ERR_NOT_FINISHED, ESP_OK, 1, 500, 600);
        expect_sample(LV_INDEV_STATE_RELEASED, 0, 0);
        assert(read_calls == 1 && get_calls == 0);
        assert(touch_state.error == ESP_OK);
    } else if (strcmp(argv[1], "initial_get_not_finished") == 0) {
        frame(ESP_OK, ESP_ERR_NOT_FINISHED, 1, 500, 600);
        expect_sample(LV_INDEV_STATE_RELEASED, 0, 0);
        assert(read_calls == 1 && get_calls == 1);
        assert(touch_state.error == ESP_OK);
    } else if (strcmp(argv[1], "press_skip_move_release") == 0) {
        press();
        frame(ESP_ERR_NOT_FINISHED, ESP_OK, 0, 0, 0);
        expect_sample(LV_INDEV_STATE_PRESSED, 10, 20);
        assert(read_calls == 2 && get_calls == 1);
        frame(ESP_OK, ESP_OK, 1, 30, 40);
        expect_sample(LV_INDEV_STATE_PRESSED, 30, 40);
        frame(ESP_OK, ESP_ERR_NOT_FINISHED, 0, 500, 600);
        expect_sample(LV_INDEV_STATE_PRESSED, 30, 40);
        frame(ESP_OK, ESP_OK, 0, 0, 0);
        expect_sample(LV_INDEV_STATE_RELEASED, 30, 40);
        frame(ESP_ERR_NOT_FINISHED, ESP_OK, 1, 800, 900);
        expect_sample(LV_INDEV_STATE_RELEASED, 30, 40);
        assert(touch_state.error == ESP_OK);
    } else if (strcmp(argv[1], "first_of_multiple_points") == 0) {
        frame(ESP_OK, ESP_OK, 2, 100, 200);
        expect_sample(LV_INDEV_STATE_PRESSED, 100, 200);
        assert(touch_state.error == ESP_OK);
    } else if (strcmp(argv[1], "read_error") == 0 || strcmp(argv[1], "get_error") == 0) {
        press();
        bool read_error = strcmp(argv[1], "read_error") == 0;
        frame(read_error ? 0x4321 : ESP_OK, read_error ? ESP_OK : 0x4321, 0, 0, 0);
        expect_sample(LV_INDEV_STATE_PRESSED, 10, 20);
        assert(touch_state.error == 0x4321);
        assert(read_calls == 2 && get_calls == (read_error ? 1U : 2U));
        unsigned reads = read_calls, gets = get_calls;
        frame(ESP_OK, ESP_OK, 0, 0, 0);
        expect_sample(LV_INDEV_STATE_PRESSED, 10, 20);
        assert(touch_state.error == 0x4321);
        assert(read_calls == reads && get_calls == gets);
    } else if (strcmp(argv[1], "invalid_count") == 0) {
        press();
        frame(ESP_OK, ESP_OK, CONFIG_ESP_LCD_TOUCH_MAX_POINTS + 1, 500, 600);
        expect_sample(LV_INDEV_STATE_PRESSED, 10, 20);
        assert(touch_state.error == ESP_ERR_INVALID_RESPONSE);
    } else if (strcmp(argv[1], "fresh_session") == 0) {
        press();
        frame(0x4321, ESP_OK, 0, 0, 0);
        expect_sample(LV_INDEV_STATE_PRESSED, 10, 20);
        assert(touch_state.error == 0x4321);
        reset();
        frame(ESP_ERR_NOT_FINISHED, ESP_OK, 0, 0, 0);
        expect_sample(LV_INDEV_STATE_RELEASED, 0, 0);
        assert(touch_state.error == ESP_OK);
        press();
        assert(read_calls == 2 && get_calls == 1);
    } else {
        assert(!"unknown scenario");
    }
    return 0;
}
'''


@pytest.fixture(scope='module')
def lvgl_touch_binary(tmp_path_factory, bmgr_root, host_board_linker_flags):
    cc = shutil.which('cc')
    if cc is None:
        pytest.skip('host C compiler is unavailable')
    source = (bmgr_root / 'test_apps/main/test_dev_lcd_init.c').read_text(encoding='utf-8')
    state, callback = _callback_source(source)
    harness = HARNESS.replace('/* REAL_STATE_TYPE */', state).replace(
        '/* REAL_READ_CALLBACK */', callback,
    )
    directory = tmp_path_factory.mktemp('lcd_lvgl_touch')
    main = directory / 'main.c'
    main.write_text(harness, encoding='utf-8')
    binary = directory / 'lcd_lvgl_touch'
    result = subprocess.run(
        [cc, '-std=c11', '-Wall', '-Wextra', '-Werror', *host_board_linker_flags,
         str(main), '-o', str(binary)],
        capture_output=True, text=True, timeout=30,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return binary


@pytest.mark.parametrize('scenario', [
    'initial_read_not_finished', 'initial_get_not_finished', 'press_skip_move_release',
    'first_of_multiple_points', 'read_error', 'get_error', 'invalid_count', 'fresh_session',
])
def test_lvgl_touch_report_handling(lvgl_touch_binary, scenario):
    result = subprocess.run(
        [str(lvgl_touch_binary), scenario], capture_output=True, text=True, timeout=5,
    )
    assert result.returncode == 0, result.stdout + result.stderr


INTEGRATION = r'''
#define CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT 1
#define LVGL_VERSION_MAJOR 9
#define ESP_FAIL (-1)
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_TIMEOUT 0x107
#define BMGR_TEST_NAME_LCD_TOUCH "lcd_touch"
#define ESP_LOGI ESP_LOGE
typedef int lv_display_t;
typedef struct { esp_lcd_touch_handle_t touch_handle; } dev_lcd_touch_handles_t;
typedef struct { lv_display_t *disp; esp_lcd_touch_handle_t handle; } lvgl_port_touch_cfg_t;
static int display;
static lv_display_t *disp = &display;
static void *touch_handle;
static lv_indev_t *touch_indev;
static lv_indev_t registered_input;
static void (*registered_read)(lv_indev_t *, lv_indev_data_t *);
static dev_lcd_touch_handles_t board_touch = {.touch_handle = (void *)0x1234};
static int lock_depth, lock_calls, unlock_calls;
static unsigned pending_calls, remove_calls;
static bool pending_read, registered, fail_lock, fail_add;
static bool board_has_touch = true;
static esp_err_t remove_result;

static bool lvgl_port_lock(unsigned timeout)
{
    assert(timeout == 0);
    lock_calls++;
    if (fail_lock) {
        fail_lock = false;
        return false;
    }
    lock_depth++;
    return true;
}

static void lvgl_port_unlock(void)
{
    assert(lock_depth > 0);
    unlock_calls++;
    lock_depth--;
    if (lock_depth == 0 && pending_read) {
        pending_read = false;
        pending_calls++;
        lv_indev_data_t data = {0};
        /* A ready input can run as soon as registration releases the lock. */
        assert(registered_read == lcd_touch_read_cb);
        registered_read(&registered_input, &data);
    }
}

static void original_fatal_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    (void)data;
    assert(!"the port's fatal read callback ran before replacement");
}

static lv_indev_t *lvgl_port_add_touch(const lvgl_port_touch_cfg_t *cfg)
{
    assert(cfg->disp == disp && cfg->handle == board_touch.touch_handle);
    assert(lvgl_port_lock(0));
    if (fail_add) {
        lvgl_port_unlock();
        return NULL;
    }
    registered = true;
    registered_read = original_fatal_read;
    pending_read = true;
    lvgl_port_unlock();
    return &registered_input;
}

static void lv_indev_set_read_cb(lv_indev_t *indev,
                               void (*callback)(lv_indev_t *, lv_indev_data_t *))
{
    assert(indev == &registered_input && registered && lock_depth > 0);
    registered_read = callback;
}

static esp_err_t lvgl_port_remove_touch(lv_indev_t *indev)
{
    assert(indev == &registered_input && registered);
    assert(lvgl_port_lock(0));
    remove_calls++;
    if (remove_result == ESP_OK) {
        registered = false;
        pending_read = false;
        registered_read = NULL;
    }
    lvgl_port_unlock();
    return remove_result;
}

static bool esp_board_manager_check_name(const char *name)
{
    assert(strcmp(name, BMGR_TEST_NAME_LCD_TOUCH) == 0);
    return board_has_touch;
}

static esp_err_t esp_board_manager_get_device_handle(const char *name, void **handle)
{
    assert(strcmp(name, BMGR_TEST_NAME_LCD_TOUCH) == 0);
    *handle = &board_touch;
    return ESP_OK;
}

/* REAL_TOUCH_LIFECYCLE */

int main(int argc, char **argv)
{
    assert(argc == 2);
    /* A new run must discard both a stale press and an old latched error. */
    touch_state = (lcd_touch_state_t) {
        .point = {.x = 700, .y = 800}, .state = LV_INDEV_STATE_PRESSED, .error = 0x4321,
    };
    frame(ESP_ERR_NOT_FINISHED, ESP_OK, 0, 0, 0);

    if (strcmp(argv[1], "registration_and_restart") == 0) {
        assert(test_dev_lcd_touch_init() == ESP_OK);
        assert(lock_depth == 0 && lock_calls == unlock_calls);
        assert(pending_calls == 1 && registered);
        assert(touch_state.state == LV_INDEV_STATE_RELEASED);
        assert(touch_state.point.x == 0 && touch_state.point.y == 0);
        assert(test_dev_lcd_touch_get_error() == ESP_OK);
        frame(0x4322, ESP_OK, 0, 0, 0);
        expect_sample(LV_INDEV_STATE_RELEASED, 0, 0);
        assert(test_dev_lcd_touch_get_error() == 0x4322);
        assert(test_dev_lcd_touch_deinit() == ESP_OK);
        assert(touch_indev == NULL && tp == NULL && touch_handle == NULL && !registered);
        assert(remove_calls == 1 && lock_depth == 0 && lock_calls == unlock_calls);
        assert(test_dev_lcd_touch_get_error() == ESP_OK);
        frame(ESP_ERR_NOT_FINISHED, ESP_OK, 0, 0, 0);
        assert(test_dev_lcd_touch_init() == ESP_OK);
        assert(test_dev_lcd_touch_get_error() == ESP_OK && pending_calls == 2);
        assert(touch_state.state == LV_INDEV_STATE_RELEASED);
        assert(test_dev_lcd_touch_deinit() == ESP_OK);
    } else if (strcmp(argv[1], "registration_lock_failure") == 0) {
        fail_lock = true;
        assert(test_dev_lcd_touch_init() == ESP_ERR_TIMEOUT);
        assert(!registered && touch_indev == NULL && lock_depth == 0);
        assert(unlock_calls == 0 && pending_calls == 0);
    } else if (strcmp(argv[1], "registration_add_failure") == 0) {
        fail_add = true;
        assert(test_dev_lcd_touch_init() == ESP_FAIL);
        assert(!registered && touch_indev == NULL && lock_depth == 0);
        assert(lock_calls == unlock_calls && pending_calls == 0);
    } else if (strcmp(argv[1], "remove_failure") == 0) {
        assert(test_dev_lcd_touch_init() == ESP_OK);
        remove_result = 0x4323;
        assert(test_dev_lcd_touch_deinit() == 0x4323);
        assert(registered && touch_indev != NULL && tp != NULL && touch_handle != NULL);
        assert(lock_depth == 0 && lock_calls == unlock_calls);
        remove_result = ESP_OK;
        assert(test_dev_lcd_touch_deinit() == ESP_OK);
        assert(!registered && touch_indev == NULL && tp == NULL && touch_handle == NULL);
    } else if (strcmp(argv[1], "no_touch") == 0) {
        board_has_touch = false;
        assert(test_dev_lcd_touch_init() == ESP_FAIL);
        assert(test_dev_lcd_touch_get_error() == ESP_OK);
        assert(test_dev_lcd_touch_deinit() == ESP_OK);
        assert(lock_calls == 0 && remove_calls == 0);
    } else {
        assert(!"unknown integration scenario");
    }
    assert(lock_depth == 0);
    return 0;
}
'''


@pytest.fixture(scope='module')
def lvgl_touch_integration_binary(tmp_path_factory, bmgr_root, host_board_linker_flags):
    source = (bmgr_root / 'test_apps/main/test_dev_lcd_init.c').read_text(encoding='utf-8')
    state, callback = _callback_source(source)
    # Reuse the same mocks and real callback, but replace the unit-test main.
    unit_main = HARNESS.index('int main(int argc, char **argv)')
    harness = HARNESS[:unit_main].replace('/* REAL_STATE_TYPE */', state).replace(
        '/* REAL_READ_CALLBACK */', callback,
    )
    lifecycle = '\n\n'.join(_function_source(source, name) for name in (
        'test_dev_lcd_touch_init', 'test_dev_lcd_touch_get_error', 'test_dev_lcd_touch_deinit',
    ))
    harness += INTEGRATION.replace('/* REAL_TOUCH_LIFECYCLE */', lifecycle)
    directory = tmp_path_factory.mktemp('lcd_lvgl_touch_integration')
    main = directory / 'main.c'
    main.write_text(harness, encoding='utf-8')
    binary = directory / 'lcd_lvgl_touch_integration'
    result = subprocess.run(
        [shutil.which('cc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
         '-Wno-unused-function', *host_board_linker_flags, str(main), '-o', str(binary)],
        capture_output=True, text=True, timeout=30,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return binary


@pytest.mark.parametrize('scenario', [
    'registration_and_restart', 'registration_lock_failure', 'registration_add_failure',
    'remove_failure', 'no_touch',
])
def test_lvgl_touch_lifecycle(lvgl_touch_integration_binary, scenario):
    result = subprocess.run(
        [str(lvgl_touch_integration_binary), scenario],
        capture_output=True, text=True, timeout=5,
    )
    assert result.returncode == 0, result.stdout + result.stderr


CASE_HARNESS = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
typedef int esp_err_t;
typedef struct { int unused; } bmgr_test_context_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_TIMEOUT 0x107
#define TAG "LCD_CASE"
#define ESP_LOGW(tag, ...) do { (void)(tag); printf(__VA_ARGS__); puts(""); } while (0)
#define pdMS_TO_TICKS(ms) (ms)
static bool touch_present = true;
static esp_err_t menu_result, read_result;
static unsigned lock_depth, lock_epoch, query_epoch;
static unsigned menu_calls, query_calls, touch_cleanup_calls, lcd_cleanup_calls;

static const char *esp_err_to_name(esp_err_t error) { (void)error; return "injected error"; }
static esp_err_t test_dev_lcd_auto_clear(void) { return ESP_OK; }
static esp_err_t test_dev_lcd_lvgl_init(void) { return ESP_OK; }
static void *lv_disp_get_default(void) { return (void *)1; }
static void *lv_scr_act(void) { return (void *)1; }
static esp_err_t test_dev_lcd_touch_init(void) { return touch_present ? ESP_OK : ESP_FAIL; }
static void vTaskDelay(unsigned ticks) { (void)ticks; }
static esp_err_t test_dev_lcd_lvgl_show_menu(void) { menu_calls++; return menu_result; }
static bool lvgl_port_lock(unsigned timeout)
{
    assert(timeout == 0);
    lock_depth++;
    lock_epoch++;
    return true;
}
static void lvgl_port_unlock(void) { assert(lock_depth > 0); lock_depth--; }
static esp_err_t test_dev_lcd_touch_get_error(void)
{
    assert(lock_depth > 0);
    query_epoch = lock_epoch;
    query_calls++;
    return read_result;
}
static esp_err_t test_dev_lcd_touch_deinit(void)
{
    assert(lock_depth > 0 && query_epoch == lock_epoch);
    touch_cleanup_calls++;
    return ESP_OK;
}
static esp_err_t test_dev_lcd_lvgl_deinit(void)
{
    assert(lock_depth == 0);
    lcd_cleanup_calls++;
    return ESP_OK;
}

/* REAL_LCD_CASE */

int main(int argc, char **argv)
{
    assert(argc == 2);
    esp_err_t expected = ESP_OK;
    if (strcmp(argv[1], "read_error") == 0) {
        read_result = expected = 0x4321;
    } else if (strcmp(argv[1], "read_error_and_ui_failure") == 0) {
        read_result = expected = 0x4321;
        menu_result = ESP_FAIL;
    } else if (strcmp(argv[1], "manual_failure") == 0) {
        menu_result = expected = ESP_FAIL;
    } else if (strcmp(argv[1], "no_touch") == 0) {
        touch_present = false;
        read_result = 0x4321;  /* An absent optional input must not be queried. */
    } else {
        assert(strcmp(argv[1], "success") == 0);
    }
    assert(run_lcd_lvgl(NULL, 0, NULL) == expected);
    assert(menu_calls == 1 && lcd_cleanup_calls == 1 && lock_depth == 0);
    assert(query_calls == (touch_present ? 1U : 0U));
    assert(touch_cleanup_calls == (touch_present ? 1U : 0U));
    return 0;
}
'''


@pytest.fixture(scope='module')
def lcd_case_binary(tmp_path_factory, bmgr_root, host_board_linker_flags):
    source = (bmgr_root / 'test_apps/main/cases/bmgr_case_lcd.c').read_text(encoding='utf-8')
    harness = CASE_HARNESS.replace('/* REAL_LCD_CASE */', _function_source(source, 'run_lcd_lvgl'))
    directory = tmp_path_factory.mktemp('lcd_lvgl_case')
    main = directory / 'main.c'
    main.write_text(harness, encoding='utf-8')
    binary = directory / 'lcd_lvgl_case'
    result = subprocess.run(
        [shutil.which('cc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
         *host_board_linker_flags, str(main), '-o', str(binary)],
        capture_output=True, text=True, timeout=30,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return binary


@pytest.mark.parametrize('scenario', [
    'read_error', 'read_error_and_ui_failure', 'manual_failure', 'no_touch', 'success',
])
def test_lcd_case_returns_touch_error_after_cleanup(lcd_case_binary, scenario):
    result = subprocess.run(
        [str(lcd_case_binary), scenario], capture_output=True, text=True, timeout=5,
    )
    assert result.returncode == 0, result.stdout + result.stderr
