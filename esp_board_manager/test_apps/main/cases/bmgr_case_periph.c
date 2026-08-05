/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_check.h"
#include "sdkconfig.h"
#include "bmgr_test_names.h"
#include "bmgr_test_registry.h"
#include "periph/test_periphs.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)  (sizeof(array) / sizeof((array)[0]))
#endif  /* ARRAY_SIZE */

static esp_err_t run_periph_i2c(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_periph_i2c();
    return ESP_OK;
}

static esp_err_t run_periph_i2c_probe(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    return test_periph_i2c_probe();
}

static esp_err_t run_periph_gpio(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_periph_gpio();
    return ESP_OK;
}

#ifdef CONFIG_ESP_BOARD_PERIPH_UART_SUPPORT
static esp_err_t run_periph_uart(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_periph_uart();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_PERIPH_UART_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_ADC_SUPPORT
static esp_err_t run_periph_adc(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_periph_adc();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_PERIPH_ADC_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_RMT_SUPPORT
static esp_err_t run_periph_rmt(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_periph_rmt();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_PERIPH_RMT_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_PCNT_SUPPORT
static esp_err_t run_periph_pcnt(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_periph_pcnt();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_PERIPH_PCNT_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_DAC_SUPPORT
static esp_err_t run_periph_dac(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_periph_dac();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_PERIPH_DAC_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_SDM_SUPPORT
static esp_err_t run_periph_sdm(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_periph_sdm();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_PERIPH_SDM_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_MCPWM_SUPPORT
static esp_err_t run_periph_mcpwm(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_periph_mcpwm();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_PERIPH_MCPWM_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_ANACMPR_SUPPORT
static esp_err_t run_periph_anacmpr(bmgr_test_context_t *ctx, int argc, char **argv)
{
    (void)ctx;
    (void)argc;
    (void)argv;
    test_periph_anacmpr();
    return ESP_OK;
}
#endif  /* CONFIG_ESP_BOARD_PERIPH_ANACMPR_SUPPORT */

void bmgr_register_periph_cases(void)
{
    const bmgr_test_case_t base_cases[] = {
        {
            .name = "periph.i2c",
            .group = "periph",
            .help = "Run I2C peripheral test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_I2C_MASTER),
            },
            .run = run_periph_i2c,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
        {
            .name = "periph.i2c_probe",
            .group = "periph",
            .help = "Probe all valid I2C addresses",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_I2C_MASTER),
            },
            .run = run_periph_i2c_probe,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
        {
            .name = "periph.gpio",
            .group = "periph",
            .help = "Run GPIO peripheral test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_GPIO_PA_CONTROL),
            },
            .run = run_periph_gpio,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(base_cases, ARRAY_SIZE(base_cases)));

#ifdef CONFIG_ESP_BOARD_PERIPH_UART_SUPPORT
    const bmgr_test_case_t uart_cases[] = {
        {
            .name = "periph.uart",
            .group = "periph",
            .help = "Run UART peripheral test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_UART),
            },
            .run = run_periph_uart,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(uart_cases, ARRAY_SIZE(uart_cases)));
#endif  /* CONFIG_ESP_BOARD_PERIPH_UART_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_ADC_SUPPORT
    const bmgr_test_case_t adc_cases[] = {
        {
            .name = "periph.adc",
            .group = "periph",
            .help = "Run ADC peripheral test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_ADC),
            },
            .run = run_periph_adc,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(adc_cases, ARRAY_SIZE(adc_cases)));
#endif  /* CONFIG_ESP_BOARD_PERIPH_ADC_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_RMT_SUPPORT
    const bmgr_test_case_t rmt_cases[] = {
        {
            .name = "periph.rmt",
            .group = "periph",
            .help = "Run RMT peripheral test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_RMT_TX),
            },
            .run = run_periph_rmt,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(rmt_cases, ARRAY_SIZE(rmt_cases)));
#endif  /* CONFIG_ESP_BOARD_PERIPH_RMT_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_PCNT_SUPPORT
    const bmgr_test_case_t pcnt_cases[] = {
        {
            .name = "periph.pcnt",
            .group = "periph",
            .help = "Run PCNT peripheral test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_PCNT),
            },
            .run = run_periph_pcnt,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(pcnt_cases, ARRAY_SIZE(pcnt_cases)));
#endif  /* CONFIG_ESP_BOARD_PERIPH_PCNT_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_DAC_SUPPORT
    const bmgr_test_case_t dac_cases[] = {
        {
            .name = "periph.dac",
            .group = "periph",
            .help = "Run DAC peripheral test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_DAC),
            },
            .run = run_periph_dac,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(dac_cases, ARRAY_SIZE(dac_cases)));
#endif  /* CONFIG_ESP_BOARD_PERIPH_DAC_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_SDM_SUPPORT
    const bmgr_test_case_t sdm_cases[] = {
        {
            .name = "periph.sdm",
            .group = "periph",
            .help = "Run SDM peripheral test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_SDM),
            },
            .run = run_periph_sdm,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(sdm_cases, ARRAY_SIZE(sdm_cases)));
#endif  /* CONFIG_ESP_BOARD_PERIPH_SDM_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_MCPWM_SUPPORT
    const bmgr_test_case_t mcpwm_cases[] = {
        {
            .name = "periph.mcpwm",
            .group = "periph",
            .help = "Run MCPWM peripheral test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_MCPWM),
            },
            .run = run_periph_mcpwm,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(mcpwm_cases, ARRAY_SIZE(mcpwm_cases)));
#endif  /* CONFIG_ESP_BOARD_PERIPH_MCPWM_SUPPORT */

#ifdef CONFIG_ESP_BOARD_PERIPH_ANACMPR_SUPPORT
    const bmgr_test_case_t anacmpr_cases[] = {
        {
            .name = "periph.anacmpr",
            .group = "periph",
            .help = "Run analog comparator peripheral test",
            .resources = {
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_ANACMPR),
                BMGR_TEST_RESOURCE_REQUIRED(BMGR_TEST_NAME_GPIO_MONITOR),
            },
            .run = run_periph_anacmpr,
            .flags = BMGR_TEST_CASE_FLAG_MANUAL | BMGR_TEST_CASE_FLAG_NEEDS_BOARD,
        },
    };
    ESP_ERROR_CHECK(bmgr_test_registry_add(anacmpr_cases, ARRAY_SIZE(anacmpr_cases)));
#endif  /* CONFIG_ESP_BOARD_PERIPH_ANACMPR_SUPPORT */
}
