/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "dev_gpio_expander.h"
#include "esp_board_device.h"
#include "esp_board_periph.h"
#include "periph_i2c_internal.h"

static dev_io_expander_config_t cfg = {
    .name              = "gpio_expander",
    .i2c_name          = "i2c_master",
    .i2c_addr_count    = 1,
    .i2c_addr          = {0x58},
    .max_pins          = 10,
    .output_io_mask    = 1,
    .gpio_start_io_num = -1,
};
static esp_io_expander_t expander;
static int refs, creates, deletes, appends, removes;
static int append_error, direction_error, delete_error, factory_error;
static bool mapped;
static uint32_t mapped_start;
static uint32_t expander_count = 10;

esp_err_t esp_board_periph_ref_handle(const char *name, void **handle)
{
    (void)name;
    refs++;
    *handle = &expander;
    return ESP_OK;
}

esp_err_t esp_board_periph_unref_handle(const char *name)
{
    (void)name;
    refs--;
    return ESP_OK;
}

esp_err_t i2c_master_probe(i2c_master_bus_handle_t bus, uint16_t addr, int timeout)
{
    (void)bus;
    (void)timeout;
    assert(addr == 0x2C);
    return ESP_OK;
}

esp_err_t io_expander_factory_entry_t(i2c_master_bus_handle_t bus, uint16_t addr,
                                      esp_io_expander_handle_t *handle)
{
    (void)bus;
    assert(addr == 0x2C);
    creates++;
    expander.config.io_count = expander_count;
    *handle = &expander;
    return factory_error;
}

esp_err_t esp_io_expander_set_dir(esp_io_expander_handle_t handle, uint32_t mask, int dir)
{
    (void)handle;
    (void)mask;
    (void)dir;
    return direction_error;
}

esp_err_t esp_io_expander_set_level(esp_io_expander_handle_t handle, uint32_t mask, uint8_t level)
{
    (void)handle;
    (void)mask;
    (void)level;
    return ESP_OK;
}

esp_err_t esp_io_expander_set_output_mode(esp_io_expander_handle_t handle, uint32_t mask, int mode)
{
    (void)handle;
    (void)mask;
    (void)mode;
    return ESP_OK;
}

esp_err_t esp_io_expander_set_pullupdown(esp_io_expander_handle_t handle, uint32_t mask, int pull)
{
    (void)handle;
    (void)mask;
    (void)pull;
    return ESP_OK;
}

esp_err_t esp_io_expander_gpio_wrapper_append_handler(esp_io_expander_handle_t handle, uint32_t start)
{
    assert(handle == &expander);
    appends++;
    if (append_error) {
        return append_error;
    }
    assert(!mapped);
    assert(start >= GPIO_NUM_MAX);
    mapped = true;
    mapped_start = start;
    return ESP_OK;
}

esp_err_t esp_io_expander_gpio_wrapper_remove_handler(esp_io_expander_handle_t handle)
{
    assert(handle == &expander);
    removes++;
    mapped = false;
    return ESP_OK;
}

esp_err_t esp_io_expander_del(esp_io_expander_handle_t handle)
{
    assert(handle == &expander);
    assert(!mapped);  /* Never free a handle still reachable by the GPIO wrapper. */
    deletes++;
    return delete_error;
}

esp_err_t esp_board_device_get_config_by_handle(void *handle, void **config)
{
    (void)handle;
    *config = &cfg;
    return ESP_OK;
}

esp_err_t periph_i2c_set_effective_addr_internal(const char *bus, const char *name, uint16_t addr)
{
    (void)bus;
    (void)name;
    assert(addr == 0x58);
    return ESP_OK;
}

esp_err_t periph_i2c_clear_effective_addr_internal(const char *name)
{
    (void)name;
    return ESP_OK;
}

int main(int argc, char **argv)
{
    if (argc == 2) {
        cfg.gpio_mapping_enabled = false;
        if (strcmp(argv[1], "failed-direction-cleanup") == 0) {
            direction_error = -1;
        } else if (strcmp(argv[1], "failed-factory-cleanup") == 0) {
            factory_error = -1;
#if CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER
        } else if (strcmp(argv[1], "failed-mapping-cleanup") == 0) {
            cfg.gpio_mapping_enabled = true;
            append_error = -1;
#endif  /* CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER */
        } else {
            return 2;
        }
        delete_error = -1;
        void *retained = NULL;
        assert(dev_gpio_expander_init(&cfg, sizeof(cfg), &retained) != 0);
        assert(retained != NULL);
        assert(*(esp_io_expander_handle_t *)retained == &expander);
        assert(refs == 1 && creates == 1 && deletes == 1 && !mapped);
        /* BMGR retains this handle until restart; this process models that boundary. */
        return 0;
    }
    void *handle = NULL;
    assert(dev_gpio_expander_init(&cfg, sizeof(cfg), &handle) == 0);
    assert(handle && !mapped && appends == 0 && refs == 1);
    assert(dev_gpio_expander_deinit(handle) == 0);
    assert(refs == 0);
    handle = NULL;
    assert(dev_gpio_expander_init(&cfg, -1, &handle) != 0);

    factory_error = -1;
    int before_factory_cleanup = deletes;
    assert(dev_gpio_expander_init(&cfg, sizeof(cfg), &handle) != 0);
    assert(!handle && refs == 0 && deletes == before_factory_cleanup + 1);
    factory_error = 0;

    cfg.gpio_mapping_enabled = true;
    handle = NULL;
#if CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER
    assert(dev_gpio_expander_init(&cfg, sizeof(cfg), &handle) == 0);
    assert(mapped && mapped_start == GPIO_NUM_MAX && refs == 1);
    assert(dev_gpio_expander_deinit(handle) == 0);
    assert(!mapped && refs == 0);

    expander_count = 12;
    handle = NULL;
    int before_mismatch = appends;
    assert(dev_gpio_expander_init(&cfg, sizeof(cfg), &handle) != 0);
    assert(!handle && !mapped && refs == 0 && appends == before_mismatch);
    expander_count = 10;

    cfg.gpio_start_io_num = 64;
    handle = NULL;
    assert(dev_gpio_expander_init(&cfg, sizeof(cfg), &handle) == 0);
    assert(mapped && mapped_start == 64);
    delete_error = -1;
    assert(dev_gpio_expander_deinit(handle) != 0);
    assert(mapped && mapped_start == 64 && refs == 1);
    delete_error = 0;
    assert(dev_gpio_expander_deinit(handle) == 0);
    assert(!mapped && refs == 0);

    int before = deletes;
    append_error = -1;
    handle = NULL;
    assert(dev_gpio_expander_init(&cfg, sizeof(cfg), &handle) != 0);
    assert(!handle && !mapped && refs == 0 && deletes == before + 1);
    append_error = 0;

    before = appends;
    direction_error = -1;
    assert(dev_gpio_expander_init(&cfg, sizeof(cfg), &handle) != 0);
    assert(!handle && !mapped && refs == 0 && appends == before);
    direction_error = 0;

    cfg.gpio_start_io_num = GPIO_NUM_MAX - 1;
    assert(dev_gpio_expander_init(&cfg, sizeof(cfg), &handle) != 0);
    assert(!handle && !mapped && refs == 0);
#else
    int before = creates;
    assert(dev_gpio_expander_init(&cfg, sizeof(cfg), &handle) != 0);
    assert(!handle && refs == 0 && creates == before && appends == 0);
#endif  /* CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER */
    assert(dev_gpio_expander_init(&cfg, 0, &handle) != 0);
    assert(refs == 0);
    return 0;
}
