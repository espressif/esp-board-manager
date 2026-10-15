/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>

typedef void *TaskHandle_t;
typedef int32_t BaseType_t;
typedef uint32_t TickType_t;

#define pdFALSE        ((BaseType_t)0)
#define pdTRUE         ((BaseType_t)1)
#define portMAX_DELAY  ((TickType_t)0xffffffffUL)

typedef struct {
    int  dummy;
} portMUX_TYPE;

#define portMUX_INITIALIZER_UNLOCKED  {  \
    .dummy = 0                           \
}

static inline void portENTER_CRITICAL(portMUX_TYPE *mux)
{
    (void)mux;
}

static inline void portEXIT_CRITICAL(portMUX_TYPE *mux)
{
    (void)mux;
}
