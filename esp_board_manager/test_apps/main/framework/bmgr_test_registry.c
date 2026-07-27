/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_board_manager.h"
#include "bmgr_test_registry.h"

#define BMGR_TEST_MAX_CASES  64

static const char *TAG = "BMGR_TEST_REG";
static bmgr_test_case_t s_cases[BMGR_TEST_MAX_CASES];
static size_t s_case_count;

static bool should_skip_case(const bmgr_test_case_t *test_case, const char *group, bool include_manual)
{
    if (group != NULL && group[0] != '\0' && strcmp(test_case->group, group) != 0) {
        return true;
    }
    if (!include_manual && (test_case->flags & (BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_DESTRUCTIVE))) {
        return true;
    }
    return false;
}

esp_err_t bmgr_test_registry_add(const bmgr_test_case_t *cases, size_t count)
{
    if (cases == NULL && count > 0) {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t validate_idx = 0; validate_idx < count; validate_idx++) {
        const bmgr_test_case_t *test_case = &cases[validate_idx];
        if (test_case->name == NULL || test_case->group == NULL || test_case->run == NULL) {
            return ESP_ERR_INVALID_ARG;
        }
    }

    for (size_t new_idx = 0; new_idx < count; new_idx++) {
        const bmgr_test_case_t *test_case = &cases[new_idx];
        for (size_t i = 0; i < s_case_count; i++) {
            if (strcmp(s_cases[i].name, test_case->name) == 0) {
                ESP_LOGE(TAG, "Duplicate test case: %s", test_case->name);
                return ESP_ERR_INVALID_STATE;
            }
        }
    }

    for (size_t new_idx = 0; new_idx < count; new_idx++) {
        const bmgr_test_case_t *test_case = &cases[new_idx];
        for (size_t dup_idx = new_idx + 1; dup_idx < count; dup_idx++) {
            if (strcmp(cases[dup_idx].name, test_case->name) == 0) {
                ESP_LOGE(TAG, "Duplicate test case: %s", test_case->name);
                return ESP_ERR_INVALID_STATE;
            }
        }
    }

    if (s_case_count + count > BMGR_TEST_MAX_CASES) {
        return ESP_ERR_NO_MEM;
    }

    for (size_t i = 0; i < count; i++) {
        s_cases[s_case_count++] = cases[i];
    }
    return ESP_OK;
}

const bmgr_test_case_t *bmgr_test_registry_find(const char *name)
{
    if (name == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < s_case_count; i++) {
        if (strcmp(s_cases[i].name, name) == 0) {
            return &s_cases[i];
        }
    }
    return NULL;
}

void bmgr_test_registry_print_all(void)
{
    bmgr_test_registry_print_group(NULL);
}

void bmgr_test_registry_print_group(const char *group)
{
    printf("Registered test cases:\n");
    for (size_t i = 0; i < s_case_count; i++) {
        const bmgr_test_case_t *test_case = &s_cases[i];
        if (group != NULL && group[0] != '\0' && strcmp(test_case->group, group) != 0) {
            continue;
        }
        printf("  %-28s group=%-14s flags=0x%08" PRIx32 " %s\n",
               test_case->name,
               test_case->group,
               test_case->flags,
               test_case->help ? test_case->help : "");
    }
}

esp_err_t bmgr_test_case_run(bmgr_test_context_t *ctx, const bmgr_test_case_t *test_case,
                             int argc, char **argv, bool *out_skipped)
{
    if (out_skipped != NULL) {
        *out_skipped = false;
    }
    if (ctx == NULL || test_case == NULL || test_case->run == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Skip only when a required resource is not present on this board. */
    for (size_t i = 0; i < BMGR_TEST_MAX_RESOURCES && test_case->resources[i].name != NULL; i++) {
        const bmgr_test_resource_t *resource = &test_case->resources[i];
        if (!esp_board_manager_check_name(resource->name)) {
            if (resource->policy == BMGR_TEST_RESOURCE_POLICY_OPTIONAL) {
                printf("[CASE] INFO %s reason=optional-missing:%s\n", test_case->name, resource->name);
                continue;
            }
            printf("[CASE] SKIP %s reason=missing:%s\n", test_case->name, resource->name);
            if (out_skipped != NULL) {
                *out_skipped = true;
            }
            return ESP_OK;
        }
    }

    printf("[CASE] START %s\n", test_case->name);
    ctx->running_case_count++;

    /* Initialize present resources (reference counted). Track exactly what
     * succeeded so optional missing resources do not affect teardown order. */
    esp_err_t resources_ret = ESP_OK;
    const char *inited_resources[BMGR_TEST_MAX_RESOURCES] = {0};
    size_t inited_count = 0;
    for (size_t i = 0; i < BMGR_TEST_MAX_RESOURCES && test_case->resources[i].name != NULL; i++) {
        const bmgr_test_resource_t *resource = &test_case->resources[i];
        if (!esp_board_manager_check_name(resource->name)) {
            continue;
        }
        resources_ret = bmgr_test_target_init(resource->name);
        if (resources_ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init resource '%s' for %s: %s",
                     resource->name, test_case->name, esp_err_to_name(resources_ret));
            break;
        }
        inited_resources[inited_count++] = resource->name;
    }

    esp_err_t prepare_ret = ESP_OK;
    esp_err_t run_ret = ESP_OK;
    esp_err_t cleanup_ret = ESP_OK;

    if (resources_ret == ESP_OK) {
        if (test_case->prepare != NULL) {
            prepare_ret = test_case->prepare(ctx, argc, argv);
        }
        if (prepare_ret == ESP_OK) {
            run_ret = test_case->run(ctx, argc, argv);
        }
        if (test_case->cleanup != NULL) {
            cleanup_ret = test_case->cleanup(ctx, argc, argv);
            if (cleanup_ret != ESP_OK) {
                ESP_LOGW(TAG, "Cleanup failed for %s: %s", test_case->name, esp_err_to_name(cleanup_ret));
            }
        }
    }

    /* Deinitialize the resources we brought up, in reverse order. */
    for (size_t j = inited_count; j > 0; j--) {
        esp_err_t deinit_ret = bmgr_test_target_deinit(inited_resources[j - 1]);
        if (deinit_ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to deinit resource '%s' for %s: %s",
                     inited_resources[j - 1], test_case->name, esp_err_to_name(deinit_ret));
        }
    }

    esp_err_t final_ret = resources_ret;
    if (final_ret == ESP_OK) {
        final_ret = prepare_ret;
    }
    if (final_ret == ESP_OK) {
        final_ret = run_ret;
    }
    if (final_ret == ESP_OK) {
        final_ret = cleanup_ret;
    }

    printf("[CASE] %s %s%s%s\n",
           final_ret == ESP_OK ? "PASS" : "FAIL",
           test_case->name,
           final_ret == ESP_OK ? "" : " ",
           final_ret == ESP_OK ? "" : esp_err_to_name(final_ret));
    ctx->running_case_count--;
    return final_ret;
}

esp_err_t bmgr_test_registry_run_all(bmgr_test_context_t *ctx, const char *group,
                                     bool include_manual, bmgr_test_case_summary_t *summary)
{
    if (ctx == NULL || summary == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(summary, 0, sizeof(*summary));

    esp_err_t final_ret = ESP_OK;
    for (size_t i = 0; i < s_case_count; i++) {
        const bmgr_test_case_t *test_case = &s_cases[i];
        summary->total++;
        if (should_skip_case(test_case, group, include_manual)) {
            summary->skip++;
            printf("[CASE] SKIP %s reason=filtered\n", test_case->name);
            continue;
        }

        bool skipped = false;
        esp_err_t ret = bmgr_test_case_run(ctx, test_case, 0, NULL, &skipped);
        if (skipped) {
            summary->skip++;
        } else if (ret == ESP_OK) {
            summary->pass++;
        } else {
            summary->fail++;
            final_ret = ret;
        }
    }

    printf("[CASE] SUMMARY total=%u pass=%u fail=%u skip=%u\n",
           (unsigned)summary->total,
           (unsigned)summary->pass,
           (unsigned)summary->fail,
           (unsigned)summary->skip);
    return final_ret;
}
