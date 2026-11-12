/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_log.h"
#include "sdkconfig.h"
#if defined(CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT) && defined(CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT)
#include "esp_board_manager.h"
#endif  /* defined(CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT) && defined(CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT) */
#include "bmgr_test_names.h"
#include "bmgr_test_registry.h"

#ifdef CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT
#include "test_dev_lcd_touch.h"

#ifdef CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT
#include "test_dev_lcd_touch_view.h"
#endif  /* CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT */

static esp_err_t run_touch_multitouch(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)argv;
    if (ctx == NULL || argc != 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (ctx->lvgl_inited || ctx->lcd_touch_inited) {
        ESP_LOGE("TEST_LCD_TOUCH", "Stop the active LVGL/touch reader before running this case");
        return ESP_ERR_INVALID_STATE;
    }
#ifdef CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT
    test_dev_lcd_touch_view_t *view = NULL;
    if (esp_board_manager_check_name(BMGR_TEST_NAME_DISPLAY_LCD)) {
        esp_err_t ret = test_dev_lcd_touch_view_start(&view);
        if (ret != ESP_OK) {
            return ret;
        }
    }
    esp_err_t ret = test_dev_lcd_touch_multitouch(view != NULL ? test_dev_lcd_touch_view_update : NULL, view);
    if (view != NULL) {
        test_dev_lcd_touch_view_finish(view, ret);
        esp_err_t stop_ret = test_dev_lcd_touch_view_stop(view);
        if (stop_ret != ESP_OK) {
            ESP_LOGE("TEST_LCD_TOUCH", "Display cleanup failed: %s", esp_err_to_name(stop_ret));
            if (ret == ESP_OK) {
                ret = stop_ret;
            }
        }
    }
    return ret;
#else
    return test_dev_lcd_touch_multitouch(NULL, NULL);
#endif  /* CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT */
}
#endif  /* CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT */

void bmgr_register_touch_cases(void)
{
#ifdef CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT
    const bmgr_test_case_t touch_cases[] = {
        {
            .name = "touch.multitouch",
            .group = "touch",
            .help = "Track native IDs through simultaneous touch, partial release, and full release (30s)",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_LCD_TOUCH),
#ifdef CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_DISPLAY_LCD),
                BMGR_TEST_RESOURCE_OPTIONAL(BMGR_TEST_NAME_LCD_BRIGHTNESS),
#endif  /* CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT */
            },
            .run = run_touch_multitouch,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(touch_cases, sizeof(touch_cases) / sizeof(touch_cases[0])));
#endif  /* CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT */
}
