/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/**
 * Host-side stubs for compiling esp_board_runtime_state, esp_board_manager,
 * and esp_board_periph without ESP-IDF / hardware.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int32_t esp_err_t;

#ifndef ESP_BOARD_TABLE_CONST
#define ESP_BOARD_TABLE_CONST
#endif  /* ESP_BOARD_TABLE_CONST */

#define ESP_OK                 0
#define ESP_FAIL               -1
#define ESP_ERR_NO_MEM         0x101
#define ESP_ERR_INVALID_ARG    0x102
#define ESP_ERR_INVALID_STATE  0x103
#define ESP_ERR_INVALID_SIZE   0x104
#define ESP_ERR_NOT_FOUND      0x105
#define ESP_ERR_NOT_SUPPORTED  0x106

#define ESP_BOARD_ERR_BASE          (0x80000)
#define ESP_BOARD_ERR_MANAGER_BASE  (ESP_BOARD_ERR_BASE + 0x000)
#define ESP_BOARD_ERR_PERIPH_BASE   (ESP_BOARD_ERR_BASE + 0x100)
#define ESP_BOARD_ERR_DEVICE_BASE   (ESP_BOARD_ERR_BASE + 0x200)

#define ESP_BOARD_ERR_MANAGER_NOT_INIT          (ESP_BOARD_ERR_MANAGER_BASE - 0x00)
#define ESP_BOARD_ERR_MANAGER_ALREADY_INIT      (ESP_BOARD_ERR_MANAGER_BASE - 0x01)
#define ESP_BOARD_ERR_MANAGER_PERIPH_NOT_FOUND  (ESP_BOARD_ERR_MANAGER_BASE - 0x02)
#define ESP_BOARD_ERR_MANAGER_DEVICE_NOT_FOUND  (ESP_BOARD_ERR_MANAGER_BASE - 0x03)
#define ESP_BOARD_ERR_MANAGER_INVALID_ARG       (ESP_BOARD_ERR_MANAGER_BASE - 0x04)

#define ESP_BOARD_ERR_PERIPH_NOT_FOUND      (ESP_BOARD_ERR_PERIPH_BASE - 0x00)
#define ESP_BOARD_ERR_PERIPH_INVALID_ARG    (ESP_BOARD_ERR_PERIPH_BASE - 0x01)
#define ESP_BOARD_ERR_PERIPH_NO_HANDLE      (ESP_BOARD_ERR_PERIPH_BASE - 0x02)
#define ESP_BOARD_ERR_PERIPH_NO_INIT        (ESP_BOARD_ERR_PERIPH_BASE - 0x03)
#define ESP_BOARD_ERR_PERIPH_INIT_FAILED    (ESP_BOARD_ERR_PERIPH_BASE - 0x04)
#define ESP_BOARD_ERR_PERIPH_DEINIT_FAILED  (ESP_BOARD_ERR_PERIPH_BASE - 0x05)
#define ESP_BOARD_ERR_PERIPH_NOT_SUPPORTED  (ESP_BOARD_ERR_PERIPH_BASE - 0x06)

#define ESP_BOARD_ERR_DEVICE_NOT_FOUND      (ESP_BOARD_ERR_DEVICE_BASE - 0x00)
#define ESP_BOARD_ERR_DEVICE_INVALID_ARG    (ESP_BOARD_ERR_DEVICE_BASE - 0x01)
#define ESP_BOARD_ERR_DEVICE_NO_HANDLE      (ESP_BOARD_ERR_DEVICE_BASE - 0x02)
#define ESP_BOARD_ERR_DEVICE_NO_INIT        (ESP_BOARD_ERR_DEVICE_BASE - 0x03)
#define ESP_BOARD_ERR_DEVICE_INIT_FAILED    (ESP_BOARD_ERR_DEVICE_BASE - 0x04)
#define ESP_BOARD_ERR_DEVICE_DEINIT_FAILED  (ESP_BOARD_ERR_DEVICE_BASE - 0x05)
#define ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED  (ESP_BOARD_ERR_DEVICE_BASE - 0x06)
#define ESP_BOARD_ERR_DEVICE_DEP_IN_USE     (ESP_BOARD_ERR_DEVICE_BASE - 0x07)

static inline const char *esp_err_to_name(esp_err_t err)
{
    switch (err) {
        case ESP_OK:
            return "ESP_OK";
        case ESP_FAIL:
            return "ESP_FAIL";
        case ESP_ERR_INVALID_STATE:
            return "ESP_ERR_INVALID_STATE";
        case ESP_ERR_NOT_FOUND:
            return "ESP_ERR_NOT_FOUND";
        case ESP_BOARD_ERR_DEVICE_NOT_FOUND:
            return "ESP_BOARD_ERR_DEVICE_NOT_FOUND";
        case ESP_BOARD_ERR_DEVICE_INVALID_ARG:
            return "ESP_BOARD_ERR_DEVICE_INVALID_ARG";
        case ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED:
            return "ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED";
        case ESP_BOARD_ERR_PERIPH_NOT_FOUND:
            return "ESP_BOARD_ERR_PERIPH_NOT_FOUND";
        case ESP_BOARD_ERR_PERIPH_INVALID_ARG:
            return "ESP_BOARD_ERR_PERIPH_INVALID_ARG";
        case ESP_BOARD_ERR_PERIPH_NOT_SUPPORTED:
            return "ESP_BOARD_ERR_PERIPH_NOT_SUPPORTED";
        case ESP_BOARD_ERR_MANAGER_NOT_INIT:
            return "ESP_BOARD_ERR_MANAGER_NOT_INIT";
        case ESP_BOARD_ERR_MANAGER_INVALID_ARG:
            return "ESP_BOARD_ERR_MANAGER_INVALID_ARG";
        default:
            return "ESP_ERR_UNKNOWN";
    }
}

#define ESP_LOGE(tag, fmt, ...)  fprintf(stderr, "E (%s) " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...)  fprintf(stderr, "W (%s) " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...)  ((void)0)
#define ESP_LOGD(tag, fmt, ...)  ((void)0)

#define ESP_BOARD_RETURN_ON_FALSE(condition, error_code, tag, format, ...)  do {  \
    if (!(condition)) {                                                           \
        ESP_LOGE(tag, "[%s] " format, __func__, ##__VA_ARGS__);                   \
        return error_code;                                                        \
    }                                                                             \
} while (0)

#define ESP_BOARD_RETURN_ON_ERROR(operation, tag, ...)  do {  \
    esp_err_t err_rc_ = (operation);                          \
    if (err_rc_ != ESP_OK) {                                  \
        ESP_LOGE(tag, "[%s] " __VA_ARGS__, __func__);         \
        return err_rc_;                                       \
    }                                                         \
} while (0)

#define ESP_BOARD_RETURN_ON_PERIPH_NOT_FOUND(desc, name, tag, ...)  \
    ESP_BOARD_RETURN_ON_FALSE(desc, ESP_BOARD_ERR_PERIPH_NOT_FOUND, tag, ##__VA_ARGS__)

#define ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(desc, name, tag, ...)  \
    ESP_BOARD_RETURN_ON_FALSE(desc, ESP_BOARD_ERR_DEVICE_NOT_FOUND, tag, ##__VA_ARGS__)

typedef enum {
    ESP_BOARD_PERIPH_ROLE_NONE   = 0,
    ESP_BOARD_PERIPH_ROLE_MASTER = 1,
} esp_board_periph_role_t;

static const char *role_names_map[] __attribute__((unused)) = {
    [ESP_BOARD_PERIPH_ROLE_NONE]   = "NONE",
    [ESP_BOARD_PERIPH_ROLE_MASTER] = "MASTER",
};

typedef int (*esp_board_device_init_func)(void *cfg, int cfg_size, void **device_handle);
typedef int (*esp_board_device_deinit_func)(void *device_handle);
typedef int (*esp_board_device_power_ctrl_func)(void *dev_handle, const char *device_name, bool power_on);
typedef int (*esp_board_device_callback_register_func)(void *dev_handle, const void *cfg, int cfg_size,
                                                       void *call_back_func, void *user_data);

typedef esp_err_t (*esp_board_periph_init_func)(void *cfg, int cfg_size, void **periph_handle);
typedef esp_err_t (*esp_board_periph_deinit_func)(void *periph_handle);

typedef struct esp_board_device_desc {
    const struct esp_board_device_desc *next;
    const char                         *name;
    const char                         *chip;
    const char                         *type;
    const char                         *sub_type;
    const void                         *cfg;
    uint16_t                            cfg_size;
    uint8_t                             init_skip : 1;
    const char                         *power_ctrl_device;
    const char *const                  *depends_on;
    uint8_t                             depends_on_num;
} esp_board_device_desc_t;

typedef struct esp_board_periph_desc {
    const struct esp_board_periph_desc *next;
    const char                         *name;
    const char                         *type;
    esp_board_periph_role_t             role;
    const char                         *format;
    const void                         *cfg;
    int                                 cfg_size;
    int                                 id;
    uint8_t                             init_skip : 1;
} esp_board_periph_desc_t;

typedef struct esp_board_device_handle {
    struct esp_board_device_handle *next;
    const char                     *name;
    const char                     *chip;
    const char                     *type;
    void                           *device_handle;
    uint8_t                         ref_count;
    esp_board_device_init_func      init;
    esp_board_device_deinit_func    deinit;
} esp_board_device_handle_t;

typedef struct esp_board_periph_entry {
    struct esp_board_periph_entry *next;
    const char                    *type;
    esp_board_periph_role_t        role;
    esp_board_periph_init_func     init;
    esp_board_periph_deinit_func   deinit;
} esp_board_periph_entry_t;

typedef struct esp_board_periph_list {
    struct esp_board_periph_list *next;
    const char                   *name;
    const char                   *type;
    esp_board_periph_role_t       role;
    void                         *periph_handle;
    uint8_t                       ref_count;
} esp_board_periph_list_t;

typedef struct esp_board_info {
    const char *name;
    const char *chip;
    const char *version;
    const char *description;
    const char *manufacturer;
} esp_board_info_t;

#define EXTRA_FUNC_IMPLEMENT(name, extra_func)  /* host stub */

extern const esp_board_info_t g_esp_board_info;
extern ESP_BOARD_TABLE_CONST esp_board_device_desc_t g_esp_board_devices[];
extern ESP_BOARD_TABLE_CONST esp_board_periph_desc_t g_esp_board_peripherals[];
extern esp_board_device_handle_t g_esp_board_device_handles[];
extern esp_board_periph_entry_t g_esp_board_periph_handles[];

static inline const esp_board_device_desc_t *esp_board_find_device_desc(const char *name)
{
    if (!name) {
        return NULL;
    }
    const esp_board_device_desc_t *desc = g_esp_board_devices;
    while (desc && desc->name) {
        if (strcmp(desc->name, name) == 0) {
            return desc;
        }
        desc = desc->next;
    }
    return NULL;
}

static inline const esp_board_periph_desc_t *esp_board_find_periph_desc(const char *name)
{
    if (!name) {
        return NULL;
    }
    const esp_board_periph_desc_t *desc = g_esp_board_peripherals;
    while (desc && desc->name) {
        if (strcmp(desc->name, name) == 0) {
            return desc;
        }
        desc = desc->next;
    }
    return NULL;
}

static inline esp_board_device_handle_t *esp_board_find_device_handle(const char *name)
{
    if (!name) {
        return NULL;
    }
    esp_board_device_handle_t *handle = g_esp_board_device_handles;
    while (handle && handle->name) {
        if (strcmp(handle->name, name) == 0) {
            return handle;
        }
        handle = handle->next;
    }
    return NULL;
}

static inline esp_board_periph_entry_t *esp_board_find_periph_handle(const char *type, esp_board_periph_role_t role)
{
    if (!type) {
        return NULL;
    }
    esp_board_periph_entry_t *handle = g_esp_board_periph_handles;
    while (handle && handle->type) {
        if (strcmp(handle->type, type) == 0 && handle->role == role) {
            return handle;
        }
        handle = handle->next;
    }
    return NULL;
}
