/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <inttypes.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "dev_gpio_expander.h"
#include "esp_board_device.h"
#include "esp_board_periph.h"
#include "esp_io_expander.h"
#include "esp_io_expander_gpio_wrapper.h"
#include "periph_i2c_internal.h"

static const char *TAG = "DEV_IO_EXPANDER";

extern esp_err_t io_expander_factory_entry_t(i2c_master_bus_handle_t i2c_bus, const uint16_t dev_addr, esp_io_expander_handle_t *handle_ret);

int dev_gpio_expander_init(void *cfg, int cfg_size, void **device_handle)
{
    if (!cfg || cfg_size < (int)sizeof(dev_io_expander_config_t) || !device_handle) {
        ESP_LOGE(TAG, "Invalid parameters, cfg: %p, device_handle: %p", cfg, device_handle);
        return -1;
    }

    const dev_io_expander_config_t *config = (const dev_io_expander_config_t *)cfg;
    if (config->max_pins < 1 || config->max_pins > 32) {
        ESP_LOGE(TAG, "max_pins must be from 1 to 32");
        return -1;
    }
    if (config->gpio_mapping_enabled) {
#if CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER
        if (config->gpio_start_io_num != -1 &&
            (config->gpio_start_io_num < GPIO_NUM_MAX || config->gpio_start_io_num > INT32_MAX - 31)) {
            ESP_LOGE(TAG, "Invalid virtual GPIO start: %" PRId32 ", minimum: %d",
                     config->gpio_start_io_num, GPIO_NUM_MAX);
            return -1;
        }
#else
        ESP_LOGE(TAG, "GPIO mapping requires CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER=y");
        return -1;
#endif  /* CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER */
    }
    void *i2c_handle = NULL;
    esp_err_t ret = esp_board_periph_ref_handle(config->i2c_name, &i2c_handle);
    if (ret != ESP_OK || !i2c_handle) {
        ESP_LOGE(TAG, "Failed to get I2C (%s) handle, ret:%d, i2c_handle:%p\n", config->i2c_name, ret, i2c_handle);
        return -1;
    }

    i2c_master_bus_handle_t bus_handle = (i2c_master_bus_handle_t)i2c_handle;
    uint16_t dev_addr = 0xFF;
    uint16_t effective_addr = 0;
    for (size_t i = 0; i < config->i2c_addr_count; i++) {
        if (i2c_master_probe(bus_handle, config->i2c_addr[i] >> 1, 100) == ESP_OK) {
            ESP_LOGI(TAG, "IO Expander found at address 0x%02x", (unsigned int)config->i2c_addr[i]);
            dev_addr = config->i2c_addr[i] >> 1;
            effective_addr = config->i2c_addr[i];
            break;
        }
    }
    if (dev_addr == 0xFF) {
        ESP_LOGE(TAG, "No IO Expander found on the I2C bus");
        goto init_err_unref_i2c;
    }

    esp_io_expander_handle_t *dev = (esp_io_expander_handle_t *)calloc(1, sizeof(esp_io_expander_handle_t));
    if (!dev) {
        ESP_LOGE(TAG, "Failed to allocate memory for io_expander device");
        goto init_err_unref_i2c;
    }

    ret = io_expander_factory_entry_t(bus_handle, dev_addr, dev);
    if (ret != ESP_OK || !*dev) {
        ESP_LOGE(TAG, "Failed to create IO expander handle\n");
        goto io_expander_del;
    }
    if (config->max_pins > (*dev)->config.io_count) {
        ESP_LOGE(TAG, "max_pins exceeds the IO expander's pin count");
        goto io_expander_del;
    }
    if (config->gpio_mapping_enabled && config->max_pins != (*dev)->config.io_count) {
        ESP_LOGE(TAG, "Mapped max_pins must match the IO expander's pin count");
        goto io_expander_del;
    }

    for (uint32_t i = 0; i < config->max_pins; i++) {
        uint32_t pin_mask = (1U << i);
        if (config->output_io_mask & pin_mask) {
            ESP_GOTO_ON_ERROR(esp_io_expander_set_dir(*dev, pin_mask, IO_EXPANDER_OUTPUT),
                              io_expander_del, TAG, "Set IO expander pin %" PRIu32 " as output failed", i);
            uint8_t level = (config->output_io_level_mask >> i) & 1;
            ESP_GOTO_ON_ERROR(esp_io_expander_set_level(*dev, pin_mask, level),
                              io_expander_del, TAG, "Set IO expander pin %" PRIu32 " default level failed", i);
            if (config->enable_mode_set) {
                esp_io_expander_output_mode_t mode = (esp_io_expander_output_mode_t)((config->output_io_mode_mask >> i) & 1);
                ESP_GOTO_ON_ERROR(esp_io_expander_set_output_mode(*dev, pin_mask, mode),
                                  io_expander_del, TAG, "Set IO expander pin %" PRIu32 " output mode failed", i);
            }
            ESP_LOGI(TAG, "Set IO expander pin %" PRIu32 " as output, level: %d", i, level);
        } else if (config->input_io_mask & pin_mask) {
            ESP_GOTO_ON_ERROR(esp_io_expander_set_dir(*dev, pin_mask, IO_EXPANDER_INPUT),
                              io_expander_del, TAG, "Set IO expander pin %" PRIu32 " as input failed", i);
            ESP_LOGI(TAG, "Set IO expander pin %" PRIu32 " as input", i);
        }
        if (config->io_pullup_mask & pin_mask) {
            esp_io_expander_pullupdown_t state = IO_EXPANDER_PULL_UP;
            ESP_GOTO_ON_ERROR(esp_io_expander_set_pullupdown(*dev, pin_mask, state),
                              io_expander_del, TAG, "Set IO expander pin %" PRIu32 " pull-up failed", i);
            ESP_LOGI(TAG, "Enable IO expander pin %" PRIu32 " pull-up", i);
        }
        if (config->io_pulldown_mask & pin_mask) {
            esp_io_expander_pullupdown_t state = IO_EXPANDER_PULL_DOWN;
            ESP_GOTO_ON_ERROR(esp_io_expander_set_pullupdown(*dev, pin_mask, state),
                              io_expander_del, TAG, "Set IO expander pin %" PRIu32 " pull-down failed", i);
            ESP_LOGI(TAG, "Enable IO expander pin %" PRIu32 " pull-down", i);
        }
    }
#if CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER
    if (config->gpio_mapping_enabled) {
        uint32_t start = config->gpio_start_io_num == -1 ? GPIO_NUM_MAX : (uint32_t)config->gpio_start_io_num;
        ESP_GOTO_ON_ERROR(esp_io_expander_gpio_wrapper_append_handler(*dev, start),
                          io_expander_del, TAG, "Register virtual GPIO mapping failed");
        ESP_LOGI(TAG, "Mapped %s IOs to GPIO %" PRIu32 "..%" PRIu32,
                 config->name, start, start + (*dev)->config.io_count - 1);
    }
#endif  /* CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER */
    ret = periph_i2c_set_effective_addr_internal(config->i2c_name, config->name, effective_addr);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to publish effective I2C addr for %s: %s", config->name, esp_err_to_name(ret));
    }
    ESP_LOGD(TAG, "Successfully initialized: %s, dev: %p", config->name, dev);
    *device_handle = dev;
    return 0;

io_expander_del:
    if (*dev) {
        esp_err_t delete_ret = esp_io_expander_del(*dev);
        if (delete_ret != ESP_OK) {
            *device_handle = dev;
            ESP_LOGE(TAG, "IO expander init rollback failed: %s; resources retained, restart required",
                     esp_err_to_name(delete_ret));
            return -1;
        }
    }
    free(dev);
init_err_unref_i2c:
    esp_board_periph_unref_handle(config->i2c_name);
    return -1;
}

int dev_gpio_expander_deinit(void *device_handle)
{
    if (device_handle == NULL) {
        return -1;
    }
    dev_io_expander_config_t *cfg = NULL;
    if (esp_board_device_get_config_by_handle(device_handle, (void **)&cfg) != ESP_OK || cfg == NULL) {
        ESP_LOGE(TAG, "Failed to find IO expander config for deinit");
        return -1;
    }

    esp_io_expander_handle_t *io_expander_dev = (esp_io_expander_handle_t *)device_handle;
    if (*io_expander_dev) {
#if CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER
        esp_err_t ret = esp_io_expander_gpio_wrapper_remove_handler(*io_expander_dev);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Remove virtual GPIO mapping failed: %s", esp_err_to_name(ret));
            return -1;
        }
#endif  /* CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER */
        esp_err_t delete_ret = esp_io_expander_del(*io_expander_dev);
        if (delete_ret != ESP_OK) {
#if CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER
            if (cfg->gpio_mapping_enabled) {
                uint32_t start = cfg->gpio_start_io_num == -1 ? GPIO_NUM_MAX : (uint32_t)cfg->gpio_start_io_num;
                esp_err_t restore_ret = esp_io_expander_gpio_wrapper_append_handler(*io_expander_dev, start);
                if (restore_ret != ESP_OK) {
                    ESP_LOGE(TAG, "Restore virtual GPIO mapping failed: %s", esp_err_to_name(restore_ret));
                }
            }
#endif  /* CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER */
            ESP_LOGE(TAG, "Delete IO expander failed: %s", esp_err_to_name(delete_ret));
            return -1;
        }
    }

    esp_err_t ret = periph_i2c_clear_effective_addr_internal(cfg->name);
    if (ret != ESP_OK && ret != ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to clear effective I2C addr for %s: %s", cfg->name, esp_err_to_name(ret));
    }
    esp_board_periph_unref_handle(cfg->i2c_name);
    ESP_LOGD(TAG, "Successfully deinitialized: %s, dev: %p", cfg ? cfg->name : "(unknown)", io_expander_dev);
    free(io_expander_dev);
    return 0;
}
