/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/**
 * One firmware for ESP-Mosaico v1.0 and v1.2.
 *
 * Generated YAML is the v1.0 pin map. This hook runs from
 * esp_board_runtime_pre_init(), applies revision descriptor changes and installs
 * BOARD-layer camera ops.
 * Driver handles are created later, when a peripheral is initialized.
 *
 * Pin map follows esp-mosaico-bsp (v1.1 uses the v1.2 pins):
 *   v1.0 main I2C0  SDA GPIO0  SCL GPIO1; codec power GPIO56; status LED GPIO3
 *   v1.2 main I2C0  SDA GPIO56 SCL GPIO3; expansion I2C1 SDA GPIO0 SCL GPIO1
 *   LCD SCLK/reset swap: v1.0 44/42, v1.2 42/44
 * The board revision applies the complete v1.2 table before any drivers start.
 */

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#if SOC_USB_SERIAL_JTAG_SUPPORTED
#include "hal/usb_serial_jtag_ll.h"
#endif  /* SOC_USB_SERIAL_JTAG_SUPPORTED */
#if defined(CONFIG_ESP_BOARD_DEV_CAMERA_SUB_DVP_SUPPORT) && CONFIG_ESP_BOARD_DEV_CAMERA_SUB_DVP_SUPPORT
#include "dev_camera.h"
#define MOSAICO_HAS_CAMERA  1
#else
#define MOSAICO_HAS_CAMERA  0
#endif  /* defined(CONFIG_ESP_BOARD_DEV_CAMERA_SUB_DVP_SUPPORT) && CONFIG_ESP_BOARD_DEV_CAMERA_SUB_DVP_SUPPORT */
#if defined(CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUB_CUSTOM_SUPPORT) && CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUB_CUSTOM_SUPPORT
#include "dev_power_ctrl.h"
#endif  /* defined(CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUB_CUSTOM_SUPPORT) && CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUB_CUSTOM_SUPPORT */
#include "esp_board_device.h"
#include "esp_board_manager.h"
#include "gen_board_revisions.h"

extern ESP_BOARD_TABLE_CONST esp_board_device_desc_t g_esp_board_devices[];

#define MOSAICO_HW_VERSION(major, minor)  \
    ((uint16_t)(((uint16_t)(major) << 8) | ((uint16_t)(minor) & 0xFFU)))
#define MOSAICO_VCC_GPIO            GPIO_NUM_60
#define MOSAICO_VCC_RAMP_HZ         100000
#define MOSAICO_VCC_RAMP_MS         50
#define MOSAICO_XCLK_SETTLE_MS      20
/* Camera D2 is GPIO33, which is also the USB Serial/JTAG D- pad. Set to 0 to
 * leave USB Serial/JTAG attached for A/B testing. */
#define MOSAICO_CAMERA_RELEASE_USJ  1
#define MOSAICO_CAMERA_D2_GPIO      GPIO_NUM_33
#define MOSAICO_CAMERA_FLASH_GPIO   GPIO_NUM_34
#define MOSAICO_CAMERA_FLASH_OFF    1

#if MOSAICO_HAS_CAMERA
int dev_camera_sub_dvp_init(void *cfg, int cfg_size, void **device_handle);
int dev_camera_sub_dvp_deinit(void *device_handle);
#endif  /* MOSAICO_HAS_CAMERA */

static const char *TAG = "ESP_MOSAICO";

typedef enum {
    MOSAICO_REV_V1_0 = 0,
    MOSAICO_REV_V1_2,
} mosaico_rev_t;

#if defined(CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUB_CUSTOM_SUPPORT) && CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUB_CUSTOM_SUPPORT
static bool s_vcc_on;
#endif  /* defined(CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUB_CUSTOM_SUPPORT) && CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUB_CUSTOM_SUPPORT */
#if MOSAICO_HAS_CAMERA && MOSAICO_CAMERA_RELEASE_USJ && SOC_USB_SERIAL_JTAG_SUPPORTED
static bool s_usj_released;
static bool s_usj_pad_was_enabled;
static bool s_usj_clock_was_enabled;
static uint32_t s_usj_intr_mask;
#endif  /* MOSAICO_HAS_CAMERA && MOSAICO_CAMERA_RELEASE_USJ && SOC_USB_SERIAL_JTAG_SUPPORTED */

static esp_err_t read_revision(mosaico_rev_t *rev)
{
    uint16_t version = 0;
    esp_err_t ret = esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA, &version, sizeof(version) * 8U);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "read hardware version from eFuse failed: %s", esp_err_to_name(ret));
        return ret;
    }

    switch (version) {
        case MOSAICO_HW_VERSION(1, 0):
            *rev = MOSAICO_REV_V1_0;
            break;
        case MOSAICO_HW_VERSION(1, 1):
        case MOSAICO_HW_VERSION(1, 2):
            *rev = MOSAICO_REV_V1_2;
            break;
        default:
            ESP_LOGE(TAG, "unsupported hardware version: v%u.%u (raw=0x%04X)",
                     (unsigned)(version >> 8), (unsigned)(version & 0xFFU), (unsigned)version);
            return ESP_ERR_NOT_SUPPORTED;
    }

    ESP_LOGI(TAG, "hardware version: v%u.%u", (unsigned)(version >> 8), (unsigned)(version & 0xFFU));
    return ESP_OK;
}

#if defined(CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUB_CUSTOM_SUPPORT) && CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUB_CUSTOM_SUPPORT
static void mosaico_record_cleanup_error(esp_err_t *ret, esp_err_t cleanup_ret)
{
    if (*ret == ESP_OK && cleanup_ret != ESP_OK) {
        *ret = cleanup_ret;
    }
}

static esp_err_t mosaico_ramp_vcc(gpio_num_t gpio_num)
{
    if (s_vcc_on) {
        return ESP_OK;
    }

    const ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = MOSAICO_VCC_RAMP_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&timer_config);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = gpio_reset_pin(gpio_num);
    if (ret != ESP_OK) {
        const ledc_timer_config_t timer_deconfig = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .timer_num = LEDC_TIMER_1,
            .deconfigure = true,
        };
        mosaico_record_cleanup_error(&ret, ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1));
        mosaico_record_cleanup_error(&ret, ledc_timer_config(&timer_deconfig));
        return ret;
    }

    const ledc_channel_config_t channel_config = {
        .gpio_num = gpio_num,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_1,
        .duty = 255,
        .hpoint = 0,
    };
    bool channel_configured = false;
    ret = ledc_channel_config(&channel_config);
    bool fade_installed_here = false;
    if (ret == ESP_OK) {
        channel_configured = true;
        ret = ledc_fade_func_install(0);
        if (ret == ESP_OK) {
            fade_installed_here = true;
        } else if (ret == ESP_ERR_INVALID_STATE) {
            ret = ESP_OK;
        }
    }
    if (ret == ESP_OK) {
        ret = ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0, MOSAICO_VCC_RAMP_MS);
    }
    if (ret == ESP_OK) {
        ret = ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, LEDC_FADE_WAIT_DONE);
    }

    if (channel_configured) {
        mosaico_record_cleanup_error(&ret, ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, ret == ESP_OK ? 0 : 1));
        const ledc_channel_config_t channel_deconfig = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LEDC_CHANNEL_1,
            .deconfigure = true,
        };
        mosaico_record_cleanup_error(&ret, ledc_channel_config(&channel_deconfig));
    }
    if (fade_installed_here) {
        ledc_fade_func_uninstall();
    }
    mosaico_record_cleanup_error(&ret, ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1));
    const ledc_timer_config_t timer_deconfig = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .deconfigure = true,
    };
    mosaico_record_cleanup_error(&ret, ledc_timer_config(&timer_deconfig));

    const gpio_config_t output_config = {
        .pin_bit_mask = 1ULL << gpio_num,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    /* Load the output latch before enabling GPIO output after LEDC detaches. */
    esp_err_t output_ret = gpio_set_level(gpio_num, ret == ESP_OK ? 0 : 1);
    if (output_ret == ESP_OK) {
        output_ret = gpio_config(&output_config);
    }
    if (ret == ESP_OK && output_ret == ESP_OK) {
        s_vcc_on = true;
        ESP_LOGI(TAG, "VCC GPIO%d ramped on over %d ms", (int)gpio_num, MOSAICO_VCC_RAMP_MS);
    }
    return ret != ESP_OK ? ret : output_ret;
}

static int mosaico_lcd_power_custom_init(const dev_power_ctrl_config_t *config, void **context)
{
    esp_err_t ret = mosaico_ramp_vcc(MOSAICO_VCC_GPIO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "VCC ramp failed: %s", esp_err_to_name(ret));
        return -1;
    }
    (void)config;
    *context = NULL;
    return 0;
}

static int mosaico_lcd_power_custom_deinit(void *context)
{
    (void)context;
    esp_err_t ret = gpio_set_level(MOSAICO_VCC_GPIO, 1);
    if (ret != ESP_OK) {
        return -1;
    }
    s_vcc_on = false;
    return 0;
}

static int mosaico_lcd_power_custom_set_power(void *context, const char *device_name, bool power_on)
{
    (void)context;
    (void)device_name;
    if (power_on) {
        return mosaico_ramp_vcc(MOSAICO_VCC_GPIO) == ESP_OK ? 0 : -1;
    }
    esp_err_t ret = gpio_set_level(MOSAICO_VCC_GPIO, 1);
    if (ret == ESP_OK) {
        s_vcc_on = false;
    }
    return ret == ESP_OK ? 0 : -1;
}

static const dev_power_ctrl_custom_ops_t s_mosaico_lcd_power_ops = {
    .init      = mosaico_lcd_power_custom_init,
    .deinit    = mosaico_lcd_power_custom_deinit,
    .set_power = mosaico_lcd_power_custom_set_power,
};

DEVICE_EXTRA_FUNC_REGISTER(lcd_power_ctrl, &s_mosaico_lcd_power_ops);
#endif  /* defined(CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUB_CUSTOM_SUPPORT) && CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUB_CUSTOM_SUPPORT */

#if MOSAICO_HAS_CAMERA
static void mosaico_camera_release_usj(void)
{
#if MOSAICO_CAMERA_RELEASE_USJ && SOC_USB_SERIAL_JTAG_SUPPORTED
    if (s_usj_released) {
        return;
    }
    s_usj_pad_was_enabled = usb_serial_jtag_ll_phy_is_pad_enabled();
    s_usj_clock_was_enabled = usb_serial_jtag_ll_module_is_enabled();
    s_usj_intr_mask = s_usj_clock_was_enabled ? usb_serial_jtag_ll_get_intr_ena_status() : 0;
    if (s_usj_clock_was_enabled) {
        usb_serial_jtag_ll_disable_intr_mask(USB_SERIAL_JTAG_LL_INTR_MASK);
    }
    usb_serial_jtag_ll_phy_enable_pad(false);
    usb_serial_jtag_ll_enable_bus_clock(false);
    gpio_reset_pin(MOSAICO_CAMERA_D2_GPIO);
    s_usj_released = true;
    ESP_LOGI(TAG, "USB Serial/JTAG released for camera D2 GPIO%d (pad=%d clk=%d intr=0x%" PRIx32 ")",
             (int)MOSAICO_CAMERA_D2_GPIO, s_usj_pad_was_enabled, s_usj_clock_was_enabled, s_usj_intr_mask);
#else
    ESP_LOGW(TAG, "USB Serial/JTAG left attached to camera D2 GPIO%d", (int)MOSAICO_CAMERA_D2_GPIO);
#endif  /* MOSAICO_CAMERA_RELEASE_USJ && SOC_USB_SERIAL_JTAG_SUPPORTED */
}

static void mosaico_camera_restore_usj(void)
{
#if MOSAICO_CAMERA_RELEASE_USJ && SOC_USB_SERIAL_JTAG_SUPPORTED
    if (!s_usj_released) {
        return;
    }
    if (s_usj_clock_was_enabled) {
        usb_serial_jtag_ll_enable_bus_clock(true);
    }
    usb_serial_jtag_ll_phy_enable_pad(s_usj_pad_was_enabled);
    if (s_usj_clock_was_enabled && s_usj_intr_mask != 0) {
        usb_serial_jtag_ll_ena_intr_mask(s_usj_intr_mask);
    }
    s_usj_released = false;
    ESP_LOGI(TAG, "USB Serial/JTAG restored");
#endif  /* MOSAICO_CAMERA_RELEASE_USJ && SOC_USB_SERIAL_JTAG_SUPPORTED */
}

static esp_err_t mosaico_camera_flash_off(void)
{
    const gpio_config_t flash_config = {
        .pin_bit_mask = 1ULL << MOSAICO_CAMERA_FLASH_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_set_level(MOSAICO_CAMERA_FLASH_GPIO, MOSAICO_CAMERA_FLASH_OFF);
    if (ret == ESP_OK) {
        ret = gpio_config(&flash_config);
    }
    return ret;
}

static int mosaico_camera_init(void *cfg, int cfg_size, void **device_handle)
{
    mosaico_camera_release_usj();
    esp_err_t ret = mosaico_camera_flash_off();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "force camera flash GPIO%d off failed: %s", (int)MOSAICO_CAMERA_FLASH_GPIO, esp_err_to_name(ret));
        mosaico_camera_restore_usj();
        return -1;
    }

    const dev_camera_config_t *config = (const dev_camera_config_t *)cfg;
    if (config != NULL && config->sub_cfg.dvp.dvp_io.xclk_io < 0) {
        ESP_LOGI(TAG, "wait %d ms for camera module clock before SCCB", MOSAICO_XCLK_SETTLE_MS);
        vTaskDelay(pdMS_TO_TICKS(MOSAICO_XCLK_SETTLE_MS));
    }
    int init_ret = dev_camera_sub_dvp_init(cfg, cfg_size, device_handle);
    if (init_ret != 0) {
        mosaico_camera_restore_usj();
    }
    return init_ret;
}

static int mosaico_camera_deinit(void *device_handle)
{
    int ret = dev_camera_sub_dvp_deinit(device_handle);
    if (ret == 0) {
        mosaico_camera_restore_usj();
    }
    return ret;
}
#endif  /* MOSAICO_HAS_CAMERA */

esp_err_t esp_board_runtime_pre_init(void)
{
    mosaico_rev_t rev = MOSAICO_REV_V1_0;
    esp_err_t ret = read_revision(&rev);
    if (ret != ESP_OK) {
        return ret;
    }
    if (rev == MOSAICO_REV_V1_2) {
        ret = gen_board_revision_apply_v1_2();
        if (ret != ESP_OK) {
            return ret;
        }
    }

#if MOSAICO_HAS_CAMERA
    const esp_board_device_desc_t *camera = g_esp_board_devices;
    while (camera && camera->name && strcmp(camera->name, "camera") != 0) {
        camera = camera->next;
    }
    if (camera && camera->name && camera->type && camera->sub_type &&
        strcmp(camera->type, "camera") == 0 && strcmp(camera->sub_type, "dvp") == 0) {
        ret = esp_board_device_set_ops("camera", mosaico_camera_init, mosaico_camera_deinit);
        if (ret != ESP_OK) {
            return ret;
        }
    }
#endif  /* MOSAICO_HAS_CAMERA */

    return ESP_OK;
}
