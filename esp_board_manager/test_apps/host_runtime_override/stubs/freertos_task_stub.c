/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "freertos/task.h"
#include "freertos/semphr.h"

static TaskHandle_t s_host_current_task = (TaskHandle_t)(uintptr_t)0x1;

void host_set_current_task(TaskHandle_t task)
{
    s_host_current_task = task;
}

TaskHandle_t xTaskGetCurrentTaskHandle(void)
{
    return s_host_current_task;
}

SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    StaticSemaphore_t *m = calloc(1, sizeof(*m));
    if (m == NULL) {
        return NULL;
    }
    m->in_use = 1;
    return m;
}

SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *pxMutexBuffer)
{
    if (pxMutexBuffer == NULL) {
        return NULL;
    }
    memset(pxMutexBuffer, 0, sizeof(*pxMutexBuffer));
    pxMutexBuffer->in_use = 1;
    return pxMutexBuffer;
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait)
{
    (void)xTicksToWait;
    if (xSemaphore == NULL || !xSemaphore->in_use) {
        return pdFALSE;
    }
    if (xSemaphore->locked) {
        /* Host harness is single-threaded; nested take is a test bug. */
        return pdFALSE;
    }
    xSemaphore->locked = 1;
    return pdTRUE;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore)
{
    if (xSemaphore == NULL || !xSemaphore->in_use || !xSemaphore->locked) {
        return pdFALSE;
    }
    xSemaphore->locked = 0;
    return pdTRUE;
}

void vSemaphoreDelete(SemaphoreHandle_t xSemaphore)
{
    (void)xSemaphore;
    /* Host tests only use the static mutex; nothing to free. */
}
