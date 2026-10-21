/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_console.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_board_device.h"
#include "cmd_io.h"

static const char *TAG = "CMD_IO";
static bool s_registered;

static bool parse_nonnegative_int(const char *text, int *value)
{
    char *end = NULL;
    errno = 0;
    long parsed = strtol(text, &end, 0);
    if (errno != 0 || text == end || *end != '\0' || parsed < 0 || parsed > INT_MAX) {
        return false;
    }
    *value = (int)parsed;
    return true;
}

static esp_err_t board_io(const char *target, bool write, int pin, int *level)
{
    void *handle = NULL;
    esp_err_t ret = esp_board_device_get_handle(target, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initialize %s first with 'bmgr init %s'", target, target);
        return ret;
    }
    const char *suffix = write ? "_io_set" : "_io_get";
    size_t key_size = strlen(target) + strlen(suffix) + 1;
    char *key = malloc(key_size);
    if (key == NULL) {
        return ESP_ERR_NO_MEM;
    }
    snprintf(key, key_size, "%s%s", target, suffix);
    void *extra_func = NULL;
    int found = esp_board_extra_func_get(key, &extra_func);
    free(key);
    if (found != 0 || extra_func == NULL) {
        ESP_LOGE(TAG, "%s does not provide an IO debug adapter", target);
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (write) {
        return ((bmgr_test_io_set_fn_t)extra_func)(handle, pin, *level);
    }
    return ((bmgr_test_io_get_fn_t)extra_func)(handle, pin, level);
}

static int io_command_handler(int argc, char **argv)
{
    if (argc < 4 || argc > 5) {
        ESP_LOGE(TAG, "Usage: io get <gpio|device> <pin> | io set <gpio|device> <pin> <0|1>");
        return 1;
    }
    bool write = strcmp(argv[1], "set") == 0;
    int pin = -1;
    int level = -1;
    if ((!write && strcmp(argv[1], "get") != 0) || argc != (write ? 5 : 4) ||
        !parse_nonnegative_int(argv[3], &pin) ||
        (write && (!parse_nonnegative_int(argv[4], &level) || level > 1))) {
        ESP_LOGE(TAG, "Invalid IO command, pin, or level");
        return 1;
    }

    esp_err_t ret = ESP_OK;
    if (strcmp(argv[2], "gpio") == 0) {
        bool mapped = false;
#if CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER
        mapped = pin >= GPIO_NUM_MAX;
#endif  /* CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER */
        if (!mapped && (!GPIO_IS_VALID_GPIO(pin) || (write && !GPIO_IS_VALID_OUTPUT_GPIO(pin)))) {
            ESP_LOGE(TAG, "Invalid %s GPIO: %d", write ? "output" : "input", pin);
            return 1;
        }
        if (write) {
            /* The expander wrapper supports OUTPUT, but not INPUT_OUTPUT mode.
             * Its get_level() reads the input register regardless of direction. */
            ret = gpio_set_direction((gpio_num_t)pin, mapped ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT_OUTPUT);
            if (ret == ESP_OK) {
                ret = gpio_set_level((gpio_num_t)pin, level);
            }
        } else {
            level = gpio_get_level((gpio_num_t)pin);
            if (level < 0) {
                ret = ESP_FAIL;
            }
        }
    } else {
        ret = board_io(argv[2], write, pin, &level);
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "IO operation failed: %s", esp_err_to_name(ret));
        return 1;
    }
    printf("IO %s %s pin=%d level=%d\n", write ? "set" : "get", argv[2], pin, level);
    return 0;
}

esp_err_t register_io_command(void)
{
    const esp_console_cmd_t command = {
        .command = "io",
        .help = "Physical IO levels: get <gpio|device> <pin>; set <gpio|device> <pin> <0|1>. "
                "Device IO requires an active device and <device>_io_get/_io_set adapters. "
                "GPIO supports native pads and active expander mappings when the wrapper is enabled. "
                "Native GPIO get requires input enabled; set retains input/output mode (mapped pins: output).",
        .hint = NULL,
        .func = io_command_handler,
    };
    esp_err_t ret = esp_console_cmd_register(&command);
    if (ret == ESP_OK) {
        s_registered = true;
    }
    return ret;
}

void unregister_io_command(void)
{
    if (s_registered) {
        (void)esp_console_cmd_deregister("io");
        s_registered = false;
    }
}
