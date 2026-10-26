/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "sdkconfig.h"
#if CONFIG_ESP_BOARD_DEV_CUSTOM_SUPPORT && __has_include("gen_board_device_custom.h") && __has_include("bq27220.h")
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "bq27220.h"
#include "dev_custom.h"
#include "esp_board_periph.h"
#include "gen_board_device_custom.h"
#include "i2c_bus.h"
#include "periph_i2c.h"
#include "battery_monitor.h"

#if CONFIG_I2C_BUS_BACKWARD_CONFIG
#error "Mosaico battery_monitor requires the native I2C backend of i2c_bus"
#endif  /* CONFIG_I2C_BUS_BACKWARD_CONFIG */

#define BATTERY_I2C_SPEED_HZ         (400000U)
#define BATTERY_I2C_PERIPHERAL_NAME  "i2c_master"

typedef struct {
    bq27220_handle_t  handle;
    bq27220_handle_t  gauge;
    const char       *peripheral_name;
} battery_monitor_context_t;

static const char *TAG = "ESP_MOSAICO_BATTERY";
static battery_monitor_context_t s_battery;
/* Keep the wrapper while BMGR's shared bus is alive. The board I2C deinit hook
 * releases both together after the final peripheral reference is released. */
static i2c_bus_handle_t s_i2c_bridge[I2C_NUM_MAX];

/* Interim BSP profile, characterized on 80 mAh cells at 25 C. The stock cell's
 * 65 mAh capacity follows esp-mosaico-claw; its EDV/DOD values still require
 * characterization for that cell. Changing capacity alone is not calibration.
 * Applications using a different cell should replace the complete profile. */
static const parameter_cedv_t s_cedv = {
    .reserve_cap         = 0,
    .near_full           = 5,
    .self_discharge_rate = 20,
    .EDV0                = 3000,
    .EDV1                = 3410,
    .EDV2                = 3530,
    .EMF                 = 3670,
    .C0                  = 115,
    .R0                  = 968,
    .T0                  = 4547,
    .R1                  = 4764,
    .TC                  = 11,
    .C1                  = 0,
    .DOD0                = 4147,
    .DOD10               = 4002,
    .DOD20               = 3969,
    .DOD30               = 3938,
    .DOD40               = 3880,
    .DOD50               = 3824,
    .DOD60               = 3794,
    .DOD70               = 3753,
    .DOD80               = 3677,
    .DOD90               = 3574,
    .DOD100              = 3490,
};

static const gauging_config_t s_gauging = {
    .CCT        = true,
    .SC         = true,
    .FCC_LIM    = true,
    .FC_FOR_VDQ = true,
    .IGNORE_SD  = true,
};

static int battery_monitor_i2c_deinit(void *periph_handle)
{
    for (unsigned int port = 0; port < I2C_NUM_MAX; ++port) {
        if (s_i2c_bridge[port] != NULL &&
            i2c_bus_get_internal_bus_handle(s_i2c_bridge[port]) == periph_handle) {
            /* i2c_bus_delete() is safe only here, when BMGR has no other bus
             * users. Refuse teardown while a wrapper endpoint is still alive. */
            if (i2c_bus_get_created_device_num(s_i2c_bridge[port]) != 0) {
                return ESP_ERR_INVALID_STATE;
            }
            return i2c_bus_delete(&s_i2c_bridge[port]);
        }
    }
    return periph_i2c_deinit(periph_handle);
}

static esp_err_t battery_monitor_cleanup(void)
{
    if (s_battery.gauge != NULL) {
        esp_err_t ret = bq27220_delete(s_battery.gauge);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete BQ27220: %s", esp_err_to_name(ret));
            return ret;
        }
        s_battery.gauge = NULL;
    }
    if (s_battery.peripheral_name != NULL) {
        esp_err_t ret = esp_board_periph_unref_handle(s_battery.peripheral_name);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to release I2C peripheral %s: %s",
                     s_battery.peripheral_name, esp_err_to_name(ret));
            return ret;
        }
        s_battery.peripheral_name = NULL;
    }
    s_battery.handle = NULL;
    return ESP_OK;
}

static int battery_monitor_init(void *config, int config_size, void **device_handle)
{
    if (config == NULL || device_handle == NULL ||
        config_size != sizeof(dev_custom_battery_monitor_config_t)) {
        return ESP_ERR_INVALID_ARG;
    }
    *device_handle = NULL;
    const dev_custom_battery_monitor_config_t *cfg = config;
    const int32_t capacity = cfg->design_capacity;
    if (cfg->chip == NULL || strcmp(cfg->chip, "bq27220") != 0 ||
        cfg->peripheral_count != 1 || cfg->peripheral_name == NULL ||
        strcmp(cfg->peripheral_name, BATTERY_I2C_PERIPHERAL_NAME) != 0 ||
        capacity <= 0 || capacity > UINT16_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_battery.handle != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    /* Retry an unref that failed while rolling back an earlier init attempt. */
    esp_err_t ret = battery_monitor_cleanup();
    if (ret != ESP_OK) {
        return ret;
    }

    i2c_master_bus_handle_t bus = NULL;
    ret = esp_board_periph_ref_handle(cfg->peripheral_name, (void **)&bus);
    if (ret != ESP_OK) {
        return ret;
    }
    s_battery.peripheral_name = cfg->peripheral_name;

    i2c_master_bus_config_t *i2c_cfg = NULL;
    ret = esp_board_periph_get_config(cfg->peripheral_name, (void **)&i2c_cfg);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    if (bus == NULL || i2c_cfg == NULL || i2c_cfg->i2c_port < 0 || i2c_cfg->i2c_port >= I2C_NUM_MAX) {
        ret = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }

    i2c_bus_handle_t *bridge = &s_i2c_bridge[i2c_cfg->i2c_port];
    if (*bridge == NULL || i2c_bus_get_internal_bus_handle(*bridge) != bus) {
        /* Read the live configuration after board_runtime.c selects v1.0/v1.2. */
        const i2c_config_t bridge_cfg = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = i2c_cfg->sda_io_num,
            .scl_io_num = i2c_cfg->scl_io_num,
            .sda_pullup_en = i2c_cfg->flags.enable_internal_pullup,
            .scl_pullup_en = i2c_cfg->flags.enable_internal_pullup,
            .master.clk_speed = BATTERY_I2C_SPEED_HZ,
        };
        *bridge = i2c_bus_create(i2c_cfg->i2c_port, &bridge_cfg);
        if (*bridge == NULL) {
            ret = ESP_FAIL;
            goto cleanup;
        }
    }

    parameter_cedv_t cedv = s_cedv;
    cedv.full_charge_cap = capacity;
    cedv.design_cap = capacity;
    const bq27220_config_t gauge_cfg = {
        .i2c_bus = *bridge,
        .cfg = &s_gauging,
        .cedv = &cedv,
    };
    s_battery.gauge = bq27220_create(&gauge_cfg);
    if (s_battery.gauge == NULL) {
        ESP_LOGE(TAG, "Failed to initialize BQ27220");
        ret = ESP_FAIL;
        goto cleanup;
    }
    /* Driver >=0.1.3 verifies the profile and seals the gauge before returning. */
    s_battery.handle = s_battery.gauge;
    *device_handle = s_battery.handle;
    return ESP_OK;

cleanup:
    /* The board I2C deinit hook owns bridge cleanup on the final bus unref. */
    if (battery_monitor_cleanup() != ESP_OK) {
        ESP_LOGW(TAG, "I2C release will be retried on the next battery init");
    }
    return ret;
}

static int battery_monitor_deinit(void *device_handle)
{
    if (device_handle == NULL || device_handle != s_battery.handle) {
        return ESP_ERR_INVALID_ARG;
    }
    /* Keep the original opaque handle until unref succeeds, so a teardown retry
     * does not delete the gauge twice after a peripheral release failure. */
    return battery_monitor_cleanup();
}

esp_err_t mosaico_battery_monitor_setup(void)
{
    return esp_board_periph_set_ops(BATTERY_I2C_PERIPHERAL_NAME, periph_i2c_init, battery_monitor_i2c_deinit);
}

CUSTOM_DEVICE_IMPLEMENT(battery_monitor, battery_monitor_init, battery_monitor_deinit);
#endif  /* CONFIG_ESP_BOARD_DEV_CUSTOM_SUPPORT && __has_include("gen_board_device_custom.h") && __has_include("bq27220.h") */
