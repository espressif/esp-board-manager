/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "dev_power_ctrl.h"
#include "driver/i2c_master.h"
#include "esp_board_periph.h"
#include "esp_err.h"
#include "esp_log.h"
#include "gen_board_device_custom.h"

static const char *TAG = "M5STACK_CORE_POWER_CTRL";

static int m5stack_core_power_ctrl_init(const dev_power_ctrl_config_t *config, void **context)
{
    const dev_power_ctrl_custom_sub_config_t *custom_cfg =
        &config->sub_cfg.custom;
    const dev_custom_ip5306_power_manager_custom_config_t *power_cfg =
        custom_cfg->user_cfg;
    if (power_cfg == NULL || custom_cfg->periph_count != 1 ||
        custom_cfg->periph_names == NULL) {
        ESP_LOGE(TAG, "Invalid IP5306 power controller configuration");
        return ESP_ERR_INVALID_ARG;
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
    *context = handle;
    return ESP_OK;
}

static int m5stack_core_power_ctrl_deinit(void *context)
{
    if (context == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_bus_rm_device((i2c_master_dev_handle_t)context);
}

static int m5stack_core_power_ctrl_set_power(void *context, const char *device_name, bool power_on)
{
    if (context == NULL || device_name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    /* IP5306 rails are enabled by the board's default power policy. */
    (void)power_on;
    return ESP_OK;
}

static const dev_power_ctrl_custom_ops_t s_m5stack_core_power_ctrl_ops = {
    .init      = m5stack_core_power_ctrl_init,
    .deinit    = m5stack_core_power_ctrl_deinit,
    .set_power = m5stack_core_power_ctrl_set_power,
};

DEVICE_EXTRA_FUNC_REGISTER(ip5306_power_manager,
                           &s_m5stack_core_power_ctrl_ops);
