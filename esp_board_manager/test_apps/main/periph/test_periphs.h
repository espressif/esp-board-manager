/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void test_periph_gpio(void);

void test_periph_i2c(void);

esp_err_t test_periph_i2c_probe(void);

void test_periph_uart(void);

void test_periph_adc(void);

void test_periph_rmt(void);

void test_periph_pcnt(void);

void test_periph_dac(void);

void test_periph_sdm(void);

void test_periph_mcpwm(void);

void test_periph_anacmpr(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
