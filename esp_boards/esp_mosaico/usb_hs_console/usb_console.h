/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Start the USB-OTG CDC console.
 *
 *         CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_INIT enables this automatically before
 *         app_main(). Call it manually only when automatic initialization is disabled.
 *         On success, stdin, stdout, stderr and subsequent ESP_LOG output use TinyUSB
 *         CDC-ACM interface 0. Repeated calls are harmless.
 *
 *         When CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_DOWNLOAD is enabled, the same interface also
 *         emulates the USB-Serial/JTAG DTR/RTS reset behavior and uses its VID/PID, so
 *         idf.py can reset into ROM download mode without extra esptool arguments.
 */
esp_err_t bsp_usb_console_init(void);

/** @return true after the USB CDC console has initialized successfully. */
bool bsp_usb_console_is_initialized(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
