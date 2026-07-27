/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "test_dev_lcd_auto.h"
#include "test_dev_lcd_lvgl.h"

#define LCD_AUTO_LOCK_TIMEOUT_MS  100
#define LCD_AUTO_CHECKER_COLS     8
#define LCD_AUTO_CHECKER_ROWS     8
#define LCD_AUTO_ARROW_BAR_COUNT  7
#define LCD_AUTO_HOLD_MS          1000
#define LCD_AUTO_SETTLE_MS        150

static const char *TAG = "TEST_DEV_LCD_AUTO";

static lv_obj_t *s_auto_root;

static void lcd_auto_ui_acquire(void)
{
    lvgl_port_lock(LCD_AUTO_LOCK_TIMEOUT_MS);
}

static void lcd_auto_ui_release(void)
{
    lvgl_port_unlock();
}

static bool lcd_auto_lvgl_ready(void)
{
    return lv_disp_get_default() != NULL && lv_scr_act() != NULL;
}

static esp_err_t lcd_auto_ensure_ready(void)
{
    if (lcd_auto_lvgl_ready()) {
        return ESP_OK;
    }

    esp_err_t ret = test_dev_lcd_lvgl_init();
    if (ret != ESP_OK && !lcd_auto_lvgl_ready()) {
        ESP_LOGE(TAG, "Failed to initialize LCD LVGL automation view: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

static void lcd_auto_get_screen_size(lv_coord_t *width, lv_coord_t *height)
{
    lv_disp_t *disp = lv_disp_get_default();

    if (disp != NULL) {
        *width = lv_disp_get_hor_res(disp);
        *height = lv_disp_get_ver_res(disp);
        return;
    }

    *width = 320;
    *height = 240;
}

static void lcd_auto_clear_locked(void)
{
    if (s_auto_root != NULL && lv_obj_is_valid(s_auto_root)) {
        lv_obj_del(s_auto_root);
    }
    s_auto_root = NULL;
}

static esp_err_t lcd_auto_create_root(lv_color_t bg_color, lv_obj_t **root_out)
{
    lv_coord_t screen_width = 0;
    lv_coord_t screen_height = 0;
    lv_obj_t *root = NULL;

    if (root_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!lcd_auto_lvgl_ready()) {
        return ESP_ERR_INVALID_STATE;
    }

    lcd_auto_get_screen_size(&screen_width, &screen_height);
    root = lv_obj_create(lv_scr_act());
    if (root == NULL) {
        return ESP_ERR_NO_MEM;
    }

    lv_obj_set_size(root, screen_width, screen_height);
    lv_obj_set_pos(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(root, bg_color, 0);

    *root_out = root;
    return ESP_OK;
}

static esp_err_t lcd_auto_draw_checker_locked(void)
{
    lv_coord_t screen_width = 0;
    lv_coord_t screen_height = 0;
    lv_coord_t cell_width = 0;
    lv_coord_t cell_height = 0;
    lv_obj_t *root = NULL;

    esp_err_t ret = lcd_auto_create_root(lv_color_black(), &root);
    if (ret != ESP_OK) {
        return ret;
    }

    s_auto_root = root;
    lcd_auto_get_screen_size(&screen_width, &screen_height);
    cell_width = screen_width / LCD_AUTO_CHECKER_COLS;
    cell_height = screen_height / LCD_AUTO_CHECKER_ROWS;

    for (int row = 0; row < LCD_AUTO_CHECKER_ROWS; row++) {
        for (int col = 0; col < LCD_AUTO_CHECKER_COLS; col++) {
            lv_coord_t x = col * cell_width;
            lv_coord_t y = row * cell_height;
            lv_coord_t width = (col == LCD_AUTO_CHECKER_COLS - 1) ? (screen_width - x) : cell_width;
            lv_coord_t height = (row == LCD_AUTO_CHECKER_ROWS - 1) ? (screen_height - y) : cell_height;
            lv_obj_t *cell = lv_obj_create(root);

            if (cell == NULL) {
                lcd_auto_clear_locked();
                return ESP_ERR_NO_MEM;
            }

            lv_obj_set_size(cell, width, height);
            lv_obj_set_pos(cell, x, y);
            lv_obj_set_style_radius(cell, 0, 0);
            lv_obj_set_style_border_width(cell, 0, 0);
            lv_obj_set_style_pad_all(cell, 0, 0);
            lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
            lv_obj_set_style_bg_color(cell,
                                      ((row + col) % 2 == 0) ? lv_color_white() : lv_color_black(),
                                      0);
        }
    }
    return ESP_OK;
}

static esp_err_t lcd_auto_draw_arrow_locked(void)
{
    lv_coord_t screen_width = 0;
    lv_coord_t screen_height = 0;
    lv_obj_t *root = NULL;

    esp_err_t ret = lcd_auto_create_root(lv_color_black(), &root);
    if (ret != ESP_OK) {
        return ret;
    }

    s_auto_root = root;
    lcd_auto_get_screen_size(&screen_width, &screen_height);

    /* Shaft: left half of the screen, vertically centered */
    lv_obj_t *shaft = lv_obj_create(root);
    if (shaft == NULL) {
        lcd_auto_clear_locked();
        return ESP_ERR_NO_MEM;
    }
    lv_obj_set_pos(shaft, (screen_width * 10) / 100, (screen_height * 42) / 100);
    lv_obj_set_size(shaft, (screen_width * 45) / 100, (screen_height * 16) / 100);
    lv_obj_set_style_radius(shaft, 0, 0);
    lv_obj_set_style_border_width(shaft, 0, 0);
    lv_obj_set_style_pad_all(shaft, 0, 0);
    lv_obj_set_style_bg_opa(shaft, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(shaft, lv_color_white(), 0);

    /**
     * Arrowhead: 7 horizontal bars, all starting from the same left edge (55%),
     * with the middle bar reaching furthest right (90%) and top/bottom bars
     * becoming progressively shorter — this creates a right-pointing triangular tip.
     *
     * Bar layout (top to bottom, y% / end_x%):
     *   0: y=10-24%  end=64%
     *   1: y=24-33%  end=73%
     *   2: y=33-42%  end=82%
     *   3: y=42-58%  end=90%   ← widest / tip row (aligns with shaft)
     *   4: y=58-67%  end=82%
     *   5: y=67-76%  end=73%
     *   6: y=76-90%  end=64%
     */
    static const int top_pcts[LCD_AUTO_ARROW_BAR_COUNT] = {10, 24, 33, 42, 58, 67, 76};
    static const int bot_pcts[LCD_AUTO_ARROW_BAR_COUNT] = {24, 33, 42, 58, 67, 76, 90};
    static const int end_pcts[LCD_AUTO_ARROW_BAR_COUNT] = {64, 73, 82, 90, 82, 73, 64};
    static const int start_pct = 55;

    for (int i = 0; i < LCD_AUTO_ARROW_BAR_COUNT; i++) {
        lv_coord_t y = (screen_height * top_pcts[i]) / 100;
        lv_coord_t height = (screen_height * bot_pcts[i]) / 100 - y;
        lv_coord_t x = (screen_width * start_pct) / 100;
        lv_coord_t width = (screen_width * end_pcts[i]) / 100 - x;
        lv_obj_t *bar = lv_obj_create(root);

        if (bar == NULL) {
            lcd_auto_clear_locked();
            return ESP_ERR_NO_MEM;
        }

        lv_obj_set_pos(bar, x, y);
        lv_obj_set_size(bar, width, height);
        lv_obj_set_style_radius(bar, 0, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_pad_all(bar, 0, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(bar, lv_color_white(), 0);
    }
    return ESP_OK;
}

esp_err_t test_dev_lcd_auto_show_color(test_dev_lcd_auto_color_t color)
{
    static const lv_color_t colors[TEST_DEV_LCD_AUTO_COLOR_COUNT] = {
        [TEST_DEV_LCD_AUTO_COLOR_RED] = LV_COLOR_MAKE(255, 0, 0),
        [TEST_DEV_LCD_AUTO_COLOR_GREEN] = LV_COLOR_MAKE(0, 255, 0),
        [TEST_DEV_LCD_AUTO_COLOR_BLUE] = LV_COLOR_MAKE(0, 0, 255),
        [TEST_DEV_LCD_AUTO_COLOR_WHITE] = LV_COLOR_MAKE(255, 255, 255),
        [TEST_DEV_LCD_AUTO_COLOR_BLACK] = LV_COLOR_MAKE(0, 0, 0),
    };

    if ((unsigned)color >= TEST_DEV_LCD_AUTO_COLOR_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = lcd_auto_ensure_ready();
    if (ret != ESP_OK) {
        return ret;
    }

    lcd_auto_ui_acquire();
    lcd_auto_clear_locked();
    ret = lcd_auto_create_root(colors[color], &s_auto_root);
    lcd_auto_ui_release();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to show LCD color test pattern: %s", esp_err_to_name(ret));
    } else {
        /* Wait for LVGL to render and DMA2D to flush at least one frame */
        vTaskDelay(pdMS_TO_TICKS(LCD_AUTO_HOLD_MS));
    }

    /* Deinit LVGL before returning so the framework can cleanly deinit the display */
    esp_err_t deinit_ret = test_dev_lcd_lvgl_deinit();
    if (deinit_ret != ESP_OK) {
        ESP_LOGW(TAG, "LCD LVGL deinit failed: %s", esp_err_to_name(deinit_ret));
    }

    /* Wait for LVGL task stop log to flush to the console before [CASE] PASS appears */
    vTaskDelay(pdMS_TO_TICKS(LCD_AUTO_SETTLE_MS));
    return ret;
}

esp_err_t test_dev_lcd_auto_show_pattern(test_dev_lcd_auto_pattern_t pattern)
{
    if ((unsigned)pattern >= TEST_DEV_LCD_AUTO_PATTERN_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = lcd_auto_ensure_ready();
    if (ret != ESP_OK) {
        return ret;
    }

    lcd_auto_ui_acquire();
    lcd_auto_clear_locked();

    switch (pattern) {
        case TEST_DEV_LCD_AUTO_PATTERN_CHECKER:
            ret = lcd_auto_draw_checker_locked();
            break;
        case TEST_DEV_LCD_AUTO_PATTERN_ARROW:
            ret = lcd_auto_draw_arrow_locked();
            break;
        default:
            ret = ESP_ERR_INVALID_ARG;
            break;
    }

    lcd_auto_ui_release();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to show LCD automation pattern %d: %s", pattern, esp_err_to_name(ret));
    } else {
        /* Wait for LVGL to render and DMA2D to flush at least one frame */
        vTaskDelay(pdMS_TO_TICKS(LCD_AUTO_HOLD_MS));
    }

    /* Deinit LVGL before returning so the framework can cleanly deinit the display */
    esp_err_t deinit_ret = test_dev_lcd_lvgl_deinit();
    if (deinit_ret != ESP_OK) {
        ESP_LOGW(TAG, "LCD LVGL deinit failed: %s", esp_err_to_name(deinit_ret));
    }

    /* Wait for LVGL task stop log to flush to the console before [CASE] PASS appears */
    vTaskDelay(pdMS_TO_TICKS(LCD_AUTO_SETTLE_MS));
    return ret;
}

esp_err_t test_dev_lcd_auto_clear(void)
{
    if (!lcd_auto_lvgl_ready()) {
        s_auto_root = NULL;
        return ESP_OK;
    }

    lcd_auto_ui_acquire();
    lcd_auto_clear_locked();
    lcd_auto_ui_release();
    return ESP_OK;
}
