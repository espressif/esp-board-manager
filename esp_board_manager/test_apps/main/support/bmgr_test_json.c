/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include "bmgr_test_json.h"
#include "bmgr_test_options.h"

static bool s_metric_open;

static bool bmgr_test_json_enabled(void)
{
    if (!bmgr_test_options_get().json) {
        return false;
    }
    return true;
}

static void bmgr_test_json_escape(const char *value)
{
    const char *text = value != NULL ? value : "";

    for (const unsigned char *cursor = (const unsigned char *)text; *cursor != '\0'; cursor++) {
        switch (*cursor) {
            case '\\':
                printf("\\\\");
                break;
            case '"':
                printf("\\\"");
                break;
            case '\b':
                printf("\\b");
                break;
            case '\f':
                printf("\\f");
                break;
            case '\n':
                printf("\\n");
                break;
            case '\r':
                printf("\\r");
                break;
            case '\t':
                printf("\\t");
                break;
            default:
                if (*cursor < 0x20) {
                    printf("\\u%04x", *cursor);
                } else {
                    putchar(*cursor);
                }
                break;
        }
    }
}

static void bmgr_test_json_metric_key(const char *key)
{
    printf(",\"");
    bmgr_test_json_escape(key);
    printf("\":");
}

void bmgr_test_json_metric_begin(const char *case_name, const char *name)
{
    if (!bmgr_test_json_enabled()) {
        return;
    }
    s_metric_open = true;
    printf("{\"event\":\"metric\",\"case\":\"");
    bmgr_test_json_escape(case_name);
    printf("\",\"name\":\"");
    bmgr_test_json_escape(name);
    printf("\"");
}

void bmgr_test_json_metric_end(void)
{
    if (!bmgr_test_json_enabled() || !s_metric_open) {
        return;
    }
    printf("}\n");
    s_metric_open = false;
}

void bmgr_test_json_metric_u32(const char *key, uint32_t value)
{
    if (!bmgr_test_json_enabled() || !s_metric_open) {
        return;
    }
    bmgr_test_json_metric_key(key);
    printf("%" PRIu32, value);
}

void bmgr_test_json_metric_i32(const char *key, int32_t value)
{
    if (!bmgr_test_json_enabled() || !s_metric_open) {
        return;
    }
    bmgr_test_json_metric_key(key);
    printf("%" PRId32, value);
}

void bmgr_test_json_metric_float(const char *key, float value)
{
    if (!bmgr_test_json_enabled() || !s_metric_open) {
        return;
    }
    bmgr_test_json_metric_key(key);
    printf("%.6f", (double)value);
}

void bmgr_test_json_metric_str(const char *key, const char *value)
{
    if (!bmgr_test_json_enabled() || !s_metric_open) {
        return;
    }
    bmgr_test_json_metric_key(key);
    printf("\"");
    bmgr_test_json_escape(value);
    printf("\"");
}
