/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <inttypes.h>
#include <stdbool.h>
#include "esp_check.h"
#include "esp_io_expander.h"
#include "esp_log.h"
#include "dev_gpio_expander.h"
#include "esp_board_device.h"
#include "esp_board_manager.h"
#include "esp_board_manager_defs.h"
#include "bmgr_test_names.h"

#define GPIO_EXPANDER_TEST  IO_EXPANDER_PIN_NUM_1

static const char *TAG = "TEST_GPIO_EXPANDER";

static esp_err_t restore_test_pin(esp_io_expander_handle_t gpio_expander,
                                  const dev_io_expander_config_t *gpio_config)
{
    if (gpio_config->output_io_mask & GPIO_EXPANDER_TEST) {
        esp_err_t ret = esp_io_expander_set_dir(
            gpio_expander,
            GPIO_EXPANDER_TEST,
            IO_EXPANDER_OUTPUT
        );
        if (ret != ESP_OK) {
            return ret;
        }

        const uint8_t level =
            (gpio_config->output_io_level_mask & GPIO_EXPANDER_TEST) ? 1 : 0;

        return esp_io_expander_set_level(
            gpio_expander,
            GPIO_EXPANDER_TEST,
            level
        );
    }

    if (gpio_config->input_io_mask & GPIO_EXPANDER_TEST) {
        return esp_io_expander_set_dir(
            gpio_expander,
            GPIO_EXPANDER_TEST,
            IO_EXPANDER_INPUT
        );
    }

    /*
     * The test temporarily drives the pin, so if the board configuration does
     * not assign it a direction, release it as an input before returning.
     */
    return esp_io_expander_set_dir(
        gpio_expander,
        GPIO_EXPANDER_TEST,
        IO_EXPANDER_INPUT
    );
}

static bool check_operation(const char *operation, esp_err_t ret)
{
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "%s: OK", operation);
        return true;
    }

    ESP_LOGE(
        TAG,
        "%s failed: %s (0x%x)",
        operation,
        esp_err_to_name(ret),
        (unsigned)ret
    );
    return false;
}

void test_dev_gpio_expander(void)
{
    void *dev_cfg = NULL;
    esp_err_t ret = esp_board_manager_get_device_config(
        BMGR_TEST_NAME_GPIO_EXPANDER,
        &dev_cfg
    );
    if (ret != ESP_OK || dev_cfg == NULL) {
        ESP_LOGE(
            TAG,
            "Failed to get gpio_expander device config: %s",
            esp_err_to_name(ret)
        );
        return;
    }

    void *dev_handle = NULL;
    ret = esp_board_manager_get_device_handle(
        BMGR_TEST_NAME_GPIO_EXPANDER,
        &dev_handle
    );
    if (ret != ESP_OK || dev_handle == NULL) {
        ESP_LOGE(
            TAG,
            "Failed to get gpio_expander device handle: %s",
            esp_err_to_name(ret)
        );
        return;
    }

    dev_io_expander_config_t *gpio_config =
        (dev_io_expander_config_t *)dev_cfg;

    /*
     * gpio_expander publishes the actual esp_io_expander_handle_t.
     * This direct cast is the public handle contract under test.
     */
    esp_io_expander_handle_t gpio_expander =
        (esp_io_expander_handle_t)dev_handle;

    ESP_LOGI(TAG, "GPIO Expander Device Config:");
    ESP_LOGI(TAG, "  Name: %s", gpio_config->name);
    ESP_LOGI(TAG, "  Handle: %p", gpio_expander);

    /*
     * The original test dereferenced dev_handle as an
     * esp_io_expander_handle_t *, which masked an extra-indirection bug in the
     * gpio_expander adapter. Exercise the public handle directly so that such a
     * regression fails in the test instead of application code.
     */
    if (!check_operation(
            "print initial state",
            esp_io_expander_print_state(gpio_expander))) {
        return;
    }

    bool pin_modified = false;

    ret = esp_io_expander_set_dir(
        gpio_expander,
        GPIO_EXPANDER_TEST,
        IO_EXPANDER_OUTPUT
    );
    if (!check_operation("set test pin OUTPUT", ret)) {
        goto restore;
    }
    pin_modified = true;

    ret = esp_io_expander_set_level(
        gpio_expander,
        GPIO_EXPANDER_TEST,
        0
    );
    if (!check_operation("set test pin LOW", ret)) {
        goto restore;
    }

    ret = esp_io_expander_set_level(
        gpio_expander,
        GPIO_EXPANDER_TEST,
        1
    );
    if (!check_operation("set test pin HIGH", ret)) {
        goto restore;
    }

    ret = esp_io_expander_set_dir(
        gpio_expander,
        GPIO_EXPANDER_TEST,
        IO_EXPANDER_INPUT
    );
    if (!check_operation("set test pin INPUT", ret)) {
        goto restore;
    }

    uint32_t level_mask = 0;
    ret = esp_io_expander_get_level(
        gpio_expander,
        GPIO_EXPANDER_TEST,
        &level_mask
    );
    if (!check_operation("read test pin", ret)) {
        goto restore;
    }

    /*
     * Validate the API/driver operation, not a specific physical voltage.
     * External circuitry may legitimately determine the input level.
     */
    ESP_LOGI(
        TAG,
        "Test pin physical level: %s",
        (level_mask & GPIO_EXPANDER_TEST) ? "HIGH" : "LOW"
    );

restore:
    if (pin_modified) {
        const esp_err_t restore_ret =
            restore_test_pin(gpio_expander, gpio_config);

        if (!check_operation("restore test pin", restore_ret)) {
            return;
        }
    }

    if (ret != ESP_OK) {
        return;
    }

    if (!check_operation(
            "print final state",
            esp_io_expander_print_state(gpio_expander))) {
        return;
    }

    ESP_LOGI(TAG, "GPIO Expander Device test completed successfully!");
}