/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "esp_board_device.h"
#include "esp_board_periph.h"
#include "gen_board_device_custom.h"
#include "esp_io_expander.h"
#include "dev_power_ctrl.h"

static const char *TAG = "CORES3_POWER_CTRL";

typedef enum {
    CORES3_POWER_CTRL_FEATURE_LCD,
    CORES3_POWER_CTRL_FEATURE_TOUCH,
    CORES3_POWER_CTRL_FEATURE_SPEAKER,
    CORES3_POWER_CTRL_FEATURE_SD,
    CORES3_POWER_CTRL_FEATURE_CAMERA,
} cores3_power_ctrl_feature_t;

static esp_err_t cores3_axp2101_set_voltage(i2c_master_dev_handle_t pm_handle, uint8_t reg,
                                            uint8_t voltage, uint8_t enable_mask)
{
    uint8_t data[2] = {reg, voltage};
    esp_err_t err = i2c_master_transmit(pm_handle, data, sizeof(data), 1000);
    if (err != ESP_OK) {
        return err;
    }

    uint8_t enable_reg = 0x90;
    uint8_t enable_value = 0;
    err = i2c_master_transmit_receive(pm_handle, &enable_reg, sizeof(enable_reg),
                                      &enable_value, sizeof(enable_value), 1000);
    if (err != ESP_OK || (enable_value & enable_mask) == enable_mask) {
        return err;
    }

    data[0] = enable_reg;
    data[1] = enable_value | enable_mask;
    return i2c_master_transmit(pm_handle, data, sizeof(data), 1000);
}

static esp_err_t cores3_power_ctrl_enable(i2c_master_dev_handle_t pm_handle, cores3_power_ctrl_feature_t feature)
{
    esp_err_t err = ESP_OK;
    uint8_t data[2];
    esp_io_expander_handle_t *gpio_exp_aw9523 = NULL;
    err = esp_board_device_get_handle("gpio_expander", (void **)&gpio_exp_aw9523);
    if (err != ESP_OK || gpio_exp_aw9523 == NULL || *gpio_exp_aw9523 == NULL) {
        return err != ESP_OK ? err : ESP_ERR_INVALID_STATE;
    }
    switch (feature) {
        case CORES3_POWER_CTRL_FEATURE_LCD:
            /* Enable LCD */
            err = esp_io_expander_set_level(*gpio_exp_aw9523, (1 << 9), 1);
            if (err != ESP_OK) {
                return err;
            }
            /* AXP DLDO1 Enable / LCD backlight (moved out of AXP init into LCD power path) */
            data[0] = 0x90;
            data[1] = 0xBF;
            err = i2c_master_transmit(pm_handle, data, sizeof(data), 1000);
            if (err != ESP_OK) {
                return err;
            }
            /* AXP DLDO1 voltage / LCD backlight */
            data[0] = 0x99;
            data[1] = 0b00011000;
            err = i2c_master_transmit(pm_handle, data, sizeof(data), 1000);
            if (err != ESP_OK) {
                return err;
            }
            break;
        case CORES3_POWER_CTRL_FEATURE_TOUCH:
            /* Enable Touch */
            err = esp_io_expander_set_level(*gpio_exp_aw9523, (1 << 0), 1);
            break;
        case CORES3_POWER_CTRL_FEATURE_SPEAKER:
            /* AXP ALDO1 voltage / PA PVDD / 1V8 */
            data[0] = 0x92;
            data[1] = 0b00001101;  // 1V8
            err = i2c_master_transmit(pm_handle, data, sizeof(data), 1000);
            if (err != ESP_OK) {
                return err;
            }
            /* AXP ALDO2 voltage / Codec / 3V3 */
            data[0] = 0x93;
            data[1] = 0b00011100;  // 3V3
            err = i2c_master_transmit(pm_handle, data, sizeof(data), 1000);
            if (err != ESP_OK) {
                return err;
            }
            /* AXP ALDO3 voltage / Codec+Mic / 3V3 */
            data[0] = 0x94;
            data[1] = 0b00011100;  // 3V3
            err = i2c_master_transmit(pm_handle, data, sizeof(data), 1000);
            if (err != ESP_OK) {
                return err;
            }
            err = esp_io_expander_set_level(*gpio_exp_aw9523, (1 << 2), 1);
            break;
        case CORES3_POWER_CTRL_FEATURE_SD:
            /* AXP ALDO4 voltage / SD Card / 3V3 */
            data[0] = 0x95;
            data[1] = 0b00011100;  // 3V3
            err = i2c_master_transmit(pm_handle, data, sizeof(data), 1000);
            if (err != ESP_OK) {
                return err;
            }
            /* Enable SD */
            err = esp_io_expander_set_level(*gpio_exp_aw9523, (1 << 4), 1);
            break;
        case CORES3_POWER_CTRL_FEATURE_CAMERA:
            err = esp_io_expander_set_level(*gpio_exp_aw9523, (1 << 8), 1);
            if (err != ESP_OK) {
                return err;
            }
            /* AXP ALDO3 voltage / camera rails / 3V3. */
            err = cores3_axp2101_set_voltage(pm_handle, 0x94, 0b00011100, 0b00110100);
            break;
        default:
            ESP_LOGE(TAG, "Unsupported feature");
            return ESP_ERR_INVALID_ARG;
    }

    return err;
}

/**
 * @brief  Initialize the CoreS3 board power controller.
 */
static int cores3_power_ctrl_init(const dev_power_ctrl_config_t *config, void **context)
{
    const dev_power_ctrl_custom_sub_config_t *custom_cfg = &config->sub_cfg.custom;
    const dev_custom_axp2101_power_manager_custom_config_t *power_cfg = custom_cfg->user_cfg;
    if (power_cfg == NULL || custom_cfg->periph_count != 1 || custom_cfg->periph_names == NULL) {
        ESP_LOGE(TAG, "Invalid AXP2101 power controller configuration");
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_bus_handle_t i2c_master_handle = NULL;
    esp_err_t err = esp_board_periph_get_handle(custom_cfg->periph_names[0], (void **)&i2c_master_handle);
    if (err != ESP_OK || i2c_master_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get I2C handle %s: %d", custom_cfg->periph_names[0], err);
        return err != ESP_OK ? err : ESP_ERR_INVALID_STATE;
    }

    const i2c_device_config_t axp2101_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = power_cfg->i2c_addr,
        .scl_speed_hz = power_cfg->frequency,
    };
    i2c_master_dev_handle_t pm_handle = NULL;
    err = i2c_master_bus_add_device(i2c_master_handle, &axp2101_config, &pm_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add AXP2101 device to I2C bus");
        return err;
    }

    *context = pm_handle;
    return ESP_OK;
}

static int cores3_power_ctrl_deinit(void *context)
{
    i2c_master_dev_handle_t pm_handle = (i2c_master_dev_handle_t)context;
    if (pm_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = i2c_master_bus_rm_device(pm_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to remove AXP2101 device from I2C bus: %d", err);
    }
    return err;
}

/**
 * @brief  Set the power state for a CoreS3 consumer device.
 */
static int cores3_power_ctrl_set_power(void *context, const char *device_name, bool power_on)
{
    i2c_master_dev_handle_t pm_handle = (i2c_master_dev_handle_t)context;
    if (pm_handle == NULL || device_name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* The original board only ever powered rails up; powering off is a no-op. */
    if (!power_on) {
        return ESP_OK;
    }

    cores3_power_ctrl_feature_t feature;
    if (strcmp(device_name, "display_lcd") == 0) {
        feature = CORES3_POWER_CTRL_FEATURE_LCD;
    } else if (strcmp(device_name, "lcd_touch") == 0) {
        feature = CORES3_POWER_CTRL_FEATURE_TOUCH;
    } else if (strcmp(device_name, "audio_dac") == 0) {
        feature = CORES3_POWER_CTRL_FEATURE_SPEAKER;
    } else if (strcmp(device_name, "audio_adc") == 0) {
        feature = CORES3_POWER_CTRL_FEATURE_SPEAKER;
    } else if (strcmp(device_name, "fs_sdcard") == 0) {
        feature = CORES3_POWER_CTRL_FEATURE_SD;
    } else if (strcmp(device_name, "camera") == 0) {
        feature = CORES3_POWER_CTRL_FEATURE_CAMERA;
    } else {
        ESP_LOGW(TAG, "no power branch for device %s", device_name);
        return 0;
    }

    return cores3_power_ctrl_enable(pm_handle, feature);
}

static const dev_power_ctrl_custom_ops_t s_cores3_power_ctrl_ops = {
    .init      = cores3_power_ctrl_init,
    .deinit    = cores3_power_ctrl_deinit,
    .set_power = cores3_power_ctrl_set_power,
};

DEVICE_EXTRA_FUNC_REGISTER(axp2101_power_manager, &s_cores3_power_ctrl_ops);
