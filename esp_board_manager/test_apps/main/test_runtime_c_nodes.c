/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_board_device.h"
#include "esp_board_manager.h"
#include "esp_board_manager_defs.h"
#include "esp_board_manager_err.h"
#include "esp_board_periph.h"
#include "esp_board_runtime_state.h"
#include "bmgr_test_registry.h"

extern esp_board_device_desc_t g_esp_board_devices[];
extern esp_board_device_handle_t g_esp_board_device_handles[];
extern esp_board_periph_desc_t g_esp_board_peripherals[];

static uint8_t s_dev_cfg = 0x31;
static uint8_t s_override_handle = 0x33;
static uint8_t s_periph_cfg = 0x32;
static bool s_hook_enabled;
static bool s_remove_enabled;
static bool s_replace_enabled;
static esp_err_t s_hook_i2c_result = ESP_FAIL;
static const char *s_remove_device_name;
static const char *s_remove_periph_name;
static esp_board_device_desc_t *s_device_tail;
static esp_board_device_handle_t *s_handle_tail;
static esp_board_periph_desc_t *s_periph_tail;
static esp_board_device_desc_t *s_remove_device_prev;
static esp_board_device_handle_t *s_remove_handle_prev;
static esp_board_periph_desc_t *s_remove_periph_prev;
static const esp_board_device_desc_t *s_removed_device;
static esp_board_device_handle_t *s_removed_handle;
static const esp_board_periph_desc_t *s_removed_periph;
static esp_board_device_desc_t *s_replace_device;
static esp_board_device_handle_t *s_replace_handle;
static esp_board_periph_desc_t *s_replace_periph;
static esp_board_device_desc_t s_old_device;
static esp_board_device_handle_t s_old_handle;
static esp_board_periph_desc_t s_old_periph;

static int virtual_device_init(void *cfg, int cfg_size, void **handle)
{
    if (cfg != &s_dev_cfg || cfg_size != (int)sizeof(s_dev_cfg)) {
        return ESP_FAIL;
    }
    *handle = &s_dev_cfg;
    return ESP_OK;
}

static int virtual_device_deinit(void *handle)
{
    return handle == &s_dev_cfg ? ESP_OK : ESP_FAIL;
}

static int overridden_device_init(void *cfg, int cfg_size, void **handle)
{
    if (cfg != &s_dev_cfg || cfg_size != (int)sizeof(s_dev_cfg)) {
        return ESP_FAIL;
    }
    *handle = &s_override_handle;
    return ESP_OK;
}

static int overridden_device_deinit(void *handle)
{
    return handle == &s_override_handle ? ESP_OK : ESP_FAIL;
}

static esp_err_t virtual_periph_init(void *cfg, int cfg_size, void **handle)
{
    if (cfg != &s_periph_cfg || cfg_size != (int)sizeof(s_periph_cfg)) {
        return ESP_FAIL;
    }
    *handle = &s_periph_cfg;
    return ESP_OK;
}

static esp_err_t virtual_periph_deinit(void *handle)
{
    return handle == &s_periph_cfg ? ESP_OK : ESP_FAIL;
}

static int replaced_device_init(void *cfg, int cfg_size, void **handle)
{
    (void)cfg;
    (void)cfg_size;
    *handle = &s_override_handle;
    return ESP_OK;
}

static esp_err_t replaced_periph_init(void *cfg, int cfg_size, void **handle)
{
    (void)cfg;
    (void)cfg_size;
    *handle = &s_periph_cfg;
    return ESP_OK;
}

static esp_board_device_desc_t s_device_desc = {
    .name     = "runtime_c_device",
    .type     = "my_sensor",
    .cfg      = &s_dev_cfg,
    .cfg_size = sizeof(s_dev_cfg),
};

static esp_board_device_handle_t s_device_handle = {
    .name   = "runtime_c_device",
    .type   = "my_sensor",
    .init   = virtual_device_init,
    .deinit = virtual_device_deinit,
};

static esp_board_periph_desc_t s_periph_desc = {
    .name     = "runtime_c_periph",
    .type     = "my_periph",
    .role     = ESP_BOARD_PERIPH_ROLE_NONE,
    .cfg      = &s_periph_cfg,
    .cfg_size = sizeof(s_periph_cfg),
};

esp_err_t esp_board_runtime_pre_init(void)
{
    if (s_remove_enabled) {
        for (esp_board_device_desc_t *entry = g_esp_board_devices; entry->next;
             entry = (esp_board_device_desc_t *)entry->next) {
            if (strcmp(entry->next->name, s_remove_device_name) == 0) {
                s_remove_device_prev = entry;
                s_removed_device = entry->next;
                break;
            }
        }
        for (esp_board_device_handle_t *entry = g_esp_board_device_handles; entry->next;
             entry = entry->next) {
            if (strcmp(entry->next->name, s_remove_device_name) == 0) {
                s_remove_handle_prev = entry;
                s_removed_handle = entry->next;
                break;
            }
        }
        for (esp_board_periph_desc_t *entry = g_esp_board_peripherals; entry->next;
             entry = (esp_board_periph_desc_t *)entry->next) {
            if (strcmp(entry->next->name, s_remove_periph_name) == 0) {
                s_remove_periph_prev = entry;
                s_removed_periph = entry->next;
                break;
            }
        }
        if (!s_remove_device_prev || !s_remove_handle_prev || !s_remove_periph_prev) {
            return ESP_ERR_NOT_FOUND;
        }
        s_remove_device_prev->next = s_removed_device->next;
        s_remove_handle_prev->next = s_removed_handle->next;
        s_remove_periph_prev->next = s_removed_periph->next;
        return ESP_OK;
    }

    if (s_replace_enabled) {
        for (esp_board_device_desc_t *entry = g_esp_board_devices; entry && entry->name;
             entry = (esp_board_device_desc_t *)entry->next) {
            if (strcmp(entry->name, "boot_button") == 0) {
                s_replace_device = entry;
                break;
            }
        }
        for (esp_board_device_handle_t *entry = g_esp_board_device_handles; entry && entry->name;
             entry = entry->next) {
            if (strcmp(entry->name, "boot_button") == 0) {
                s_replace_handle = entry;
                break;
            }
        }
        for (esp_board_periph_desc_t *entry = g_esp_board_peripherals; entry && entry->name;
             entry = (esp_board_periph_desc_t *)entry->next) {
            if (strcmp(entry->name, "gpio_boot_button") == 0) {
                s_replace_periph = entry;
                break;
            }
        }
        if (!s_replace_device || !s_replace_handle || !s_replace_periph) {
            return ESP_ERR_NOT_FOUND;
        }
        s_old_device = *s_replace_device;
        s_old_handle = *s_replace_handle;
        s_old_periph = *s_replace_periph;
        s_replace_device->type = "test_button";
        s_replace_handle->type = "test_button";
        s_replace_handle->init = replaced_device_init;
        s_replace_handle->deinit = overridden_device_deinit;
        s_replace_periph->type = "test_gpio";
        return esp_board_periph_set_ops("gpio_boot_button", replaced_periph_init,
                                        virtual_periph_deinit);
    }

    if (!s_hook_enabled) {
        return ESP_OK;
    }

    s_device_tail = g_esp_board_devices;
    while (s_device_tail->next) {
        s_device_tail = (esp_board_device_desc_t *)s_device_tail->next;
    }
    s_handle_tail = g_esp_board_device_handles;
    while (s_handle_tail->next) {
        s_handle_tail = s_handle_tail->next;
    }
    s_periph_tail = g_esp_board_peripherals;
    while (s_periph_tail->next) {
        s_periph_tail = (esp_board_periph_desc_t *)s_periph_tail->next;
    }
    s_device_tail->next = &s_device_desc;
    s_handle_tail->next = &s_device_handle;
    s_periph_tail->next = &s_periph_desc;

    esp_err_t ret = esp_board_periph_set_ops("runtime_c_periph", virtual_periph_init,
                                             virtual_periph_deinit);
    if (ret != ESP_OK) {
        return ret;
    }

    void *i2c_handle = NULL;
    ret = esp_board_periph_init("i2c_master");
    bool i2c_initialized = ret == ESP_OK;
    if (ret == ESP_OK) {
        ret = esp_board_periph_get_handle("i2c_master", &i2c_handle);
    }
    if (i2c_initialized) {
        esp_err_t deinit_ret = esp_board_periph_deinit("i2c_master");
        if (ret == ESP_OK) {
            ret = deinit_ret;
        }
    }
    s_hook_i2c_result = ret == ESP_OK && i2c_handle != NULL ? ESP_OK : ESP_FAIL;
    return s_hook_i2c_result;
}

static esp_err_t run_runtime_c_nodes(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    esp_err_t ret = esp_board_manager_deinit();
    if (ret != ESP_OK && ret != ESP_BOARD_ERR_MANAGER_NOT_INIT) {
        return ret;
    }
    esp_board_runtime_test_reset();
    s_hook_enabled = true;
    s_hook_i2c_result = ESP_FAIL;

    void *device_handle = NULL;
    void *periph_handle = NULL;
    bool device_initialized = false;
    bool periph_initialized = false;
    ret = esp_board_manager_prepare();
    if (ret == ESP_OK && s_hook_i2c_result == ESP_OK) {
        ret = esp_board_device_set_ops("runtime_c_device", overridden_device_init,
                                       overridden_device_deinit);
    }
    if (ret == ESP_OK) {
        ret = esp_board_device_init("runtime_c_device");
        device_initialized = ret == ESP_OK;
    }
    if (ret == ESP_OK) {
        ret = esp_board_device_get_handle("runtime_c_device", &device_handle);
    }
    if (ret == ESP_OK) {
        ret = esp_board_periph_init("runtime_c_periph");
        periph_initialized = ret == ESP_OK;
    }
    if (ret == ESP_OK) {
        ret = esp_board_periph_get_handle("runtime_c_periph", &periph_handle);
    }
    if (ret == ESP_OK && (device_handle != &s_override_handle || periph_handle != &s_periph_cfg)) {
        ret = ESP_FAIL;
    }

    if (periph_initialized) {
        esp_err_t deinit_ret = esp_board_periph_deinit("runtime_c_periph");
        if (ret == ESP_OK) {
            ret = deinit_ret;
        }
    }
    if (device_initialized) {
        esp_err_t deinit_ret = esp_board_device_deinit("runtime_c_device");
        if (ret == ESP_OK) {
            ret = deinit_ret;
        }
    }
    if (s_device_tail) {
        s_device_tail->next = NULL;
    }
    if (s_handle_tail) {
        s_handle_tail->next = NULL;
    }
    if (s_periph_tail) {
        s_periph_tail->next = NULL;
    }
    s_hook_enabled = false;
    esp_board_runtime_test_reset();
    printf("runtime C nodes and hook I2C: %s\n", ret == ESP_OK ? "OK" : "FAIL");
    return ret;
}

static esp_err_t run_runtime_c_remove(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    esp_err_t ret = esp_board_manager_deinit();
    if (ret != ESP_OK && ret != ESP_BOARD_ERR_MANAGER_NOT_INIT) {
        return ret;
    }
    esp_board_runtime_test_reset();

    for (esp_board_device_desc_t *prev = g_esp_board_devices; prev && prev->next && prev->next->name;
         prev = (esp_board_device_desc_t *)prev->next) {
        const char *candidate = prev->next->name;
        for (esp_board_device_handle_t *handle_prev = g_esp_board_device_handles;
             handle_prev && handle_prev->next && handle_prev->next->name;
             handle_prev = handle_prev->next) {
            if (strcmp(handle_prev->next->name, candidate) == 0) {
                s_remove_device_name = candidate;
                break;
            }
        }
        if (s_remove_device_name) {
            break;
        }
    }
    for (esp_board_periph_desc_t *prev = g_esp_board_peripherals; prev && prev->next && prev->next->name;
         prev = (esp_board_periph_desc_t *)prev->next) {
        s_remove_periph_name = prev->next->name;
        break;
    }
    if (!s_remove_device_name || !s_remove_periph_name) {
        printf("[CASE] SKIP runtime.c_remove_s31 reason=no_removable_non_head_nodes\n");
        s_remove_device_name = NULL;
        s_remove_periph_name = NULL;
        return ESP_OK;
    }

    s_remove_enabled = true;
    ret = esp_board_manager_prepare();
    if (ret == ESP_OK) {
        esp_err_t device_ret = esp_board_device_init(s_remove_device_name);
        esp_err_t periph_ret = esp_board_periph_init(s_remove_periph_name);
        if (device_ret == ESP_OK) {
            (void)esp_board_device_deinit(s_remove_device_name);
        }
        if (periph_ret == ESP_OK) {
            (void)esp_board_periph_deinit(s_remove_periph_name);
        }
        if (device_ret == ESP_OK || periph_ret == ESP_OK) {
            ret = ESP_FAIL;
        }
    }

    if (s_remove_device_prev) {
        s_remove_device_prev->next = s_removed_device;
    }
    if (s_remove_handle_prev) {
        s_remove_handle_prev->next = s_removed_handle;
    }
    if (s_remove_periph_prev) {
        s_remove_periph_prev->next = s_removed_periph;
    }
    s_remove_enabled = false;
    s_remove_device_prev = NULL;
    s_remove_handle_prev = NULL;
    s_remove_periph_prev = NULL;
    s_remove_device_name = NULL;
    s_remove_periph_name = NULL;
    esp_board_runtime_test_reset();
    printf("runtime C remove: %s\n", ret == ESP_OK ? "OK" : "FAIL");
    return ret;
}

static esp_err_t run_runtime_c_replace(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    esp_err_t ret = esp_board_manager_deinit();
    if (ret != ESP_OK && ret != ESP_BOARD_ERR_MANAGER_NOT_INIT) {
        return ret;
    }
    esp_board_runtime_test_reset();
    s_replace_enabled = true;
    ret = esp_board_manager_prepare();

    bool device_initialized = false;
    bool periph_initialized = false;
    void *device_handle = NULL;
    void *periph_handle = NULL;
    if (ret == ESP_OK) {
        ret = esp_board_device_init("boot_button");
        device_initialized = ret == ESP_OK;
    }
    if (ret == ESP_OK) {
        ret = esp_board_device_get_handle("boot_button", &device_handle);
    }
    if (ret == ESP_OK) {
        ret = esp_board_periph_init("gpio_boot_button");
        periph_initialized = ret == ESP_OK;
    }
    if (ret == ESP_OK) {
        ret = esp_board_periph_get_handle("gpio_boot_button", &periph_handle);
    }
    if (ret == ESP_OK && (device_handle != &s_override_handle || periph_handle != &s_periph_cfg)) {
        ret = ESP_FAIL;
    }
    if (periph_initialized) {
        esp_err_t deinit_ret = esp_board_periph_deinit("gpio_boot_button");
        if (ret == ESP_OK) {
            ret = deinit_ret;
        }
    }
    if (device_initialized) {
        esp_err_t deinit_ret = esp_board_device_deinit("boot_button");
        if (ret == ESP_OK) {
            ret = deinit_ret;
        }
    }

    if (s_replace_device) {
        *s_replace_device = s_old_device;
    }
    if (s_replace_handle) {
        *s_replace_handle = s_old_handle;
    }
    if (s_replace_periph) {
        *s_replace_periph = s_old_periph;
    }
    s_replace_enabled = false;
    s_replace_device = NULL;
    s_replace_handle = NULL;
    s_replace_periph = NULL;
    esp_board_runtime_test_reset();
    printf("runtime C replace: %s\n", ret == ESP_OK ? "OK" : "FAIL");
    return ret;
}

void bmgr_register_runtime_c_node_case(void)
{
    const bmgr_test_case_t tests[] = {
        {
            .name = "runtime.c_nodes_s31",
            .group = "runtime",
            .help = "C-added device/peripheral, set_ops, hook I2C init/get/deinit",
            .run = run_runtime_c_nodes,
            .flags = 0,
        },
        {
            .name = "runtime.c_remove_s31",
            .group = "runtime",
            .help = "Remove generated device, handle and peripheral nodes in pre-init",
            .run = run_runtime_c_remove,
            .flags = 0,
        },
        {
            .name = "runtime.c_replace_s31",
            .group = "runtime",
            .help = "Replace generated device and peripheral implementations in pre-init",
            .run = run_runtime_c_replace,
            .flags = 0,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(tests, sizeof(tests) / sizeof(tests[0])));
}
