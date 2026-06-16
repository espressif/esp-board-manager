/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_log.h"
#include "esp_board_manager_includes.h"
#include "esp_io_expander_pi4ioe5v6408.h"
#include "esp_lcd_ili9881c.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_st7121.h"
#include "esp_lcd_st7123.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lcd_touch_st7123.h"
#include "disp_init_data.h"

static const char *TAG = "M5STACK_TAB5_SETUP_DEVICE";

#define TAB5_TOUCH_ADDR_GT911   0x28u  /* 8-bit / left-shifted */
#define TAB5_TOUCH_ADDR_ST712X  0xaau  /* 8-bit / left-shifted */
#define TAB5_ST712X_FW_CMD      0x0000u
#define TAB5_ST7121_FW_VERSION  1u
#define TAB5_ST7123_FW_VERSION  3u

typedef struct {
    lcd_rgb_element_order_t  data_endian;
    uint32_t                 dpi_clock_freq_mhz;
    uint8_t                  num_fbs;
    uint16_t                 hsync_back_porch;
    uint16_t                 hsync_pulse_width;
    uint16_t                 hsync_front_porch;
    uint16_t                 vsync_back_porch;
    uint16_t                 vsync_pulse_width;
    uint16_t                 vsync_front_porch;
} tab5_panel_timing_t;

typedef enum {
    TAB5_PANEL_VARIANT_ILI9881C,
    TAB5_PANEL_VARIANT_ST7121,
    TAB5_PANEL_VARIANT_ST7123,
} tab5_panel_variant_t;

static const tab5_panel_timing_t s_timing_ili9881c = {
    .data_endian        = LCD_RGB_DATA_ENDIAN_BIG,
    .dpi_clock_freq_mhz = 80,
    .num_fbs            = 1,
    .hsync_back_porch   = 140,
    .hsync_pulse_width  = 40,
    .hsync_front_porch  = 40,
    .vsync_back_porch   = 20,
    .vsync_pulse_width  = 4,
    .vsync_front_porch  = 20,
};

static const tab5_panel_timing_t s_timing_st7121 = {
    .data_endian        = LCD_RGB_DATA_ENDIAN_LITTLE,
    .dpi_clock_freq_mhz = 70,
    .num_fbs            = 1,
    .hsync_back_porch   = 40,
    .hsync_pulse_width  = 2,
    .hsync_front_porch  = 40,
    .vsync_back_porch   = 24,
    .vsync_pulse_width  = 20,
    .vsync_front_porch  = 200,
};

static const tab5_panel_timing_t s_timing_st7123 = {
    .data_endian        = LCD_RGB_DATA_ENDIAN_LITTLE,
    .dpi_clock_freq_mhz = 80,
    .num_fbs            = 2,
    .hsync_back_porch   = 40,
    .hsync_pulse_width  = 2,
    .hsync_front_porch  = 40,
    .vsync_back_porch   = 8,
    .vsync_pulse_width  = 2,
    .vsync_front_porch  = 220,
};

static void apply_timing(dev_display_lcd_config_t *cfg, const tab5_panel_timing_t *t)
{
    cfg->data_endian = t->data_endian;
    cfg->sub_cfg.dsi.dpi_config.dpi_clock_freq_mhz = t->dpi_clock_freq_mhz;
    cfg->sub_cfg.dsi.dpi_config.num_fbs = t->num_fbs;
    cfg->sub_cfg.dsi.dpi_config.video_timing.hsync_back_porch = t->hsync_back_porch;
    cfg->sub_cfg.dsi.dpi_config.video_timing.hsync_pulse_width = t->hsync_pulse_width;
    cfg->sub_cfg.dsi.dpi_config.video_timing.hsync_front_porch = t->hsync_front_porch;
    cfg->sub_cfg.dsi.dpi_config.video_timing.vsync_back_porch = t->vsync_back_porch;
    cfg->sub_cfg.dsi.dpi_config.video_timing.vsync_pulse_width = t->vsync_pulse_width;
    cfg->sub_cfg.dsi.dpi_config.video_timing.vsync_front_porch = t->vsync_front_porch;
}

static tab5_panel_variant_t detect_panel_variant(uint16_t touch_addr)
{
    if (touch_addr != TAB5_TOUCH_ADDR_ST712X) {
        return TAB5_PANEL_VARIANT_ILI9881C;
    }

    dev_lcd_touch_handles_t *touch_handles = NULL;
    esp_err_t ret = esp_board_device_get_handle("lcd_touch", (void **)&touch_handles);
    if (ret != ESP_OK || !touch_handles || !touch_handles->io_handle) {
        ESP_LOGW(TAG, "ST712x touch handle unavailable, defaulting to ST7123: %s", esp_err_to_name(ret));
        return TAB5_PANEL_VARIANT_ST7123;
    }

    uint8_t fw_version = 0;
    ret = esp_lcd_panel_io_rx_param(touch_handles->io_handle, TAB5_ST712X_FW_CMD, &fw_version, sizeof(fw_version));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read ST712x touch firmware version, defaulting to ST7123: %s", esp_err_to_name(ret));
        return TAB5_PANEL_VARIANT_ST7123;
    }

    if (fw_version == TAB5_ST7121_FW_VERSION) {
        ESP_LOGI(TAG, "Detected ST712x touch firmware version %u, using ST7121 display", fw_version);
        return TAB5_PANEL_VARIANT_ST7121;
    }
    if (fw_version == TAB5_ST7123_FW_VERSION) {
        ESP_LOGI(TAG, "Detected ST712x touch firmware version %u, using ST7123 display", fw_version);
        return TAB5_PANEL_VARIANT_ST7123;
    }

    ESP_LOGW(TAG, "Unknown ST712x touch firmware version %u, defaulting to ST7123", fw_version);
    return TAB5_PANEL_VARIANT_ST7123;
}

esp_err_t io_expander_factory_entry_t(i2c_master_bus_handle_t i2c_handle,
                                      const uint16_t dev_addr,
                                      esp_io_expander_handle_t *handle_ret)
{
    esp_err_t ret = esp_io_expander_new_i2c_pi4ioe5v6408(i2c_handle, dev_addr, handle_ret);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create IO expander: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t lcd_dsi_panel_factory_entry_t(esp_lcd_dsi_bus_handle_t dsi_handle,
                                        dev_display_lcd_config_t *lcd_cfg,
                                        dev_display_lcd_handles_t *lcd_handles)
{
    uint16_t touch_addr = 0;
    esp_err_t ret = esp_board_device_get_i2c_effective_addr("lcd_touch", &touch_addr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "lcd_touch effective addr unavailable: %s", esp_err_to_name(ret));
        return ret;
    }

    dev_display_lcd_config_t local = *lcd_cfg;
    tab5_panel_variant_t panel_variant = detect_panel_variant(touch_addr);
    const tab5_panel_timing_t *timing = &s_timing_ili9881c;
    if (panel_variant == TAB5_PANEL_VARIANT_ST7121) {
        timing = &s_timing_st7121;
    } else if (panel_variant == TAB5_PANEL_VARIANT_ST7123) {
        timing = &s_timing_st7123;
    }
    apply_timing(&local, timing);
    if (panel_variant == TAB5_PANEL_VARIANT_ST7121 || panel_variant == TAB5_PANEL_VARIANT_ST7123) {
        local.bits_per_pixel = 24;
    }

    esp_lcd_panel_dev_config_t dev_config = {
        .reset_gpio_num = local.sub_cfg.dsi.reset_gpio_num,
        .rgb_ele_order = local.rgb_ele_order,
        .bits_per_pixel = local.bits_per_pixel,
        .data_endian = local.data_endian,
        .flags = {
            .reset_active_high = local.sub_cfg.dsi.reset_active_high,
        },
    };

    if (panel_variant == TAB5_PANEL_VARIANT_ST7121) {
        ESP_LOGI(TAG, "Tab5 panel variant: ST7121");
        st7121_vendor_config_t vendor = {
            .init_cmds = NULL,
            .init_cmds_size = 0,
            .mipi_config = {
                .dsi_bus = dsi_handle,
                .dpi_config = &local.sub_cfg.dsi.dpi_config,
            },
        };
        dev_config.vendor_config = &vendor;
        ret = esp_lcd_new_panel_st7121(lcd_handles->io_handle, &dev_config, &lcd_handles->panel_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create st7121 panel: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    if (panel_variant == TAB5_PANEL_VARIANT_ST7123) {
        ESP_LOGI(TAG, "Tab5 panel variant: ST7123");
        st7123_vendor_config_t vendor = {
            .init_cmds = tab5_st7123_init_cmds,
            .init_cmds_size = sizeof(tab5_st7123_init_cmds) / sizeof(tab5_st7123_init_cmds[0]),
            .mipi_config = {
                .dsi_bus = dsi_handle,
                .dpi_config = &local.sub_cfg.dsi.dpi_config,
            },
        };
        dev_config.vendor_config = &vendor;
        ret = esp_lcd_new_panel_st7123(lcd_handles->io_handle, &dev_config, &lcd_handles->panel_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create st7123 panel: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    ESP_LOGI(TAG, "Tab5 panel variant: ILI9881C (touch addr=0x%02x)", touch_addr);
    ili9881c_vendor_config_t vendor = {
        .init_cmds = tab5_ili9881c_init_cmds,
        .init_cmds_size = sizeof(tab5_ili9881c_init_cmds) / sizeof(tab5_ili9881c_init_cmds[0]),
        .mipi_config = {
            .dsi_bus = dsi_handle,
            .dpi_config = &local.sub_cfg.dsi.dpi_config,
        },
    };
    dev_config.vendor_config = &vendor;
    ret = esp_lcd_new_panel_ili9881c(lcd_handles->io_handle, &dev_config, &lcd_handles->panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ili9881c panel: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t lcd_touch_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                    const esp_lcd_touch_config_t *touch_dev_config,
                                    esp_lcd_touch_handle_t *ret_touch)
{
    uint16_t touch_addr = 0;
    esp_err_t ret = esp_board_device_get_i2c_effective_addr("lcd_touch", &touch_addr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "lcd_touch effective addr unavailable: %s", esp_err_to_name(ret));
        return ret;
    }

    if (touch_addr == TAB5_TOUCH_ADDR_ST712X) {
        ret = esp_lcd_touch_new_i2c_st7123(io, touch_dev_config, ret_touch);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create st7123 touch: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    ret = esp_lcd_touch_new_i2c_gt911(io, touch_dev_config, ret_touch);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create gt911 touch: %s", esp_err_to_name(ret));
    }
    return ret;
}
