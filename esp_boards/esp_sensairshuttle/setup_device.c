/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#if __has_include(<esp_lcd_panel_vendor.h>)
#define HAS_LCD_PANEL_VENDOR  1
#include "esp_lcd_panel_vendor.h"
#endif  /* __has_include(<esp_lcd_panel_vendor.h>) */
#if __has_include(<esp_lcd_touch_cst816s.h>)
#define HAS_CST816S  1
#include "esp_lcd_touch_cst816s.h"
#endif  /* __has_include(<esp_lcd_touch_cst816s.h>) */

#if defined(HAS_LCD_PANEL_VENDOR)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_panel_dev_config_t *panel_dev_config,
                                                          esp_lcd_panel_handle_t *ret_panel)
{
    return esp_lcd_new_panel_st7789(io, panel_dev_config, ret_panel);
}
#endif  /* defined(HAS_LCD_PANEL_VENDOR) */

#if defined(HAS_CST816S)
__attribute__((weak)) esp_err_t lcd_touch_factory_entry_t(const esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_touch_config_t *config,
                                                          esp_lcd_touch_handle_t *ret_touch)
{
    return esp_lcd_touch_new_i2c_cst816s(io, config, ret_touch);
}
#endif  /* defined(HAS_CST816S) */
