/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"
#include "esp_err.h"

extern void __real_app_main(void);

#include <stdarg.h>
#include <stdio.h>
#include <stdatomic.h>

#include "esp_log.h"
#include "esp_log_write.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "hal/usb_utmi_ll.h"
#include "soc/lp_system_reg.h"
#include "soc/soc.h"
#include "device/dcd.h"
#include "tinyusb.h"
#include "tinyusb_cdc_acm.h"
#include "tinyusb_console.h"
#include "tinyusb_default_config.h"

static const char *TAG = "bsp_usb_console";
static bool s_initialized;
static uint8_t s_rhport;

/**
 * The S31 CPU reset leaves the USB OTG HS DMA running. Stop it before the next
 * image reuses RAM, without waiting for or freeing the running USB task.
 */
static void usb_console_shutdown(void)
{
    dcd_int_disable(s_rhport);
    usb_utmi_ll_reset_register();
    usb_utmi_ll_enable_bus_clock(false);
}

static void usb_console_driver_uninstall(void)
{
    (void)esp_unregister_shutdown_handler(usb_console_shutdown);
    (void)tinyusb_driver_uninstall();
}

#if CONFIG_USB_HS_CONSOLE_USB_CDC_MIRROR_LOG_TO_CONSOLE

static FILE *s_console_log_stream;

/**
 * tinyusb_console_init() moves stdout to the CDC port. /dev/console still
 * reaches the console selected by CONFIG_ESP_CONSOLE_* (and the secondary one),
 * so ESP_LOG output is written to both.
 */
static int log_mirror_vprintf(const char *format, va_list args)
{
    va_list console_args;
    va_copy(console_args, args);
    const int ret = vprintf(format, args);
    (void)vfprintf(s_console_log_stream, format, console_args);
    va_end(console_args);
    return ret;
}

static esp_err_t log_mirror_init(void)
{
    s_console_log_stream = fopen("/dev/console", "w");
    if (!s_console_log_stream) {
        return ESP_FAIL;
    }
    setvbuf(s_console_log_stream, NULL, _IONBF, 0);
    (void)esp_log_set_vprintf(log_mirror_vprintf);
    return ESP_OK;
}

#endif  /* CONFIG_USB_HS_CONSOLE_USB_CDC_MIRROR_LOG_TO_CONSOLE */

/**
 * The host identifies a serial port by VID, PID and serial number.
 * esp_tinyusb's default serial is a fixed Kconfig string (version < 2.4.0) or
 * MAC address without colons (version >= 2.4.0). The bootloader's CDC device has
 * serial number in format `xx:xx:xx:xx:xx:xx`.
 *
 * If the serial number is formatted without colons, macOS assigns a unique port
 * per device (/dev/cu.usbmodem-xxxxxxxxxxxx) - which is different from the path
 * of bootloader's CDC device. This causes `idf.py flash monitor` to fail due to
 * the mismatch in the port path for flashing and monitoring.
 *
 * If the serial number is formatted with colons, macOS assigns port based on the
 * location of the device, similar behaviour as the bootloader's CDC device.
 */

static char s_usb_serial[18];

/**
 * Copy of esp_tinyusb's default string table (descriptor_str_default in
 * usb_descriptors.c, esp_tinyusb v2.0.1 through v3.0.0) with only the serial
 * replaced. The Kconfig-built configuration descriptor indexes into it.
 */
static const char *s_usb_strings[] = {
    (const char[]){0x09, 0x04},               // 0: English (0x0409)
    CONFIG_TINYUSB_DESC_MANUFACTURER_STRING,  // 1: Manufacturer
    CONFIG_TINYUSB_DESC_PRODUCT_STRING,       // 2: Product
    s_usb_serial,                             // 3: Serial number
#if CONFIG_TINYUSB_CDC_ENABLED
    CONFIG_TINYUSB_DESC_CDC_STRING,           // 4: CDC interface
#endif  /* CONFIG_TINYUSB_CDC_ENABLED */
#if CONFIG_TINYUSB_MSC_ENABLED
    CONFIG_TINYUSB_DESC_MSC_STRING,           // MSC interface
#endif  /* CONFIG_TINYUSB_MSC_ENABLED */
#if CONFIG_TINYUSB_MTP_ENABLED
    CONFIG_TINYUSB_DESC_MTP_STRING,           // MTP interface
#endif  /* CONFIG_TINYUSB_MTP_ENABLED */
#if CONFIG_TINYUSB_NET_MODE_ECM_RNDIS || CONFIG_TINYUSB_NET_MODE_NCM
    "USB net",                                // NET interface
    "",                                       // MAC
#endif  /* CONFIG_TINYUSB_NET_MODE_ECM_RNDIS || CONFIG_TINYUSB_NET_MODE_NCM */
#if CFG_TUD_VENDOR
    "Vendor specific",                        // Vendor interface
#endif  /* CFG_TUD_VENDOR */
};

static void usb_serial_from_mac(void)
{
    uint8_t mac[6] = {0};
    if (esp_read_mac(mac, ESP_MAC_BASE) != ESP_OK) {
        ESP_LOGW(TAG, "Base MAC unavailable, USB serial number is all zeros");
    }
    snprintf(s_usb_serial, sizeof(s_usb_serial), MACSTR, MAC2STR(mac));
}

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
    usb_serial_from_mac();
    tusb_config.descriptor.string = s_usb_strings;
    tusb_config.descriptor.string_count = sizeof(s_usb_strings) / sizeof(s_usb_strings[0]);

    ret = tinyusb_driver_install(&tusb_config);
    if (ret != ESP_OK) {
#if CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD
        auto_download_deinit();
#endif  /* CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD */
        return ret;
    }

    s_rhport = tusb_config.port;
    ret = esp_register_shutdown_handler(usb_console_shutdown);
    if (ret != ESP_OK) {
        (void)tinyusb_driver_uninstall();
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
        usb_console_driver_uninstall();
#if CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD
        auto_download_deinit();
#endif  /* CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD */
        return ret;
    }

    ret = tinyusb_console_init(TINYUSB_CDC_ACM_0);
    if (ret != ESP_OK) {
        (void)tinyusb_cdcacm_deinit(TINYUSB_CDC_ACM_0);
        usb_console_driver_uninstall();
#if CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD
        auto_download_deinit();
#endif  /* CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD */
        return ret;
    }

    /* Keep application logs observable even when the host opens CDC after app_main starts. */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

#if CONFIG_USB_HS_CONSOLE_USB_CDC_MIRROR_LOG_TO_CONSOLE
    if (log_mirror_init() != ESP_OK) {
        ESP_LOGW(TAG, "Cannot open /dev/console; ESP_LOG output goes to USB only");
    }
#endif  /* CONFIG_USB_HS_CONSOLE_USB_CDC_MIRROR_LOG_TO_CONSOLE */

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
