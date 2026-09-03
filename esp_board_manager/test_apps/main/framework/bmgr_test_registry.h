/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_bit_defs.h"
#include "esp_err.h"
#include "bmgr_test_context.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define BMGR_TEST_CASE_FLAG_MANUAL         BIT(0)
#define BMGR_TEST_CASE_FLAG_DESTRUCTIVE    BIT(1)
#define BMGR_TEST_CASE_FLAG_LONG_RUNNING   BIT(2)
#define BMGR_TEST_CASE_FLAG_NEEDS_BOARD    BIT(3)
#define BMGR_TEST_CASE_FLAG_NEEDS_DISPLAY  BIT(4)
#define BMGR_TEST_CASE_FLAG_NEEDS_AUDIO    BIT(5)

/* Maximum number of devices/peripherals a single case can declare as resources. */
#define BMGR_TEST_MAX_RESOURCES  6

#define BMGR_TEST_RESOURCE_REQUIRED(name_)  {                      \
    .name = (name_), .policy = BMGR_TEST_RESOURCE_POLICY_REQUIRED  \
}
#define BMGR_TEST_RESOURCE_OPTIONAL(name_)  {                      \
    .name = (name_), .policy = BMGR_TEST_RESOURCE_POLICY_OPTIONAL  \
}

typedef esp_err_t (*bmgr_test_case_cb_t)(bmgr_test_context_t *ctx, int argc, char **argv);

typedef enum {
    BMGR_TEST_RESOURCE_POLICY_REQUIRED = 0,
    BMGR_TEST_RESOURCE_POLICY_OPTIONAL,
} bmgr_test_resource_policy_t;

typedef struct {
    const char                  *name;
    bmgr_test_resource_policy_t  policy;
} bmgr_test_resource_t;

typedef struct {
    const char           *name;
    const char           *group;
    const char           *help;
    /* Devices/peripherals this case uses. The framework initializes present
     * resources (reference counted) before the case runs and deinitializes them
     * after, so the case no longer depends on a prior whole-board `bmgr init`.
     * If a required resource is missing, the case is skipped. If an optional
     * resource is missing, the case continues without that capability.
     * NULL-terminated; leave unset for cases that manage their own resources. */
    bmgr_test_resource_t  resources[BMGR_TEST_MAX_RESOURCES];
    bmgr_test_case_cb_t   prepare;
    bmgr_test_case_cb_t   run;
    bmgr_test_case_cb_t   cleanup;
    uint32_t              flags;
} bmgr_test_case_t;

typedef struct {
    size_t  total;
    size_t  pass;
    size_t  fail;
    size_t  skip;
} bmgr_test_case_summary_t;

esp_err_t bmgr_test_registry_add(const bmgr_test_case_t *cases, size_t count);
const bmgr_test_case_t *bmgr_test_registry_find(const char *name);
void bmgr_test_registry_print_all(void);
void bmgr_test_registry_print_group(const char *group);
esp_err_t bmgr_test_case_run(bmgr_test_context_t *ctx, const bmgr_test_case_t *test_case,
                             int argc, char **argv, bool *out_skipped);
esp_err_t bmgr_test_registry_run_all(bmgr_test_context_t *ctx, const char *group,
                                     bool include_manual, bmgr_test_case_summary_t *summary);

void bmgr_register_audio_cases(void);
void bmgr_register_button_cases(void);
void bmgr_register_camera_cases(void);
void bmgr_register_custom_cases(void);
void bmgr_register_fs_cases(void);
void bmgr_register_gpio_expander_cases(void);
void bmgr_register_knob_cases(void);
void bmgr_register_lcd_cases(void);
void bmgr_register_led_cases(void);
void bmgr_register_periph_cases(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
