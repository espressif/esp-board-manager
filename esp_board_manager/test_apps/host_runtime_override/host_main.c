/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 *
 * Host driver for runtime prepare / sparse sidecar / peripheral and device
 * APP overrides. Compiles the real esp_board_runtime_state.c,
 * esp_board_manager.c, esp_board_periph.c, and esp_board_device.c against
 * lightweight stubs (no hardware).
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/task.h"
#include "esp_board_runtime_state.h"
#include "esp_board_manager.h"
#include "esp_board_manager_err.h"
#include "esp_board_periph.h"
#include "esp_board_device.h"

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
} hook_mode_t;

static hook_mode_t s_hook_mode = HOOK_OK;
static int s_failures = 0;
static int s_hook_call_count = 0;
static esp_err_t s_hook_nested_init_ret = ESP_OK;
static esp_err_t s_hook_owner_get_ret = ESP_FAIL;
static esp_err_t s_hook_non_owner_get_ret = ESP_FAIL;
static esp_err_t s_non_owner_override_ret = ESP_OK;
static esp_err_t s_non_owner_restore_ret = ESP_OK;
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
        case HOOK_INACTIVE_HANDLE_STATE_FAIL:
            g_esp_board_device_handles[0].device_handle = (void *)(uintptr_t)0xDEAD;
            g_esp_board_device_handles[0].ref_count = 7;
            return ESP_FAIL;
        case HOOK_OK:
        default:
            return ESP_OK;
    }
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
    host_test_cleanup();
    s_hook_mode = HOOK_ACTIVE_CFG_FAIL;
    EXPECT(esp_board_runtime_prepare() == ESP_FAIL, "failed hook with active resource propagates");
    esp_board_runtime_node_t *node = esp_board_runtime_find_node("i2c_master");
    EXPECT(node && node->board.has_cfg && node->board.cfg != NULL,
           "rollback retains config still bound to active resource");
    EXPECT(esp_board_runtime_is_initialized("i2c_master"),
           "rollback retains active resource mark for explicit deinit");
    EXPECT(esp_board_periph_deinit("i2c_master") == ESP_OK,
           "hook-owned resource can be deinitialized after failed prepare");
    host_test_cleanup();
}

static void test_failed_hook_preserves_active_device_handle(void)
{
    host_test_cleanup();
    s_hook_mode = HOOK_ACTIVE_DEVICE_FAIL;
    EXPECT(esp_board_runtime_prepare() == ESP_FAIL, "failed hook after device init propagates");
    EXPECT(s_dev_handle_no_cfg.next == NULL, "failed hook restores device handle link");
    EXPECT(g_esp_board_device_handles[0].device_handle != NULL &&
               g_esp_board_device_handles[0].ref_count == 1,
           "rollback preserves initialized device handle and refcount");
    EXPECT(esp_board_device_deinit("audio_dac") == ESP_OK,
           "hook-owned device can be deinitialized after failed prepare");
    host_test_cleanup();
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

int main(void)
{
    test_duplicate_names();
    test_runtime_preflight_rejects_bad_tables();
    test_periph_set_ops_without_shared_handler();
    test_preflight_uses_effective_ops_priority();
    test_handle_link_rollback();
    test_active_config_survives_failed_hook();
    test_failed_hook_preserves_active_device_handle();
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

    if (s_failures != 0) {
        fprintf(stderr, "%d host runtime tests failed\n", s_failures);
        return 1;
    }
    printf("All host runtime override tests passed\n");
    return 0;
}
