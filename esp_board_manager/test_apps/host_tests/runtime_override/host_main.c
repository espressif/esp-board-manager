/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 *
 * Host driver for runtime prepare / sparse sidecar / peripheral and device
 * APP overrides. Compiles the real esp_board_runtime_state.c,
 * esp_board_manager.c, esp_board_periph.c, and esp_board_device.c against
 * the production BMGR headers and platform stubs (no hardware).
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/task.h"
#include "dev_power_ctrl.h"
#include "esp_board_device.h"
#include "esp_board_find_utils.h"
#include "esp_board_manager.h"
#include "esp_board_manager_err.h"
#include "esp_board_periph.h"
#include "esp_board_runtime_state.h"
#ifdef HOST_TEST_BOARD_REVISION
#include "gen_board_revisions.h"
#endif  /* HOST_TEST_BOARD_REVISION */

static uint8_t s_dev_cfg = 0x11;
static uint8_t s_i2c_cfg_a = 0x21;
static uint8_t s_i2c_cfg_b = 0x22;

static esp_board_device_desc_t s_dev_no_cfg = {
    .next     = NULL,
    .name     = "gpio_button",
    .type     = "button",
    .cfg      = NULL,
    .cfg_size = 0,
};

static const esp_board_device_desc_t s_devices[] = {
    {
        .next     = &s_dev_no_cfg,
        .name     = "audio_dac",
        .type     = "audio_codec",
        .cfg      = &s_dev_cfg,
        .cfg_size = sizeof(s_dev_cfg),
    },
};

static esp_board_periph_desc_t s_periph_b = {
    .next     = NULL,
    .name     = "i2c_master_1",
    .type     = "i2c",
    .role     = ESP_BOARD_PERIPH_ROLE_MASTER,
    .cfg      = &s_i2c_cfg_b,
    .cfg_size = sizeof(s_i2c_cfg_b),
};

static const esp_board_periph_desc_t s_periphs[] = {
    {
        .next     = &s_periph_b,
        .name     = "i2c_master",
        .type     = "i2c",
        .role     = ESP_BOARD_PERIPH_ROLE_MASTER,
        .cfg      = &s_i2c_cfg_a,
        .cfg_size = sizeof(s_i2c_cfg_a),
    },
};

const esp_board_info_t g_esp_board_info = {
    .name         = "host_test_board",
    .chip         = "esp32s3",
    .version      = "1.0",
    .description  = "host runtime override test",
    .manufacturer = "test",
};

ESP_BOARD_TABLE_CONST esp_board_device_desc_t g_esp_board_devices[] = {
    {
        .next     = &s_dev_no_cfg,
        .name     = "audio_dac",
        .type     = "audio_codec",
        .cfg      = &s_dev_cfg,
        .cfg_size = sizeof(s_dev_cfg),
    },
};

ESP_BOARD_TABLE_CONST esp_board_periph_desc_t g_esp_board_peripherals[] = {
    {
        .next     = &s_periph_b,
        .name     = "i2c_master",
        .type     = "i2c",
        .role     = ESP_BOARD_PERIPH_ROLE_MASTER,
        .cfg      = &s_i2c_cfg_a,
        .cfg_size = sizeof(s_i2c_cfg_a),
    },
};

static int s_shared_init_count;
static int s_shared_deinit_count;
static int s_ops_a_init_count;
static int s_ops_b_init_count;
static int s_ops_a_deinit_count;
static int s_ops_b_deinit_count;
static int s_dev_init_count;
static int s_dev_deinit_count;
static int s_dev_ops_a_init_count;
static int s_dev_ops_a_deinit_count;
static int s_dev_ops_b_init_count;
static int s_dev_ops_b_deinit_count;
static void *s_last_init_cfg;
static int s_last_init_cfg_size;
static void *s_last_dev_init_cfg;
static int s_last_dev_init_cfg_size;

static int audio_dac_init(void *cfg, int cfg_size, void **device_handle)
{
    s_dev_init_count++;
    s_last_dev_init_cfg = cfg;
    s_last_dev_init_cfg_size = cfg_size;
    *device_handle = (void *)(uintptr_t)0x4000;
    return ESP_OK;
}

static int audio_dac_deinit(void *device_handle)
{
    (void)device_handle;
    s_dev_deinit_count++;
    return ESP_OK;
}

static int gpio_button_init(void *cfg, int cfg_size, void **device_handle)
{
    (void)cfg;
    (void)cfg_size;
    *device_handle = (void *)(uintptr_t)0x4001;
    return ESP_OK;
}

static int gpio_button_deinit(void *device_handle)
{
    (void)device_handle;
    return ESP_OK;
}

static int dev_ops_a_init(void *cfg, int cfg_size, void **device_handle)
{
    s_dev_ops_a_init_count++;
    s_last_dev_init_cfg = cfg;
    s_last_dev_init_cfg_size = cfg_size;
    *device_handle = (void *)(uintptr_t)0x4100;
    return ESP_OK;
}

static int dev_ops_a_deinit(void *device_handle)
{
    (void)device_handle;
    s_dev_ops_a_deinit_count++;
    return ESP_OK;
}

static int dev_ops_b_init(void *cfg, int cfg_size, void **device_handle)
{
    s_dev_ops_b_init_count++;
    s_last_dev_init_cfg = cfg;
    s_last_dev_init_cfg_size = cfg_size;
    *device_handle = (void *)(uintptr_t)0x4200;
    return ESP_OK;
}

static int dev_ops_b_deinit(void *device_handle)
{
    (void)device_handle;
    s_dev_ops_b_deinit_count++;
    return ESP_OK;
}

static esp_board_device_handle_t s_dev_handle_no_cfg = {
    .next   = NULL,
    .name   = "gpio_button",
    .type   = "button",
    .init   = gpio_button_init,
    .deinit = gpio_button_deinit,
};

esp_board_device_handle_t g_esp_board_device_handles[] = {
    {
        .next   = &s_dev_handle_no_cfg,
        .name   = "audio_dac",
        .type   = "audio_codec",
        .init   = audio_dac_init,
        .deinit = audio_dac_deinit,
    },
};

static esp_err_t shared_init(void *cfg, int cfg_size, void **periph_handle)
{
    s_shared_init_count++;
    s_last_init_cfg = cfg;
    s_last_init_cfg_size = cfg_size;
    *periph_handle = (void *)(uintptr_t)0x1000;
    return ESP_OK;
}

static esp_err_t shared_deinit(void *periph_handle)
{
    (void)periph_handle;
    s_shared_deinit_count++;
    return ESP_OK;
}

static esp_err_t ops_a_init(void *cfg, int cfg_size, void **periph_handle)
{
    s_ops_a_init_count++;
    s_last_init_cfg = cfg;
    s_last_init_cfg_size = cfg_size;
    *periph_handle = (void *)(uintptr_t)0x2000;
    return ESP_OK;
}

static esp_err_t ops_a_deinit(void *periph_handle)
{
    (void)periph_handle;
    s_ops_a_deinit_count++;
    return ESP_OK;
}

static esp_err_t ops_b_init(void *cfg, int cfg_size, void **periph_handle)
{
    s_ops_b_init_count++;
    s_last_init_cfg = cfg;
    s_last_init_cfg_size = cfg_size;
    *periph_handle = (void *)(uintptr_t)0x3000;
    return ESP_OK;
}

static esp_err_t ops_b_deinit(void *periph_handle)
{
    (void)periph_handle;
    s_ops_b_deinit_count++;
    return ESP_OK;
}

esp_board_periph_entry_t g_esp_board_periph_handles[] = {
    {
        .next   = NULL,
        .type   = "i2c",
        .role   = ESP_BOARD_PERIPH_ROLE_MASTER,
        .init   = shared_init,
        .deinit = shared_deinit,
    },
};

typedef enum {
    HOOK_OK = 0,
    HOOK_FAIL_AFTER_OVERRIDE,
    HOOK_REENTER,
    HOOK_BOARD_OVERRIDE,
    HOOK_UNKNOWN,
    HOOK_BOARD_PERIPH_CFG_OPS,
    HOOK_BOARD_DEVICE_CFG_OPS,
    HOOK_CALLS_INIT,
    HOOK_NON_OWNER,
    HOOK_CONCURRENT_PREPARE,
    HOOK_DIRECT_TABLE_INVALID,
    HOOK_DIRECT_TABLE_FAIL,
    HOOK_DIRECT_APPEND_INVALID,
    HOOK_DIRECT_CYCLE,
    HOOK_PERIPH_ONLY_SET_OPS,
    HOOK_HANDLE_LINK_FAIL,
    HOOK_ACTIVE_CFG_FAIL,
    HOOK_ACTIVE_DEVICE_FAIL,
    HOOK_TOPOLOGY_ADD,
    HOOK_TOPOLOGY_ADD_FAIL,
    HOOK_TOPOLOGY_REMOVE,
    HOOK_TOPOLOGY_REMOVE_FAIL,
    HOOK_TOPOLOGY_REPLACE,
    HOOK_TOPOLOGY_REPLACE_FAIL,
    HOOK_INACTIVE_HANDLE_STATE_FAIL,
    HOOK_ACTIVE_CFG_LOOKUP_FAIL,
    HOOK_PARTIAL_BASE_FAIL,
    HOOK_PARTIAL_REPLACE_FAIL,
    HOOK_PARTIAL_ADD_FAIL,
    HOOK_PARTIAL_PREFLIGHT_FAIL,
    HOOK_PARTIAL_PERIPH_PREFLIGHT_FAIL,
    HOOK_PARTIAL_HANDLE_REPLACE_FAIL,
#ifdef HOST_TEST_BOARD_REVISION
    HOOK_PARTIAL_REVISION_FAIL,
#endif  /* HOST_TEST_BOARD_REVISION */
} hook_mode_t;

static int partial_device_init(void *cfg, int cfg_size, void **device_handle);
static int partial_device_deinit(void *device_handle);

static esp_board_device_handle_t s_replacement_handle = {
    .name   = "gpio_button",
    .type   = "button",
    .init   = gpio_button_init,
    .deinit = gpio_button_deinit,
};

static hook_mode_t s_hook_mode = HOOK_OK;
static int s_failures = 0;
static int s_hook_call_count = 0;
static esp_err_t s_hook_nested_init_ret = ESP_OK;
static esp_err_t s_hook_owner_get_ret = ESP_FAIL;
static esp_err_t s_hook_non_owner_get_ret = ESP_FAIL;
static esp_err_t s_non_owner_override_ret = ESP_OK;
static esp_err_t s_non_owner_restore_ret = ESP_OK;
static esp_err_t s_non_owner_idle_ret = ESP_OK;
static esp_board_runtime_source_t s_non_owner_source = ESP_BOARD_RUNTIME_SOURCE_BOARD;
static esp_err_t s_concurrent_prepare_ret = ESP_OK;
static esp_err_t s_override_during_init_ret = ESP_FAIL;
static esp_err_t s_restore_during_init_ret = ESP_FAIL;
static uint8_t s_cfg_byte_during_init = 0;
static const void *s_cfg_ptr_during_init = NULL;
static int s_cfg_size_during_init = 0;

static esp_board_device_desc_t s_hook_added_device = {
    .next = NULL,
    .name = "hook_added_device",
    .type = "button",
};

static esp_board_device_handle_t s_hook_added_handle = {
    .name   = "hook_extra_device",
    .type   = "custom",
    .init   = gpio_button_init,
    .deinit = gpio_button_deinit,
};

static esp_board_periph_entry_t s_hook_added_ops = {
    .type   = "custom",
    .role   = ESP_BOARD_PERIPH_ROLE_MASTER,
    .init   = shared_init,
    .deinit = shared_deinit,
};

static esp_board_device_desc_t s_added_device_desc = {
    .name     = "c_added_device",
    .type     = "my_sensor",
    .cfg      = &s_dev_cfg,
    .cfg_size = sizeof(s_dev_cfg),
};

static esp_board_device_handle_t s_added_device_handle = {
    .name   = "c_added_device",
    .type   = "my_sensor",
    .init   = dev_ops_a_init,
    .deinit = dev_ops_a_deinit,
};

static esp_board_periph_desc_t s_added_periph_desc = {
    .name     = "c_added_periph",
    .type     = "my_bus",
    .role     = ESP_BOARD_PERIPH_ROLE_MASTER,
    .cfg      = &s_i2c_cfg_b,
    .cfg_size = sizeof(s_i2c_cfg_b),
};

static esp_board_periph_entry_t s_added_shared_ops = {
    .type   = "my_bus",
    .role   = ESP_BOARD_PERIPH_ROLE_MASTER,
    .init   = ops_a_init,
    .deinit = ops_a_deinit,
};

static esp_board_device_desc_t s_overflow_device_desc = {
    .name = "overflow_device",
    .type = "my_sensor",
};

static esp_board_periph_desc_t s_overflow_periph_desc = {
    .name = "overflow_periph",
    .type = "my_bus",
    .role = ESP_BOARD_PERIPH_ROLE_MASTER,
};

/**
 * Driver init runs after the initialized mark is set. Nested restore/override
 * must see the mark under the runtime mutex and must not free this cfg pointer.
 */
static esp_err_t probing_init_rejects_override(void *cfg, int cfg_size, void **periph_handle)
{
    uint8_t attempt = 0xEE;

    s_cfg_ptr_during_init = cfg;
    s_cfg_size_during_init = cfg_size;
    s_cfg_byte_during_init = (cfg != NULL && cfg_size > 0) ? *(const uint8_t *)cfg : 0;
    s_override_during_init_ret = esp_board_periph_override_config("i2c_master", &attempt, sizeof(attempt));
    s_restore_during_init_ret = esp_board_periph_restore_config("i2c_master");
    *periph_handle = (void *)(uintptr_t)0x2100;
    return ESP_OK;
}

static esp_err_t probing_deinit(void *periph_handle)
{
    (void)periph_handle;
    return ESP_OK;
}

#define EXPECT(cond, msg)  do {              \
    if (!(cond)) {                           \
        fprintf(stderr, "FAIL: %s\n", msg);  \
        s_failures++;                        \
    } else {                                 \
        printf("PASS: %s\n", msg);           \
    }                                        \
} while (0)

static void host_test_cleanup(void)
{
    (void)esp_board_device_deinit_all();
    (void)esp_board_periph_deinit_all();
    (void)esp_board_manager_deinit();
    esp_board_runtime_test_reset();
    host_set_current_task((TaskHandle_t)(uintptr_t)0x1);
    s_hook_mode = HOOK_OK;
    s_hook_call_count = 0;
    s_hook_nested_init_ret = ESP_OK;
    s_hook_owner_get_ret = ESP_FAIL;
    s_hook_non_owner_get_ret = ESP_FAIL;
    s_non_owner_override_ret = ESP_OK;
    s_non_owner_restore_ret = ESP_OK;
    s_non_owner_idle_ret = ESP_OK;
    s_non_owner_source = ESP_BOARD_RUNTIME_SOURCE_BOARD;
    s_concurrent_prepare_ret = ESP_OK;
    s_shared_init_count = 0;
    s_shared_deinit_count = 0;
    s_ops_a_init_count = 0;
    s_ops_b_init_count = 0;
    s_ops_a_deinit_count = 0;
    s_ops_b_deinit_count = 0;
    s_dev_init_count = 0;
    s_dev_deinit_count = 0;
    s_dev_ops_a_init_count = 0;
    s_dev_ops_a_deinit_count = 0;
    s_dev_ops_b_init_count = 0;
    s_dev_ops_b_deinit_count = 0;
    s_last_init_cfg = NULL;
    s_last_init_cfg_size = 0;
    s_last_dev_init_cfg = NULL;
    s_last_dev_init_cfg_size = 0;
}

esp_err_t esp_board_runtime_pre_init(void)
{
    uint8_t cfg = 0xA5;
    uint8_t board_cfg = 0xB0;
    uint8_t board_dev_cfg = 0xB1;
    s_hook_call_count++;
    switch (s_hook_mode) {
        case HOOK_FAIL_AFTER_OVERRIDE:
            (void)esp_board_runtime_copy_config("audio_dac", &cfg, sizeof(cfg));
            return ESP_FAIL;
        case HOOK_REENTER:
            return esp_board_manager_prepare();
        case HOOK_BOARD_OVERRIDE:
            return esp_board_runtime_copy_config("audio_dac", &cfg, sizeof(cfg));
        case HOOK_UNKNOWN:
            return esp_board_runtime_copy_config("no_such_runtime_resource", &cfg, sizeof(cfg));
        case HOOK_BOARD_PERIPH_CFG_OPS:
            if (esp_board_runtime_copy_config("i2c_master", &board_cfg, sizeof(board_cfg)) != ESP_OK) {
                return ESP_FAIL;
            }
            return esp_board_runtime_set_ops("i2c_master", (void *)ops_b_init, (void *)ops_b_deinit);
        case HOOK_BOARD_DEVICE_CFG_OPS:
            if (esp_board_runtime_copy_config("audio_dac", &board_dev_cfg, sizeof(board_dev_cfg)) != ESP_OK) {
                return ESP_FAIL;
            }
            return esp_board_runtime_set_ops("audio_dac", (void *)dev_ops_b_init, (void *)dev_ops_b_deinit);
        case HOOK_CALLS_INIT:
            s_hook_nested_init_ret = esp_board_periph_init("i2c_master");
            if (s_hook_nested_init_ret == ESP_OK) {
                void *handle = NULL;
                s_hook_owner_get_ret = esp_board_periph_get_handle("i2c_master", &handle);
            }
            return ESP_OK;
        case HOOK_NON_OWNER: {
            TaskHandle_t owner = xTaskGetCurrentTaskHandle();
            uint8_t owner_cfg = 0xB2;
            if (esp_board_runtime_copy_config("i2c_master", &owner_cfg, sizeof(owner_cfg)) != ESP_OK) {
                return ESP_FAIL;
            }
            if (esp_board_runtime_get_override_source() != ESP_BOARD_RUNTIME_SOURCE_BOARD) {
                return ESP_FAIL;
            }
            host_set_current_task((TaskHandle_t)(uintptr_t)0xDEAD);
            s_non_owner_source = esp_board_runtime_get_override_source();
            s_non_owner_idle_ret = esp_board_runtime_check_idle();
            void *handle = NULL;
            s_hook_non_owner_get_ret = esp_board_periph_get_handle("i2c_master", &handle);
            s_non_owner_override_ret =
                esp_board_runtime_copy_config("i2c_master_1", &owner_cfg, sizeof(owner_cfg));
            s_non_owner_restore_ret = esp_board_periph_restore_config("i2c_master");
            host_set_current_task(owner);
            return ESP_OK;
        }
        case HOOK_CONCURRENT_PREPARE: {
            TaskHandle_t owner = xTaskGetCurrentTaskHandle();
            host_set_current_task((TaskHandle_t)(uintptr_t)0x2);
            s_concurrent_prepare_ret = esp_board_runtime_prepare();
            host_set_current_task(owner);
            return ESP_OK;
        }
        case HOOK_DIRECT_TABLE_INVALID:
            g_esp_board_devices[0].type = NULL;
            return ESP_OK;
        case HOOK_DIRECT_TABLE_FAIL:
            g_esp_board_devices[0].type = "changed_type";
            return ESP_FAIL;
        case HOOK_DIRECT_APPEND_INVALID:
            g_esp_board_devices[0].next = &s_hook_added_device;
            return ESP_OK;
        case HOOK_DIRECT_CYCLE:
            s_dev_no_cfg.next = &g_esp_board_devices[0];
            return ESP_OK;
        case HOOK_PERIPH_ONLY_SET_OPS:
            g_esp_board_peripherals[0].type = "custom";
            return esp_board_periph_set_ops("i2c_master", ops_a_init, ops_a_deinit);
        case HOOK_HANDLE_LINK_FAIL:
            s_dev_handle_no_cfg.next = &s_hook_added_handle;
            g_esp_board_periph_handles[0].next = &s_hook_added_ops;
            return ESP_FAIL;
        case HOOK_ACTIVE_CFG_LOOKUP_FAIL:
            if (esp_board_periph_set_ops("i2c_master", shared_init, shared_deinit) != ESP_OK) {
                return ESP_FAIL;
            }
            /* fall through */
        case HOOK_ACTIVE_CFG_FAIL:
            if (esp_board_runtime_copy_config("i2c_master", &board_cfg, sizeof(board_cfg)) != ESP_OK ||
                esp_board_periph_init("i2c_master") != ESP_OK) {
                return ESP_FAIL;
            }
            return ESP_FAIL;
        case HOOK_ACTIVE_DEVICE_FAIL:
            if (esp_board_device_init("audio_dac") != ESP_OK) {
                return ESP_FAIL;
            }
            s_dev_handle_no_cfg.next = &s_hook_added_handle;
            return ESP_FAIL;
        case HOOK_TOPOLOGY_ADD:
        case HOOK_TOPOLOGY_ADD_FAIL: {
            s_dev_no_cfg.next = &s_added_device_desc;
            s_dev_handle_no_cfg.next = &s_added_device_handle;
            s_periph_b.next = &s_added_periph_desc;
            g_esp_board_periph_handles[0].next = &s_added_shared_ops;
            return s_hook_mode == HOOK_TOPOLOGY_ADD_FAIL ? ESP_FAIL : ESP_OK;
        }
        case HOOK_TOPOLOGY_REMOVE:
        case HOOK_TOPOLOGY_REMOVE_FAIL: {
            g_esp_board_devices[0].next = NULL;
            g_esp_board_peripherals[0].next = NULL;
            g_esp_board_periph_handles[0].type = NULL;
            esp_err_t ret = esp_board_periph_set_ops("i2c_master", ops_a_init, ops_a_deinit);
            return ret == ESP_OK && s_hook_mode == HOOK_TOPOLOGY_REMOVE_FAIL ? ESP_FAIL : ret;
        }
        case HOOK_TOPOLOGY_REPLACE:
        case HOOK_TOPOLOGY_REPLACE_FAIL: {
            s_dev_no_cfg.type = "my_sensor";
            s_dev_no_cfg.cfg = &s_dev_cfg;
            s_dev_no_cfg.cfg_size = sizeof(s_dev_cfg);
            s_dev_handle_no_cfg.type = "my_sensor";
            s_dev_handle_no_cfg.init = dev_ops_a_init;
            s_dev_handle_no_cfg.deinit = dev_ops_a_deinit;
            s_periph_b.type = "my_bus";
            g_esp_board_periph_handles[0].init = ops_b_init;
            g_esp_board_periph_handles[0].deinit = ops_b_deinit;
            esp_err_t ret = esp_board_periph_set_ops("i2c_master_1", ops_b_init, ops_b_deinit);
            return ret == ESP_OK && s_hook_mode == HOOK_TOPOLOGY_REPLACE_FAIL ? ESP_FAIL : ret;
        }
#ifdef HOST_TEST_BOARD_REVISION
        case HOOK_PARTIAL_REVISION_FAIL:
            if (gen_board_revision_apply_cleanup() != ESP_OK) {
                return ESP_FAIL;
            }
            s_dev_handle_no_cfg.next = &s_added_device_handle;
            if (esp_board_device_set_ops("c_added_device", partial_device_init, partial_device_deinit) != ESP_OK) {
                return ESP_FAIL;
            }
            s_hook_nested_init_ret = esp_board_device_init("c_added_device");
            return ESP_FAIL;
#endif  /* HOST_TEST_BOARD_REVISION */
        case HOOK_PARTIAL_BASE_FAIL:
        case HOOK_PARTIAL_REPLACE_FAIL:
        case HOOK_PARTIAL_ADD_FAIL:
        case HOOK_PARTIAL_PREFLIGHT_FAIL:
        case HOOK_PARTIAL_PERIPH_PREFLIGHT_FAIL:
        case HOOK_PARTIAL_HANDLE_REPLACE_FAIL: {
            const char *name = "gpio_button";
            if (s_hook_mode == HOOK_PARTIAL_HANDLE_REPLACE_FAIL) {
                g_esp_board_device_handles[0].next = &s_replacement_handle;
            } else if (s_hook_mode == HOOK_PARTIAL_REPLACE_FAIL || s_hook_mode == HOOK_PARTIAL_PREFLIGHT_FAIL ||
                       s_hook_mode == HOOK_PARTIAL_PERIPH_PREFLIGHT_FAIL) {
                s_dev_no_cfg.cfg = &s_dev_cfg;
                s_dev_no_cfg.cfg_size = sizeof(s_dev_cfg);
            } else if (s_hook_mode == HOOK_PARTIAL_ADD_FAIL) {
                s_dev_no_cfg.next = &s_added_device_desc;
                s_dev_handle_no_cfg.next = &s_added_device_handle;
                name = "c_added_device";
            }
            if (s_hook_mode != HOOK_PARTIAL_BASE_FAIL) {
                s_hook_nested_init_ret = esp_board_device_set_ops(name, partial_device_init, partial_device_deinit);
            } else {
                s_hook_nested_init_ret = esp_board_device_override_config(name, &board_dev_cfg, sizeof(board_dev_cfg));
            }
            if (s_hook_nested_init_ret != ESP_OK) {
                return s_hook_nested_init_ret;
            }
            if (s_hook_mode == HOOK_PARTIAL_BASE_FAIL &&
                esp_board_device_set_ops(name, partial_device_init, partial_device_deinit) != ESP_OK) {
                return ESP_FAIL;
            }
            s_hook_nested_init_ret = esp_board_device_init(name);
            if (s_hook_mode == HOOK_PARTIAL_PERIPH_PREFLIGHT_FAIL) {
                s_periph_b.next = &g_esp_board_peripherals[0];
                return ESP_OK;
            }
            if (s_hook_mode == HOOK_PARTIAL_PREFLIGHT_FAIL) {
                s_dev_no_cfg.type = NULL;
                s_dev_no_cfg.next = &g_esp_board_devices[0];
                return ESP_OK;
            }
            return ESP_FAIL;
        }
        case HOOK_INACTIVE_HANDLE_STATE_FAIL:
            g_esp_board_device_handles[0].device_handle = (void *)(uintptr_t)0xDEAD;
            g_esp_board_device_handles[0].ref_count = 7;
            return ESP_FAIL;
        case HOOK_OK:
        default:
            return ESP_OK;
    }
}

static void test_config_by_handle_prepares_before_invalid_links(void)
{
    host_test_cleanup();
    g_esp_board_device_handles[0].next = &g_esp_board_device_handles[0];
    void *cfg = NULL;
    EXPECT(esp_board_device_get_config_by_handle((void *)(uintptr_t)0xDEAD, &cfg) == ESP_ERR_INVALID_STATE,
           "first config-by-handle lookup rejects a cycle instead of hanging before prepare");
    g_esp_board_device_handles[0].next = &s_dev_handle_no_cfg;
    host_test_cleanup();
}

static void test_duplicate_names(void)
{
    static const esp_board_device_desc_t dup_b = {.next = NULL, .name = "dup"};
    static const esp_board_device_desc_t dup_a = {.next = &dup_b, .name = "dup"};
    EXPECT(esp_board_runtime_check_unique_names(&dup_a, s_periphs) == ESP_ERR_INVALID_STATE,
           "duplicate device names rejected");
    EXPECT(esp_board_runtime_check_unique_names(s_devices, s_periphs) == ESP_OK,
           "unique names accepted");
}

static void test_runtime_preflight_rejects_bad_tables(void)
{
    EXPECT(esp_board_runtime_preflight() == ESP_OK,
           "preflight accepts base descriptor tables below configured capacity");

    const char *saved_power_ctrl = g_esp_board_devices[0].power_ctrl_device;
    g_esp_board_devices[0].power_ctrl_device = "missing_device";
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "preflight rejects dangling power control reference");
    g_esp_board_devices[0].power_ctrl_device = saved_power_ctrl;

    const char *const dangling_dependencies[] = {"missing_dependency"};
    const char *const *saved_dependencies = g_esp_board_devices[0].depends_on;
    size_t saved_dependency_count = g_esp_board_devices[0].depends_on_num;
    g_esp_board_devices[0].depends_on = dangling_dependencies;
    g_esp_board_devices[0].depends_on_num = 1;
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "preflight rejects dangling device dependency");
    g_esp_board_devices[0].depends_on = saved_dependencies;
    g_esp_board_devices[0].depends_on_num = saved_dependency_count;

    const char *const audio_dependency[] = {"gpio_button"};
    const char *const button_dependency[] = {"audio_dac"};
    const char *const *saved_button_dependencies = s_dev_no_cfg.depends_on;
    uint8_t saved_button_dependency_count = s_dev_no_cfg.depends_on_num;
    g_esp_board_devices[0].depends_on = audio_dependency;
    g_esp_board_devices[0].depends_on_num = 1;
    s_dev_no_cfg.depends_on = button_dependency;
    s_dev_no_cfg.depends_on_num = 1;
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "preflight rejects cyclic device dependencies");
    g_esp_board_devices[0].depends_on = saved_dependencies;
    g_esp_board_devices[0].depends_on_num = saved_dependency_count;
    s_dev_no_cfg.depends_on = saved_button_dependencies;
    s_dev_no_cfg.depends_on_num = saved_button_dependency_count;

    const char *saved_handle_name = g_esp_board_device_handles[0].name;
    g_esp_board_device_handles[0].name = "unmatched_device";
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "preflight rejects missing device handle");
    g_esp_board_device_handles[0].name = saved_handle_name;

    const char *saved_handle_type = g_esp_board_device_handles[0].type;
    g_esp_board_device_handles[0].type = NULL;
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "preflight rejects NULL device handle type");
    g_esp_board_device_handles[0].type = saved_handle_type;

    s_hook_added_handle.name = "audio_dac";
    s_dev_handle_no_cfg.next = &s_hook_added_handle;
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "preflight rejects duplicate device handle names");
    s_dev_handle_no_cfg.next = NULL;
    s_hook_added_handle.name = "hook_extra_device";

    s_dev_handle_no_cfg.next = &g_esp_board_device_handles[0];
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "preflight rejects cyclic device handle list");
    s_dev_handle_no_cfg.next = NULL;

    g_esp_board_periph_handles[0].next = &g_esp_board_periph_handles[0];
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "preflight rejects cyclic peripheral ops list");
    g_esp_board_periph_handles[0].next = NULL;

    s_hook_added_ops.type = "i2c";
    g_esp_board_periph_handles[0].next = &s_hook_added_ops;
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "preflight rejects duplicate peripheral type/role keys");
    g_esp_board_periph_handles[0].next = NULL;
    s_hook_added_ops.type = "custom";

    const char *saved_periph_type = g_esp_board_peripherals[0].type;
    g_esp_board_peripherals[0].type = "spi";
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "preflight rejects missing peripheral type/role handle");
    g_esp_board_peripherals[0].type = saved_periph_type;

    const esp_board_device_desc_t *saved_next = s_dev_no_cfg.next;
    s_dev_no_cfg.next = &s_dev_no_cfg;
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "preflight rejects cyclic descriptor list");
    s_dev_no_cfg.next = saved_next;
    EXPECT(esp_board_runtime_preflight() == ESP_OK, "preflight accepts restored tables");
}

static void test_periph_set_ops_without_shared_handler(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_PERIPH_ONLY_SET_OPS;
    EXPECT(esp_board_runtime_prepare() == ESP_OK,
           "peripheral with only per-name ops passes prepare");
    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK,
           "peripheral with only per-name ops initializes");
    EXPECT(s_ops_a_init_count == 1, "per-name init selected without shared handler");
    host_test_cleanup();
    g_esp_board_peripherals[0].type = "i2c";
}

static void test_preflight_uses_effective_ops_priority(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_BOARD_PERIPH_CFG_OPS;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "BOARD ops pass preflight");
    EXPECT(esp_board_periph_set_ops("i2c_master", ops_a_init, ops_a_deinit) == ESP_OK,
           "APP ops installed above BOARD ops");
    esp_board_runtime_node_t *node = esp_board_runtime_find_node("i2c_master");
    EXPECT(node && node->app.has_ops, "APP layer is effective");
    void *saved_deinit = node->app.deinit_fn;
    node->app.deinit_fn = NULL;
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "incomplete APP ops reject even when BOARD and shared ops exist");
    node->app.deinit_fn = saved_deinit;
    EXPECT(esp_board_runtime_preflight() == ESP_OK, "restored APP ops pass preflight");
    host_test_cleanup();
}

static void test_handle_link_rollback(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_HANDLE_LINK_FAIL;
    EXPECT(esp_board_runtime_prepare() == ESP_FAIL, "hook link failure propagates");
    EXPECT(s_dev_handle_no_cfg.next == NULL, "failed hook restores device handle link");
    EXPECT(g_esp_board_periph_handles[0].next == NULL,
           "failed hook restores peripheral ops link");
    s_hook_mode = HOOK_OK;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "prepare retries after handle link rollback");
    host_test_cleanup();
}

static void test_active_config_survives_failed_hook(void)
{
    s_hook_mode = HOOK_ACTIVE_CFG_FAIL;
    EXPECT(esp_board_runtime_prepare() == ESP_FAIL, "failed hook with active resource propagates");
    esp_board_runtime_node_t *node = esp_board_runtime_find_node("i2c_master");
    EXPECT(node && node->board.has_cfg && node->board.cfg != NULL,
           "failed prepare retains configuration still used by hardware");
    EXPECT(esp_board_runtime_prepare() == ESP_ERR_INVALID_STATE && s_hook_call_count == 1,
           "failed prepare with live peripheral cannot rerun hook");
    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_ERR_INVALID_STATE,
           "failed prepare does not offer peripheral recovery through deinit");
}

static void test_failed_hook_peripheral_cleanup_lookups(void)
{
    s_hook_mode = HOOK_ACTIVE_CFG_LOOKUP_FAIL;
    EXPECT(esp_board_runtime_prepare() == ESP_FAIL, "hook failure retains live peripheral");
    const char *name = NULL;
    void *cfg = NULL;
    EXPECT(esp_board_periph_get_name_by_handle((void *)(uintptr_t)0x1000, &name) == ESP_ERR_INVALID_STATE,
           "failed prepare rejects peripheral name lookup");
    EXPECT(esp_board_periph_get_config("i2c_master", &cfg) == ESP_ERR_INVALID_STATE,
           "failed prepare rejects config lookup rather than entering cleanup");
    EXPECT(esp_board_periph_deinit_all() == ESP_ERR_INVALID_STATE && s_shared_deinit_count == 0,
           "failed prepare bulk cleanup does not call drivers");
    EXPECT(esp_board_runtime_prepare() == ESP_ERR_INVALID_STATE && s_hook_call_count == 1,
           "failed prepare stays failed without automatic retry");
}

static void test_failed_hook_preserves_active_device_handle(void)
{
    s_hook_mode = HOOK_ACTIVE_DEVICE_FAIL;
    EXPECT(esp_board_runtime_prepare() == ESP_FAIL, "failed hook after device init propagates");
    EXPECT(s_dev_handle_no_cfg.next == &s_hook_added_handle,
           "failed prepare preserves links needed by live resources");
    EXPECT(g_esp_board_device_handles[0].device_handle != NULL &&
               g_esp_board_device_handles[0].ref_count == 1, "failed prepare retains device instance");
    EXPECT(esp_board_device_deinit("audio_dac") == ESP_ERR_INVALID_STATE,
           "failed prepare does not offer device recovery through deinit");
    uint8_t cfg = 0;
    EXPECT(esp_board_device_override_config("gpio_button", &cfg, sizeof(cfg)) == ESP_ERR_INVALID_STATE,
           "failed prepare rejects overrides even for inactive resources");
    EXPECT(esp_board_runtime_check_idle() == ESP_ERR_INVALID_STATE,
           "failed prepare rejects revision table updates");
    void *handle = NULL;
    EXPECT(esp_board_manager_get_device_handle("audio_dac", &handle) == ESP_ERR_INVALID_STATE,
           "manager device lookup reports failed prepare");
    EXPECT(esp_board_manager_get_periph_handle("i2c_master", &handle) == ESP_ERR_INVALID_STATE,
           "manager peripheral lookup reports failed prepare");
    EXPECT(esp_board_manager_init_device_by_name("missing") == ESP_ERR_INVALID_STATE,
           "manager init rejects failed prepare before descriptor lookup");
    EXPECT(esp_board_manager_print() == ESP_ERR_INVALID_STATE,
           "manager print rejects failed prepare before traversing tables");
    EXPECT(esp_board_device_callback_register("gpio_button", &s_dev_cfg, NULL) == ESP_ERR_INVALID_STATE,
           "callback registration rejects failed prepare before descriptor lookup");
    uint16_t addr = 0;
    EXPECT(esp_board_device_get_i2c_effective_addr("missing", &addr) == ESP_ERR_INVALID_STATE,
           "I2C address lookup rejects failed prepare before descriptor lookup");
    EXPECT(esp_board_device_find_by_handle(g_esp_board_device_handles[0].device_handle) == NULL,
           "reverse handle lookup rejects failed prepare");
}

static void test_direct_table_mutation_rolls_back_on_preflight_failure(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_DIRECT_TABLE_INVALID;
    EXPECT(esp_board_runtime_prepare() == ESP_ERR_INVALID_STATE,
           "prepare rejects invalid descriptor table changed by board hook");
    EXPECT(strcmp(g_esp_board_devices[0].type, "audio_codec") == 0,
           "failed prepare restores direct descriptor edits made by board hook");
    EXPECT(esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_UNPREPARED,
           "failed direct table edit leaves prepare unprepared");
    s_hook_mode = HOOK_OK;
    EXPECT(esp_board_runtime_prepare() == ESP_OK,
           "prepare can retry after direct table edit rollback");
    host_test_cleanup();

    s_hook_mode = HOOK_DIRECT_TABLE_FAIL;
    EXPECT(esp_board_runtime_prepare() == ESP_FAIL,
           "prepare propagates board hook failure after direct descriptor edit");
    EXPECT(strcmp(g_esp_board_devices[0].type, "audio_codec") == 0,
           "hook error restores direct descriptor edits");
    host_test_cleanup();

    s_hook_mode = HOOK_DIRECT_APPEND_INVALID;
    EXPECT(esp_board_runtime_prepare() == ESP_ERR_INVALID_STATE,
           "preflight rejects manually appended descriptor without handle");
    EXPECT(g_esp_board_devices[0].next == &s_dev_no_cfg,
           "failed preflight unlinks manually appended descriptor");
    host_test_cleanup();

    s_hook_mode = HOOK_DIRECT_CYCLE;
    EXPECT(esp_board_runtime_prepare() == ESP_ERR_INVALID_STATE,
           "preflight rejects cycle manually created by board hook");
    EXPECT(s_dev_no_cfg.next == NULL,
           "failed preflight restores manually changed cycle link");
    host_test_cleanup();
}

static void test_manual_topology_add(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_TOPOLOGY_ADD;
    EXPECT(esp_board_runtime_prepare() == ESP_OK,
           "prepare accepts manually appended device and peripheral nodes");
    EXPECT(esp_board_runtime_preflight() == ESP_OK,
           "three device and peripheral nodes fit configured capacity");
    s_added_device_desc.next = &s_overflow_device_desc;
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "fourth device descriptor exceeds configured capacity");
    s_added_device_desc.next = NULL;
    s_added_periph_desc.next = &s_overflow_periph_desc;
    EXPECT(esp_board_runtime_preflight() == ESP_ERR_INVALID_STATE,
           "fourth peripheral descriptor exceeds configured capacity");
    s_added_periph_desc.next = NULL;

    void *handle = NULL;
    EXPECT(esp_board_device_init("c_added_device") == ESP_OK,
           "manually added device initializes");
    EXPECT(esp_board_device_get_handle("c_added_device", &handle) == ESP_OK &&
               handle == (void *)(uintptr_t)0x4100,
           "manually added device handle is discoverable");
    EXPECT(esp_board_device_deinit("c_added_device") == ESP_OK,
           "manually added device deinitializes");

    handle = NULL;
    EXPECT(esp_board_periph_init("c_added_periph") == ESP_OK,
           "manually added peripheral uses appended shared ops");
    EXPECT(esp_board_periph_get_handle("c_added_periph", &handle) == ESP_OK &&
               handle == (void *)(uintptr_t)0x2000,
           "manually added peripheral handle is discoverable");
    EXPECT(esp_board_periph_deinit("c_added_periph") == ESP_OK,
           "manually added peripheral deinitializes");

    host_test_cleanup();
    s_dev_no_cfg.next = NULL;
    s_dev_handle_no_cfg.next = NULL;
    s_periph_b.next = NULL;
    g_esp_board_periph_handles[0].next = NULL;
}

static void test_manual_topology_remove(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_TOPOLOGY_REMOVE;
    EXPECT(esp_board_runtime_prepare() == ESP_OK,
           "prepare accepts manually removed device and peripheral nodes");
    void *handle = NULL;
    EXPECT(esp_board_device_init("gpio_button") != ESP_OK,
           "removed device is no longer discoverable for init");
    EXPECT(esp_board_periph_init("i2c_master_1") != ESP_OK,
           "removed peripheral is no longer discoverable for init");
    handle = NULL;
    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK,
           "remaining peripheral initializes with per-name ops after shared ops removal");
    EXPECT(esp_board_periph_get_handle("i2c_master", &handle) == ESP_OK &&
               handle == (void *)(uintptr_t)0x2000,
           "remaining peripheral uses per-name ops");
    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK,
           "remaining peripheral deinitializes");
    EXPECT(esp_board_device_init("audio_dac") == ESP_OK,
           "remaining device still initializes");
    EXPECT(esp_board_device_deinit("audio_dac") == ESP_OK,
           "remaining device deinitializes");

    host_test_cleanup();
    g_esp_board_devices[0].next = &s_dev_no_cfg;
    g_esp_board_peripherals[0].next = &s_periph_b;
    g_esp_board_periph_handles[0].type = "i2c";
}

static void test_manual_topology_replace(void)
{
    host_test_cleanup();
    esp_board_device_desc_t old_device = s_dev_no_cfg;
    esp_board_device_handle_t old_handle = s_dev_handle_no_cfg;
    esp_board_periph_desc_t old_periph = s_periph_b;
    esp_board_periph_entry_t old_shared = g_esp_board_periph_handles[0];
    s_hook_mode = HOOK_TOPOLOGY_REPLACE;
    EXPECT(esp_board_runtime_prepare() == ESP_OK,
           "prepare accepts manual descriptor and handle replacement");

    void *handle = NULL;
    EXPECT(esp_board_device_init("gpio_button") == ESP_OK,
           "replaced device initializes with custom type");
    EXPECT(esp_board_device_get_handle("gpio_button", &handle) == ESP_OK &&
               handle == (void *)(uintptr_t)0x4100,
           "replaced device uses updated handle ops");
    EXPECT(esp_board_device_deinit("gpio_button") == ESP_OK,
           "replaced device deinitializes");

    handle = NULL;
    EXPECT(esp_board_periph_init("i2c_master_1") == ESP_OK,
           "replaced peripheral initializes without shared type/role ops");
    EXPECT(esp_board_periph_get_handle("i2c_master_1", &handle) == ESP_OK &&
               handle == (void *)(uintptr_t)0x3000,
           "replaced peripheral uses per-name ops");
    EXPECT(esp_board_periph_deinit("i2c_master_1") == ESP_OK,
           "replaced peripheral deinitializes");

    handle = NULL;
    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK,
           "replaced shared ops initialize existing peripheral");
    EXPECT(esp_board_periph_get_handle("i2c_master", &handle) == ESP_OK &&
               handle == (void *)(uintptr_t)0x3000,
           "replaced shared ops take effect");
    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK,
           "peripheral using replaced shared ops deinitializes");

    host_test_cleanup();
    s_dev_no_cfg = old_device;
    s_dev_handle_no_cfg = old_handle;
    s_periph_b = old_periph;
    g_esp_board_periph_handles[0] = old_shared;
}

static void test_manual_topology_failure_rollback(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_TOPOLOGY_ADD_FAIL;
    EXPECT(esp_board_runtime_prepare() == ESP_FAIL, "failed append hook propagates");
    EXPECT(s_dev_no_cfg.next == NULL && s_dev_handle_no_cfg.next == NULL && s_periph_b.next == NULL &&
               g_esp_board_periph_handles[0].next == NULL,
           "failed append restores descriptor, device handle, and peripheral ops links");
    EXPECT(esp_board_runtime_find_node("c_added_periph") == NULL,
           "failed append clears per-name BOARD ops");
    s_hook_mode = HOOK_OK;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "prepare retries after failed append");
    host_test_cleanup();

    s_hook_mode = HOOK_TOPOLOGY_REMOVE_FAIL;
    EXPECT(esp_board_runtime_prepare() == ESP_FAIL, "failed remove hook propagates");
    EXPECT(g_esp_board_devices[0].next == &s_dev_no_cfg &&
               g_esp_board_peripherals[0].next == &s_periph_b &&
               strcmp(g_esp_board_periph_handles[0].type, "i2c") == 0,
           "failed remove restores descriptor links and shared ops");
    s_hook_mode = HOOK_OK;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "prepare retries after failed remove");
    host_test_cleanup();

    s_hook_mode = HOOK_TOPOLOGY_REPLACE_FAIL;
    EXPECT(esp_board_runtime_prepare() == ESP_FAIL, "failed replace hook propagates");
    EXPECT(strcmp(s_dev_no_cfg.type, "button") == 0 &&
               strcmp(s_dev_handle_no_cfg.type, "button") == 0 &&
               s_dev_handle_no_cfg.init == gpio_button_init &&
               strcmp(s_periph_b.type, "i2c") == 0 &&
               g_esp_board_periph_handles[0].init == shared_init,
           "failed replace restores descriptor, handle, and shared ops metadata");
    EXPECT(esp_board_runtime_find_node("i2c_master_1") == NULL,
           "failed replace clears per-name BOARD ops");
    s_hook_mode = HOOK_OK;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "prepare retries after failed replace");
    host_test_cleanup();
}

static void test_inactive_handle_state_rollback(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_INACTIVE_HANDLE_STATE_FAIL;
    EXPECT(esp_board_runtime_prepare() == ESP_FAIL,
           "failed hook after direct inactive handle write propagates");
    EXPECT(g_esp_board_device_handles[0].device_handle == NULL &&
               g_esp_board_device_handles[0].ref_count == 0,
           "failed hook restores inactive device handle and refcount");
    s_hook_mode = HOOK_OK;
    EXPECT(esp_board_runtime_prepare() == ESP_OK,
           "prepare retries after inactive handle state rollback");
    host_test_cleanup();
}

static void test_name_api_auto_prepares(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_BOARD_PERIPH_CFG_OPS;
    void *config = NULL;
    EXPECT(esp_board_periph_get_config("i2c_master", &config) == ESP_OK,
           "get_config auto-prepares");
    EXPECT(s_hook_call_count == 1, "name API runs board hook once");
    EXPECT(config != NULL && *(const uint8_t *)config == 0xB0,
           "name API observes hook-updated configuration");
    host_test_cleanup();
}

static void test_manager_init_rechecks_writable_tables(void)
{
    host_test_cleanup();
    EXPECT(esp_board_manager_prepare() == ESP_OK, "prepare before manual table edit");
    const char *saved_power_ctrl = g_esp_board_devices[0].power_ctrl_device;
    g_esp_board_devices[0].power_ctrl_device = "missing_device";
    EXPECT(esp_board_manager_init() == ESP_ERR_INVALID_STATE,
           "manager init rechecks writable descriptor tables");
    EXPECT(s_dev_init_count == 0 && s_shared_init_count == 0,
           "invalid table is rejected before hardware initialization");
    g_esp_board_devices[0].power_ctrl_device = saved_power_ctrl;
    host_test_cleanup();
}

static void test_unknown_board_override(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_UNKNOWN;
    EXPECT(esp_board_runtime_prepare() == ESP_BOARD_ERR_DEVICE_NOT_FOUND,
           "unknown board override rejected");
    EXPECT(esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_UNPREPARED,
           "unknown override leaves UNPREPARED");
    host_test_cleanup();
}

static void test_prepare_idempotent(void)
{
    host_test_cleanup();
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "first prepare");
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "second prepare idempotent");
    EXPECT(esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_PREPARED,
           "state stays PREPARED");
    host_test_cleanup();
}

static void test_reentry(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_REENTER;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "prepare owner reentry is idempotent");
    EXPECT(esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_PREPARED,
           "owner reentry leaves prepare successful");
    host_test_cleanup();
}

static void test_hook_fail_rollback(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_FAIL_AFTER_OVERRIDE;
    EXPECT(esp_board_runtime_prepare() == ESP_FAIL, "hook failure propagates");
    EXPECT(esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_UNPREPARED,
           "hook failure returns UNPREPARED");
    EXPECT(esp_board_runtime_find_node("audio_dac") == NULL, "BOARD sidecar rolled back");
    EXPECT(esp_board_runtime_get_override_source() == ESP_BOARD_RUNTIME_SOURCE_APP,
           "source restored to APP");
    host_test_cleanup();
}

static void test_initialized_mark_no_sidecar(void)
{
    uint8_t cfg = 0x3C;

    host_test_cleanup();

    EXPECT(esp_board_runtime_set_initialized("audio_dac", true) == ESP_OK,
           "set_initialized(true) succeeds");
    EXPECT(esp_board_runtime_is_initialized("audio_dac"), "name marked initialized");
    EXPECT(esp_board_runtime_find_node("audio_dac") == NULL,
           "set_initialized does not create sidecar");

    EXPECT(esp_board_runtime_copy_config("audio_dac", &cfg, sizeof(cfg)) == ESP_ERR_INVALID_STATE,
           "override rejected while initialized");
    EXPECT(esp_board_runtime_find_node("audio_dac") == NULL,
           "rejected override still creates no sidecar");

    EXPECT(esp_board_runtime_set_initialized("audio_dac", false) == ESP_OK,
           "set_initialized(false) succeeds");
    EXPECT(!esp_board_runtime_is_initialized("audio_dac"), "mark cleared");
    EXPECT(esp_board_runtime_copy_config("audio_dac", &cfg, sizeof(cfg)) == ESP_OK,
           "override allowed after mark clear");
    esp_board_runtime_node_t *node = esp_board_runtime_find_node("audio_dac");
    EXPECT(node != NULL && node->app.has_cfg && node->app.cfg_size == sizeof(cfg),
           "override creates sidecar with config storage");
    EXPECT(node != NULL && node->app.cfg != NULL &&
               memcmp(node->app.cfg, &cfg, sizeof(cfg)) == 0,
           "sidecar holds override bytes");

    host_test_cleanup();
}

static void test_mark_blocks_free_during_driver_init(void)
{
    const uint8_t expected = 0x4D;
    esp_board_runtime_node_t *node = NULL;

    host_test_cleanup();
    s_override_during_init_ret = ESP_FAIL;
    s_restore_during_init_ret = ESP_FAIL;
    s_cfg_byte_during_init = 0;
    s_cfg_ptr_during_init = NULL;
    s_cfg_size_during_init = 0;

    EXPECT(esp_board_periph_override_config("i2c_master", &expected, sizeof(expected)) == ESP_OK,
           "seed APP override before probing init");
    EXPECT(esp_board_periph_set_ops("i2c_master", probing_init_rejects_override, probing_deinit) ==
               ESP_OK,
           "install probing ops");
    node = esp_board_runtime_find_node("i2c_master");
    EXPECT(node != NULL && node->app.has_cfg && node->app.cfg != NULL,
           "APP config present before init");
    const void *cfg_before = node->app.cfg;

    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK, "init runs probing driver");
    EXPECT(s_override_during_init_ret == ESP_ERR_INVALID_STATE,
           "override during driver init rejected after mark");
    EXPECT(s_restore_during_init_ret == ESP_ERR_INVALID_STATE,
           "restore during driver init rejected after mark");
    EXPECT(s_cfg_ptr_during_init == cfg_before,
           "driver still saw the same APP config pointer");
    EXPECT(s_cfg_size_during_init == (int)sizeof(expected) && s_cfg_byte_during_init == expected,
           "driver still saw unfreed APP config bytes");
    node = esp_board_runtime_find_node("i2c_master");
    EXPECT(node != NULL && node->app.cfg == cfg_before &&
               node->app.cfg != NULL && *(const uint8_t *)node->app.cfg == expected,
           "APP config buffer survives nested restore/override");

    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "deinit after probing init");
    EXPECT(esp_board_runtime_get_bound_deinit("i2c_master") == NULL,
           "bound deinit cleared when mark cleared");
    host_test_cleanup();
}

static void test_size_mismatch_stores_caller_bytes(void)
{
    /* Generated cfg_size is 1; pass a longer caller buffer. */
    const uint8_t caller_cfg[4] = {0xDE, 0xAD, 0xBE, 0xEF};

    host_test_cleanup();
    EXPECT(esp_board_runtime_copy_config("audio_dac", caller_cfg, sizeof(caller_cfg)) == ESP_OK,
           "size mismatch override succeeds");
    esp_board_runtime_node_t *node = esp_board_runtime_find_node("audio_dac");
    EXPECT(node != NULL && node->app.has_cfg && node->app.cfg_size == sizeof(caller_cfg),
           "sidecar stores caller length");
    EXPECT(node != NULL && node->app.cfg != NULL &&
               memcmp(node->app.cfg, caller_cfg, sizeof(caller_cfg)) == 0,
           "sidecar stores caller bytes");
    host_test_cleanup();
}

static void test_deinit_keeps_sidecar_after_init(void)
{
    const uint8_t expected = 0xA5;

    host_test_cleanup();
    s_hook_mode = HOOK_BOARD_OVERRIDE;

    EXPECT(esp_board_manager_init() == ESP_OK, "manager init returns ESP_OK");
    EXPECT(esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_PREPARED,
           "PREPARED after init");
    esp_board_runtime_node_t *node = esp_board_runtime_find_node("audio_dac");
    EXPECT(node != NULL && node->board.has_cfg && node->board.cfg != NULL,
           "BOARD sidecar present after init");
    EXPECT(node != NULL && node->board.cfg_size == sizeof(expected) &&
               memcmp(node->board.cfg, &expected, sizeof(expected)) == 0,
           "sidecar bytes present after init");

    EXPECT(esp_board_manager_deinit() == ESP_OK, "manager deinit returns ESP_OK");
    EXPECT(esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_PREPARED,
           "deinit leaves PREPARED");
    node = esp_board_runtime_find_node("audio_dac");
    EXPECT(node != NULL && node->board.has_cfg && node->board.cfg != NULL,
           "deinit keeps BOARD sidecar");
    EXPECT(node != NULL && node->board.cfg_size == sizeof(expected) &&
               memcmp(node->board.cfg, &expected, sizeof(expected)) == 0,
           "deinit keeps sidecar bytes");

    host_test_cleanup();
}

static void test_periph_ops_isolation(void)
{
    esp_board_periph_init_func shared_init_saved = g_esp_board_periph_handles[0].init;
    esp_board_periph_deinit_func shared_deinit_saved = g_esp_board_periph_handles[0].deinit;

    host_test_cleanup();

    EXPECT(esp_board_periph_set_ops("i2c_master", ops_a_init, ops_a_deinit) == ESP_OK,
           "set_ops on i2c_master");
    EXPECT(esp_board_periph_set_ops("i2c_master_1", ops_b_init, ops_b_deinit) == ESP_OK,
           "set_ops on i2c_master_1");
    EXPECT(g_esp_board_periph_handles[0].init == shared_init_saved &&
               g_esp_board_periph_handles[0].deinit == shared_deinit_saved,
           "shared handler table unchanged");

    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK, "init i2c_master with ops_a");
    EXPECT(s_ops_a_init_count == 1 && s_ops_b_init_count == 0 && s_shared_init_count == 0,
           "only ops_a init ran for i2c_master");

    EXPECT(esp_board_periph_init("i2c_master_1") == ESP_OK, "init i2c_master_1 with ops_b");
    EXPECT(s_ops_a_init_count == 1 && s_ops_b_init_count == 1 && s_shared_init_count == 0,
           "ops isolated across same type/role names");

    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "deinit i2c_master");
    EXPECT(s_ops_a_deinit_count == 1 && s_ops_b_deinit_count == 0,
           "bound deinit_a used for i2c_master");
    EXPECT(esp_board_periph_deinit("i2c_master_1") == ESP_OK, "deinit i2c_master_1");
    EXPECT(s_ops_b_deinit_count == 1, "bound deinit_b used for i2c_master_1");

    host_test_cleanup();
}

static void test_periph_config_copy_and_get(void)
{
    uint8_t caller = 0x5A;
    void *cfg = NULL;

    host_test_cleanup();

    EXPECT(esp_board_periph_override_config("i2c_master", &caller, sizeof(caller)) == ESP_OK,
           "override_config succeeds");
    caller = 0x00;
    EXPECT(esp_board_periph_get_config("i2c_master", &cfg) == ESP_OK, "get_config after override");
    EXPECT(cfg != NULL && cfg != &caller && *(const uint8_t *)cfg == 0x5A,
           "get_config returns copied bytes");

    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK, "init uses override config");
    EXPECT(s_last_init_cfg != NULL && *(const uint8_t *)s_last_init_cfg == 0x5A &&
               s_last_init_cfg_size == (int)sizeof(caller),
           "init saw effective override config");

    EXPECT(esp_board_periph_override_config("i2c_master", &caller, sizeof(caller)) ==
               ESP_ERR_INVALID_STATE,
           "override rejected while active");
    EXPECT(esp_board_periph_set_ops("i2c_master", ops_a_init, ops_a_deinit) == ESP_ERR_INVALID_STATE,
           "set_ops rejected while active");
    EXPECT(esp_board_periph_restore_config("i2c_master") == ESP_ERR_INVALID_STATE,
           "restore_config rejected while active");
    EXPECT(esp_board_periph_restore_ops("i2c_master") == ESP_ERR_INVALID_STATE,
           "restore_ops rejected while active");

    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "deinit clears active mark");
    EXPECT(esp_board_periph_override_config("i2c_master", &caller, sizeof(caller)) == ESP_OK,
           "override allowed after ref_count 0");

    host_test_cleanup();
}

static void test_periph_restore_falls_back_to_board(void)
{
    uint8_t app_cfg = 0xAA;
    void *cfg = NULL;

    host_test_cleanup();
    s_hook_mode = HOOK_BOARD_PERIPH_CFG_OPS;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "prepare with BOARD periph overrides");

    EXPECT(esp_board_periph_override_config("i2c_master", &app_cfg, sizeof(app_cfg)) == ESP_OK,
           "APP config override");
    EXPECT(esp_board_periph_set_ops("i2c_master", ops_a_init, ops_a_deinit) == ESP_OK,
           "APP ops override");
    EXPECT(esp_board_periph_get_config("i2c_master", &cfg) == ESP_OK &&
               cfg != NULL && *(const uint8_t *)cfg == 0xAA,
           "effective config is APP");

    EXPECT(esp_board_periph_restore_config("i2c_master") == ESP_OK, "restore APP config");
    EXPECT(esp_board_periph_get_config("i2c_master", &cfg) == ESP_OK &&
               cfg != NULL && *(const uint8_t *)cfg == 0xB0,
           "effective config falls back to BOARD");

    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK, "init with APP ops + BOARD cfg");
    EXPECT(s_ops_a_init_count == 1 && s_ops_b_init_count == 0,
           "APP ops still win when config restored");
    EXPECT(s_last_init_cfg != NULL && *(const uint8_t *)s_last_init_cfg == 0xB0,
           "init used BOARD config");

    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "deinit after mixed layers");
    EXPECT(esp_board_periph_restore_ops("i2c_master") == ESP_OK, "restore APP ops");
    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK, "init after restore ops");
    EXPECT(s_ops_b_init_count == 1, "BOARD ops used after APP ops restore");

    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "final deinit");
    EXPECT(esp_board_periph_restore_config("i2c_master") == ESP_ERR_NOT_FOUND,
           "restore_config with no APP returns NOT_FOUND");
    EXPECT(esp_board_periph_restore_ops("i2c_master") == ESP_ERR_NOT_FOUND,
           "restore_ops with no APP returns NOT_FOUND");

    host_test_cleanup();
}

static void test_periph_invalid_args_and_size_mismatch(void)
{
    uint8_t cfg = 0x11;
    const uint8_t long_cfg[3] = {1, 2, 3};
    void *out = NULL;

    host_test_cleanup();

    EXPECT(esp_board_periph_override_config(NULL, &cfg, sizeof(cfg)) == ESP_BOARD_ERR_PERIPH_INVALID_ARG,
           "NULL name rejected");
    EXPECT(esp_board_periph_override_config("i2c_master", NULL, sizeof(cfg)) ==
               ESP_BOARD_ERR_PERIPH_INVALID_ARG,
           "NULL config rejected");
    EXPECT(esp_board_periph_override_config("i2c_master", &cfg, 0) == ESP_BOARD_ERR_PERIPH_INVALID_ARG,
           "size 0 rejected");
    EXPECT(esp_board_periph_set_ops(NULL, ops_a_init, ops_a_deinit) == ESP_BOARD_ERR_PERIPH_INVALID_ARG,
           "set_ops NULL name rejected");
    EXPECT(esp_board_periph_override_config("no_such_periph", &cfg, sizeof(cfg)) ==
               ESP_BOARD_ERR_PERIPH_NOT_FOUND,
           "unknown name maps to PERIPH_NOT_FOUND");
    EXPECT(esp_board_periph_restore_config("no_such_periph") == ESP_BOARD_ERR_PERIPH_NOT_FOUND,
           "restore_config unknown maps to PERIPH_NOT_FOUND");
    EXPECT(esp_board_periph_restore_ops("no_such_periph") == ESP_BOARD_ERR_PERIPH_NOT_FOUND,
           "restore_ops unknown maps to PERIPH_NOT_FOUND");
    EXPECT(esp_board_periph_override_config("i2c_master", long_cfg, sizeof(long_cfg)) == ESP_OK,
           "size mismatch still succeeds");
    EXPECT(esp_board_periph_get_config("i2c_master", &out) == ESP_OK && out != NULL &&
               memcmp(out, long_cfg, sizeof(long_cfg)) == 0,
           "get_config stores caller length bytes");
    EXPECT(esp_board_periph_init("i2c_master") == ESP_ERR_INVALID_SIZE,
           "peripheral init rejects mismatched override size");
    EXPECT(s_shared_init_count == 0, "mismatched peripheral config never reaches driver");

    host_test_cleanup();
}

static void test_periph_restore_ignores_device_sidecar(void)
{
    const uint8_t device_cfg = 0xD1;

    host_test_cleanup();
    EXPECT(esp_board_runtime_copy_config("audio_dac", &device_cfg, sizeof(device_cfg)) == ESP_OK,
           "device APP sidecar installed");
    esp_board_runtime_node_t *node = esp_board_runtime_find_node("audio_dac");
    EXPECT(node != NULL && node->app.has_cfg &&
               node->app.cfg != NULL && *(const uint8_t *)node->app.cfg == 0xD1,
           "device APP config present before periph restore");

    EXPECT(esp_board_periph_restore_config("audio_dac") == ESP_BOARD_ERR_PERIPH_NOT_FOUND,
           "periph restore rejects device-only name");
    EXPECT(esp_board_periph_restore_ops("audio_dac") == ESP_BOARD_ERR_PERIPH_NOT_FOUND,
           "periph restore_ops rejects device-only name");

    node = esp_board_runtime_find_node("audio_dac");
    EXPECT(node != NULL && node->app.has_cfg &&
               node->app.cfg != NULL && *(const uint8_t *)node->app.cfg == 0xD1,
           "device APP sidecar survives periph restore_config");

    host_test_cleanup();
}

static void test_init_custom_marks_initialized(void)
{
    uint8_t cfg = 0x11;
    esp_board_periph_init_func shared_init_saved = g_esp_board_periph_handles[0].init;
    esp_board_periph_deinit_func shared_deinit_saved = g_esp_board_periph_handles[0].deinit;

    host_test_cleanup();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    EXPECT(esp_board_periph_init_custom("i2c_master", ops_a_init, ops_a_deinit) == ESP_OK,
           "init_custom succeeds");
#pragma GCC diagnostic pop
    EXPECT(g_esp_board_periph_handles[0].init == ops_a_init &&
               g_esp_board_periph_handles[0].deinit == ops_a_deinit,
           "init_custom updates shared type/role handler");
    EXPECT(esp_board_runtime_is_initialized("i2c_master"),
           "init_custom sets initialized mark");
    EXPECT(esp_board_periph_override_config("i2c_master", &cfg, sizeof(cfg)) == ESP_ERR_INVALID_STATE,
           "override rejected while custom init is live");

    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "deinit after init_custom");
    EXPECT(!esp_board_runtime_is_initialized("i2c_master"),
           "deinit clears initialized mark from init_custom");
    EXPECT(esp_board_periph_override_config("i2c_master", &cfg, sizeof(cfg)) == ESP_OK,
           "override allowed after custom deinit");

    /* Restore shared handler table mutated by init_custom */
    g_esp_board_periph_handles[0].init = shared_init_saved;
    g_esp_board_periph_handles[0].deinit = shared_deinit_saved;

    host_test_cleanup();
}

static void test_set_ops_isolated_from_init_custom(void)
{
    esp_board_periph_init_func shared_init_saved = g_esp_board_periph_handles[0].init;
    esp_board_periph_deinit_func shared_deinit_saved = g_esp_board_periph_handles[0].deinit;

    host_test_cleanup();

    /* set_ops first, then init_custom on a different name of the same type/role. */
    EXPECT(esp_board_periph_set_ops("i2c_master", ops_a_init, ops_a_deinit) == ESP_OK,
           "set_ops on i2c_master before init_custom");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    EXPECT(esp_board_periph_init_custom("i2c_master_1", ops_b_init, ops_b_deinit) == ESP_OK,
           "init_custom on i2c_master_1");
#pragma GCC diagnostic pop
    EXPECT(g_esp_board_periph_handles[0].init == ops_b_init &&
               g_esp_board_periph_handles[0].deinit == ops_b_deinit,
           "init_custom still rewrites shared handler");
    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK, "init i2c_master after peer init_custom");
    EXPECT(s_ops_a_init_count == 1 && s_ops_b_init_count == 1 && s_shared_init_count == 0,
           "per-name set_ops wins over later shared-handler rewrite");

    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "deinit i2c_master");
    EXPECT(esp_board_periph_deinit("i2c_master_1") == ESP_OK, "deinit i2c_master_1");
    g_esp_board_periph_handles[0].init = shared_init_saved;
    g_esp_board_periph_handles[0].deinit = shared_deinit_saved;
    host_test_cleanup();

    /* init_custom first, then set_ops on a different name. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    EXPECT(esp_board_periph_init_custom("i2c_master_1", ops_b_init, ops_b_deinit) == ESP_OK,
           "init_custom first on i2c_master_1");
#pragma GCC diagnostic pop
    EXPECT(esp_board_periph_set_ops("i2c_master", ops_a_init, ops_a_deinit) == ESP_OK,
           "set_ops after peer init_custom");
    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK, "init i2c_master after earlier init_custom");
    EXPECT(s_ops_a_init_count == 1 && s_ops_b_init_count == 1 && s_shared_init_count == 0,
           "per-name set_ops wins over earlier shared-handler rewrite");

    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "deinit i2c_master second case");
    EXPECT(esp_board_periph_deinit("i2c_master_1") == ESP_OK, "deinit i2c_master_1 second case");
    g_esp_board_periph_handles[0].init = shared_init_saved;
    g_esp_board_periph_handles[0].deinit = shared_deinit_saved;
    host_test_cleanup();
}

static int reg_custom_init(void *cfg, int cfg_size, void **device_handle)
{
    (void)cfg;
    (void)cfg_size;
    *device_handle = (void *)(uintptr_t)0x4300;
    return ESP_OK;
}

static int reg_custom_deinit(void *device_handle)
{
    (void)device_handle;
    return ESP_OK;
}

static void test_register_device_handle_no_sidecar(void)
{
    esp_board_device_handle_t *tail_saved = s_dev_handle_no_cfg.next;
    static esp_board_device_handle_t s_reg = {
        .next = NULL,
        .name = "custom_injected",
        .type = "custom",
        .device_handle = NULL,
        .init = reg_custom_init,
        .deinit = reg_custom_deinit,
    };

    host_test_cleanup();
    s_reg.next = NULL;
    s_reg.device_handle = NULL;
    s_reg.ref_count = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    EXPECT(esp_board_manager_register_device_handle(&s_reg) == ESP_OK,
           "register_device_handle inserts handle");
#pragma GCC diagnostic pop
    EXPECT(esp_board_find_device_handle("custom_injected") == &s_reg,
           "registered handle is findable by name");
    EXPECT(esp_board_runtime_find_node("custom_injected") == NULL,
           "register_device_handle does not create sidecar");
    EXPECT(s_dev_handle_no_cfg.next == &s_reg, "handle spliced onto generated list");
    static esp_board_device_handle_t s_reg_extra[2];
    s_reg_extra[0] = s_reg;
    s_reg_extra[0].name = "legacy_extra_a";
    s_reg_extra[1] = s_reg;
    s_reg_extra[1].name = "legacy_extra_b";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    EXPECT(esp_board_manager_register_device_handle(&s_reg_extra[0]) == ESP_OK,
           "register legacy handle above configured descriptor capacity");
    EXPECT(esp_board_manager_register_device_handle(&s_reg_extra[1]) == ESP_OK,
           "register second legacy handle above snapshot initial capacity");
#pragma GCC diagnostic pop
    EXPECT(esp_board_runtime_preflight() == ESP_OK,
           "legacy handle beyond descriptor capacity passes preflight");
    EXPECT(esp_board_manager_prepare() == ESP_OK,
           "prepare snapshots legacy handle beyond descriptor capacity");

    /* Unsplice so later tests see the original list. */
    s_dev_handle_no_cfg.next = tail_saved;
    host_test_cleanup();
}

static void test_device_config_copy_and_get(void)
{
    uint8_t caller = 0x5A;
    uint8_t replace = 0x5B;
    void *cfg = NULL;
    esp_board_device_init_func saved_init = g_esp_board_device_handles[0].init;
    esp_board_device_deinit_func saved_deinit = g_esp_board_device_handles[0].deinit;

    host_test_cleanup();

    EXPECT(esp_board_device_override_config("audio_dac", &caller, sizeof(caller)) == ESP_OK,
           "device override_config succeeds");
    caller = 0x00;
    EXPECT(esp_board_device_get_config("audio_dac", &cfg) == ESP_OK, "device get_config after override");
    EXPECT(cfg != NULL && cfg != &caller && *(const uint8_t *)cfg == 0x5A,
           "device get_config returns copied bytes");

    EXPECT(esp_board_device_override_config("audio_dac", &replace, sizeof(replace)) == ESP_OK,
           "repeated device override replaces bytes");
    EXPECT(esp_board_device_get_config("audio_dac", &cfg) == ESP_OK &&
               cfg != NULL && *(const uint8_t *)cfg == 0x5B,
           "replaced override visible via get_config");

    EXPECT(esp_board_device_init("audio_dac") == ESP_OK, "device init uses override config");
    EXPECT(s_last_dev_init_cfg != NULL && *(const uint8_t *)s_last_dev_init_cfg == 0x5B &&
               s_last_dev_init_cfg_size == (int)sizeof(replace),
           "device init saw effective override config");
    EXPECT(g_esp_board_device_handles[0].init == saved_init &&
               g_esp_board_device_handles[0].deinit == saved_deinit,
           "generated device ops table unchanged by override");

    EXPECT(esp_board_device_override_config("audio_dac", &caller, sizeof(caller)) ==
               ESP_ERR_INVALID_STATE,
           "device override rejected while active");
    EXPECT(esp_board_device_set_ops("audio_dac", dev_ops_a_init, dev_ops_a_deinit) ==
               ESP_ERR_INVALID_STATE,
           "device set_ops rejected while active");
    EXPECT(esp_board_device_restore_config("audio_dac") == ESP_ERR_INVALID_STATE,
           "device restore_config rejected while active");
    EXPECT(esp_board_device_restore_ops("audio_dac") == ESP_ERR_INVALID_STATE,
           "device restore_ops rejected while active");

    EXPECT(esp_board_device_deinit("audio_dac") == ESP_OK, "device deinit clears active mark");
    EXPECT(s_dev_deinit_count == 1, "bound generated deinit used");
    EXPECT(esp_board_device_override_config("audio_dac", &caller, sizeof(caller)) == ESP_OK,
           "device override allowed after ref_count 0");

    host_test_cleanup();
}

static void test_device_restore_falls_back_to_board(void)
{
    uint8_t app_cfg = 0xAA;
    void *cfg = NULL;
    esp_board_device_init_func saved_init = g_esp_board_device_handles[0].init;
    esp_board_device_deinit_func saved_deinit = g_esp_board_device_handles[0].deinit;

    host_test_cleanup();
    s_hook_mode = HOOK_BOARD_DEVICE_CFG_OPS;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "prepare with BOARD device overrides");

    EXPECT(esp_board_device_override_config("audio_dac", &app_cfg, sizeof(app_cfg)) == ESP_OK,
           "device APP config override");
    EXPECT(esp_board_device_set_ops("audio_dac", dev_ops_a_init, dev_ops_a_deinit) == ESP_OK,
           "device APP ops override");
    EXPECT(g_esp_board_device_handles[0].init == saved_init &&
               g_esp_board_device_handles[0].deinit == saved_deinit,
           "set_ops does not mutate generated device table");
    EXPECT(esp_board_device_get_config("audio_dac", &cfg) == ESP_OK &&
               cfg != NULL && *(const uint8_t *)cfg == 0xAA,
           "device effective config is APP");

    EXPECT(esp_board_device_restore_config("audio_dac") == ESP_OK, "device restore APP config");
    EXPECT(esp_board_device_get_config("audio_dac", &cfg) == ESP_OK &&
               cfg != NULL && *(const uint8_t *)cfg == 0xB1,
           "device effective config falls back to BOARD");

    EXPECT(esp_board_device_init("audio_dac") == ESP_OK, "device init with APP ops + BOARD cfg");
    EXPECT(s_dev_ops_a_init_count == 1 && s_dev_ops_b_init_count == 0 && s_dev_init_count == 0,
           "device APP ops still win when config restored");
    EXPECT(s_last_dev_init_cfg != NULL && *(const uint8_t *)s_last_dev_init_cfg == 0xB1,
           "device init used BOARD config");

    EXPECT(esp_board_device_deinit("audio_dac") == ESP_OK, "device deinit after mixed layers");
    EXPECT(s_dev_ops_a_deinit_count == 1, "bound APP deinit used");
    EXPECT(esp_board_device_restore_ops("audio_dac") == ESP_OK, "device restore APP ops");
    EXPECT(esp_board_device_init("audio_dac") == ESP_OK, "device init after restore ops");
    EXPECT(s_dev_ops_b_init_count == 1, "BOARD device ops used after APP ops restore");

    EXPECT(esp_board_device_deinit("audio_dac") == ESP_OK, "final device deinit");
    EXPECT(s_dev_ops_b_deinit_count == 1, "bound BOARD deinit used");
    EXPECT(esp_board_device_restore_config("audio_dac") == ESP_ERR_NOT_FOUND,
           "device restore_config with no APP returns NOT_FOUND");
    EXPECT(esp_board_device_restore_ops("audio_dac") == ESP_ERR_NOT_FOUND,
           "device restore_ops with no APP returns NOT_FOUND");

    host_test_cleanup();
}

static void test_device_restore_ignores_periph_sidecar(void)
{
    const uint8_t periph_cfg = 0xC1;

    host_test_cleanup();
    EXPECT(esp_board_runtime_copy_config("i2c_master", &periph_cfg, sizeof(periph_cfg)) == ESP_OK,
           "periph APP sidecar installed");
    esp_board_runtime_node_t *node = esp_board_runtime_find_node("i2c_master");
    EXPECT(node != NULL && node->app.has_cfg &&
               node->app.cfg != NULL && *(const uint8_t *)node->app.cfg == 0xC1,
           "periph APP config present before device restore");

    EXPECT(esp_board_device_restore_config("i2c_master") == ESP_BOARD_ERR_DEVICE_NOT_FOUND,
           "device restore rejects periph-only name");
    EXPECT(esp_board_device_restore_ops("i2c_master") == ESP_BOARD_ERR_DEVICE_NOT_FOUND,
           "device restore_ops rejects periph-only name");

    node = esp_board_runtime_find_node("i2c_master");
    EXPECT(node != NULL && node->app.has_cfg &&
               node->app.cfg != NULL && *(const uint8_t *)node->app.cfg == 0xC1,
           "periph APP sidecar survives device restore_config");

    host_test_cleanup();
}

static void test_device_no_config_and_invalid_args(void)
{
    uint8_t cfg = 0x11;
    const uint8_t long_cfg[3] = {1, 2, 3};
    void *out = NULL;

    host_test_cleanup();

    EXPECT(esp_board_device_override_config(NULL, &cfg, sizeof(cfg)) == ESP_BOARD_ERR_DEVICE_INVALID_ARG,
           "device NULL name rejected");
    EXPECT(esp_board_device_override_config("audio_dac", NULL, sizeof(cfg)) ==
               ESP_BOARD_ERR_DEVICE_INVALID_ARG,
           "device NULL config rejected");
    EXPECT(esp_board_device_override_config("audio_dac", &cfg, 0) == ESP_BOARD_ERR_DEVICE_INVALID_ARG,
           "device size 0 rejected");
    EXPECT(esp_board_device_override_config("gpio_button", &cfg, sizeof(cfg)) ==
               ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED,
           "no-config device override rejected");
    EXPECT(esp_board_device_override_config("no_such_device", &cfg, sizeof(cfg)) ==
               ESP_BOARD_ERR_DEVICE_NOT_FOUND,
           "unknown device maps to DEVICE_NOT_FOUND");
    EXPECT(esp_board_device_restore_config("no_such_device") == ESP_BOARD_ERR_DEVICE_NOT_FOUND,
           "device restore_config unknown maps to DEVICE_NOT_FOUND");
    EXPECT(esp_board_device_restore_ops("no_such_device") == ESP_BOARD_ERR_DEVICE_NOT_FOUND,
           "device restore_ops unknown maps to DEVICE_NOT_FOUND");
    EXPECT(esp_board_device_override_config("audio_dac", long_cfg, sizeof(long_cfg)) == ESP_OK,
           "device size mismatch still succeeds");
    EXPECT(esp_board_device_get_config("audio_dac", &out) == ESP_OK && out != NULL &&
               memcmp(out, long_cfg, sizeof(long_cfg)) == 0,
           "device get_config stores caller length bytes");
    EXPECT(esp_board_device_init("audio_dac") == ESP_ERR_INVALID_SIZE,
           "device init rejects mismatched override size");
    EXPECT(s_dev_init_count == 0, "mismatched device config never reaches driver");

    host_test_cleanup();
}

static void test_auto_prepare_on_periph_and_device_init(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_BOARD_PERIPH_CFG_OPS;

    EXPECT(s_hook_call_count == 0, "hook not run before first init");
    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK, "periph init auto-prepares");
    EXPECT(s_hook_call_count == 1, "hook ran once before periph handle");
    EXPECT(esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_PREPARED,
           "PREPARED after first periph init");
    EXPECT(esp_board_runtime_get_override_source() == ESP_BOARD_RUNTIME_SOURCE_APP,
           "source is APP after successful prepare");
    EXPECT(s_ops_b_init_count == 1 && s_shared_init_count == 0,
           "BOARD ops applied before handle creation");
    EXPECT(s_last_init_cfg != NULL && *(const uint8_t *)s_last_init_cfg == 0xB0,
           "BOARD config applied before handle creation");

    EXPECT(esp_board_periph_init("i2c_master_1") == ESP_OK, "second periph init");
    EXPECT(s_hook_call_count == 1, "hook not run again on second periph init");

    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "deinit i2c_master");
    EXPECT(esp_board_periph_deinit("i2c_master_1") == ESP_OK, "deinit i2c_master_1");
    host_test_cleanup();

    s_hook_mode = HOOK_BOARD_DEVICE_CFG_OPS;
    EXPECT(esp_board_device_init("audio_dac") == ESP_OK, "device init auto-prepares");
    EXPECT(s_hook_call_count == 1, "hook ran once before device handle");
    EXPECT(s_dev_ops_b_init_count == 1 && s_dev_init_count == 0,
           "BOARD device ops applied before handle");
    EXPECT(s_last_dev_init_cfg != NULL && *(const uint8_t *)s_last_dev_init_cfg == 0xB1,
           "BOARD device config applied before handle");
    EXPECT(esp_board_device_init("gpio_button") == ESP_OK, "second device init");
    EXPECT(s_hook_call_count == 1, "hook not run again on second device init");

    EXPECT(esp_board_device_deinit("audio_dac") == ESP_OK, "deinit audio_dac");
    EXPECT(esp_board_device_deinit("gpio_button") == ESP_OK, "deinit gpio_button");
    host_test_cleanup();
}

static void test_app_wins_over_board(void)
{
    uint8_t app_cfg = 0xAA;

    host_test_cleanup();
    s_hook_mode = HOOK_BOARD_PERIPH_CFG_OPS;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "prepare BOARD layers");
    EXPECT(esp_board_runtime_get_override_source() == ESP_BOARD_RUNTIME_SOURCE_APP,
           "source APP after prepare success");

    EXPECT(esp_board_periph_override_config("i2c_master", &app_cfg, sizeof(app_cfg)) == ESP_OK,
           "APP config override");
    EXPECT(esp_board_periph_set_ops("i2c_master", ops_a_init, ops_a_deinit) == ESP_OK,
           "APP ops override");
    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK, "init with both layers");
    EXPECT(s_ops_a_init_count == 1 && s_ops_b_init_count == 0,
           "APP ops win over BOARD");
    EXPECT(s_last_init_cfg != NULL && *(const uint8_t *)s_last_init_cfg == 0xAA,
           "APP config wins over BOARD");

    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "deinit after app-wins");
    host_test_cleanup();
}

static void test_generated_only_no_sidecar(void)
{
    void *cfg = NULL;

    host_test_cleanup();
    s_hook_mode = HOOK_OK;

    EXPECT(esp_board_runtime_find_node("i2c_master") == NULL, "no sidecar before init");
    EXPECT(esp_board_periph_get_config("i2c_master", &cfg) == ESP_OK && cfg != NULL,
           "get_config returns generated without sidecar");
    EXPECT(esp_board_runtime_find_node("i2c_master") == NULL,
           "get_config does not create sidecar");
    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK, "init uses generated only");
    EXPECT(s_shared_init_count == 1 && s_ops_a_init_count == 0 && s_ops_b_init_count == 0,
           "generated shared ops used with no override");
    EXPECT(esp_board_runtime_find_node("i2c_master") == NULL,
           "init without override creates no sidecar");

    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "deinit generated-only");
    host_test_cleanup();
}

static void test_config_ops_layer_independence(void)
{
    uint8_t app_cfg = 0xAA;
    void *cfg = NULL;

    host_test_cleanup();
    s_hook_mode = HOOK_BOARD_PERIPH_CFG_OPS;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "prepare BOARD config and ops");

    /* APP sets only config: BOARD ops remain effective. */
    EXPECT(esp_board_periph_override_config("i2c_master", &app_cfg, sizeof(app_cfg)) == ESP_OK,
           "APP config only");
    EXPECT(esp_board_periph_get_config("i2c_master", &cfg) == ESP_OK &&
               cfg != NULL && *(const uint8_t *)cfg == 0xAA,
           "effective config is APP");
    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK, "init APP cfg + BOARD ops");
    EXPECT(s_ops_b_init_count == 1 && s_ops_a_init_count == 0,
           "BOARD ops remain when APP set only config");
    EXPECT(s_last_init_cfg != NULL && *(const uint8_t *)s_last_init_cfg == 0xAA,
           "APP config used with BOARD ops");
    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "deinit after config-only");
    EXPECT(esp_board_periph_restore_config("i2c_master") == ESP_OK, "clear APP config");

    /* APP sets only ops: BOARD config remains effective. */
    EXPECT(esp_board_periph_set_ops("i2c_master", ops_a_init, ops_a_deinit) == ESP_OK,
           "APP ops only");
    EXPECT(esp_board_periph_get_config("i2c_master", &cfg) == ESP_OK &&
               cfg != NULL && *(const uint8_t *)cfg == 0xB0,
           "BOARD config remains when APP set only ops");
    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK, "init BOARD cfg + APP ops");
    EXPECT(s_ops_a_init_count == 1, "APP ops used with BOARD config");
    EXPECT(s_last_init_cfg != NULL && *(const uint8_t *)s_last_init_cfg == 0xB0,
           "BOARD config used with APP ops");

    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "final deinit independence");
    host_test_cleanup();
}

static void test_hook_calls_init(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_CALLS_INIT;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "prepare with nested init attempt");
    EXPECT(s_hook_nested_init_ret == ESP_OK,
           "prepare owner can initialize a peripheral during PREPARING");
    EXPECT(s_hook_owner_get_ret == ESP_OK,
           "prepare owner can get handle initialized in hook");
    EXPECT(s_shared_init_count == 1,
           "nested init created the driver handle");
    EXPECT(esp_board_runtime_is_initialized("i2c_master"),
           "nested init marked peripheral initialized");

    host_test_cleanup();
}

static void test_non_owner_rejected_during_preparing(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_NON_OWNER;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "prepare with non-owner probe");
    EXPECT(s_non_owner_idle_ret == ESP_ERR_INVALID_STATE, "non-owner cannot update runtime tables during prepare");
    EXPECT(s_non_owner_source == ESP_BOARD_RUNTIME_SOURCE_APP,
           "non-owner get_override_source is APP during PREPARING");
    EXPECT(s_hook_non_owner_get_ret == ESP_ERR_INVALID_STATE,
           "non-owner cannot query handle during PREPARING");
    EXPECT(s_non_owner_override_ret == ESP_ERR_INVALID_STATE,
           "non-owner override rejected during PREPARING");
    EXPECT(s_non_owner_restore_ret == ESP_ERR_INVALID_STATE,
           "non-owner restore rejected during PREPARING");
    EXPECT(esp_board_runtime_find_node("i2c_master_1") == NULL,
           "non-owner did not write any layer");
    esp_board_runtime_node_t *node = esp_board_runtime_find_node("i2c_master");
    EXPECT(node != NULL && node->board.has_cfg &&
               node->board.cfg != NULL && *(const uint8_t *)node->board.cfg == 0xB2,
           "owner BOARD write still succeeded");
    EXPECT(esp_board_runtime_get_override_source() == ESP_BOARD_RUNTIME_SOURCE_APP,
           "source APP after prepare exit");

    /* Former prepare task writes APP outside PREPARING. */
    uint8_t app_cfg = 0xCC;
    EXPECT(esp_board_runtime_copy_config("i2c_master", &app_cfg, sizeof(app_cfg)) == ESP_OK,
           "former owner writes APP after PREPARED");
    node = esp_board_runtime_find_node("i2c_master");
    EXPECT(node != NULL && node->app.has_cfg &&
               node->app.cfg != NULL && *(const uint8_t *)node->app.cfg == 0xCC,
           "post-prepare write landed in APP");

    host_test_cleanup();
}

static void test_init_custom_also_prepares(void)
{
    esp_board_periph_init_func shared_init_saved = g_esp_board_periph_handles[0].init;
    esp_board_periph_deinit_func shared_deinit_saved = g_esp_board_periph_handles[0].deinit;

    host_test_cleanup();
    s_hook_mode = HOOK_BOARD_PERIPH_CFG_OPS;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    EXPECT(esp_board_periph_init_custom("i2c_master", ops_a_init, ops_a_deinit) == ESP_OK,
           "init_custom auto-prepares");
#pragma GCC diagnostic pop
    EXPECT(s_hook_call_count == 1, "hook ran before init_custom handle");
    EXPECT(esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_PREPARED,
           "PREPARED after init_custom");

    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK, "deinit after init_custom prepare");
    g_esp_board_periph_handles[0].init = shared_init_saved;
    g_esp_board_periph_handles[0].deinit = shared_deinit_saved;
    host_test_cleanup();
}

static void test_null_task_prepare_rejected(void)
{
    host_test_cleanup();
    host_set_current_task(NULL);
    int hooks_before = s_hook_call_count;
    EXPECT(esp_board_runtime_prepare() == ESP_ERR_INVALID_STATE,
           "NULL task prepare rejected");
    EXPECT(s_hook_call_count == hooks_before, "NULL task does not call hook");
    EXPECT(esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_UNPREPARED,
           "NULL task leaves UNPREPARED");
    host_set_current_task((TaskHandle_t)(uintptr_t)0x1);
    host_test_cleanup();
}

static void test_concurrent_prepare_rejected(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_CONCURRENT_PREPARE;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "owner prepare succeeds");
    EXPECT(s_concurrent_prepare_ret == ESP_ERR_INVALID_STATE,
           "second prepare during hook rejected");
    EXPECT(s_hook_call_count == 1, "hook ran only for the owner prepare");
    EXPECT(esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_PREPARED,
           "owner prepare still reaches PREPARED");
    host_test_cleanup();
}

static int s_partial_init_calls;
static int s_partial_cleanup_calls;
static bool s_partial_deinit_fails;
static bool s_partial_requires_power;

static int partial_device_init(void *cfg, int cfg_size, void **device_handle)
{
    (void)cfg;
    (void)cfg_size;
    s_partial_init_calls++;
    *device_handle = (void *)(uintptr_t)0x4300;
    return ESP_FAIL;
}

static int partial_device_deinit(void *device_handle)
{
    s_partial_cleanup_calls++;
    EXPECT(device_handle == (void *)(uintptr_t)0x4300, "cleanup receives the original handle");
    if (s_partial_requires_power && g_esp_board_device_handles[0].device_handle == NULL) {
        EXPECT(false, "power controller remains live during device cleanup");
        return ESP_FAIL;
    }
    return s_partial_deinit_fails ? ESP_FAIL : ESP_OK;
}

static void test_failed_device_init_retains_resources(void)
{
    static const char *const dependencies[] = {"audio_dac"};
    s_dev_no_cfg.depends_on = dependencies;
    s_dev_no_cfg.depends_on_num = 1;
    s_partial_deinit_fails = false;
    EXPECT(esp_board_device_set_ops("gpio_button", partial_device_init, partial_device_deinit) == ESP_OK,
           "install driver that reports unreleased resources");
    EXPECT(esp_board_device_init("gpio_button") == ESP_FAIL, "failed init reports driver error");
    EXPECT(s_dev_handle_no_cfg.device_handle == (void *)(uintptr_t)0x4300 &&
               s_dev_handle_no_cfg.ref_count == 0, "failed init retains record without a successful reference");
    EXPECT(g_esp_board_device_handles[0].ref_count == 1 && esp_board_runtime_is_initialized("gpio_button"),
           "failed init retains dependencies and protects configuration");
    void *handle = NULL;
    EXPECT(esp_board_device_get_handle("gpio_button", &handle) == ESP_BOARD_ERR_DEVICE_NO_HANDLE,
           "failed init does not expose a usable handle");
    EXPECT(esp_board_device_init("gpio_button") == ESP_ERR_INVALID_STATE && s_partial_init_calls == 1,
           "failed init cannot overwrite unreleased resources");
    EXPECT(esp_board_device_restore_ops("gpio_button") == ESP_ERR_INVALID_STATE,
           "failed init prevents ops replacement");
    EXPECT(esp_board_device_deinit("audio_dac") == ESP_BOARD_ERR_DEVICE_DEP_IN_USE,
           "failed init protects dependencies");
    EXPECT(esp_board_device_deinit("gpio_button") == ESP_ERR_INVALID_STATE,
           "failed init has no caller-driven deinit recovery contract");
    EXPECT(esp_board_device_deinit_all() == ESP_FAIL && s_partial_cleanup_calls == 0,
           "device bulk cleanup skips failed init without invoking cleanup");
}

static int clean_device_init_fails(void *cfg, int cfg_size, void **device_handle)
{
    (void)cfg;
    (void)cfg_size;
    *device_handle = NULL;
    return ESP_FAIL;
}

static void test_clean_peripheral_init_failure(void)
{
    host_test_cleanup();
    EXPECT(esp_board_periph_set_ops("i2c_master", clean_device_init_fails, shared_deinit) == ESP_OK,
           "install peripheral initializer that rolls back on failure");
    EXPECT(esp_board_periph_init("i2c_master") == ESP_FAIL,
           "ordinary peripheral init failure returns driver error");
    EXPECT(!esp_board_runtime_is_initialized("i2c_master"),
           "ordinary peripheral failure releases configuration");
    EXPECT(esp_board_periph_set_ops("i2c_master", shared_init, shared_deinit) == ESP_OK,
           "ordinary peripheral failure allows ops update without deinit");
    EXPECT(esp_board_periph_init("i2c_master") == ESP_OK,
           "ordinary peripheral failure retries without deinit");
    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK,
           "successful retry can be deinitialized normally");
    host_test_cleanup();
}

static void test_dependency_init_failure(void)
{
    static const char *const dependencies[] = {"c_added_device", "gpio_button"};
    host_test_cleanup();
    s_hook_mode = HOOK_TOPOLOGY_ADD;
    EXPECT(esp_board_runtime_prepare() == ESP_OK, "prepare dependency failure fixture");
    g_esp_board_devices[0].depends_on = dependencies;
    g_esp_board_devices[0].depends_on_num = 2;
    s_partial_deinit_fails = false;
    EXPECT(esp_board_device_set_ops("gpio_button", clean_device_init_fails,
                                    partial_device_deinit) == ESP_OK, "install failing dependency");
    EXPECT(esp_board_device_init("audio_dac") == ESP_FAIL, "parent returns dependency init error");
    EXPECT(s_dev_init_count == 0 && g_esp_board_device_handles[0].device_handle == NULL,
           "parent driver is not called after dependency failure");
    EXPECT(s_added_device_handle.ref_count == 0 && s_added_device_handle.device_handle == NULL,
           "earlier successful dependency reference is released");
    EXPECT(!esp_board_runtime_is_initialized("gpio_button"), "clean dependency failure releases config");
    EXPECT(esp_board_device_set_ops("gpio_button", gpio_button_init, gpio_button_deinit) == ESP_OK,
           "clean dependency failure can retry without a deinit call");
    EXPECT(esp_board_device_init("audio_dac") == ESP_OK, "parent initializes after dependency is fixed");
    g_esp_board_devices[0].depends_on = NULL;
    g_esp_board_devices[0].depends_on_num = 0;
    host_test_cleanup();
    s_dev_no_cfg.next = NULL;
    s_dev_handle_no_cfg.next = NULL;
    s_periph_b.next = NULL;
    g_esp_board_periph_handles[0].next = NULL;
}

static int powered_device_init(void *cfg, int cfg_size, void **device_handle)
{
    (void)partial_device_init(cfg, cfg_size, device_handle);
    return ESP_OK;
}

static void test_cleanup_keeps_power_controller(void)
{
    host_test_cleanup();
    s_partial_requires_power = true;
    s_partial_deinit_fails = true;
    EXPECT(esp_board_device_init("audio_dac") == ESP_OK, "initialize power controller fixture");
    s_dev_no_cfg.power_ctrl_device = "audio_dac";
    EXPECT(esp_board_device_set_ops("gpio_button", powered_device_init, partial_device_deinit) == ESP_OK,
           "install powered driver");
    EXPECT(esp_board_device_init("gpio_button") == ESP_OK, "powered driver returns expected init result");
    EXPECT(esp_board_device_deinit("audio_dac") == ESP_BOARD_ERR_DEVICE_DEP_IN_USE,
           "initialized device protects its power controller");
    EXPECT(esp_board_device_deinit("gpio_button") == ESP_FAIL, "device cleanup failure propagates");
    EXPECT(g_esp_board_device_handles[0].ref_count == 1 && s_dev_handle_no_cfg.device_handle != NULL,
           "failed cleanup retains power controller and device handle");
    s_partial_deinit_fails = false;
    EXPECT(esp_board_device_deinit_all() == ESP_OK, "global cleanup processes powered device before controller");
    EXPECT(g_esp_board_device_handles[0].device_handle == NULL, "power controller released after device cleanup");
    s_partial_requires_power = false;
    s_dev_no_cfg.power_ctrl_device = NULL;
    host_test_cleanup();
}

static void test_bulk_init_prepares_before_traversal(void)
{
    for (int kind = 0; kind < 2; kind++) {
        for (int skip = 0; skip < 2; skip++) {
            host_test_cleanup();
            g_esp_board_devices[0].init_skip = skip;
            s_dev_no_cfg.init_skip = skip;
            g_esp_board_peripherals[0].init_skip = skip;
            s_periph_b.init_skip = skip;
            s_hook_mode = HOOK_FAIL_AFTER_OVERRIDE;
            esp_err_t ret = kind == 0 ? esp_board_device_init_all() : esp_board_periph_init_all();
            EXPECT(ret == ESP_FAIL, "bulk init propagates prepare error before traversing descriptors");
            EXPECT(s_hook_call_count == 1, "bulk init attempts failed prepare only once");
            EXPECT(s_dev_init_count == 0 && s_shared_init_count == 0,
                   "failed bulk prepare never calls a driver");
            g_esp_board_devices[0].init_skip = false;
            s_dev_no_cfg.init_skip = false;
            g_esp_board_peripherals[0].init_skip = false;
            s_periph_b.init_skip = false;
        }
        host_test_cleanup();
        g_esp_board_devices[0].init_skip = true;
        s_dev_no_cfg.init_skip = true;
        g_esp_board_peripherals[0].init_skip = true;
        s_periph_b.init_skip = true;
        esp_err_t ret = kind == 0 ? esp_board_device_init_all() : esp_board_periph_init_all();
        EXPECT(ret == ESP_OK && s_hook_call_count == 1,
               "all-skipped bulk init still completes prepare");
        EXPECT(s_dev_init_count == 0 && s_shared_init_count == 0,
               "all-skipped bulk init does not call drivers");
        g_esp_board_devices[0].init_skip = false;
        s_dev_no_cfg.init_skip = false;
        g_esp_board_peripherals[0].init_skip = false;
        s_periph_b.init_skip = false;
    }
    host_test_cleanup();
    EXPECT(esp_board_device_set_ops("audio_dac", clean_device_init_fails, audio_dac_deinit) == ESP_OK,
           "install clean device failure for best-effort bulk init");
    EXPECT(esp_board_device_init_all() == ESP_OK,
           "bulk device init still logs ordinary driver errors and continues");
    EXPECT(s_dev_handle_no_cfg.device_handle != NULL,
           "bulk device init reaches the next device after a clean failure");
    host_test_cleanup();
    EXPECT(esp_board_periph_set_ops("i2c_master", clean_device_init_fails, shared_deinit) == ESP_OK,
           "install clean peripheral failure for best-effort bulk init");
    EXPECT(esp_board_periph_init_all() == ESP_OK && s_shared_init_count == 1,
           "bulk peripheral init still reaches the next peripheral after a clean failure");
    host_test_cleanup();
}

static void test_bulk_init_rejects_failed_prepare(bool peripheral)
{
    s_hook_mode = peripheral ? HOOK_PARTIAL_PERIPH_PREFLIGHT_FAIL : HOOK_PARTIAL_PREFLIGHT_FAIL;
    esp_err_t ret = peripheral ? esp_board_periph_init_all() : esp_board_device_init_all();
    EXPECT(ret != ESP_OK && s_hook_call_count == 1,
           "fresh bulk init returns from failed prepare without following retained cyclic links");
    EXPECT(esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_FAILED,
           "bulk init preserves terminal prepare failure with retained resources");
    EXPECT(esp_board_device_init_all() == ESP_ERR_INVALID_STATE &&
               esp_board_periph_init_all() == ESP_ERR_INVALID_STATE && s_hook_call_count == 1,
           "later bulk init rejects failed prepare without revisiting descriptors");
}

static void test_failed_prepare_stops_recovery(hook_mode_t mode)
{
    s_dev_no_cfg.cfg = &s_dev_cfg;
    s_dev_no_cfg.cfg_size = sizeof(s_dev_cfg);
    s_hook_mode = mode;
    EXPECT(esp_board_runtime_prepare() != ESP_OK && s_hook_nested_init_ret == ESP_FAIL,
           "hook or preflight failure reports unreleased init resources");
    EXPECT(esp_board_runtime_prepare() == ESP_ERR_INVALID_STATE && s_hook_call_count == 1,
           "failed prepare cannot rerun hook");
    EXPECT(esp_board_device_deinit_all() == ESP_ERR_INVALID_STATE && s_partial_cleanup_calls == 0,
           "failed prepare does not offer bulk recovery");
    EXPECT(esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_FAILED,
           "failed prepare records terminal failure");
    EXPECT(esp_board_runtime_check_idle() == ESP_ERR_INVALID_STATE,
           "failed prepare keeps table updates blocked");
    EXPECT(esp_board_device_set_ops("audio_dac", audio_dac_init, audio_dac_deinit) == ESP_ERR_INVALID_STATE,
           "failed prepare rejects ops updates");
    EXPECT(esp_board_manager_init_device_by_name("missing") == ESP_ERR_INVALID_STATE,
           "manager init does not traverse invalid retained descriptors");
    EXPECT(esp_board_manager_print() == ESP_ERR_INVALID_STATE,
           "manager print does not traverse invalid retained descriptors");
    EXPECT(esp_board_device_callback_register("gpio_button", &s_dev_cfg, NULL) == ESP_ERR_INVALID_STATE,
           "callback registration does not read invalid retained device type");
    uint16_t addr = 0;
    EXPECT(esp_board_device_get_i2c_effective_addr("missing", &addr) == ESP_ERR_INVALID_STATE,
           "I2C address lookup does not traverse invalid retained descriptors");
    const char *name = mode == HOOK_PARTIAL_ADD_FAIL ? "c_added_device" : "gpio_button";
    EXPECT(esp_board_runtime_set_initialized(name, false) == ESP_OK,
           "test removes mark to verify failure state is independent of cleanup");
    EXPECT(esp_board_runtime_prepare() == ESP_ERR_INVALID_STATE && s_hook_call_count == 1,
           "clearing marks cannot reset failed prepare");
}

#ifdef HOST_TEST_BOARD_REVISION
static void test_generated_revision_rejects_active_resources(void)
{
    for (int kind = 0; kind < 3; kind += 2) {
        host_test_cleanup();
        EXPECT(esp_board_runtime_prepare() == ESP_OK, "prepare revision idle guard fixture");
        if (kind == 0) {
            EXPECT(esp_board_device_init("gpio_button") == ESP_OK, "revision guard sees initialized device");
        } else {
            EXPECT(esp_board_periph_init("i2c_master_1") == ESP_OK, "revision guard sees initialized peripheral");
        }
        esp_board_device_desc_t device = g_esp_board_devices[0];
        esp_board_device_desc_t button = s_dev_no_cfg;
        esp_board_periph_desc_t peripheral = g_esp_board_peripherals[0];
        esp_board_periph_desc_t bus = s_periph_b;
        EXPECT(gen_board_revision_apply_cleanup() == ESP_ERR_INVALID_STATE,
               "generated revision rejects live resources");
        EXPECT(memcmp(&device, &g_esp_board_devices[0], sizeof(device)) == 0 &&
                   memcmp(&peripheral, &g_esp_board_peripherals[0], sizeof(peripheral)) == 0,
               "rejected revision leaves descriptor tables unchanged");
        /* Keep the fixture recoverable even when checking a broken guard. */
        g_esp_board_devices[0] = device;
        s_dev_no_cfg = button;
        g_esp_board_peripherals[0] = peripheral;
        s_periph_b = bus;
        host_test_cleanup();
    }
}

static void test_failed_revision_prepare_stops_recovery(void)
{
    s_hook_mode = HOOK_PARTIAL_REVISION_FAIL;
    EXPECT(esp_board_runtime_prepare() == ESP_FAIL && s_hook_nested_init_ret == ESP_FAIL,
           "generated revision hook reports init failure with unreleased resources");
    EXPECT(esp_board_runtime_prepare() == ESP_ERR_INVALID_STATE && s_hook_call_count == 1,
           "failed revision prepare cannot rerun hook");
    EXPECT(gen_board_revision_apply_cleanup() == ESP_ERR_INVALID_STATE,
           "failed revision prepare rejects revision changes");
    EXPECT(esp_board_device_deinit_all() == ESP_ERR_INVALID_STATE,
           "failed revision prepare has no bulk recovery path");
    EXPECT(g_esp_board_devices[0].cfg != &s_dev_cfg &&
               *(const uint8_t *)g_esp_board_devices[0].cfg == 0xD8 &&
               s_dev_handle_no_cfg.next == &s_added_device_handle,
           "failed revision keeps current topology instead of restoring old tables");
}
#endif  /* HOST_TEST_BOARD_REVISION */

int dev_power_ctrl_sub_custom_deinit(void *device_handle);

static int s_custom_power_cleanup_calls;
static bool s_custom_power_cleanup_fails;

static int custom_power_cleanup(void *context)
{
    EXPECT(context == &s_custom_power_cleanup_calls, "custom power cleanup receives retained context");
    s_custom_power_cleanup_calls++;
    return s_custom_power_cleanup_fails ? ESP_FAIL : ESP_OK;
}

static const dev_power_ctrl_custom_ops_t s_custom_power_ops = {
    .deinit = custom_power_cleanup,
};

static int custom_power_test_init(void *cfg, int cfg_size, void **device_handle)
{
    (void)cfg;
    (void)cfg_size;
    dev_power_ctrl_handle_t *handle = calloc(1, sizeof(*handle));
    if (handle == NULL) {
        return ESP_ERR_NO_MEM;
    }
    void *periph_handle = NULL;
    int ret = esp_board_periph_ref_handle("i2c_master", &periph_handle);
    if (ret != ESP_OK) {
        free(handle);
        return ret;
    }
    handle->custom_ops = &s_custom_power_ops;
    handle->custom_context = &s_custom_power_cleanup_calls;
    *device_handle = handle;
    return ESP_OK;
}

static void test_custom_power_deinit_retry(void)
{
    host_test_cleanup();
    static const char *const peripherals[] = {"i2c_master"};
    static const dev_power_ctrl_config_t cfg = {
        .name = "audio_dac",
        .type = "power_ctrl",
        .sub_type = "custom",
        .sub_cfg.custom = {.periph_names = peripherals, .periph_count = 1},
    };
    g_esp_board_devices[0].cfg = &cfg;
    g_esp_board_devices[0].cfg_size = sizeof(cfg);
    s_custom_power_cleanup_calls = 0;
    s_custom_power_cleanup_fails = true;
    EXPECT(esp_board_device_set_ops("audio_dac", custom_power_test_init,
                                    dev_power_ctrl_sub_custom_deinit) == ESP_OK, "install real custom power cleanup");
    EXPECT(esp_board_device_init("audio_dac") == ESP_OK, "initialize custom power cleanup fixture");
    EXPECT(esp_board_device_deinit("audio_dac") == ESP_FAIL, "custom power cleanup failure reaches core");
    EXPECT(s_shared_deinit_count == 0, "failed custom power cleanup retains peripheral reference");
    if (s_shared_deinit_count != 0) {
        return;  /* The broken wrapper has already freed the handle; do not retry it. */
    }
    void *handle = NULL;
    EXPECT(esp_board_device_get_handle("audio_dac", &handle) == ESP_OK && handle != NULL,
           "failed custom power cleanup retains device handle");
    s_custom_power_cleanup_fails = false;
    EXPECT(esp_board_device_deinit("audio_dac") == ESP_OK, "custom power cleanup retry succeeds");
    EXPECT(s_custom_power_cleanup_calls == 2, "custom power cleanup retries original context");
    EXPECT(s_shared_deinit_count == 1, "successful custom power cleanup releases peripheral once");
    EXPECT(!esp_board_runtime_is_initialized("audio_dac") &&
               !esp_board_runtime_is_initialized("i2c_master"), "successful custom power cleanup clears both marks");
    host_test_cleanup();
    g_esp_board_devices[0].cfg = &s_dev_cfg;
    g_esp_board_devices[0].cfg_size = sizeof(s_dev_cfg);
}

int main(int argc, char **argv)
{
    if (argc == 2) {
        if (strcmp(argv[1], "failed-device-init") == 0) {
            test_failed_device_init_retains_resources();
        } else if (strcmp(argv[1], "failed-legacy-periph-init") == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            EXPECT(esp_board_periph_init_custom("i2c_master", partial_device_init, partial_device_deinit) ==
                       ESP_BOARD_ERR_PERIPH_INIT_FAILED, "legacy peripheral init reports unreleased resources");
            EXPECT(esp_board_runtime_is_initialized("i2c_master"), "failed legacy init retains config mark");
            EXPECT(esp_board_periph_init_custom("i2c_master", partial_device_init, partial_device_deinit) ==
                       ESP_ERR_INVALID_STATE, "failed legacy init cannot overwrite unreleased resources");
#pragma GCC diagnostic pop
            EXPECT(esp_board_periph_deinit("i2c_master") == ESP_ERR_INVALID_STATE,
                   "failed legacy init has no deinit recovery");
        } else if (strcmp(argv[1], "failed-periph-init") == 0) {
            EXPECT(esp_board_periph_set_ops("i2c_master", partial_device_init, partial_device_deinit) == ESP_OK,
                   "install peripheral that reports unreleased resources");
            EXPECT(esp_board_periph_init("i2c_master") == ESP_FAIL, "failed peripheral init returns error");
            EXPECT(esp_board_runtime_is_initialized("i2c_master"), "failed peripheral init retains config mark");
            void *handle = NULL;
            EXPECT(esp_board_periph_get_handle("i2c_master", &handle) == ESP_BOARD_ERR_PERIPH_NO_HANDLE,
                   "failed peripheral init does not expose handle");
            EXPECT(esp_board_periph_deinit("i2c_master") == ESP_ERR_INVALID_STATE,
                   "failed peripheral init has no deinit recovery");
            EXPECT(esp_board_periph_init("i2c_master") == ESP_ERR_INVALID_STATE && s_partial_init_calls == 1,
                   "failed peripheral init cannot overwrite unreleased resources");
        } else if (strcmp(argv[1], "failed-hook-periph") == 0) {
            test_active_config_survives_failed_hook();
        } else if (strcmp(argv[1], "failed-hook-lookups") == 0) {
            test_failed_hook_peripheral_cleanup_lookups();
        } else if (strcmp(argv[1], "failed-hook-device") == 0) {
            test_failed_hook_preserves_active_device_handle();
        } else if (strcmp(argv[1], "failed-bulk-device-init") == 0) {
            test_bulk_init_rejects_failed_prepare(false);
        } else if (strcmp(argv[1], "failed-bulk-periph-init") == 0) {
            test_bulk_init_rejects_failed_prepare(true);
        } else if (strcmp(argv[1], "failed-hook-base") == 0) {
            test_failed_prepare_stops_recovery(HOOK_PARTIAL_BASE_FAIL);
        } else if (strcmp(argv[1], "failed-hook-replace") == 0) {
            test_failed_prepare_stops_recovery(HOOK_PARTIAL_REPLACE_FAIL);
        } else if (strcmp(argv[1], "failed-hook-add") == 0) {
            test_failed_prepare_stops_recovery(HOOK_PARTIAL_ADD_FAIL);
        } else if (strcmp(argv[1], "failed-hook-preflight") == 0) {
            test_failed_prepare_stops_recovery(HOOK_PARTIAL_PREFLIGHT_FAIL);
        } else if (strcmp(argv[1], "failed-hook-handle") == 0) {
            test_failed_prepare_stops_recovery(HOOK_PARTIAL_HANDLE_REPLACE_FAIL);
#ifdef HOST_TEST_BOARD_REVISION
        } else if (strcmp(argv[1], "failed-hook-revision") == 0) {
            test_failed_revision_prepare_stops_recovery();
#endif  /* HOST_TEST_BOARD_REVISION */
        } else {
            return 2;
        }
        return s_failures != 0;
    }
    test_config_by_handle_prepares_before_invalid_links();
    test_duplicate_names();
    test_runtime_preflight_rejects_bad_tables();
    test_periph_set_ops_without_shared_handler();
    test_preflight_uses_effective_ops_priority();
    test_handle_link_rollback();
    test_direct_table_mutation_rolls_back_on_preflight_failure();
    test_manual_topology_add();
    test_manual_topology_remove();
    test_manual_topology_replace();
    test_manual_topology_failure_rollback();
    test_inactive_handle_state_rollback();
    test_name_api_auto_prepares();
    test_manager_init_rechecks_writable_tables();
    test_unknown_board_override();
    test_prepare_idempotent();
    test_bulk_init_prepares_before_traversal();
    test_reentry();
    test_hook_fail_rollback();
    test_initialized_mark_no_sidecar();
    test_mark_blocks_free_during_driver_init();
    test_size_mismatch_stores_caller_bytes();
    test_deinit_keeps_sidecar_after_init();
    test_periph_ops_isolation();
    test_periph_config_copy_and_get();
    test_periph_restore_falls_back_to_board();
    test_periph_invalid_args_and_size_mismatch();
    test_periph_restore_ignores_device_sidecar();
    test_init_custom_marks_initialized();
    test_set_ops_isolated_from_init_custom();
    test_register_device_handle_no_sidecar();
    test_device_config_copy_and_get();
    test_device_restore_falls_back_to_board();
    test_device_restore_ignores_periph_sidecar();
    test_device_no_config_and_invalid_args();
    test_auto_prepare_on_periph_and_device_init();
    test_app_wins_over_board();
    test_generated_only_no_sidecar();
    test_config_ops_layer_independence();
    test_hook_calls_init();
    test_non_owner_rejected_during_preparing();
    test_init_custom_also_prepares();
    test_null_task_prepare_rejected();
    test_concurrent_prepare_rejected();
    test_clean_peripheral_init_failure();
    test_dependency_init_failure();
    test_cleanup_keeps_power_controller();
#ifdef HOST_TEST_BOARD_REVISION
    test_generated_revision_rejects_active_resources();
#endif  /* HOST_TEST_BOARD_REVISION */

    test_custom_power_deinit_retry();
    if (s_failures != 0) {
        fprintf(stderr, "%d host runtime tests failed\n", s_failures);
        return 1;
    }
    printf("All host runtime override tests passed\n");
    return 0;
}
