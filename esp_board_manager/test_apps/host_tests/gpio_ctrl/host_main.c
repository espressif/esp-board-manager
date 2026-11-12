/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "dev_gpio_ctrl.h"
#include "esp_board_device.h"
#include "esp_board_periph.h"

static periph_gpio_handle_t s_gpio = {.gpio_num = 2};
static periph_gpio_config_t s_periph_config;
static dev_gpio_ctrl_config_t *s_device_config;
static int s_config_calls;
static int s_ref_count;
static int s_unref_calls;
static int s_level_calls;
static uint32_t s_level;
static esp_err_t s_level_error;

esp_err_t esp_board_periph_get_config(const char *name, void **config)
{
    assert(strcmp(name, "gpio_control") == 0);
    s_config_calls++;
    *config = &s_periph_config;
    return ESP_OK;
}

esp_err_t esp_board_periph_ref_handle(const char *name, void **handle)
{
    assert(strcmp(name, "gpio_control") == 0);
    s_ref_count++;
    *handle = &s_gpio;
    return ESP_OK;
}

esp_err_t esp_board_periph_unref_handle(const char *name)
{
    assert(strcmp(name, "gpio_control") == 0);
    assert(s_ref_count > 0);
    s_ref_count--;
    s_unref_calls++;
    return ESP_OK;
}

esp_err_t esp_board_device_get_config_by_handle(void *handle, void **config)
{
    assert(handle == &s_gpio);
    *config = s_device_config;
    return ESP_OK;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    assert(gpio_num == s_gpio.gpio_num);
    assert(level <= 1);
    s_level_calls++;
    if (s_level_error != ESP_OK) {
        return s_level_error;
    }
    s_level = level;
    return ESP_OK;
}

static dev_gpio_ctrl_config_t make_config(void)
{
    dev_gpio_ctrl_config_t config = {
        .name = "control",
        .type = "gpio_ctrl",
        .gpio_name = "gpio_control",
        .active_level = 1,
        .default_level = 0,
        .init_level = -1,
    };
    return config;
}

static void test_levels(void)
{
    for (int active = 0; active <= 1; active++) {
        for (int inactive = 0; inactive <= 1; inactive++) {
            for (int initial = -1; initial <= 1; initial++) {
                dev_gpio_ctrl_config_t config = make_config();
                config.active_level = active;
                config.default_level = inactive;
                config.init_level = initial;
                s_device_config = &config;
                void *handle = NULL;
                int calls_before = s_level_calls;
                assert(dev_gpio_ctrl_init(&config, sizeof(config), &handle) == ESP_OK);
                assert(handle == &s_gpio && s_ref_count == 1);
                assert(s_level_calls == calls_before + 1);
                assert(s_level == (uint32_t)(initial == -1 ? active : initial));
                assert(config.init_level == initial);
                assert(config.active_level == active && config.default_level == inactive);
                assert(dev_gpio_ctrl_deinit(handle) == ESP_OK);
                assert(s_ref_count == 0 && s_level_calls == calls_before + 1);
            }
        }
    }
}

static void expect_invalid(dev_gpio_ctrl_config_t *config, int size)
{
    void *handle = NULL;
    assert(dev_gpio_ctrl_init(config, size, &handle) != ESP_OK);
    assert(handle == NULL);
    assert(s_ref_count == 0 && s_config_calls == 0 && s_level_calls == 0);
}

static void test_validation(void)
{
    dev_gpio_ctrl_config_t config = make_config();
    const int8_t invalid_levels[] = {-128, -2, 2, 127};
    for (size_t i = 0; i < sizeof(invalid_levels) / sizeof(invalid_levels[0]); i++) {
        config.init_level = invalid_levels[i];
        expect_invalid(&config, sizeof(config));
    }
    config.init_level = -1;
    config.active_level = 2;
    expect_invalid(&config, sizeof(config));
    config.active_level = 1;
    config.default_level = 2;
    expect_invalid(&config, sizeof(config));
    config.default_level = 0;
    expect_invalid(&config, sizeof(config) - 1);
    expect_invalid(&config, 0);
    expect_invalid(&config, -1);
    expect_invalid(NULL, sizeof(config));
    assert(dev_gpio_ctrl_init(&config, sizeof(config), NULL) != ESP_OK);
    assert(s_config_calls == 0 && s_ref_count == 0 && s_level_calls == 0);
}

static void test_lifecycle(void)
{
    dev_gpio_ctrl_config_t config = make_config();
    config.init_level = 0;
    s_device_config = &config;
    void *handle = NULL;
    assert(dev_gpio_ctrl_init(&config, sizeof(config), &handle) == ESP_OK);
    assert(s_level == 0);
    assert(gpio_set_level(s_gpio.gpio_num, config.active_level) == ESP_OK);
    assert(dev_gpio_ctrl_deinit(handle) == ESP_OK);
    assert(s_level == 1 && s_level_calls == 2 && s_ref_count == 0);
    handle = NULL;
    assert(dev_gpio_ctrl_init(&config, sizeof(config), &handle) == ESP_OK);
    assert(s_level == 0 && s_level_calls == 3);
    assert(dev_gpio_ctrl_deinit(handle) == ESP_OK);
    assert(s_ref_count == 0 && s_level_calls == 3);
}

static void test_rollback(void)
{
    dev_gpio_ctrl_config_t config = make_config();
    config.init_level = 0;
    void *handle = NULL;
    s_level_error = ESP_FAIL;
    assert(dev_gpio_ctrl_init(&config, sizeof(config), &handle) == ESP_FAIL);
    assert(handle == NULL && s_ref_count == 0 && s_unref_calls == 1);
    assert(s_level_calls == 1);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (strcmp(argv[1], "levels") == 0) {
        test_levels();
    } else if (strcmp(argv[1], "validation") == 0) {
        test_validation();
    } else if (strcmp(argv[1], "lifecycle") == 0) {
        test_lifecycle();
    } else if (strcmp(argv[1], "rollback") == 0) {
        test_rollback();
    } else {
        assert(0);
    }
    return 0;
}
