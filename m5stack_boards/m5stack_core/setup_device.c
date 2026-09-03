/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 */

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"

#if __has_include(<esp_lcd_ili9341.h>)
#define HAS_ILI9341  1
#include "esp_lcd_ili9341.h"
#endif  /* __has_include(<esp_lcd_ili9341.h>) */

#if defined(HAS_ILI9341)
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_panel_dev_config_t *panel_dev_config,
                                                          esp_lcd_panel_handle_t *ret_panel)
{
    return esp_lcd_new_panel_ili9341(io, panel_dev_config, ret_panel);
}
#endif  /* defined(HAS_ILI9341) */
