/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_log.h"
#include "bmgr_test_options.h"
#include "bmgr_test_registry.h"
#include "cmd_case.h"

static const char *TAG = "CMD_CASE";
static bmgr_test_context_t *s_ctx;

static struct {
    struct arg_str *action;
    struct arg_str *name;
    struct arg_str *group;
    struct arg_lit *include_manual;
    struct arg_lit *summary;
    struct arg_lit *json;
    struct arg_end *end;
} s_case_args;

static void clear_case_args(void)
{
    s_case_args.action = NULL;
    s_case_args.name = NULL;
    s_case_args.group = NULL;
    s_case_args.include_manual = NULL;
    s_case_args.summary = NULL;
    s_case_args.json = NULL;
    s_case_args.end = NULL;
    s_ctx = NULL;
}

static void free_case_args(void)
{
    arg_freetable((void **)&s_case_args, sizeof(s_case_args) / sizeof(void *));
    clear_case_args();
}

void unregister_case_command(void)
{
    if (s_case_args.action == NULL && s_case_args.name == NULL && s_case_args.group == NULL &&
        s_case_args.include_manual == NULL && s_case_args.summary == NULL &&
        s_case_args.json == NULL && s_case_args.end == NULL) {
        return;
    }
    (void)esp_console_cmd_deregister("case");
    free_case_args();
}

static int case_command_handler(int argc, char **argv)
{
    bmgr_test_context_t *ctx = s_ctx;
    if (ctx == NULL) {
        return 1;
    }

    int arg_errors = arg_parse(argc, argv, (void **)&s_case_args);
    if (arg_errors != 0) {
        arg_print_errors(stderr, s_case_args.end, argv[0]);
        return 1;
    }

    const char *action = s_case_args.action->sval[0];
    const char *group = s_case_args.group->count > 0 ? s_case_args.group->sval[0] : NULL;
    bmgr_test_options_t previous_options = bmgr_test_options_get();
    bmgr_test_options_t run_options = {
        .summary = s_case_args.summary->count > 0,
        .json = s_case_args.json->count > 0,
    };
    int status = 1;

    bmgr_test_options_set(&run_options);

    if (strcmp(action, "list") == 0) {
        bmgr_test_registry_print_group(group);
        status = 0;
        goto exit;
    }

    if (strcmp(action, "run") == 0) {
        if (s_case_args.name->count == 0) {
            ESP_LOGE(TAG, "case run requires a case name");
            goto exit;
        }
        const bmgr_test_case_t *test_case = bmgr_test_registry_find(s_case_args.name->sval[0]);
        if (test_case == NULL) {
            ESP_LOGE(TAG, "Test case not found: %s", s_case_args.name->sval[0]);
            goto exit;
        }
        bool skipped = false;
        esp_err_t ret = bmgr_test_case_run(ctx, test_case, 0, NULL, &skipped);
        status = ret == ESP_OK ? 0 : 1;
        goto exit;
    }

    if (strcmp(action, "run-all") == 0) {
        bmgr_test_case_summary_t summary = {0};
        bool include_manual = s_case_args.include_manual->count > 0;
        esp_err_t ret = bmgr_test_registry_run_all(ctx, group, include_manual, &summary);
        status = ret == ESP_OK ? 0 : 1;
        goto exit;
    }

    ESP_LOGE(TAG, "Unknown case action: %s", action);

exit:
    bmgr_test_options_set(&previous_options);
    return status;
}

esp_err_t register_case_command(bmgr_test_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    s_ctx = ctx;

    s_case_args.action = arg_str1(NULL, NULL, "<list|run|run-all>", "Case command");
    s_case_args.name = arg_str0(NULL, NULL, "<name>", "Test case name");
    s_case_args.group = arg_str0(NULL, "group", "<group>", "Filter by group");
    s_case_args.include_manual = arg_lit0(NULL, "include-manual", "Include manual and destructive cases");
    s_case_args.summary = arg_lit0(NULL, "summary", "Emit summary metrics for this case run");
    s_case_args.json = arg_lit0(NULL, "json", "Emit summary metrics as machine-readable JSON for this case run");
    s_case_args.end = arg_end(2);
    if (s_case_args.action == NULL || s_case_args.name == NULL || s_case_args.group == NULL ||
        s_case_args.include_manual == NULL || s_case_args.summary == NULL ||
        s_case_args.json == NULL || s_case_args.end == NULL) {
        free_case_args();
        return ESP_ERR_NO_MEM;
    }

    const esp_console_cmd_t command = {
        .command = "case",
        .help = "List and run board manager test cases",
        .hint = NULL,
        .func = case_command_handler,
        .argtable = &s_case_args,
    };

    esp_err_t ret = esp_console_cmd_register(&command);
    if (ret != ESP_OK) {
        free_case_args();
    }
    return ret;
}
