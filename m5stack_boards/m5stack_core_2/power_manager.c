/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>

#include "dev_power_ctrl.h"
#include "driver/i2c_master.h"
#include "esp_board_periph.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gen_board_device_custom.h"

static const char *TAG = "M5STACK_CORE2_POWER_CTRL";

typedef struct {
    i2c_master_dev_handle_t  handle;
    bool                     axp2101;
} m5stack_core2_power_ctrl_context_t;

static esp_err_t pmu_write(i2c_master_dev_handle_t handle, uint8_t reg, uint8_t value)
{
    const uint8_t data[] = {reg, value};
    return i2c_master_transmit(handle, data, sizeof(data), 1000);
}

static esp_err_t pmu_read(i2c_master_dev_handle_t handle, uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(handle, &reg, 1, value, 1, 1000);
}

static esp_err_t pmu_update_bits(i2c_master_dev_handle_t handle, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t current = 0;
    esp_err_t err = pmu_read(handle, reg, &current);
    if (err != ESP_OK) {
        return err;
    }
    current = (uint8_t)((current & (uint8_t)~mask) | (value & mask));
    return pmu_write(handle, reg, current);
}

static esp_err_t
pmu_enable_display(const m5stack_core2_power_ctrl_context_t *ctrl_ctx, bool power_on)
{
    if (!power_on) {
        return ESP_OK;
    }

    if (ctrl_ctx->axp2101) {
        /* Match the BSP: enable ALDO1-4 and BLDO1-2, then pulse ALDO2 as LCD reset.
         */
        esp_err_t err = pmu_write(ctrl_ctx->handle, 0x90, 0x3F);
        if (err != ESP_OK) {
            return err;
        }
        err = pmu_write(ctrl_ctx->handle, 0x96, 0x18);
        if (err != ESP_OK) {
            return err;
        }
        err = pmu_update_bits(ctrl_ctx->handle, 0x90, 0x02, 0x00);
        if (err != ESP_OK) {
            return err;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
        err = pmu_update_bits(ctrl_ctx->handle, 0x90, 0x02, 0x02);
        if (err != ESP_OK) {
            return err;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        return ESP_OK;
    }

    /* AXP192: configure the backlight rail and pulse GPIO4 LCD reset. */
    esp_err_t err = pmu_write(ctrl_ctx->handle, 0x27, 0x68);
    if (err != ESP_OK) {
        return err;
    }
    err = pmu_update_bits(ctrl_ctx->handle, 0x28, 0xF0, 0xF0);
    if (err != ESP_OK) {
        return err;
    }
    err = pmu_update_bits(ctrl_ctx->handle, 0x12, 0x06, 0x06);
    if (err != ESP_OK) {
        return err;
    }
    err = pmu_update_bits(ctrl_ctx->handle, 0x96, 0x02, 0x00);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    return pmu_update_bits(ctrl_ctx->handle, 0x96, 0x02, 0x02);
}

static int m5stack_core2_power_ctrl_init(const dev_power_ctrl_config_t *config, void **context)
{
    const dev_power_ctrl_custom_sub_config_t *custom_cfg =
        &config->sub_cfg.custom;
    const dev_custom_pmu_power_manager_custom_config_t *power_cfg =
        custom_cfg->user_cfg;
    if (power_cfg == NULL || custom_cfg->periph_count != 1 ||
        custom_cfg->periph_names == NULL) {
        ESP_LOGE(TAG, "Invalid PMU configuration");
        return ESP_ERR_INVALID_ARG;
    }

    if (power_cfg->pmu_model == NULL ||
        (strcmp(power_cfg->pmu_model, "axp192") != 0 && strcmp(power_cfg->pmu_model, "axp2101") != 0)) {
        ESP_LOGE(TAG, "Unsupported PMU model: %s", power_cfg->pmu_model ? power_cfg->pmu_model : "(null)");
        return ESP_ERR_NOT_SUPPORTED;
    }

    i2c_master_bus_handle_t bus = NULL;
    esp_err_t err =
        esp_board_periph_get_handle(custom_cfg->periph_names[0], (void **)&bus);
    if (err != ESP_OK || bus == NULL) {
        return err != ESP_OK ? err : ESP_ERR_INVALID_STATE;
    }
    const i2c_device_config_t device_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = power_cfg->i2c_addr,
        .scl_speed_hz = power_cfg->frequency,
    };
    i2c_master_dev_handle_t handle = NULL;
    err = i2c_master_bus_add_device(bus, &device_cfg, &handle);
    if (err != ESP_OK) {
        return err;
    }

    m5stack_core2_power_ctrl_context_t *ctrl_ctx = calloc(1, sizeof(*ctrl_ctx));
    if (ctrl_ctx == NULL) {
        i2c_master_bus_rm_device(handle);
        return ESP_ERR_NO_MEM;
    }
    ctrl_ctx->handle = handle;
    ctrl_ctx->axp2101 = strcmp(power_cfg->pmu_model, "axp2101") == 0;
    *context = ctrl_ctx;
    return ESP_OK;
}

static int m5stack_core2_power_ctrl_deinit(void *context)
{
    if (context == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    m5stack_core2_power_ctrl_context_t *ctrl_ctx = context;
    esp_err_t err = i2c_master_bus_rm_device(ctrl_ctx->handle);
    free(ctrl_ctx);
    return err;
}

static int m5stack_core2_power_ctrl_set_power(void *context, const char *device_name, bool power_on)
{
    if (context == NULL || device_name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* The BSP uses one shared rail for LCD, touch, and SD, and a separate
     * rail for the speaker. Keep the register writes limited to those
     * consumer rails; the complete PMU policy remains board-owned. */
    m5stack_core2_power_ctrl_context_t *ctrl_ctx = context;
    i2c_master_dev_handle_t handle = ctrl_ctx->handle;
    const bool display = strcmp(device_name, "display_lcd") == 0;
    const bool speaker = strcmp(device_name, "audio_dac") == 0;
    const bool shared_rail = strcmp(device_name, "lcd_touch") == 0 || strcmp(device_name, "fs_sdcard") == 0;
    if (!display && !speaker && !shared_rail) {
        ESP_LOGW(TAG, "no power branch for device %s", device_name);
        return ESP_OK;
    }

    if (display) {
        return pmu_enable_display(ctrl_ctx, power_on);
    }

    if (ctrl_ctx->axp2101) {
        /* AXP2101: ALDO4 (LCD/touch/SD) and ALDO3 (speaker) use
         * independent voltage/enable registers. */
        const uint8_t reg = speaker ? 0x94 : 0x95;
        return pmu_write(handle, reg, power_on ? 0x1C : 0x00);
    }

    uint8_t value = 0;
    const uint8_t reg = speaker ? 0x94 : 0x28;
    esp_err_t err = pmu_read(handle, reg, &value);
    if (err != ESP_OK) {
        return err;
    }

    if (speaker) {
        value = power_on ? (uint8_t)((value | 0x04) | 0xF0) : (uint8_t)(value & ~0x04);
    } else {
        value =
            power_on ? (uint8_t)((value & 0x0F) | 0xF0) : (uint8_t)(value & 0x0F);
    }
    return pmu_write(handle, reg, value);
}

static const dev_power_ctrl_custom_ops_t s_m5stack_core2_power_ctrl_ops = {
    .init      = m5stack_core2_power_ctrl_init,
    .deinit    = m5stack_core2_power_ctrl_deinit,
    .set_power = m5stack_core2_power_ctrl_set_power,
};

DEVICE_EXTRA_FUNC_REGISTER(pmu_power_manager, &s_m5stack_core2_power_ctrl_ops);
