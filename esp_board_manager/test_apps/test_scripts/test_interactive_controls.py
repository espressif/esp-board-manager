# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

"""Execute interactive controls on the host with fake board/driver boundaries."""

import shutil
import subprocess
import os
from pathlib import Path

import pytest


def _build(tmp_path, bmgr_root, host_board_linker_flags, sources, headers, harness, extra_flags=()):
    cc = shutil.which('cc')
    if cc is None:
        pytest.skip('host C compiler is unavailable')
    for name, content in headers.items():
        path = tmp_path / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding='utf-8')
    main = tmp_path / 'harness.c'
    main.write_text(harness, encoding='utf-8')
    binary = tmp_path / 'controls'
    result = subprocess.run(
        [cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-I', str(tmp_path),
         '-I', str(bmgr_root / 'include'), '-I', str(bmgr_root / 'private_inc'),
         '-I', str(bmgr_root / 'devices/dev_button'),
         *host_board_linker_flags,
         *extra_flags,
         *map(str, sources), str(main), '-o', str(binary)],
        capture_output=True, text=True,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return binary


ERRORS = '''#pragma once
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG 1
#define ESP_ERR_NO_MEM 2
#define ESP_ERR_NOT_FOUND 3
#define ESP_ERR_NOT_SUPPORTED 4
#define ESP_ERR_INVALID_STATE 5
const char *esp_err_to_name(esp_err_t error);
'''
LOGS = '''#pragma once
#include <stdio.h>
#define ESP_LOGI(tag, ...) do { (void)(tag); printf(__VA_ARGS__); puts(""); } while (0)
#define ESP_LOGE ESP_LOGI
#define ESP_LOGW ESP_LOGI
'''


def test_button_selection_and_rollback(tmp_path, bmgr_root, host_board_linker_flags):
    source = bmgr_root / 'test_apps/main/test_dev_button.c'
    headers = {
        'esp_err.h': ERRORS,
        'esp_log.h': LOGS,
        'freertos/FreeRTOS.h': '#pragma once\ntypedef unsigned TickType_t;\n#define pdMS_TO_TICKS(ms) (ms)\n#define portMAX_DELAY (~0U)\n#define pdTRUE 1\n',
        'freertos/task.h': '#pragma once\nvoid vTaskDelay(unsigned ticks);\nunsigned xTaskGetTickCount(void);\n',
        'freertos/semphr.h': '''#pragma once
#include "freertos/FreeRTOS.h"
typedef void *SemaphoreHandle_t;
SemaphoreHandle_t xSemaphoreCreateBinary(void);
int xSemaphoreTake(SemaphoreHandle_t, TickType_t);
int xSemaphoreGive(SemaphoreHandle_t);
void vSemaphoreDelete(SemaphoreHandle_t);
''',
        'esp_timer.h': '''#pragma once
#include <stdint.h>
#include "esp_err.h"
typedef void *esp_timer_handle_t;
typedef struct { void (*callback)(void *); void *arg; int dispatch_method; const char *name; } esp_timer_create_args_t;
#define ESP_TIMER_TASK 0
esp_err_t esp_timer_create(const esp_timer_create_args_t *, esp_timer_handle_t *);
esp_err_t esp_timer_start_once(esp_timer_handle_t, uint64_t);
esp_err_t esp_timer_delete(esp_timer_handle_t);
''',
        'iot_button.h': '''#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
typedef struct { uint16_t long_press_time; uint16_t short_press_time; } button_config_t;
typedef struct button_driver button_driver_t;
typedef void *button_handle_t;
#define CONFIG_ADC_BUTTON_MAX_BUTTON_PER_CHANNEL 4
typedef enum { BUTTON_PRESS_DOWN, BUTTON_PRESS_UP, BUTTON_PRESS_REPEAT, BUTTON_PRESS_REPEAT_DONE,
    BUTTON_SINGLE_CLICK, BUTTON_DOUBLE_CLICK, BUTTON_MULTIPLE_CLICK, BUTTON_LONG_PRESS_START,
    BUTTON_LONG_PRESS_HOLD, BUTTON_LONG_PRESS_UP, BUTTON_PRESS_END, BUTTON_EVENT_MAX,
    BUTTON_NONE_PRESS } button_event_t;
typedef void (*button_cb_t)(void *, void *);
size_t iot_button_count_event_cb(button_handle_t handle, button_event_t event);
esp_err_t iot_button_register_cb(button_handle_t, button_event_t, void *, button_cb_t, void *);
esp_err_t iot_button_unregister_cb(button_handle_t, button_event_t, void *);
button_event_t iot_button_get_event(button_handle_t handle);
const char *iot_button_get_event_str(button_event_t event);
''',
    }
    binary = _build(tmp_path, bmgr_root, host_board_linker_flags, [source], headers, r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "dev_button.h"
#include "esp_board_device.h"
#include "esp_timer.h"
#include "freertos/semphr.h"
extern esp_err_t test_dev_button_named(const char *name);
static const char *names[] = {"boot_button", "key0_button", "key1_button"};
static dev_button_handles_t handles[3];
static int acquired[3], released[3], polled[3], fail_at = -1;
static button_cb_t callbacks[3][BUTTON_EVENT_MAX];
static void *callback_data[3][BUTTON_EVENT_MAX];
static button_event_t current_event;
static int register_calls, fail_register_at, fail_after_insert;
static unsigned ticks;
static esp_timer_create_args_t timer_args;
static int in_timer_task, timer_pending, sem_given, timer_created, fail_timer_create, fail_timer_start;
static int fail_sem_create, start_calls;
static int pending_button_dispatch, drained_dispatches;
SemaphoreHandle_t xSemaphoreCreateBinary(void) { return fail_sem_create ? NULL : &sem_given; }
void vSemaphoreDelete(SemaphoreHandle_t sem) { assert(sem == &sem_given && !timer_pending); }
int xSemaphoreGive(SemaphoreHandle_t sem) {
    assert(sem == &sem_given && in_timer_task); sem_given = 1; return 1;
}
int xSemaphoreTake(SemaphoreHandle_t sem, TickType_t timeout) {
    assert(sem == &sem_given && timeout == portMAX_DELAY && timer_pending);
    /* An earlier button scan must finish before the queued task-dispatch callback. */
    if (pending_button_dispatch) { pending_button_dispatch = 0; drained_dispatches++; }
    timer_pending = 0; in_timer_task = 1;
    timer_args.callback(timer_args.arg);
    in_timer_task = 0;
    assert(sem_given); sem_given = 0; return 1;
}
esp_err_t esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *out) {
    if (fail_timer_create) return ESP_ERR_NO_MEM;
    assert(args->dispatch_method == ESP_TIMER_TASK && !timer_created);
    timer_args = *args; *out = &timer_args; timer_created = 1; return ESP_OK;
}
esp_err_t esp_timer_start_once(esp_timer_handle_t timer, uint64_t delay) {
    assert(timer == &timer_args && delay && !timer_pending);
    if (++start_calls == fail_timer_start) return ESP_FAIL;
    timer_pending = 1; return ESP_OK;
}
esp_err_t esp_timer_delete(esp_timer_handle_t timer) {
    assert(timer == &timer_args && !timer_pending); timer_created = 0; return ESP_OK;
}
static int index_of(const char *name) {
    for (int i = 0; i < 3; ++i) if (!strcmp(name, names[i])) return i;
    assert(0); return -1;
}
const char *esp_err_to_name(int error) { (void)error; return "error"; }
void vTaskDelay(unsigned delay) {
    assert(delay > 0); ticks += delay;
    assert(!in_timer_task); pending_button_dispatch = 1;
    /* Emit several transitions in one timer dispatch: polling sees only PRESS_END. */
    const button_event_t events[] = { BUTTON_SINGLE_CLICK, BUTTON_DOUBLE_CLICK, BUTTON_LONG_PRESS_UP, BUTTON_PRESS_END };
    for (int i = 0; i < 3; i++) {
        for (unsigned j = 0; j < sizeof(events)/sizeof(events[0]); j++) {
            current_event = events[j];
            if (callbacks[i][current_event]) {
                polled[i]++;
                callbacks[i][current_event]((void *)(uintptr_t)(i + 1), callback_data[i][current_event]);
            }
        }
    }
}
unsigned xTaskGetTickCount(void) { return ticks; }
esp_err_t esp_board_device_iterate_name_by_type(const char *type, void **cursor, const char **name) {
    assert(!strcmp(type, "button"));
    uintptr_t index = (uintptr_t)*cursor;
    if (index == 3) { *name = NULL; return ESP_BOARD_ERR_DEVICE_NOT_FOUND; }
    *name = names[index]; *cursor = (void *)(index + 1); return ESP_OK;
}
esp_err_t esp_board_device_init(const char *name) {
    int i = index_of(name);
    if (i == fail_at) return ESP_FAIL;
    acquired[i]++; return ESP_OK;
}
esp_err_t esp_board_device_deinit(const char *name) {
    assert(in_timer_task && !pending_button_dispatch);
    released[index_of(name)]++; return ESP_OK;
}
esp_err_t esp_board_device_get_handle(const char *name, void **out) {
    *out = &handles[index_of(name)]; return ESP_OK;
}
button_event_t iot_button_get_event(button_handle_t handle) {
    (void)handle; return current_event;
}
const char *iot_button_get_event_str(button_event_t event) {
    if (event == BUTTON_SINGLE_CLICK) return "SINGLE_CLICK";
    if (event == BUTTON_DOUBLE_CLICK) return "DOUBLE_CLICK";
    if (event == BUTTON_LONG_PRESS_UP) return "LONG_PRESS_UP";
    return "PRESS_END";
}
size_t iot_button_count_event_cb(button_handle_t handle, button_event_t event) {
    return callbacks[(uintptr_t)handle - 1][event] != NULL;
}
esp_err_t iot_button_register_cb(button_handle_t handle, button_event_t event, void *args, button_cb_t cb, void *data) {
    int i = (uintptr_t)handle - 1; (void)args;
    assert(in_timer_task && !pending_button_dispatch);
    if (++register_calls == fail_register_at && !fail_after_insert) return ESP_FAIL;
    assert(!callbacks[i][event]); callbacks[i][event] = cb; callback_data[i][event] = data;
    if (register_calls == fail_register_at) return ESP_FAIL;
    return ESP_OK;
}
esp_err_t iot_button_unregister_cb(button_handle_t handle, button_event_t event, void *args) {
    int i = (uintptr_t)handle - 1; (void)args;
    assert(in_timer_task && !pending_button_dispatch);
    assert(callbacks[i][event]); callbacks[i][event] = NULL; return ESP_OK;
}
static void existing_callback(void *handle, void *data) { (void)handle; (void)data; }
static void assert_no_callbacks(void) {
    for (int i = 0; i < 3; i++) for (int e = 0; e < BUTTON_EVENT_MAX; e++) assert(!callbacks[i][e]);
}
static void reset(void) {
    assert(!timer_created && !timer_pending);
    in_timer_task = sem_given = fail_timer_create = fail_timer_start = 0;
    fail_sem_create = start_calls = 0;
    pending_button_dispatch = 1; drained_dispatches = 0;
    memset(acquired, 0, sizeof(acquired)); memset(released, 0, sizeof(released));
    memset(polled, 0, sizeof(polled)); fail_at = -1;
    memset(callbacks, 0, sizeof(callbacks)); register_calls = 0; fail_register_at = -1; fail_after_insert = 0;
    for (int i = 0; i < 3; ++i) { handles[i].num_buttons = 1; handles[i].button_handles[0] = (void *)(uintptr_t)(i + 1); }
}
int main(void) {
    reset(); assert(test_dev_button_named(NULL) == ESP_OK);
    for (int i = 0; i < 3; ++i) assert(acquired[i] == 1 && released[i] == 1 && polled[i] > 0);
    assert_no_callbacks(); assert(drained_dispatches == 2);
    reset(); fail_sem_create = 1;
    assert(test_dev_button_named(NULL) == ESP_ERR_NO_MEM);
    assert(!acquired[0] && !acquired[1] && !acquired[2]);
    reset(); fail_timer_create = 1;
    assert(test_dev_button_named(NULL) == ESP_ERR_NO_MEM);
    assert(!acquired[0] && !acquired[1] && !acquired[2]);
    reset(); fail_timer_start = 1;
    assert(test_dev_button_named(NULL) == ESP_FAIL);
    assert_no_callbacks();
    for (int i = 0; i < 3; i++) assert(released[i] == 1);
    reset(); fail_timer_start = 2;
    assert(test_dev_button_named(NULL) == ESP_FAIL);
    /* If cleanup cannot be dispatched, keep live driver references and callbacks intact. */
    for (int i = 0; i < 3; i++) assert(acquired[i] == 1 && released[i] == 0 && callbacks[i][BUTTON_SINGLE_CLICK]);
    reset(); assert(test_dev_button_named("key1_button") == ESP_OK);
    assert(!acquired[0] && !acquired[1] && acquired[2] == 1 && released[2] == 1);
    reset(); assert(test_dev_button_named("missing") == ESP_ERR_NOT_FOUND);
    assert(!acquired[0] && !acquired[1] && !acquired[2]);
    reset(); fail_at = 1; assert(test_dev_button_named(NULL) == ESP_FAIL);
    assert(released[0] == 1 && !released[1] && !released[2]);
    reset(); handles[1].num_buttons = 0;
    assert(test_dev_button_named(NULL) != ESP_OK);
    assert(released[0] == 1 && released[1] == 1 && !released[2]);
    assert_no_callbacks();
    reset(); fail_register_at = 12;
    assert(test_dev_button_named(NULL) == ESP_FAIL);
    assert_no_callbacks();
    for (int i = 0; i < 3; i++) assert(released[i] == 1);
    reset(); fail_register_at = 5; fail_after_insert = 1;
    assert(test_dev_button_named(NULL) == ESP_FAIL);
    assert_no_callbacks();
    for (int i = 0; i < 3; i++) assert(released[i] == 1);
    reset(); callbacks[1][BUTTON_SINGLE_CLICK] = existing_callback;
    assert(test_dev_button_named(NULL) == ESP_ERR_INVALID_STATE);
    assert(callbacks[1][BUTTON_SINGLE_CLICK] == existing_callback && register_calls == 0);
    assert(released[0] == 1 && released[1] == 1 && !released[2]);
    return 0;
}
''')
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'key0_button' in result.stdout and 'key1_button' in result.stdout
    for event in ('SINGLE_CLICK', 'DOUBLE_CLICK', 'LONG_PRESS_UP'):
        assert event in result.stdout


@pytest.mark.parametrize('wrapper_enabled', [0, 1])
def test_io_dispatch_and_invalid_arguments(tmp_path, bmgr_root, wrapper_enabled, host_board_linker_flags):
    source = bmgr_root / 'test_apps/main/commands/cmd_io.c'
    headers = {
        'esp_err.h': ERRORS,
        'esp_log.h': LOGS,
        'sdkconfig.h': f'#pragma once\n#define CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER {wrapper_enabled}\n',
        'esp_console.h': '''#pragma once
#include "esp_err.h"
typedef struct { const char *command; const char *help; const char *hint;
    int (*func)(int, char **); void *argtable; } esp_console_cmd_t;
esp_err_t esp_console_cmd_register(const esp_console_cmd_t *);
esp_err_t esp_console_cmd_deregister(const char *);
''',
        'driver/gpio.h': '''#pragma once
#include "esp_err.h"
typedef int gpio_num_t;
#define GPIO_MODE_INPUT_OUTPUT 3
#define GPIO_MODE_OUTPUT 2
#define GPIO_NUM_MAX 49
#define GPIO_IS_VALID_GPIO(pin) ((pin) >= 0 && (pin) < 49)
#define GPIO_IS_VALID_OUTPUT_GPIO(pin) GPIO_IS_VALID_GPIO(pin)
esp_err_t gpio_set_direction(gpio_num_t, int);
esp_err_t gpio_set_level(gpio_num_t, unsigned);
int gpio_get_level(gpio_num_t);
''',
    }
    binary = _build(tmp_path, bmgr_root, host_board_linker_flags, [source], headers, r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "esp_console.h"
#include "sdkconfig.h"
#include "esp_board_device.h"
extern esp_err_t register_io_command(void);
extern void unregister_io_command(void);
static int (*command)(int, char **);
static int native_writes, native_level, custom_writes, custom_level, active = 1;
static int token, virtual_writes, virtual_active = 1;
const char *esp_err_to_name(int error) { (void)error; return "error"; }
esp_err_t esp_console_cmd_register(const esp_console_cmd_t *cmd) { command = cmd->func; return ESP_OK; }
esp_err_t esp_console_cmd_deregister(const char *name) { assert(!strcmp(name,"io")); return ESP_OK; }
esp_err_t gpio_set_direction(int pin, int mode) {
    if (pin >= 49) { assert(mode == 2); return virtual_active && pin == 51 ? ESP_OK : ESP_ERR_INVALID_ARG; }
    assert(pin == 1 && mode == 3); return ESP_OK;
}
esp_err_t gpio_set_level(int pin, unsigned value) {
    if (pin >= 49) { assert(pin == 51); virtual_writes++; return ESP_OK; }
    assert(pin == 1); native_writes++; native_level = value; return ESP_OK;
}
int gpio_get_level(int pin) { if (pin >= 49) return virtual_active && pin == 51 ? 0 : -1; assert(pin == 1); return native_level; }
esp_err_t esp_board_device_get_handle(const char *name, void **out) {
    if (!active || strcmp(name,"xl9555")) return ESP_ERR_NOT_FOUND;
    *out = &token; return ESP_OK;
}
static esp_err_t custom_set(void *handle, int pin, int value) {
    assert(handle == &token && pin == 2); custom_writes++; custom_level = value; return ESP_OK;
}
static esp_err_t custom_get(void *handle, int pin, int *value) {
    assert(handle == &token && pin == 2); *value = custom_level; return ESP_OK;
}
DEVICE_EXTRA_FUNC_REGISTER(xl9555_io_set, custom_set);
DEVICE_EXTRA_FUNC_REGISTER(xl9555_io_get, custom_get);
int main(void) {
    assert(register_io_command() == ESP_OK && command);
    char *set_native[] = {"io","set","gpio","1","1"};
    assert(command(5,set_native) == 0 && native_writes == 1 && native_level == 1);
    char *get_native[] = {"io","get","gpio","1"}; assert(command(4,get_native) == 0);
    char *set_custom[] = {"io","set","xl9555","2","0"};
    assert(command(5,set_custom) == 0 && custom_writes == 1 && custom_level == 0);
    char *get_custom[] = {"io","get","xl9555","2"}; assert(command(4,get_custom) == 0);
    char *bad_pin[] = {"io","set","gpio","1x","1"}; assert(command(5,bad_pin) != 0);
    char *bad_level[] = {"io","set","gpio","1","2"}; assert(command(5,bad_level) != 0);
    char *negative[] = {"io","set","gpio","-1","0"}; assert(command(5,negative) != 0);
    assert(command(4,set_native) != 0 && command(5,get_native) != 0);
    active = 0; assert(command(5,set_custom) != 0 && custom_writes == 1);
    char *set_virtual[] = {"io","set","gpio","51","0"};
    char *get_virtual[] = {"io","get","gpio","51"};
#if CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER
    assert(command(5,set_virtual) == 0 && virtual_writes == 1);
    assert(command(4,get_virtual) == 0);
    virtual_active = 0;
    assert(command(5,set_virtual) != 0 && virtual_writes == 1);
    assert(command(4,get_virtual) != 0);
#else
    assert(command(5,set_virtual) != 0 && virtual_writes == 0);
    assert(command(4,get_virtual) != 0);
#endif
    unregister_io_command(); return 0;
}
''')
    subprocess.run([str(binary)], check=True, capture_output=True, text=True)


def test_case_console_forwards_button_name(tmp_path, bmgr_root, host_board_linker_flags):
    idf_path = os.environ.get('IDF_PATH')
    if not idf_path:
        pytest.skip('IDF_PATH is needed to test the real argtable parser')
    argtable = Path(idf_path) / 'components/console/argtable3'
    if not (argtable / 'argtable3.c').is_file():
        pytest.skip('ESP-IDF argtable sources are unavailable')
    headers = {
        'esp_err.h': ERRORS,
        'esp_log.h': LOGS,
        'esp_console.h': '''#pragma once
#include "esp_err.h"
typedef struct { const char *command; const char *help; const char *hint;
    int (*func)(int, char **); void *argtable; } esp_console_cmd_t;
esp_err_t esp_console_cmd_register(const esp_console_cmd_t *);
esp_err_t esp_console_cmd_deregister(const char *);
''',
        'bmgr_test_context.h': '#pragma once\ntypedef struct { int unused; } bmgr_test_context_t;\n',
        'bmgr_test_options.h': '''#pragma once
#include <stdbool.h>
typedef struct { bool summary; bool json; } bmgr_test_options_t;
bmgr_test_options_t bmgr_test_options_get(void);
void bmgr_test_options_set(const bmgr_test_options_t *);
''',
        'bmgr_test_registry.h': '''#pragma once
#include <stdbool.h>
#include "esp_err.h"
#include "bmgr_test_context.h"
typedef struct { int unused; } bmgr_test_case_t;
typedef struct { int unused; } bmgr_test_case_summary_t;
void bmgr_test_registry_print_group(const char *);
const bmgr_test_case_t *bmgr_test_registry_find(const char *);
esp_err_t bmgr_test_case_run(bmgr_test_context_t *, const bmgr_test_case_t *, int, char **, bool *);
esp_err_t bmgr_test_registry_run_all(bmgr_test_context_t *, const char *, bool, bmgr_test_case_summary_t *);
''',
    }
    sources = [bmgr_root / 'test_apps/main/commands/cmd_case.c']
    sources += [argtable / name for name in
                ('argtable3.c', 'arg_str.c', 'arg_lit.c', 'arg_end.c', 'arg_utils.c', 'arg_dstr.c')]
    binary = _build(tmp_path, bmgr_root, host_board_linker_flags, sources, headers, r'''
#include <assert.h>
#include <string.h>
#include "esp_console.h"
#include "bmgr_test_registry.h"
#include "bmgr_test_options.h"
extern esp_err_t register_case_command(bmgr_test_context_t *);
extern void unregister_case_command(void);
static int (*command)(int, char **);
static const char *expected_target;
static bmgr_test_options_t options;
static bmgr_test_case_t test_case;
static int ran, listed, run_all;
esp_err_t esp_console_cmd_register(const esp_console_cmd_t *cmd) { command = cmd->func; return ESP_OK; }
esp_err_t esp_console_cmd_deregister(const char *name) { assert(!strcmp(name,"case")); return ESP_OK; }
bmgr_test_options_t bmgr_test_options_get(void) { return options; }
void bmgr_test_options_set(const bmgr_test_options_t *value) { options = *value; }
void bmgr_test_registry_print_group(const char *group) { assert(!strcmp(group,"button")); listed++; }
const bmgr_test_case_t *bmgr_test_registry_find(const char *name) {
    assert(!strcmp(name,"button.read")); return &test_case;
}
esp_err_t bmgr_test_case_run(bmgr_test_context_t *ctx, const bmgr_test_case_t *test,
                           int argc, char **argv, bool *skipped) {
    assert(ctx && test == &test_case && skipped);
    if (expected_target) assert(argc == 1 && argv && !strcmp(argv[0],expected_target));
    else assert(argc == 0 && argv == NULL);
    ran++; return ESP_OK;
}
esp_err_t bmgr_test_registry_run_all(bmgr_test_context_t *ctx, const char *group,
                                   bool manual, bmgr_test_case_summary_t *summary) {
    assert(ctx && !strcmp(group,"button") && manual && summary); run_all++; return ESP_OK;
}
int main(void) {
    bmgr_test_context_t ctx = {0}; assert(register_case_command(&ctx) == ESP_OK);
    char *named[] = {"case","run","button.read","key0_button","--summary"};
    expected_target = "key0_button"; assert(command(5,named) == 0);
    char *all[] = {"case","run","button.read"}; expected_target = NULL; assert(command(3,all) == 0);
    char *list[] = {"case","list","--group","button"}; assert(command(4,list) == 0);
    char *batch[] = {"case","run-all","--group","button","--include-manual"}; assert(command(5,batch) == 0);
    assert(ran == 2 && listed == 1 && run_all == 1 && !options.summary && !options.json);
    unregister_case_command(); return 0;
}
''', extra_flags=('-D_GNU_SOURCE', '-I', str(argtable.parent)))
    subprocess.run([str(binary)], check=True, capture_output=True, text=True)
