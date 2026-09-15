/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * Compile-only factory stubs for CI coverage boards. These satisfy the LCD /
 * touch / IO-expander linker entries without pulling a real panel or sensor
 * component from the registry.
 */

#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_io.h"
#ifdef CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT
#include "esp_lcd_touch.h"
#endif  /* CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT */
#ifdef CONFIG_ESP_BOARD_DEV_GPIO_EXPANDER_SUPPORT
#include "driver/i2c_master.h"
#include "esp_io_expander.h"
#endif  /* CONFIG_ESP_BOARD_DEV_GPIO_EXPANDER_SUPPORT */
#ifdef CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_DSI_SUPPORT
#include "esp_lcd_mipi_dsi.h"
#include "dev_display_lcd.h"
#endif  /* CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_DSI_SUPPORT */

static const char *TAG = "CI_COVERAGE_FAKE";

#ifdef CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT
__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_panel_dev_config_t *panel_dev_config,
                                                          esp_lcd_panel_handle_t *ret_panel)
{
    (void)io;
    (void)panel_dev_config;
    if (ret_panel) {
        *ret_panel = NULL;
    }
    ESP_LOGW(TAG, "CI coverage fake LCD panel factory; not a real panel driver");
    return ESP_ERR_NOT_SUPPORTED;
}
#endif  /* CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT */

#ifdef CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_DSI_SUPPORT
__attribute__((weak)) esp_err_t lcd_dsi_panel_factory_entry_t(esp_lcd_dsi_bus_handle_t dsi_handle,
                                                              dev_display_lcd_config_t *lcd_cfg,
                                                              dev_display_lcd_handles_t *lcd_handles)
{
    (void)dsi_handle;
    (void)lcd_cfg;
    (void)lcd_handles;
    ESP_LOGW(TAG, "CI coverage fake DSI LCD factory; not a real panel driver");
    return ESP_ERR_NOT_SUPPORTED;
}
#endif  /* CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_DSI_SUPPORT */

#ifdef CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT
__attribute__((weak)) esp_err_t lcd_touch_factory_entry_t(const esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_touch_config_t *config,
                                                          esp_lcd_touch_handle_t *tp)
{
    (void)io;
    (void)config;
    if (tp) {
        *tp = NULL;
    }
    ESP_LOGW(TAG, "CI coverage fake LCD touch factory; not a real touch driver");
    return ESP_ERR_NOT_SUPPORTED;
}
#endif  /* CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT */

#ifdef CONFIG_ESP_BOARD_DEV_GPIO_EXPANDER_SUPPORT
__attribute__((weak)) esp_err_t io_expander_factory_entry_t(i2c_master_bus_handle_t i2c_bus,
                                                            const uint16_t dev_addr,
                                                            esp_io_expander_handle_t *handle_ret)
{
    (void)i2c_bus;
    (void)dev_addr;
    if (handle_ret) {
        *handle_ret = NULL;
    }
    ESP_LOGW(TAG, "CI coverage fake IO expander factory; not a real expander driver");
    return ESP_ERR_NOT_SUPPORTED;
}
#endif  /* CONFIG_ESP_BOARD_DEV_GPIO_EXPANDER_SUPPORT */
