/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "dev_lcd_touch.h"
#include "esp_board_manager.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "bmgr_test_names.h"
#include "test_dev_lcd_lvgl.h"
#include "test_dev_lcd_touch_view.h"

#define TOUCH_VIEW_LOCK_MS     100
#define TOUCH_VIEW_RESULT_MS   1500
#define TOUCH_VIEW_MARKER_PX   28
#define TOUCH_VIEW_TRAIL_SIZE  24

#if LVGL_VERSION_MAJOR >= 9
typedef lv_point_precise_t touch_view_point_t;
#else
typedef lv_point_t touch_view_point_t;
#endif  /* LVGL_VERSION_MAJOR >= 9 */

typedef struct {
    bool      active;
    uint8_t   track_id;
    lv_obj_t *marker;
    lv_obj_t *label;
#if LV_USE_LINE
    lv_obj_t           *line;
    touch_view_point_t  trail[TOUCH_VIEW_TRAIL_SIZE];
    uint8_t             trail_count;
#endif  /* LV_USE_LINE */
} touch_view_slot_t;

struct test_dev_lcd_touch_view {
    lv_obj_t          *root;
    lv_obj_t          *status;
    lv_coord_t         width;
    lv_coord_t         height;
    uint16_t           x_max;
    uint16_t           y_max;
    touch_view_slot_t  slots[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
};

static const char *TAG = "TOUCH_VIEW";
static const uint32_t s_colors[] = {0x36D7FF, 0xFFB347, 0x9DE06F, 0xFF79B0, 0xB89BFF, 0xFFFF80};

static bool frame_has_id(const esp_lcd_touch_point_data_t *points, uint8_t count, uint8_t id)
{
    for (uint8_t i = 0; i < count; i++) {
        if (points[i].track_id == id) {
            return true;
        }
    }
    return false;
}

static void hide_slot(touch_view_slot_t *slot)
{
    slot->active = false;
    lv_obj_add_flag(slot->marker, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(slot->label, LV_OBJ_FLAG_HIDDEN);
#if LV_USE_LINE
    slot->trail_count = 0;
    lv_obj_add_flag(slot->line, LV_OBJ_FLAG_HIDDEN);
#endif  /* LV_USE_LINE */
}

static lv_coord_t map_coordinate(uint16_t value, uint16_t maximum, lv_coord_t size)
{
    uint32_t pixel = (uint32_t)value * (uint32_t)size / maximum;
    return pixel >= (uint32_t)size ? size - 1 : (lv_coord_t)pixel;
}

static esp_err_t create_objects(test_dev_lcd_touch_view_t *view)
{
    view->root = lv_obj_create(lv_scr_act());
    if (view->root == NULL) {
        return ESP_ERR_NO_MEM;
    }
    lv_obj_set_size(view->root, view->width, view->height);
    lv_obj_set_pos(view->root, 0, 0);
    lv_obj_clear_flag(view->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(view->root, 0, 0);
    lv_obj_set_style_border_width(view->root, 0, 0);
    lv_obj_set_style_pad_all(view->root, 0, 0);
    lv_obj_set_style_bg_opa(view->root, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(view->root, lv_color_hex(0x101820), 0);

    view->status = lv_label_create(view->root);
    if (view->status == NULL) {
        return ESP_ERR_NO_MEM;
    }
    lv_obj_set_width(view->status, view->width - 8);
    lv_obj_set_pos(view->status, 4, 4);
    lv_obj_set_style_text_color(view->status, lv_color_white(), 0);
    lv_label_set_text(view->status, "MULTI-TOUCH: 30s\nPress 2+, move, lift one, release all");

    for (size_t i = 0; i < CONFIG_ESP_LCD_TOUCH_MAX_POINTS; i++) {
        touch_view_slot_t *slot = &view->slots[i];
        lv_color_t color = lv_color_hex(s_colors[i % (sizeof(s_colors) / sizeof(s_colors[0]))]);
#if LV_USE_LINE
        slot->line = lv_line_create(view->root);
        if (slot->line == NULL) {
            return ESP_ERR_NO_MEM;
        }
        lv_obj_set_pos(slot->line, 0, 0);
        lv_obj_set_style_line_width(slot->line, 3, 0);
        lv_obj_set_style_line_color(slot->line, color, 0);
#endif  /* LV_USE_LINE */
        slot->marker = lv_obj_create(view->root);
        slot->label = lv_label_create(view->root);
        if (slot->marker == NULL || slot->label == NULL) {
            return ESP_ERR_NO_MEM;
        }
        lv_obj_set_size(slot->marker, TOUCH_VIEW_MARKER_PX, TOUCH_VIEW_MARKER_PX);
        lv_obj_set_style_radius(slot->marker, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(slot->marker, 2, 0);
        lv_obj_set_style_border_color(slot->marker, color, 0);
        lv_obj_set_style_bg_color(slot->marker, color, 0);
        lv_obj_set_style_bg_opa(slot->marker, LV_OPA_50, 0);
        lv_obj_clear_flag(slot->marker, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_text_color(slot->label, color, 0);
        hide_slot(slot);
    }
    return ESP_OK;
}

esp_err_t test_dev_lcd_touch_view_start(test_dev_lcd_touch_view_t **out_view)
{
    if (out_view == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_view = NULL;
    dev_lcd_touch_handles_t *handles = NULL;
    esp_err_t ret = esp_board_manager_get_device_handle(BMGR_TEST_NAME_LCD_TOUCH, (void **)&handles);
    if (ret != ESP_OK) {
        return ret;
    }
    if (handles == NULL || handles->touch_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_lcd_touch_config_t *config = &handles->touch_handle->config;
    if (config->x_max == 0 || config->y_max == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    test_dev_lcd_touch_view_t *view = calloc(1, sizeof(*view));
    if (view == NULL) {
        return ESP_ERR_NO_MEM;
    }
    view->x_max = config->flags.swap_xy ? config->y_max : config->x_max;
    view->y_max = config->flags.swap_xy ? config->x_max : config->y_max;

    ret = test_dev_lcd_lvgl_init();
    if (ret != ESP_OK) {
        free(view);
        return ret;
    }
    if (!lvgl_port_lock(TOUCH_VIEW_LOCK_MS)) {
        test_dev_lcd_touch_view_stop(view);
        return ESP_ERR_TIMEOUT;
    }
    lv_disp_t *display = lv_disp_get_default();
    if (display == NULL) {
        ret = ESP_ERR_INVALID_STATE;
    } else {
        view->width = lv_disp_get_hor_res(display);
        view->height = lv_disp_get_ver_res(display);
        ret = view->width > 8 && view->height > 8 ? create_objects(view) : ESP_ERR_INVALID_SIZE;
    }
    lvgl_port_unlock();
    if (ret != ESP_OK) {
        test_dev_lcd_touch_view_stop(view);
        return ret;
    }
    *out_view = view;
    ESP_LOGI(TAG, "Display enabled; touch samples come only from the multi-touch test");
    return ESP_OK;
}

esp_err_t test_dev_lcd_touch_view_update(const esp_lcd_touch_point_data_t *points,
                                         uint8_t count, void *user_ctx)
{
    test_dev_lcd_touch_view_t *view = user_ctx;
    if (view == NULL || (count > 0 && points == NULL) || count > CONFIG_ESP_LCD_TOUCH_MAX_POINTS) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!lvgl_port_lock(TOUCH_VIEW_LOCK_MS)) {
        return ESP_ERR_TIMEOUT;
    }
    for (size_t i = 0; i < CONFIG_ESP_LCD_TOUCH_MAX_POINTS; i++) {
        touch_view_slot_t *slot = &view->slots[i];
        if (slot->active && !frame_has_id(points, count, slot->track_id)) {
            hide_slot(slot);
        }
    }
    for (uint8_t i = 0; i < count; i++) {
        touch_view_slot_t *slot = NULL;
        for (size_t j = 0; j < CONFIG_ESP_LCD_TOUCH_MAX_POINTS; j++) {
            if (view->slots[j].active && view->slots[j].track_id == points[i].track_id) {
                slot = &view->slots[j];
                break;
            }
        }
        if (slot == NULL) {
            for (size_t j = 0; j < CONFIG_ESP_LCD_TOUCH_MAX_POINTS; j++) {
                if (!view->slots[j].active) {
                    slot = &view->slots[j];
                    slot->active = true;
                    slot->track_id = points[i].track_id;
                    break;
                }
            }
        }
        lv_coord_t x = map_coordinate(points[i].x, view->x_max, view->width);
        lv_coord_t y = map_coordinate(points[i].y, view->y_max, view->height);
        lv_obj_clear_flag(slot->marker, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(slot->label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(slot->marker, x - TOUCH_VIEW_MARKER_PX / 2, y - TOUCH_VIEW_MARKER_PX / 2);
        lv_label_set_text_fmt(slot->label, "ID %u (%u,%u)",
                              (unsigned)points[i].track_id, (unsigned)points[i].x, (unsigned)points[i].y);
        lv_obj_update_layout(slot->label);
        lv_coord_t label_x = x + TOUCH_VIEW_MARKER_PX / 2;
        lv_coord_t label_y = y + TOUCH_VIEW_MARKER_PX / 2;
        if (label_x + lv_obj_get_width(slot->label) > view->width) {
            label_x = view->width - lv_obj_get_width(slot->label);
        }
        if (label_y + lv_obj_get_height(slot->label) > view->height) {
            label_y = y - TOUCH_VIEW_MARKER_PX / 2 - lv_obj_get_height(slot->label);
        }
        lv_obj_set_pos(slot->label, label_x < 0 ? 0 : label_x, label_y < 0 ? 0 : label_y);
#if LV_USE_LINE
        if (slot->trail_count == 0 || slot->trail[slot->trail_count - 1].x != x ||
            slot->trail[slot->trail_count - 1].y != y) {
            if (slot->trail_count == TOUCH_VIEW_TRAIL_SIZE) {
                memmove(slot->trail, slot->trail + 1, (TOUCH_VIEW_TRAIL_SIZE - 1) * sizeof(*slot->trail));
                slot->trail_count--;
            }
            slot->trail[slot->trail_count++] = (touch_view_point_t) {.x = x, .y = y};
            lv_line_set_points(slot->line, slot->trail, slot->trail_count);
        }
        if (slot->trail_count >= 2) {
            lv_obj_clear_flag(slot->line, LV_OBJ_FLAG_HIDDEN);
        }
#endif  /* LV_USE_LINE */
    }
    lv_label_set_text_fmt(view->status, "MULTI-TOUCH: %u contacts\nPress 2+, move, lift one, release all", (unsigned)count);
    lvgl_port_unlock();
    return ESP_OK;
}

void test_dev_lcd_touch_view_finish(test_dev_lcd_touch_view_t *view, esp_err_t result)
{
    if (view == NULL || !lvgl_port_lock(TOUCH_VIEW_LOCK_MS)) {
        return;
    }
    lv_label_set_text_fmt(view->status, "MULTI-TOUCH: %s\n%s", result == ESP_OK ? "PASS" : "FAIL",
                          result == ESP_OK ? "2+ contacts -> partial release -> all released" : esp_err_to_name(result));
    lv_obj_set_style_text_color(view->status, lv_color_hex(result == ESP_OK ? 0x9DE06F : 0xFF7979), 0);
    lvgl_port_unlock();
    vTaskDelay(pdMS_TO_TICKS(TOUCH_VIEW_RESULT_MS));
}

esp_err_t test_dev_lcd_touch_view_stop(test_dev_lcd_touch_view_t *view)
{
    if (view == NULL) {
        return ESP_OK;
    }
    /* Delete the owned objects before releasing their borrowed trail arrays,
     * even if the subsequent display/port shutdown reports an error. */
    if (view->root != NULL) {
        if (!lvgl_port_lock(0)) {
            return ESP_ERR_TIMEOUT;
        }
        lv_obj_del(view->root);
        view->root = NULL;
        lvgl_port_unlock();
    }
    esp_err_t ret = test_dev_lcd_lvgl_deinit();
    free(view);
    return ret;
}
