/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_check.h"
#include "esp_console.h"
#include "esp_log.h"
#include "bmgr_test_console.h"
#include "bmgr_test_registry.h"
#include "commands/cmd_bmgr.h"
#include "commands/cmd_case.h"

#define BMGR_TEST_CONSOLE_PROMPT  "bmgr-test>"

static const char *TAG = "BMGR_TEST_CONSOLE";

static esp_err_t bmgr_test_console_register_cases(void)
{
    bmgr_register_audio_cases();
    bmgr_register_button_cases();
    bmgr_register_camera_cases();
    bmgr_register_custom_cases();
    bmgr_register_fs_cases();
    bmgr_register_gpio_expander_cases();
    bmgr_register_lcd_cases();
    bmgr_register_led_cases();
    bmgr_register_periph_cases();
    return ESP_OK;
}

static esp_err_t bmgr_test_console_register_commands(bmgr_test_context_t *ctx)
{
    esp_err_t ret = register_bmgr_command(ctx);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register bmgr command: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = register_case_command(ctx);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register case command: %s", esp_err_to_name(ret));
        unregister_bmgr_command();
        return ret;
    }

    ret = bmgr_test_console_register_cases();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register test cases: %s", esp_err_to_name(ret));
        unregister_case_command();
        unregister_bmgr_command();
        return ret;
    }

    return ESP_OK;
}

esp_err_t bmgr_test_console_start(bmgr_test_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = BMGR_TEST_CONSOLE_PROMPT;

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_console_new_repl_uart(&hw_config, &repl_config, &repl), TAG,
                        "Failed to create UART console REPL");
#elif CONFIG_ESP_CONSOLE_USB_CDC
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl), TAG,
                        "Failed to create USB CDC console REPL");
#elif CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl), TAG,
                        "Failed to create USB serial JTAG console REPL");
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif  /* defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM) */

    ret = bmgr_test_console_register_commands(ctx);
    if (ret != ESP_OK) {
        goto cleanup;
    }

    ret = esp_console_start_repl(repl);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start console REPL: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    ctx->console_ready = true;
    return ESP_OK;

cleanup:
    unregister_case_command();
    unregister_bmgr_command();
    if (repl != NULL && repl->del != NULL) {
        repl->del(repl);
    }
    return ret;
}
