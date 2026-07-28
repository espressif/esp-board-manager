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
#include "esp_board_manager.h"
#include "cmd_bmgr.h"

static const char *TAG = "CMD_BMGR";
static bmgr_test_context_t *s_ctx;

static struct {
    struct arg_str *action;
    struct arg_str *name;
    struct arg_lit *debug;
    struct arg_end *end;
} s_bmgr_args;

static void clear_bmgr_args(void)
{
    s_bmgr_args.action = NULL;
    s_bmgr_args.name = NULL;
    s_bmgr_args.debug = NULL;
    s_bmgr_args.end = NULL;
    s_ctx = NULL;
}

static void free_bmgr_args(void)
{
    arg_freetable((void **)&s_bmgr_args, sizeof(s_bmgr_args) / sizeof(void *));
    clear_bmgr_args();
}

void unregister_bmgr_command(void)
{
    if (s_bmgr_args.action == NULL && s_bmgr_args.name == NULL && s_bmgr_args.debug == NULL &&
        s_bmgr_args.end == NULL) {
        return;
    }
    (void)esp_console_cmd_deregister("bmgr");
    free_bmgr_args();
}

static int bmgr_command_handler(int argc, char **argv)
{
    bmgr_test_context_t *ctx = s_ctx;
    if (ctx == NULL) {
        return 1;
    }

    int arg_errors = arg_parse(argc, argv, (void **)&s_bmgr_args);
    if (arg_errors != 0) {
        arg_print_errors(stderr, s_bmgr_args.end, argv[0]);
        return 1;
    }

    const char *action = s_bmgr_args.action->sval[0];
    const char *name = s_bmgr_args.name->count > 0 ? s_bmgr_args.name->sval[0] : NULL;
    bool target_all = (name == NULL) || (strcmp(name, "all") == 0);
    esp_err_t ret = ESP_OK;

    if (strcmp(action, "init") == 0) {
        bool debug = s_bmgr_args.debug->count > 0;
        if (target_all) {
            ret = bmgr_test_board_init(ctx, debug);
        } else {
            if (debug) {
                ESP_LOGW(TAG, "--debug only applies to whole-board init; ignoring for '%s'", name);
            }
            ret = bmgr_test_target_init(name);
        }
    } else if (strcmp(action, "deinit") == 0) {
        if (target_all) {
            ret = bmgr_test_board_deinit(ctx);
        } else {
            ret = bmgr_test_target_deinit(name);
        }
    } else if (strcmp(action, "info") == 0) {
        ret = esp_board_manager_print_board_info();
    } else if (strcmp(action, "print") == 0) {
        ret = esp_board_manager_print();
    } else if (strcmp(action, "status") == 0) {
        bmgr_test_context_print(ctx);
    } else {
        ESP_LOGE(TAG, "Unknown bmgr action: %s", action);
        return 1;
    }

    return ret == ESP_OK ? 0 : 1;
}

esp_err_t register_bmgr_command(bmgr_test_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    s_ctx = ctx;

    s_bmgr_args.action = arg_str1(NULL, NULL, "<init|deinit|info|print|status>", "Board manager command");
    s_bmgr_args.name = arg_str0(NULL, NULL, "[name|all]", "Device/peripheral name for init/deinit (default: all)");
    s_bmgr_args.debug = arg_lit0(NULL, "debug", "Use generated debug initialization for whole-board init");
    s_bmgr_args.end = arg_end(2);
    if (s_bmgr_args.action == NULL || s_bmgr_args.name == NULL || s_bmgr_args.debug == NULL ||
        s_bmgr_args.end == NULL) {
        free_bmgr_args();
        return ESP_ERR_NO_MEM;
    }

    const esp_console_cmd_t command = {
        .command = "bmgr",
        .help = "Board manager operations. 'init/deinit [name]' targets a single device or "
                "peripheral; 'init/deinit all' (or no name) targets the whole board",
        .hint = NULL,
        .func = bmgr_command_handler,
        .argtable = &s_bmgr_args,
    };

    esp_err_t ret = esp_console_cmd_register(&command);
    if (ret != ESP_OK) {
        free_bmgr_args();
    }
    return ret;
}
