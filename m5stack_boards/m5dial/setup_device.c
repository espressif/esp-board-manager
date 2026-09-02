/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 */

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

#if __has_include(<esp_lcd_gc9a01.h>)
#define HAS_GC9A01  1
#include "esp_lcd_gc9a01.h"
#endif  /* __has_include(<esp_lcd_gc9a01.h>) */

#if __has_include(<esp_lcd_touch_ft5x06.h>)
#define HAS_FT5X06  1
#include "esp_lcd_touch_ft5x06.h"
#endif  /* __has_include(<esp_lcd_touch_ft5x06.h>) */

#if defined(HAS_GC9A01)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_panel_dev_config_t *panel_dev_config,
                                                          esp_lcd_panel_handle_t *ret_panel)
{
    return esp_lcd_new_panel_gc9a01(io, panel_dev_config, ret_panel);
}
#endif  /* defined(HAS_GC9A01) */

#if defined(HAS_FT5X06)
__attribute__((weak)) esp_err_t lcd_touch_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_touch_config_t *touch_config,
                                                          esp_lcd_touch_handle_t *ret_touch)
{
    return esp_lcd_touch_new_i2c_ft5x06(io, touch_config, ret_touch);
}
#endif  /* defined(HAS_FT5X06) */
