/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_err.h"
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/** @brief  Owned display-only view; it never acquires touch samples. */
typedef struct test_dev_lcd_touch_view test_dev_lcd_touch_view_t;

/**
 * @brief  Start a touch view on the caller's initialized LCD and touch devices.
 *
 * @note  Creates its own LVGL display without registering a touch input reader.
 *        A successful call must be paired with stop before releasing devices.
 *
 * @param[out]  view  View handle, set to NULL on failure.
 *
 * @return
 *       - ESP_OK                 View started.
 *       - ESP_ERR_INVALID_ARG    Output pointer is NULL.
 *       - ESP_ERR_INVALID_STATE  A display is already active or handles are unavailable.
 *       - Others                 Display initialization or allocation error.
 */
esp_err_t test_dev_lcd_touch_view_start(test_dev_lcd_touch_view_t **view);

/**
 * @brief  Display a validated frame supplied by the touch test observer.
 *
 * @param[in]  points    Borrowed touch points, copied into the view before return.
 * @param[in]  count     Number of contacts, including zero for full release.
 * @param[in]  user_ctx  View handle returned by start.
 *
 * @return
 *       - ESP_OK               Frame displayed.
 *       - ESP_ERR_INVALID_ARG  Invalid frame or context.
 *       - ESP_ERR_TIMEOUT      LVGL lock could not be acquired.
 */
esp_err_t test_dev_lcd_touch_view_update(const esp_lcd_touch_point_data_t *points,
                                         uint8_t count, void *user_ctx);

/**
 * @brief  Show the final result briefly before stopping the view.
 *
 * @param[in]  view    View handle.
 * @param[in]  result  Touch test result.
 */
void test_dev_lcd_touch_view_finish(test_dev_lcd_touch_view_t *view, esp_err_t result);

/**
 * @brief  Delete the view and stop its LVGL display before devices are released.
 *
 * @param[in]  view  Owned view handle, or NULL.
 *
 * @return
 *       - ESP_OK  View stopped.
 *       - Others  Display cleanup error.
 */
esp_err_t test_dev_lcd_touch_view_stop(test_dev_lcd_touch_view_t *view);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
