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

/**
 * @brief  Observe one complete, validated touch frame, including zero contacts.
 *
 * @note  The point array is borrowed only for the duration of the callback.
 *        Skipped or malformed reports do not invoke the observer.
 *
 * @param[in]  points    Current touch points.
 * @param[in]  count     Number of current points.
 * @param[in]  user_ctx  Caller-owned context.
 *
 * @return
 *       - ESP_OK  Continue the test.
 *       - Others  Stop the test and propagate the observer error.
 */
typedef esp_err_t (*test_dev_lcd_touch_observer_t)(const esp_lcd_touch_point_data_t *points,
                                                   uint8_t count, void *user_ctx);

/**
 * @brief  Observe a complete multi-touch gesture using native contact IDs.
 *
 * @note  The caller must initialize the configured touch device and stop other
 *        touch readers. This function borrows the handle without releasing it.
 *        Within 30 seconds, press at least two contacts, release one while
 *        retaining another contact, then release all contacts. Skipped samples
 *        do not change the previous valid touch state.
 *
 * @param[in]  observer  Optional observer for validated frames, or NULL.
 * @param[in]  user_ctx  Observer context.
 *
 * @return
 *       - ESP_OK                    The complete gesture was observed.
 *       - ESP_ERR_NOT_SUPPORTED     Native IDs or multi-point capacity are unavailable.
 *       - ESP_ERR_INVALID_STATE     The touch handle is unavailable.
 *       - ESP_ERR_INVALID_RESPONSE  A valid frame has duplicate IDs or an invalid count.
 *       - ESP_ERR_TIMEOUT           The gesture was incomplete after 30 seconds.
 *       - Others                    Device lookup or native touch API errors.
 */
esp_err_t test_dev_lcd_touch_multitouch(test_dev_lcd_touch_observer_t observer, void *user_ctx);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
