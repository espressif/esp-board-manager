/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 */

#include <assert.h>
#include <stddef.h>
#include "driver/gpio.h"
#include "periph_gpio.h"
#include "generated_gpio.h"

static int config_calls;
static int level_calls;
static int reset_calls;
static int config_error;
static int level_error;

esp_err_t gpio_config(const gpio_config_t *config)
{
    assert(config->pin_bit_mask != 0);
    config_calls++;
    return config_error;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    assert(gpio_num < GPIO_NUM_MAX);
    assert(level == 1);
    level_calls++;
    return level_error;
}

esp_err_t gpio_reset_pin(gpio_num_t gpio_num)
{
    assert(gpio_num < GPIO_NUM_MAX);
    reset_calls++;
    return ESP_OK;
}

int main(void)
{
    /* Legacy native initializers omit gpio_num, including for nonzero pins. */
    periph_gpio_config_t native = {
        .gpio_config = {.pin_bit_mask = BIT64(2), .mode = GPIO_MODE_OUTPUT},
        .default_level = 1,
    };
    void *handle = NULL;
    assert(periph_gpio_init(&native, sizeof(native), &handle) == 0);
    assert(((periph_gpio_handle_t *)handle)->gpio_num == 2);
    assert(config_calls == 1 && level_calls == 1);
    assert(periph_gpio_deinit(handle) == 0);
    handle = NULL;

    native.gpio_config.pin_bit_mask = BIT64(0);
    assert(periph_gpio_init(&native, sizeof(native), &handle) == 0);
    assert(((periph_gpio_handle_t *)handle)->gpio_num == 0);
    assert(periph_gpio_deinit(handle) == 0);
    handle = NULL;

    config_error = -1;
    assert(periph_gpio_init(&native, sizeof(native), &handle) == -1);
    assert(handle == NULL);
    config_error = 0;
    level_error = -1;
    assert(periph_gpio_init(&native, sizeof(native), &handle) == -1);
    assert(handle == NULL && reset_calls == 1);
    level_error = 0;

    config_calls = level_calls = reset_calls = 0;
    periph_gpio_config_t virtual_gpio = generated_gpio;
    assert(periph_gpio_init(&virtual_gpio, sizeof(virtual_gpio), &handle) == 0);
    assert(((periph_gpio_handle_t *)handle)->gpio_num == 65);
    assert(config_calls == 0 && level_calls == 0 && reset_calls == 0);
    assert(periph_gpio_deinit(handle) == 0);
    handle = NULL;

    /* Virtual pins below 64 also delegate their configuration to the expander. */
    virtual_gpio.gpio_config.pin_bit_mask = BIT64(GPIO_NUM_MAX);
    virtual_gpio.gpio_num = GPIO_NUM_MAX;
    assert(periph_gpio_init(&virtual_gpio, sizeof(virtual_gpio), &handle) == 0);
    assert(((periph_gpio_handle_t *)handle)->gpio_num == GPIO_NUM_MAX);
    assert(config_calls == 0 && level_calls == 0 && reset_calls == 0);
    assert(periph_gpio_deinit(handle) == 0);
    handle = NULL;

    native.gpio_config.pin_bit_mask = 0;
    assert(periph_gpio_init(&native, sizeof(native), &handle) == -1);
    assert(handle == NULL);
    native.gpio_config.pin_bit_mask = BIT64(0) | BIT64(2);
    assert(periph_gpio_init(&native, sizeof(native), &handle) == -1);
    assert(handle == NULL);
    return 0;
}
