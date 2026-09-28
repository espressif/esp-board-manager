/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"
#include "esp_err.h"

extern void __real_app_main(void);

#include <stdio.h>
#include <stdatomic.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "soc/lp_system_reg.h"
#include "soc/soc.h"
#include "tinyusb.h"
#include "tinyusb_cdc_acm.h"
#include "tinyusb_console.h"
#include "tinyusb_default_config.h"

static const char *TAG = "bsp_usb_console";
static bool s_initialized;

#if CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD

/**
 * esptool and idf_monitor select the USB-Serial/JTAG reset strategy when they
 * see Espressif's USB-Serial/JTAG VID/PID. This device implements the matching
 * CDC reset protocol in software; it does not implement a JTAG interface.
 */
#define BSP_USB_SERIAL_JTAG_PID   0x1001
#define BSP_USB_RESTART_DELAY_US  (50 * 1000)

static const tusb_desc_device_t s_usb_device_descriptor = {
    .bLength            = sizeof(s_usb_device_descriptor),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = TINYUSB_ESPRESSIF_VID,
    .idProduct          = BSP_USB_SERIAL_JTAG_PID,
    .bcdDevice          = CONFIG_TINYUSB_DESC_BCD_DEVICE,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01,
};

typedef enum {
    BSP_USB_REBOOT_NONE,
    BSP_USB_REBOOT_NORMAL,
    BSP_USB_REBOOT_BOOTLOADER,
} bsp_usb_reboot_t;

static atomic_int s_reboot_mode = ATOMIC_VAR_INIT(BSP_USB_REBOOT_NONE);
static atomic_bool s_download_mode = ATOMIC_VAR_INIT(false);
static esp_timer_handle_t s_restart_timer;

static void usb_console_before_restart(void)
{
    const bsp_usb_reboot_t mode = atomic_load_explicit(&s_reboot_mode, memory_order_acquire);
    if (mode == BSP_USB_REBOOT_BOOTLOADER) {
        REG_SET_BIT(LP_SYSTEM_REG_SYS_CTRL_REG, LP_SYSTEM_REG_FORCE_DOWNLOAD_BOOT);
    } else if (mode == BSP_USB_REBOOT_NORMAL) {
        REG_CLR_BIT(LP_SYSTEM_REG_SYS_CTRL_REG, LP_SYSTEM_REG_FORCE_DOWNLOAD_BOOT);
    }
}

static void restart_timer_callback(void *arg)
{
    (void)arg;
    esp_restart();
}

static void cdc_line_state_changed_callback(int itf, cdcacm_event_t *event)
{
    (void)itf;
    const bool dtr = event->line_state_changed_data.dtr;
    const bool rts = event->line_state_changed_data.rts;

    /**
     * Match the USB-Serial/JTAG CDC state table exactly:
     *
     *   RTS DTR  action
     *    0   0   clear download-mode latch
     *    0   1   set download-mode latch
     *    1   0   reset using the latched mode
     *    1   1   no action
     *
     * esptool's USBJTAGSerialReset generates these effective phases:
     *
     *   (0,0) --100 ms-- (0,1) --100 ms-- (1,1) -> (1,0)
     *           --100 ms-- (0,0)
     *
     * Its RTS helper also re-writes the current DTR state for Windows
     * usbser.sys. Repeated states are therefore intentional. The reboot-mode
     * compare/exchange below accepts only the first (1,0) event.
     */
    if (!rts) {
        atomic_store_explicit(&s_download_mode, dtr, memory_order_release);
        return;
    }
    if (dtr) {
        return;
    }

    const bsp_usb_reboot_t requested_mode =
        atomic_load_explicit(&s_download_mode, memory_order_acquire) ? BSP_USB_REBOOT_BOOTLOADER : BSP_USB_REBOOT_NORMAL;
    int expected_mode = BSP_USB_REBOOT_NONE;
    if (!atomic_compare_exchange_strong_explicit(&s_reboot_mode, &expected_mode, requested_mode,
                                                 memory_order_acq_rel, memory_order_acquire)) {
        return;
    }

    /* Delay restart long enough for SET_CONTROL_LINE_STATE to complete. */
    if (esp_timer_start_once(s_restart_timer, BSP_USB_RESTART_DELAY_US) != ESP_OK) {
        atomic_store_explicit(&s_reboot_mode, BSP_USB_REBOOT_NONE, memory_order_release);
    }
}

static esp_err_t auto_download_init(void)
{
    const esp_timer_create_args_t timer_args = {
        .callback = restart_timer_callback,
        .name = "usb_reboot",
    };
    esp_err_t ret = esp_timer_create(&timer_args, &s_restart_timer);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = esp_register_shutdown_handler(usb_console_before_restart);
    if (ret != ESP_OK) {
        esp_timer_delete(s_restart_timer);
        s_restart_timer = NULL;
    }
    return ret;
}

static void auto_download_deinit(void)
{
    (void)esp_unregister_shutdown_handler(usb_console_before_restart);
    if (s_restart_timer) {
        (void)esp_timer_delete(s_restart_timer);
        s_restart_timer = NULL;
    }
    atomic_store_explicit(&s_reboot_mode, BSP_USB_REBOOT_NONE, memory_order_release);
    atomic_store_explicit(&s_download_mode, false, memory_order_release);
}

#endif  /* CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD */

esp_err_t bsp_usb_console_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }
#if CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD
    esp_err_t ret = auto_download_init();
    if (ret != ESP_OK) {
        return ret;
    }
    tinyusb_config_t tusb_config = TINYUSB_DEFAULT_CONFIG();
    tusb_config.descriptor.device = &s_usb_device_descriptor;
#else
    esp_err_t ret;
    tinyusb_config_t tusb_config = TINYUSB_DEFAULT_CONFIG();
#endif  /* CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD */

    ret = tinyusb_driver_install(&tusb_config);
    if (ret != ESP_OK) {
#if CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD
        auto_download_deinit();
#endif  /* CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD */
        return ret;
    }

    const tinyusb_config_cdcacm_t cdc_config = {
        .cdc_port = TINYUSB_CDC_ACM_0,
#if CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD
        .callback_line_state_changed = cdc_line_state_changed_callback,
#endif  /* CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD */
    };
    ret = tinyusb_cdcacm_init(&cdc_config);
    if (ret != ESP_OK) {
        (void)tinyusb_driver_uninstall();
#if CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD
        auto_download_deinit();
#endif  /* CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD */
        return ret;
    }

    ret = tinyusb_console_init(TINYUSB_CDC_ACM_0);
    if (ret != ESP_OK) {
        (void)tinyusb_cdcacm_deinit(TINYUSB_CDC_ACM_0);
        (void)tinyusb_driver_uninstall();
#if CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD
        auto_download_deinit();
#endif  /* CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD */
        return ret;
    }

    /* Keep application logs observable even when the host opens CDC after app_main starts. */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    s_initialized = true;
    ESP_LOGI(TAG, "USB-OTG CDC console ready%s",
#if CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD
             "; USB-Serial/JTAG-compatible reset enabled"
#else
             ""
#endif  /* CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD */
    );
    return ESP_OK;
}

bool bsp_usb_console_is_initialized(void)
{
    return s_initialized;
}

#if CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_INIT

/**
 * IDF calls app_main from its FreeRTOS main task. Linker wrapping puts TinyUSB
 * initialization at the exact point where the scheduler is available but the
 * application has not started yet.
 */
void __wrap_app_main(void)
{
    esp_rom_printf("Initializing USB-OTG CDC console...\n");
    ESP_ERROR_CHECK(bsp_usb_console_init());
    __real_app_main();
}

#endif  /* CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_INIT */
