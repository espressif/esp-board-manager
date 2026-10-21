/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 *
 * IDF test cases for runtime prepare / sidecar behavior that work with the
 * default weak esp_board_runtime_pre_init(). Hook-custom cases live in
 * test_apps/host_tests/runtime_override/.
 */

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_board_manager.h"
#include "esp_board_manager_err.h"
#include "esp_board_device.h"
#include "esp_board_periph.h"
#include "esp_board_runtime_state.h"
#include "bmgr_test_registry.h"

static const char *TAG = "TEST_RUNTIME_OV";
static const char *s_override_name = "audio_dac";

/* Defined below; used by run_deinit_keeps_sidecar() before their definition. */
static int stub_device_init(void *cfg, int cfg_size, void **device_handle);
static int stub_device_deinit(void *device_handle);

static esp_err_t expect_eq(esp_err_t got, esp_err_t want, const char *what)
{
    if (got != want) {
        ESP_LOGE(TAG, "%s: got %s (%d), want %s (%d)", what,
                 esp_err_to_name(got), (int)got, esp_err_to_name(want), (int)want);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/**
 * @brief  Deinit manager if needed, then allow the test-only runtime reset
 *
 *         test_reset must not run while the manager is initialized.
 */
static esp_err_t runtime_test_cleanup(void)
{
    esp_err_t ret = esp_board_manager_deinit();
    if (ret != ESP_OK && ret != ESP_BOARD_ERR_MANAGER_NOT_INIT) {
        ESP_LOGE(TAG, "manager deinit before test_reset failed: %s", esp_err_to_name(ret));
        return ret;
    }
    esp_board_runtime_test_reset();
    return ESP_OK;
}

static esp_err_t run_duplicate_name_preflight(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    static const esp_board_device_desc_t dup_b = {
        .next = NULL,
        .name = "dup_dev",
    };
    static const esp_board_device_desc_t dup_a = {
        .next = &dup_b,
        .name = "dup_dev",
    };
    static const esp_board_periph_desc_t periph_ok = {
        .next = NULL,
        .name = "unique_periph",
    };

    esp_err_t ret = esp_board_runtime_check_unique_names(&dup_a, &periph_ok);
    if (expect_eq(ret, ESP_ERR_INVALID_STATE, "duplicate device names") != ESP_OK) {
        return ESP_FAIL;
    }

    static const esp_board_periph_desc_t periph_b = {
        .next = NULL,
        .name = "dup_periph",
    };
    static const esp_board_periph_desc_t periph_a = {
        .next = &periph_b,
        .name = "dup_periph",
    };
    static const esp_board_device_desc_t device_ok = {
        .next = NULL,
        .name = "unique_dev",
    };

    ret = esp_board_runtime_check_unique_names(&device_ok, &periph_a);
    if (expect_eq(ret, ESP_ERR_INVALID_STATE, "duplicate peripheral names") != ESP_OK) {
        return ESP_FAIL;
    }

    ret = esp_board_runtime_check_unique_names(&device_ok, &periph_ok);
    if (expect_eq(ret, ESP_OK, "unique names") != ESP_OK) {
        return ESP_FAIL;
    }

    printf("duplicate-name preflight rejection OK\n");
    return ESP_OK;
}

static esp_err_t run_undeclared_override(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    if (runtime_test_cleanup() != ESP_OK) {
        return ESP_FAIL;
    }

    uint8_t cfg = 0x11;
    esp_err_t ret = esp_board_runtime_copy_config("no_such_runtime_resource", &cfg, sizeof(cfg));
    if (expect_eq(ret, ESP_BOARD_ERR_DEVICE_NOT_FOUND, "undeclared override") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    if (esp_board_runtime_find_node("no_such_runtime_resource") != NULL) {
        ESP_LOGE(TAG, "unknown resource should not create a sidecar");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    (void)runtime_test_cleanup();
    printf("undeclared override rejection OK\n");
    return ESP_OK;
}

static esp_err_t run_prepare_idempotent(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    if (runtime_test_cleanup() != ESP_OK) {
        return ESP_FAIL;
    }

    if (expect_eq(esp_board_manager_prepare(), ESP_OK, "first prepare") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    if (esp_board_runtime_get_prepare_state() != ESP_BOARD_RUNTIME_PREPARED) {
        ESP_LOGE(TAG, "expected PREPARED after first prepare");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    if (expect_eq(esp_board_manager_prepare(), ESP_OK, "second prepare") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    if (esp_board_runtime_get_prepare_state() != ESP_BOARD_RUNTIME_PREPARED) {
        ESP_LOGE(TAG, "expected PREPARED after second prepare");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    (void)runtime_test_cleanup();
    printf("prepare idempotent OK\n");
    return ESP_OK;
}

static esp_err_t run_deinit_keeps_sidecar(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    if (!esp_board_manager_check_name(s_override_name)) {
        printf("[CASE] SKIP runtime.deinit_keeps_override reason=missing:%s\n", s_override_name);
        return ESP_OK;
    }

    if (runtime_test_cleanup() != ESP_OK) {
        return ESP_FAIL;
    }

    /* APP-layer ops sidecar under the default weak hook (no strong board hook). */
    if (expect_eq(esp_board_manager_prepare(), ESP_OK, "prepare") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    if (expect_eq(esp_board_device_set_ops(s_override_name, stub_device_init, stub_device_deinit),
                  ESP_OK, "app ops override") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (expect_eq(esp_board_manager_init(), ESP_OK, "manager init") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    void *device_handle = NULL;
    if (expect_eq(esp_board_device_get_handle(s_override_name, &device_handle), ESP_OK,
                  "overridden device initialized") != ESP_OK || device_handle == NULL) {
        ESP_LOGE(TAG, "expected audio_dac to be initialized with stub ops");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (esp_board_runtime_get_prepare_state() != ESP_BOARD_RUNTIME_PREPARED) {
        ESP_LOGE(TAG, "expected PREPARED after init");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    esp_board_runtime_node_t *node = esp_board_runtime_find_node(s_override_name);
    if (node == NULL || !node->app.has_ops || node->app.init_fn == NULL) {
        ESP_LOGE(TAG, "expected APP ops sidecar after init");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (expect_eq(esp_board_manager_deinit(), ESP_OK, "manager deinit") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (esp_board_runtime_get_prepare_state() != ESP_BOARD_RUNTIME_PREPARED) {
        ESP_LOGE(TAG, "deinit must not move prepare back to UNPREPARED");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    node = esp_board_runtime_find_node(s_override_name);
    if (node == NULL || !node->app.has_ops || node->app.init_fn == NULL) {
        ESP_LOGE(TAG, "deinit must not clear APP ops sidecar");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    (void)runtime_test_cleanup();
    printf("deinit keeps sidecar and PREPARED OK\n");
    return ESP_OK;
}

static const char *s_periph_override_name = "i2c_master";

static esp_err_t stub_periph_init(void *cfg, int cfg_size, void **periph_handle)
{
    (void)cfg;
    (void)cfg_size;
    if (periph_handle) {
        *periph_handle = (void *)(uintptr_t)0x1;
    }
    return ESP_OK;
}

static esp_err_t stub_periph_deinit(void *periph_handle)
{
    (void)periph_handle;
    return ESP_OK;
}

static int stub_device_init(void *cfg, int cfg_size, void **device_handle)
{
    (void)cfg;
    (void)cfg_size;
    if (device_handle) {
        *device_handle = (void *)(uintptr_t)0x1;
    }
    return 0;
}

static int stub_device_deinit(void *device_handle)
{
    (void)device_handle;
    return 0;
}

static esp_err_t run_periph_app_override(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    void *peek_cfg = NULL;
    esp_err_t peek = esp_board_periph_get_config(s_periph_override_name, &peek_cfg);
    if (peek == ESP_BOARD_ERR_PERIPH_NOT_FOUND) {
        printf("[CASE] SKIP runtime.periph_app_override reason=missing:%s\n", s_periph_override_name);
        return ESP_OK;
    }

    if (runtime_test_cleanup() != ESP_OK) {
        return ESP_FAIL;
    }

    uint8_t caller = 0x5A;
    if (expect_eq(esp_board_periph_override_config(s_periph_override_name, &caller, sizeof(caller)),
                  ESP_OK, "periph override") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    caller = 0x00;

    void *cfg = NULL;
    if (expect_eq(esp_board_periph_get_config(s_periph_override_name, &cfg), ESP_OK,
                  "get_config") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    if (cfg == NULL || cfg == &caller || *(const uint8_t *)cfg != 0x5A) {
        ESP_LOGE(TAG, "get_config must return copied APP bytes");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (expect_eq(esp_board_periph_override_config(NULL, &caller, sizeof(caller)),
                  ESP_BOARD_ERR_PERIPH_INVALID_ARG, "NULL name") != ESP_OK ||
        expect_eq(esp_board_periph_override_config(s_periph_override_name, NULL, 1),
                  ESP_BOARD_ERR_PERIPH_INVALID_ARG, "NULL config") != ESP_OK ||
        expect_eq(esp_board_periph_override_config(s_periph_override_name, &caller, 0),
                  ESP_BOARD_ERR_PERIPH_INVALID_ARG, "size 0") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (expect_eq(esp_board_periph_restore_config(s_periph_override_name), ESP_OK,
                  "restore_config") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    if (expect_eq(esp_board_periph_restore_config(s_periph_override_name), ESP_ERR_NOT_FOUND,
                  "restore with no APP") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    (void)runtime_test_cleanup();
    printf("periph APP override / restore / invalid args OK\n");
    return ESP_OK;
}

static esp_err_t run_device_app_override(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    if (!esp_board_manager_check_name(s_override_name)) {
        printf("[CASE] SKIP runtime.device_app_override reason=missing:%s\n", s_override_name);
        return ESP_OK;
    }

    if (runtime_test_cleanup() != ESP_OK) {
        return ESP_FAIL;
    }

    uint8_t caller = 0x5A;
    if (expect_eq(esp_board_device_override_config(s_override_name, &caller, sizeof(caller)),
                  ESP_OK, "device override") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    caller = 0x00;

    void *cfg = NULL;
    if (expect_eq(esp_board_device_get_config(s_override_name, &cfg), ESP_OK, "get_config") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    if (cfg == NULL || cfg == &caller || *(const uint8_t *)cfg != 0x5A) {
        ESP_LOGE(TAG, "device get_config must return copied APP bytes");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (expect_eq(esp_board_device_override_config(NULL, &caller, sizeof(caller)),
                  ESP_BOARD_ERR_DEVICE_INVALID_ARG, "NULL name") != ESP_OK ||
        expect_eq(esp_board_device_override_config(s_override_name, NULL, 1),
                  ESP_BOARD_ERR_DEVICE_INVALID_ARG, "NULL config") != ESP_OK ||
        expect_eq(esp_board_device_override_config(s_override_name, &caller, 0),
                  ESP_BOARD_ERR_DEVICE_INVALID_ARG, "size 0") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (expect_eq(esp_board_device_restore_config(s_override_name), ESP_OK,
                  "restore_config") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    if (expect_eq(esp_board_device_restore_config(s_override_name), ESP_ERR_NOT_FOUND,
                  "restore with no APP") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    if (expect_eq(esp_board_device_restore_ops(s_override_name), ESP_ERR_NOT_FOUND,
                  "restore_ops with no APP") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    (void)runtime_test_cleanup();
    printf("device APP override / restore / invalid args OK\n");
    return ESP_OK;
}

static esp_err_t run_generated_only(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    /**
     * Prefer the same peripheral path as host test_generated_only_no_sidecar:
     * i2c_master init does not need external codec hardware. audio_dac device
     * init needs an ES8311 on the bus, so it is not asserted here.
     */
    void *peek_cfg = NULL;
    esp_err_t peek = esp_board_periph_get_config(s_periph_override_name, &peek_cfg);
    if (peek == ESP_BOARD_ERR_PERIPH_NOT_FOUND) {
        printf("[CASE] SKIP runtime.generated_only reason=missing:%s\n",
               s_periph_override_name);
        return ESP_OK;
    }

    if (runtime_test_cleanup() != ESP_OK) {
        return ESP_FAIL;
    }

    void *generated = NULL;
    if (expect_eq(esp_board_periph_get_config(s_periph_override_name, &generated), ESP_OK,
                  "generated get_config") != ESP_OK || generated == NULL) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    if (esp_board_runtime_find_node(s_periph_override_name) != NULL) {
        ESP_LOGE(TAG, "generated-only path must not create a sidecar on get_config");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    void *cfg = NULL;
    if (expect_eq(esp_board_periph_get_config(s_periph_override_name, &cfg), ESP_OK,
                  "second get_config") != ESP_OK || cfg != generated) {
        ESP_LOGE(TAG, "get_config must keep returning the generated pointer");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (expect_eq(esp_board_periph_init(s_periph_override_name), ESP_OK,
                  "init generated only") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    if (esp_board_runtime_find_node(s_periph_override_name) != NULL) {
        ESP_LOGE(TAG, "init without override must not create a sidecar");
        (void)esp_board_periph_deinit(s_periph_override_name);
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (expect_eq(esp_board_periph_deinit(s_periph_override_name), ESP_OK,
                  "deinit generated only") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    (void)runtime_test_cleanup();
    printf("generated only (no sidecar) OK\n");
    return ESP_OK;
}

static esp_err_t run_late_binding_rejected(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    if (!esp_board_manager_check_name(s_override_name)) {
        printf("[CASE] SKIP runtime.late_binding reason=missing:%s\n", s_override_name);
        return ESP_OK;
    }

    if (runtime_test_cleanup() != ESP_OK) {
        return ESP_FAIL;
    }

    /**
     * Init with generated config via stub ops so success does not depend on a
     * truncated APP override or on codec hardware being present.
     */
    if (expect_eq(esp_board_device_set_ops(s_override_name, stub_device_init, stub_device_deinit),
                  ESP_OK, "stub ops before init") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (expect_eq(esp_board_device_init(s_override_name), ESP_OK,
                  "init with generated config") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    uint8_t late = 0xBB;
    if (expect_eq(esp_board_device_override_config(s_override_name, &late, sizeof(late)),
                  ESP_ERR_INVALID_STATE, "override while initialized") != ESP_OK ||
        expect_eq(esp_board_device_set_ops(s_override_name, stub_device_init, stub_device_deinit),
                  ESP_ERR_INVALID_STATE, "set_ops while initialized") != ESP_OK ||
        expect_eq(esp_board_device_restore_config(s_override_name),
                  ESP_ERR_INVALID_STATE, "restore while initialized") != ESP_OK ||
        expect_eq(esp_board_device_restore_ops(s_override_name),
                  ESP_ERR_INVALID_STATE, "restore_ops while initialized") != ESP_OK) {
        (void)esp_board_device_deinit(s_override_name);
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (expect_eq(esp_board_device_deinit(s_override_name), ESP_OK, "device deinit") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    /* After deinit, ref_count is 0 and edits are allowed again. */
    if (expect_eq(esp_board_device_override_config(s_override_name, &late, sizeof(late)),
                  ESP_OK, "override after deinit") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    (void)runtime_test_cleanup();
    printf("late binding rejected while initialized OK\n");
    return ESP_OK;
}

static esp_err_t run_restore_to_generated(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    void *peek_cfg = NULL;
    esp_err_t peek = esp_board_periph_get_config(s_periph_override_name, &peek_cfg);
    if (peek == ESP_BOARD_ERR_PERIPH_NOT_FOUND) {
        printf("[CASE] SKIP runtime.restore_to_generated reason=missing:%s\n",
               s_periph_override_name);
        return ESP_OK;
    }

    if (runtime_test_cleanup() != ESP_OK) {
        return ESP_FAIL;
    }

    void *generated = NULL;
    if (expect_eq(esp_board_periph_get_config(s_periph_override_name, &generated), ESP_OK,
                  "baseline generated") != ESP_OK || generated == NULL) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    uint8_t caller = 0x5A;
    if (expect_eq(esp_board_periph_override_config(s_periph_override_name, &caller, sizeof(caller)),
                  ESP_OK, "APP override") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    void *cfg = NULL;
    if (expect_eq(esp_board_periph_get_config(s_periph_override_name, &cfg), ESP_OK,
                  "get APP") != ESP_OK ||
        cfg == NULL || *(const uint8_t *)cfg != 0x5A) {
        ESP_LOGE(TAG, "expected APP override bytes before restore");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (expect_eq(esp_board_periph_restore_config(s_periph_override_name), ESP_OK,
                  "restore APP") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    cfg = NULL;
    if (expect_eq(esp_board_periph_get_config(s_periph_override_name, &cfg), ESP_OK,
                  "get after restore") != ESP_OK || cfg != generated) {
        ESP_LOGE(TAG, "after APP restore, effective must be generated pointer");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    (void)runtime_test_cleanup();
    printf("restore clears APP and falls back to generated OK\n");
    return ESP_OK;
}

static esp_err_t run_ops_config_independence(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;

    void *peek_cfg = NULL;
    esp_err_t peek = esp_board_periph_get_config(s_periph_override_name, &peek_cfg);
    if (peek == ESP_BOARD_ERR_PERIPH_NOT_FOUND) {
        printf("[CASE] SKIP runtime.ops_config_independence reason=missing:%s\n",
               s_periph_override_name);
        return ESP_OK;
    }

    if (runtime_test_cleanup() != ESP_OK) {
        return ESP_FAIL;
    }

    void *generated = NULL;
    if (expect_eq(esp_board_periph_get_config(s_periph_override_name, &generated), ESP_OK,
                  "generated baseline") != ESP_OK || generated == NULL) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    /* Setting only ops must leave config on the generated layer. */
    if (expect_eq(esp_board_periph_set_ops(s_periph_override_name, stub_periph_init, stub_periph_deinit),
                  ESP_OK, "APP ops only") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    void *cfg = NULL;
    if (expect_eq(esp_board_periph_get_config(s_periph_override_name, &cfg), ESP_OK,
                  "config after ops-only") != ESP_OK || cfg != generated) {
        ESP_LOGE(TAG, "ops-only must leave generated config");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    if (expect_eq(esp_board_periph_restore_ops(s_periph_override_name), ESP_OK,
                  "restore ops") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    /* Setting only config must leave ops on the generated layer (no APP ops sidecar). */
    uint8_t caller = 0x5A;
    if (expect_eq(esp_board_periph_override_config(s_periph_override_name, &caller, sizeof(caller)),
                  ESP_OK, "APP config only") != ESP_OK) {
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }
    esp_board_runtime_node_t *node = esp_board_runtime_find_node(s_periph_override_name);
    if (node == NULL || !node->app.has_cfg || node->app.has_ops) {
        ESP_LOGE(TAG, "config-only must not invent APP ops");
        (void)runtime_test_cleanup();
        return ESP_FAIL;
    }

    (void)runtime_test_cleanup();
    printf("ops/config layer independence OK\n");
    return ESP_OK;
}

void bmgr_register_runtime_override_cases(void)
{
#if CONFIG_BMGR_TEST_RUNTIME_C_NODES
    extern void bmgr_register_runtime_c_node_case(void);
#endif  /* CONFIG_BMGR_TEST_RUNTIME_C_NODES */
    const bmgr_test_case_t cases[] = {
        {
            .name = "runtime.dup_name_preflight",
            .group = "runtime",
            .help = "Reject duplicate descriptor names in preflight",
            .run = run_duplicate_name_preflight,
            .flags = 0,
        },
        {
            .name = "runtime.undeclared_override",
            .group = "runtime",
            .help = "Reject override for undeclared name",
            .run = run_undeclared_override,
            .flags = 0,
        },
        {
            .name = "runtime.prepare_idempotent",
            .group = "runtime",
            .help = "prepare is idempotent once PREPARED",
            .run = run_prepare_idempotent,
            .flags = 0,
        },
        {
            .name = "runtime.deinit_keeps_override",
            .group = "runtime",
            .help = "deinit does not clear sidecar or UNPREPARE",
            .run = run_deinit_keeps_sidecar,
            .flags = 0,
        },
        {
            .name = "runtime.periph_app_override",
            .group = "runtime",
            .help = "Peripheral APP override, get_config copy, restore",
            .run = run_periph_app_override,
            .flags = 0,
        },
        {
            .name = "runtime.device_app_override",
            .group = "runtime",
            .help = "Device APP override, get_config copy, restore",
            .run = run_device_app_override,
            .flags = 0,
        },
        {
            .name = "runtime.generated_only",
            .group = "runtime",
            .help = "No sidecar when using generated config only",
            .run = run_generated_only,
            .flags = 0,
        },
        {
            .name = "runtime.late_binding",
            .group = "runtime",
            .help = "Reject override/set_ops/restore while initialized",
            .run = run_late_binding_rejected,
            .flags = 0,
        },
        {
            .name = "runtime.restore_to_generated",
            .group = "runtime",
            .help = "APP restore falls back to generated",
            .run = run_restore_to_generated,
            .flags = 0,
        },
        {
            .name = "runtime.ops_config_independence",
            .group = "runtime",
            .help = "Ops-only and config-only leave the other on generated",
            .run = run_ops_config_independence,
            .flags = 0,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(cases, sizeof(cases) / sizeof(cases[0])));
#if CONFIG_BMGR_TEST_RUNTIME_C_NODES
    bmgr_register_runtime_c_node_case();
#endif  /* CONFIG_BMGR_TEST_RUNTIME_C_NODES */
}
