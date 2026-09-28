/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#if CONFIG_ESP_CONSOLE_NONE
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif  /* CONFIG_ESP_CONSOLE_NONE */
#include "esp_check.h"
#include "esp_console.h"
#include "esp_log.h"
#include "bmgr_test_console.h"
#include "bmgr_test_registry.h"
#include "commands/cmd_bmgr.h"
#include "commands/cmd_case.h"

#define BMGR_TEST_CONSOLE_PROMPT          "bmgr-test>"
#define BMGR_TEST_CONSOLE_INPUT_RETRY_MS  10

static const char *TAG = "BMGR_TEST_CONSOLE";

static esp_err_t bmgr_test_console_register_cases(void)
{
    bmgr_register_audio_cases();
    bmgr_register_button_cases();
    bmgr_register_camera_cases();
    bmgr_register_custom_cases();
    bmgr_register_fs_cases();
    bmgr_register_gpio_expander_cases();
    bmgr_register_knob_cases();
    bmgr_register_lcd_cases();
    bmgr_register_led_cases();
    bmgr_register_periph_cases();
    bmgr_register_runtime_override_cases();
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

#if CONFIG_ESP_CONSOLE_NONE

static void bmgr_test_console_run_line(const char *line)
{
    int cmd_ret = 0;
    esp_err_t err = esp_console_run(line, &cmd_ret);
    if (err == ESP_ERR_NOT_FOUND) {
        printf("Unrecognized command\n");
    } else if (err == ESP_ERR_INVALID_ARG) {
        /* Empty command line. */
    } else if (err == ESP_OK && cmd_ret != ESP_OK) {
        printf("Command returned non-zero error code: 0x%x (%s)\n", cmd_ret, esp_err_to_name(cmd_ret));
    } else if (err != ESP_OK) {
        printf("Internal error: %s\n", esp_err_to_name(err));
    }
}

/* TinyUSB CDC fread() returns 0 when the host has not sent a byte. */
static int bmgr_test_console_read_byte(void)
{
    unsigned char ch = 0;
    while (true) {
        if (fread(&ch, 1, 1, stdin) == 1) {
            return ch;
        }
        clearerr(stdin);
        vTaskDelay(pdMS_TO_TICKS(BMGR_TEST_CONSOLE_INPUT_RETRY_MS));
    }
}

static void bmgr_test_console_prompt(void)
{
    printf(BMGR_TEST_CONSOLE_PROMPT " ");
    fflush(stdout);
}

/**
 * stdin/stdout are already the board USB CDC console. linenoise() treats a
 * non-blocking empty read as the end of the line and prints the prompt again.
 */
static esp_err_t bmgr_test_console_run_attached(bmgr_test_context_t *ctx)
{
    size_t len = 0;
    bool skip_lf = false;
    esp_console_config_t console_config = ESP_CONSOLE_CONFIG_DEFAULT();
    char line[console_config.max_cmdline_length];
    ESP_RETURN_ON_ERROR(esp_console_init(&console_config), TAG, "Failed to init console");
    ESP_RETURN_ON_ERROR(esp_console_register_help_command(), TAG, "Failed to register help command");

    esp_err_t ret = bmgr_test_console_register_commands(ctx);
    if (ret != ESP_OK) {
        return ret;
    }

    setvbuf(stdin, NULL, _IONBF, 0);
    printf("\r\nType 'help' to get the list of commands.\r\n");
    ctx->console_ready = true;
    bmgr_test_console_prompt();

    while (true) {
        const int ch = bmgr_test_console_read_byte();
        if (ch == '\n' && skip_lf) {
            skip_lf = false;
            continue;
        }
        skip_lf = false;

        if (ch == '\r' || ch == '\n') {
            skip_lf = (ch == '\r');
            printf("\r\n");
            line[len] = '\0';
            if (len > 0) {
                bmgr_test_console_run_line(line);
            }
            len = 0;
            bmgr_test_console_prompt();
            continue;
        }

        if (ch == '\b' || ch == 0x7f) {
            if (len > 0) {
                len--;
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }

        if (ch < 0x20 || ch > 0x7e || len + 1 >= sizeof(line)) {
            continue;
        }

        line[len++] = (char)ch;
        printf("%c", (char)ch);
        fflush(stdout);
    }
}

#endif  /* CONFIG_ESP_CONSOLE_NONE */

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
#elif CONFIG_ESP_CONSOLE_NONE
    (void)repl_config;
    return bmgr_test_console_run_attached(ctx);
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
