/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_board_manager.h"
#include "esp_board_device.h"
#include "esp_board_manager_err.h"
#include "esp_board_find_utils.h"
#include "esp_board_runtime_state.h"
#include "periph_i2c_internal.h"
#if CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUPPORT
#include "dev_power_ctrl.h"
#endif  /* CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUPPORT */

static const char *TAG = "BOARD_DEVICE";

static esp_err_t ensure_board_prepared(void)
{
    return esp_board_runtime_prepare();
}

extern ESP_BOARD_TABLE_CONST esp_board_device_desc_t g_esp_board_devices[];
extern esp_board_device_handle_t g_esp_board_device_handles[];

static esp_err_t esp_board_device_deinit_internal(const char *name, bool ignore_dep_check);

esp_err_t __attribute__((weak)) periph_i2c_set_effective_addr_internal(const char *i2c_name,
                                                                       const char *device_name,
                                                                       uint16_t addr)
{
    (void)i2c_name;
    (void)device_name;
    (void)addr;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t __attribute__((weak)) periph_i2c_get_effective_addr_internal(const char *device_name,
                                                                       uint16_t *addr)
{
    (void)device_name;
    (void)addr;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t __attribute__((weak)) periph_i2c_clear_effective_addr_internal(const char *device_name)
{
    (void)device_name;
    return ESP_ERR_NOT_SUPPORTED;
}

void esp_board_device_restore_all_configs(void)
{
    const esp_board_device_desc_t *desc = g_esp_board_devices;

    while (desc && desc->name) {
        (void)esp_board_runtime_clear_app_config(desc->name);
        desc = desc->next;
    }
}

static bool device_is_active(const char *name)
{
    if (esp_board_runtime_is_initialized(name)) {
        return true;
    }
    esp_board_device_handle_t *handle = esp_board_find_device_handle(name);
    return handle != NULL && handle->ref_count > 0;
}

static esp_err_t map_runtime_err_to_device(esp_err_t err)
{
    if (err == ESP_BOARD_ERR_MANAGER_INVALID_ARG) {
        return ESP_BOARD_ERR_DEVICE_INVALID_ARG;
    }
    return err;
}

static void resolve_effective_config(const char *name, const esp_board_device_desc_t *desc,
                                     void **out_cfg, uint16_t *out_cfg_size)
{
    esp_board_runtime_node_t *node = esp_board_runtime_find_node(name);
    if (node != NULL) {
        if (node->app.has_cfg) {
            *out_cfg = node->app.cfg;
            *out_cfg_size = node->app.cfg_size;
            return;
        }
        if (node->board.has_cfg) {
            *out_cfg = node->board.cfg;
            *out_cfg_size = node->board.cfg_size;
            return;
        }
    }
    *out_cfg = (void *)desc->cfg;
    *out_cfg_size = desc->cfg_size;
}

static void resolve_effective_ops(const char *name, const esp_board_device_handle_t *handle,
                                  esp_board_device_init_func *out_init,
                                  esp_board_device_deinit_func *out_deinit)
{
    esp_board_runtime_node_t *node = esp_board_runtime_find_node(name);
    if (node != NULL) {
        if (node->app.has_ops) {
            *out_init = (esp_board_device_init_func)node->app.init_fn;
            *out_deinit = (esp_board_device_deinit_func)node->app.deinit_fn;
            return;
        }
        if (node->board.has_ops) {
            *out_init = (esp_board_device_init_func)node->board.init_fn;
            *out_deinit = (esp_board_device_deinit_func)node->board.deinit_fn;
            return;
        }
    }
    *out_init = handle ? handle->init : NULL;
    *out_deinit = handle ? handle->deinit : NULL;
}

static esp_err_t check_active_device_dependents(const char *name)
{
    esp_board_device_handle_t *check_h = g_esp_board_device_handles;

    while (check_h) {
        if (check_h->device_handle) {
            const esp_board_device_desc_t *check_d = esp_board_find_device_desc(check_h->name);
            if (check_d) {
                if (check_d->power_ctrl_device && strcmp(check_d->power_ctrl_device, name) == 0) {
                    return ESP_BOARD_ERR_DEVICE_DEP_IN_USE;
                }
                for (int i = 0; i < check_d->depends_on_num; i++) {
                    if (strcmp(check_d->depends_on[i], name) == 0) {
                        ESP_LOGE(TAG, "Cannot deinit %s, it is in use by %s", name, check_d->name);
                        return ESP_BOARD_ERR_DEVICE_DEP_IN_USE;
                    }
                }
            }
        }
        check_h = check_h->next;
    }
    return ESP_OK;
}

esp_err_t esp_board_device_init(const char *name)
{
    ESP_BOARD_RETURN_ON_FALSE(name, ESP_BOARD_ERR_DEVICE_INVALID_ARG, TAG, "name is null");
    esp_err_t ret = esp_board_manager_prepare();
    ESP_BOARD_RETURN_ON_ERROR(ret, TAG, "Board prepare failed before device init");

    /* Find device descriptor */
    const esp_board_device_desc_t *desc = esp_board_find_device_desc(name);
    ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(desc, name, TAG, "Device %s not found", name);

    /* Find device handle */
    esp_board_device_handle_t *handle = esp_board_find_device_handle(name);
    ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(handle, name, TAG, "Device handle %s not found", name);

    if (handle->device_handle != NULL && handle->ref_count == 0) {
        ESP_LOGE(TAG, "Device %s init left unreleased resources; restart required", name);
        return ESP_ERR_INVALID_STATE;
    }

    /* Power on device if power control device is specified */
    if (desc->power_ctrl_device) {
        ret = esp_board_device_power_ctrl(name, true);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to power on device: %s via power control device: %s", name, desc->power_ctrl_device);
        }
    }

    /* Existing reference fast path; concurrent init/deinit of this device is not supported. */
    if (handle->ref_count > 0) {
        handle->ref_count++;
        ESP_LOGI(TAG, "Device %s already initialized, ref_count: %d", name, handle->ref_count);
        return ESP_OK;
    }

    esp_board_device_init_func init_fn = NULL;
    esp_board_device_deinit_func deinit_fn = NULL;
    ret = esp_board_runtime_lock();
    ESP_BOARD_RETURN_ON_ERROR(ret, TAG, "Failed to lock runtime for device init: %s", name);
    resolve_effective_ops(name, handle, &init_fn, &deinit_fn);
    esp_board_runtime_unlock();
    ESP_BOARD_RETURN_ON_FALSE(init_fn, ESP_BOARD_ERR_DEVICE_NO_INIT, TAG,
                              "No init function for device: %s", name);

    /* First-time initialization: initialize dependencies first (no runtime lock held) */
    for (int i = 0; i < desc->depends_on_num; i++) {
        ret = esp_board_device_init(desc->depends_on[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init dependency %s for device %s", desc->depends_on[i], name);
            for (int j = 0; j < i; j++) {
                esp_board_device_deinit_internal(desc->depends_on[j], true);
            }
            return ret;
        }
    }

    ret = esp_board_runtime_lock();
    if (ret != ESP_OK) {
        for (int i = 0; i < desc->depends_on_num; i++) {
            esp_board_device_deinit_internal(desc->depends_on[i], true);
        }
        return ret;
    }

    if (handle->ref_count > 0) {
        handle->ref_count++;
        esp_board_runtime_unlock();
        ESP_LOGI(TAG, "Device %s already initialized, ref_count: %d", name, handle->ref_count);
        return ESP_OK;
    }
    if (esp_board_runtime_is_initialized_unlocked(name)) {
        esp_board_runtime_unlock();
        for (int i = 0; i < desc->depends_on_num; i++) {
            esp_board_device_deinit_internal(desc->depends_on[i], true);
        }
        return ESP_ERR_INVALID_STATE;
    }

    void *cfg = NULL;
    uint16_t cfg_size = 0;
    init_fn = NULL;
    deinit_fn = NULL;
    resolve_effective_ops(name, handle, &init_fn, &deinit_fn);
    resolve_effective_config(name, desc, &cfg, &cfg_size);
    if (cfg != NULL && cfg_size != desc->cfg_size) {
        esp_board_runtime_unlock();
        for (int i = 0; i < desc->depends_on_num; i++) {
            esp_board_device_deinit_internal(desc->depends_on[i], true);
        }
        ESP_LOGE(TAG, "Device %s effective config size %u does not match descriptor size %u",
                 name, cfg_size, desc->cfg_size);
        return ESP_ERR_INVALID_SIZE;
    }
    if (init_fn == NULL) {
        esp_board_runtime_unlock();
        for (int i = 0; i < desc->depends_on_num; i++) {
            esp_board_device_deinit_internal(desc->depends_on[i], true);
        }
        return ESP_BOARD_ERR_DEVICE_NO_INIT;
    }

    /**
     * Mark initialized under the same mutex as override/restore before calling the
     * driver, so another task cannot free the config pointer we just resolved.
     * Do not hold the mutex across driver init.
     */
    ret = esp_board_runtime_set_initialized_unlocked(name, true);
    if (ret != ESP_OK) {
        esp_board_runtime_unlock();
        ESP_LOGE(TAG, "Failed to mark device initialized: %s", name);
        for (int i = 0; i < desc->depends_on_num; i++) {
            esp_board_device_deinit_internal(desc->depends_on[i], true);
        }
        return ret;
    }
    esp_board_runtime_unlock();

    /* Capture the matching cleanup before init; never publish an unbound instance. */
    ret = esp_board_runtime_set_bound_deinit(name, (void *)deinit_fn);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to snapshot deinit for device: %s", name);
        (void)esp_board_runtime_set_initialized(name, false);
        for (int i = 0; i < desc->depends_on_num; i++) {
            esp_board_device_deinit_internal(desc->depends_on[i], true);
        }
        return ret;
    }

    ret = init_fn(cfg, (int)cfg_size, &handle->device_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init device: %s", name);
        if (handle->device_handle == NULL) {
            (void)esp_board_runtime_set_initialized(name, false);
            for (int i = 0; i < desc->depends_on_num; i++) {
                esp_board_device_deinit_internal(desc->depends_on[i], true);
            }
        } else {
            ESP_LOGW(TAG, "Device %s init left unreleased resources; restart required", name);
        }
        return ret;
    }

    handle->ref_count = 1;
    ESP_LOGD(TAG, "initialized device %s ref_count: %d device_handle:%p", name, handle->ref_count, handle->device_handle);
    return ESP_OK;
}

esp_err_t esp_board_device_get_handle(const char *name, void **device_handle)
{
    ESP_BOARD_RETURN_ON_FALSE(name && device_handle, ESP_BOARD_ERR_DEVICE_INVALID_ARG, TAG, "Invalid args");
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before device handle lookup");

    /* Find device handle */
    esp_board_device_handle_t *handle = esp_board_find_device_handle(name);
    ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(handle, name, TAG, "Device handle %s not found", name);
    if (handle->device_handle == NULL || handle->ref_count == 0) {
        ESP_LOGE(TAG, "Device %s has no active handle", name);
        return ESP_BOARD_ERR_DEVICE_NO_HANDLE;
    }
    *device_handle = handle->device_handle;
    ESP_LOGI(TAG, "Device handle %s found, Handle: %p TO: %p", name, handle->device_handle, *device_handle);
    return ESP_OK;
}

esp_err_t esp_board_device_get_config(const char *name, void **config)
{
    ESP_BOARD_RETURN_ON_FALSE(name && config, ESP_BOARD_ERR_DEVICE_INVALID_ARG, TAG, "Invalid args");
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before device config lookup");

    /* Find device descriptor */
    const esp_board_device_desc_t *desc = esp_board_find_device_desc(name);
    ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(desc, name, TAG, "Device descriptor %s not found", name);

    void *cfg = NULL;
    uint16_t cfg_size = 0;
    esp_err_t lock_ret = esp_board_runtime_lock();
    ESP_BOARD_RETURN_ON_ERROR(lock_ret, TAG, "Failed to lock runtime for device config lookup");
    resolve_effective_config(name, desc, &cfg, &cfg_size);
    esp_board_runtime_unlock();
    if (cfg == NULL) {
        ESP_LOGW(TAG, "Device %s has no config", name);
        *config = NULL;
        return ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED;
    }

    *config = cfg;
    ESP_LOGI(TAG, "Device %s config found: %p (size: %d)", name, cfg, cfg_size);
    return ESP_OK;
}

esp_err_t esp_board_device_get_config_by_handle(void *device_handle, void **config)
{
    if (device_handle == NULL || config == NULL) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_BOARD_ERR_DEVICE_INVALID_ARG;
    }
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before device handle lookup");
    *config = NULL;
    const esp_board_device_handle_t *board_device = esp_board_device_find_by_handle(device_handle);
    if (board_device == NULL) {
        ESP_LOGE(TAG, "Device handle[%p] not found", device_handle);
        return ESP_BOARD_ERR_DEVICE_NOT_FOUND;
    }
    esp_err_t ret = esp_board_device_get_config(board_device->name, config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Get device %s config failed", board_device->name);
        return ret;
    }
    return ESP_OK;
}

esp_err_t esp_board_device_set_ops(const char *name, esp_board_device_init_func init, esp_board_device_deinit_func deinit)
{
    ESP_BOARD_RETURN_ON_FALSE(name && init && deinit, ESP_BOARD_ERR_DEVICE_INVALID_ARG, TAG, "Invalid args");
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before device ops update");

    const esp_board_device_desc_t *desc = esp_board_find_device_desc(name);
    ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(desc, name, TAG, "Device %s not found", name);

    if (device_is_active(name)) {
        return ESP_ERR_INVALID_STATE;
    }

    return map_runtime_err_to_device(esp_board_runtime_set_ops(name, (void *)init, (void *)deinit));
}

esp_err_t esp_board_device_override_config(const char *name, const void *config, uint16_t config_size)
{
    ESP_BOARD_RETURN_ON_FALSE(name && config && config_size, ESP_BOARD_ERR_DEVICE_INVALID_ARG, TAG, "Invalid args");
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before device config update");

    const esp_board_device_desc_t *desc = esp_board_find_device_desc(name);
    ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(desc, name, TAG, "Device %s not found", name);

    if (device_is_active(name)) {
        return ESP_ERR_INVALID_STATE;
    }

    return map_runtime_err_to_device(esp_board_runtime_copy_config(name, config, config_size));
}

esp_err_t esp_board_device_restore_config(const char *name)
{
    ESP_BOARD_RETURN_ON_FALSE(name, ESP_BOARD_ERR_DEVICE_INVALID_ARG, TAG, "name is null");
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before device config restore");

    const esp_board_device_desc_t *desc = esp_board_find_device_desc(name);
    ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(desc, name, TAG, "Device %s not found", name);

    if (device_is_active(name)) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_board_runtime_clear_app_config(name);
    if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "Config override for device %s not found", name);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_BOARD_RETURN_ON_ERROR(ret, TAG, "Failed to restore config for device: %s", name);
    ESP_LOGI(TAG, "Config override reset for device %s", name);
    return ESP_OK;
}

esp_err_t esp_board_device_restore_ops(const char *name)
{
    ESP_BOARD_RETURN_ON_FALSE(name, ESP_BOARD_ERR_DEVICE_INVALID_ARG, TAG, "name is null");
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before device ops restore");

    const esp_board_device_desc_t *desc = esp_board_find_device_desc(name);
    ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(desc, name, TAG, "Device %s not found", name);

    if (device_is_active(name)) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_board_runtime_clear_app_ops(name);
    if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "Ops override for device %s not found", name);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_BOARD_RETURN_ON_ERROR(ret, TAG, "Failed to restore ops for device: %s", name);
    ESP_LOGI(TAG, "Ops override reset for device %s", name);
    return ESP_OK;
}

esp_err_t esp_board_device_get_i2c_effective_addr(const char *device_name, uint16_t *addr)
{
    ESP_BOARD_RETURN_ON_FALSE(device_name && addr, ESP_BOARD_ERR_DEVICE_INVALID_ARG, TAG, "Invalid args");
    esp_err_t ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(ret, TAG, "Board prepare failed before I2C address lookup");

    const esp_board_device_desc_t *desc = esp_board_find_device_desc(device_name);
    ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(desc, device_name, TAG, "Device %s not found", device_name);

    ret = periph_i2c_get_effective_addr_internal(device_name, addr);
    if (ret == ESP_OK) {
        return ESP_OK;
    }
    if (ret == ESP_ERR_INVALID_ARG) {
        return ESP_BOARD_ERR_DEVICE_INVALID_ARG;
    }
    return ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED;
}

static esp_err_t esp_board_device_deinit_internal(const char *name, bool ignore_dep_check)
{
    if (esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_FAILED) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_BOARD_RETURN_ON_FALSE(name, ESP_BOARD_ERR_DEVICE_INVALID_ARG, TAG, "name is null");

    /* Find device handle */
    esp_board_device_handle_t *handle = esp_board_find_device_handle(name);
    ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(handle, name, TAG, "Device handle %s not found", name);
    if (!handle->device_handle && handle->ref_count == 0) {
        ESP_LOGD(TAG, "Device %s not initialized or already deinitialized", name);
        return ESP_OK;
    }

    if (!ignore_dep_check) {
        esp_err_t ret = check_active_device_dependents(name);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    if (handle->ref_count == 0) {
        ESP_LOGE(TAG, "Device %s init left unreleased resources; restart required", name);
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t old_ref_count = handle->ref_count;

    /* Decrement reference count */
    handle->ref_count--;
    ESP_LOGI(TAG, "Deinit device %s ref_count: %d device_handle:%p", name, handle->ref_count, handle->device_handle);

    /* Only deinitialize if ref_count reaches 0 */
    if (handle->ref_count == 0) {
        /* Use the cleanup bound to this successfully initialized instance. */
        esp_board_device_deinit_func bound_deinit =
            (esp_board_device_deinit_func)esp_board_runtime_get_bound_deinit(name);
        if (!bound_deinit) {
            handle->ref_count = old_ref_count;
            ESP_LOGE(TAG, "No deinit function for device: %s", name);
            return ESP_BOARD_ERR_DEVICE_NO_INIT;
        }

        const esp_board_device_desc_t *desc = esp_board_find_device_desc(name);
        if (desc == NULL) {
            handle->ref_count = old_ref_count;
            ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(desc, name, TAG, "Device %s not found", name);
        }
        esp_err_t ret = bound_deinit(handle->device_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to deinit device: %s", name);
            handle->ref_count = old_ref_count;
            return ret;
        }
        handle->device_handle = NULL;
        (void)esp_board_runtime_set_initialized(name, false);

        if (desc->power_ctrl_device != NULL) {
            /* Keep power available until the device has finished cleanup. */
            esp_err_t power_ret = esp_board_device_deinit(desc->power_ctrl_device);
            if (power_ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to release power control device %s of %s", desc->power_ctrl_device, name);
            }
        }

        for (int i = 0; i < desc->depends_on_num; i++) {
            esp_err_t dep_ret = esp_board_device_deinit_internal(desc->depends_on[i], true);
            if (dep_ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to release dependency %s of %s", desc->depends_on[i], name);
            }
        }
    } else {
        ESP_LOGD(TAG, "Device %s still has %d references, not deinitializing", name, handle->ref_count);
    }
    return ESP_OK;
}

esp_err_t esp_board_device_deinit(const char *name)
{
    return esp_board_device_deinit_internal(name, false);
}

esp_err_t esp_board_device_show(const char *name)
{
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before device show");
    if (name) {
        /* Show information for specific device */
        const esp_board_device_desc_t *desc = esp_board_find_device_desc(name);
        ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(desc, name, TAG, "Device %s not found", name);

        esp_board_device_handle_t *handle = esp_board_find_device_handle(name);
        ESP_LOGI(TAG, "Device %s:", name);
        ESP_LOGI(TAG, "  Type: %s", desc->type);
        ESP_LOGI(TAG, "  Config size: %d", desc->cfg_size);
        if (handle) {
            ESP_LOGI(TAG, "  Handle: %p", handle->device_handle);
            ESP_LOGI(TAG, "  Status: %s", handle->device_handle ? "Initialized" : "Not initialized");
            ESP_LOGI(TAG, "  Ref count: %d", handle->ref_count);
        } else {
            ESP_LOGI(TAG, "  No handle found");
        }
    } else {
        /* Show information for all devices */
        const esp_board_device_desc_t *desc = g_esp_board_devices;
        while (desc && desc->name) {
            esp_board_device_handle_t *handle = esp_board_find_device_handle(desc->name);
            ESP_LOGI(TAG, "Device %s:", desc->name);
            ESP_LOGI(TAG, "  Type: %s", desc->type);
            ESP_LOGI(TAG, "  Config size: %d", desc->cfg_size);
            if (handle) {
                ESP_LOGI(TAG, "  Handle: %p", handle->device_handle);
                ESP_LOGI(TAG, "  Status: %s", handle->device_handle ? "Initialized" : "Not initialized");
                ESP_LOGI(TAG, "  Ref count: %d", handle->ref_count);
            } else {
                ESP_LOGI(TAG, "  No handle found");
            }
            desc = desc->next;
        }
    }
    return ESP_OK;
}

esp_err_t esp_board_device_iterate_name_by_type(const char *type, void **cursor, const char **name)
{
    ESP_BOARD_RETURN_ON_FALSE(type && cursor && name, ESP_BOARD_ERR_DEVICE_INVALID_ARG, TAG, "Invalid args");
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before device iteration");

    const esp_board_device_desc_t *desc = (*cursor == NULL) ? g_esp_board_devices : ((const esp_board_device_desc_t *)(*cursor))->next;
    while (desc && desc->name) {
        if (desc->type && strcmp(desc->type, type) == 0) {
            *cursor = (void *)desc;
            *name = desc->name;
            return ESP_OK;
        }
        desc = desc->next;
    }

    *cursor = NULL;
    *name = NULL;
    return ESP_BOARD_ERR_DEVICE_NOT_FOUND;
}

esp_err_t esp_board_device_init_all(void)
{
    esp_err_t ret = esp_board_manager_prepare();
    ESP_BOARD_RETURN_ON_ERROR(ret, TAG, "Board prepare failed before device init_all");

    const esp_board_device_desc_t *desc = g_esp_board_devices;

    /* Initialize all devices in the list */
    while (desc && desc->name) {
        /* Check if device should be skipped during initialization */
        if (desc->init_skip) {
            ESP_LOGD(TAG, "Skipping initialization of device: %s (init_skip=true)", desc->name);
            desc = desc->next;
            continue;
        }

        ret = esp_board_device_init(desc->name);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize device: %s", desc->name);
        }
        desc = desc->next;
    }
    return ESP_OK;
}

esp_err_t esp_board_device_deinit_all(void)
{
    if (esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_FAILED) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret;
    bool made_progress;
    int total_devices_freed = 0;

    do {
        made_progress = false;
        esp_board_device_handle_t *handle = g_esp_board_device_handles;

        while (handle) {
            if (handle->ref_count > 0 && handle->device_handle != NULL) {
                bool has_active_dependent = false;
                esp_board_device_handle_t *dep_check = g_esp_board_device_handles;
                while (dep_check) {
                    if (dep_check->device_handle && dep_check != handle) {
                        const esp_board_device_desc_t *dep_desc = esp_board_find_device_desc(dep_check->name);
                        if (dep_desc) {
                            if (dep_desc->power_ctrl_device && strcmp(dep_desc->power_ctrl_device, handle->name) == 0) {
                                has_active_dependent = true;
                            }
                            for (int i = 0; i < dep_desc->depends_on_num; i++) {
                                if (strcmp(dep_desc->depends_on[i], handle->name) == 0) {
                                    has_active_dependent = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (has_active_dependent) {
                        break;
                    }
                    dep_check = dep_check->next;
                }
                if (has_active_dependent) {
                    ESP_LOGD(TAG, "Auto-deinit skipping %s (still has active dependents)", handle->name);
                    handle = handle->next;
                    continue;
                }

                ret = esp_board_device_deinit_internal(handle->name, true);
                if (ret == ESP_OK) {
                    made_progress = true;
                    total_devices_freed++;
                    ESP_LOGD(TAG, "Auto-deinit processed device: %s", handle->name);
                } else {
                    ESP_LOGE(TAG, "Failed to deinitialize device: %s (err: 0x%x)", handle->name, ret);
                }
            }
            handle = handle->next;
        }
    } while (made_progress);

    int remaining = 0;
    esp_board_device_handle_t *handle = g_esp_board_device_handles;
    while (handle) {
        if (handle->ref_count > 0 || handle->device_handle != NULL) {
            remaining++;
            ESP_LOGW(TAG, "Warning: Device %s was left initialized (ref_count: %d)", handle->name, handle->ref_count);
        }
        handle = handle->next;
    }

    if (remaining > 0) {
        ESP_LOGE(TAG, "Completed deinit_all with %d items successfully freed, but %d devices remain.", total_devices_freed, remaining);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Successfully deinitialized all devices (%d total)", total_devices_freed);
    return ESP_OK;
}

const esp_board_device_handle_t *esp_board_device_find_by_handle(void *device_handle)
{
    if (esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_FAILED) {
        return NULL;
    }
    if (device_handle == NULL) {
        ESP_LOGE(TAG, "Device handle is NULL in find_device_by_handle");
        return NULL;
    }
    esp_board_device_handle_t *handle = g_esp_board_device_handles;
    esp_board_device_handle_t *slow = handle;
    esp_board_device_handle_t *fast = handle;
    while (handle) {
        if (handle->device_handle == device_handle) {
            return handle;
        }
        handle = handle->next;
        slow = slow ? slow->next : NULL;
        fast = (fast && fast->next) ? fast->next->next : NULL;
        if (slow && slow == fast) {
            ESP_LOGE(TAG, "Device handle list contains a cycle");
            return NULL;
        }
    }
    return NULL;
}

esp_err_t esp_board_device_power_ctrl(const char *name, bool power_on)
{
#ifdef CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUPPORT
    if (esp_board_runtime_get_prepare_state() == ESP_BOARD_RUNTIME_FAILED) {
        return ESP_ERR_INVALID_STATE;
    }
    if (name == NULL) {
        ESP_LOGE(TAG, "Device name is NULL");
        return ESP_BOARD_ERR_MANAGER_INVALID_ARG;
    }
    /* Find the device description by name */
    const esp_board_device_desc_t *desc = esp_board_find_device_desc(name);
    ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(desc, name, TAG, "Device %s not found", name);
    if (desc->power_ctrl_device == NULL) {
        ESP_LOGW(TAG, "Device %s has no power control device configured", name);
        return ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED;
    }

    /* Find the power control device handle by name, init if not already done */
    esp_board_device_handle_t *handle = esp_board_find_device_handle(desc->power_ctrl_device);
    ESP_BOARD_RETURN_ON_DEVICE_NOT_FOUND(handle, desc->power_ctrl_device, TAG, "Power control device %s not found", desc->power_ctrl_device);
    bool power_ctrl_auto_inited = false;
    if (handle->ref_count < 1) {
        ESP_LOGI(TAG, "Power control device %s is not initialized, try to init it first", desc->power_ctrl_device);
        esp_err_t ret = esp_board_device_init(desc->power_ctrl_device);
        if (ret != ESP_OK || handle->device_handle == NULL) {
            ESP_LOGE(TAG, "Failed to initialize power control device %s: %d", desc->power_ctrl_device, ret);
            return ret != ESP_OK ? ret : ESP_BOARD_ERR_DEVICE_NO_HANDLE;
        }
        power_ctrl_auto_inited = true;
    }
    const esp_board_device_desc_t *power_ctrl_desc = esp_board_find_device_desc(desc->power_ctrl_device);
    if (power_ctrl_desc == NULL || power_ctrl_desc->sub_type == NULL) {
        ESP_LOGE(TAG, "Power control device %s has no sub_type", desc->power_ctrl_device);
        if (power_ctrl_auto_inited) {
            esp_board_device_deinit_internal(desc->power_ctrl_device, true);
        }
        return power_ctrl_desc == NULL ? ESP_BOARD_ERR_DEVICE_NOT_FOUND : ESP_BOARD_ERR_MANAGER_INVALID_ARG;
    }

    /* Get the power control function by name */
    char power_ctrl_func_name[40];
    snprintf(power_ctrl_func_name, sizeof(power_ctrl_func_name), "%s_power_ctrl", power_ctrl_desc->sub_type);
    void *extra_func = NULL;
    if (esp_board_extra_func_get(power_ctrl_func_name, &extra_func) != 0) {
        ESP_LOGE(TAG, "Power control function %s not found for device %s", power_ctrl_func_name, desc->power_ctrl_device);
        if (power_ctrl_auto_inited) {
            esp_board_device_deinit_internal(desc->power_ctrl_device, true);
        }
        return ESP_BOARD_ERR_MANAGER_INVALID_ARG;
    }
    esp_board_device_power_ctrl_func power_ctrl_func = (esp_board_device_power_ctrl_func)extra_func;

    /* Call the power control function */
    if (power_ctrl_func(handle->device_handle, name, power_on) != 0) {
        ESP_LOGE(TAG, "Failed to control power for device %s", name);
        if (power_ctrl_auto_inited) {
            esp_board_device_deinit_internal(desc->power_ctrl_device, true);
        }
        return ESP_BOARD_ERR_MANAGER_INVALID_ARG;
    }

    return ESP_OK;
#else
    (void)name;
    (void)power_on;
    ESP_LOGW(TAG, "Power control support not enabled in configuration");
    return ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED;
#endif  /* CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUPPORT */
}

esp_err_t esp_board_device_callback_register(const char *name, void *call_back_func, void *user_data)
{
    ESP_BOARD_RETURN_ON_FALSE(name && call_back_func, ESP_BOARD_ERR_DEVICE_INVALID_ARG, TAG, "Invalid arguments");
    esp_err_t err = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(err, TAG, "Board prepare failed before callback registration");

    /* Find device descriptor */
    const esp_board_device_desc_t *dev_desc = esp_board_find_device_desc(name);
    ESP_BOARD_RETURN_ON_FALSE(dev_desc, ESP_BOARD_ERR_DEVICE_NOT_FOUND, TAG, "Device %s not found", name);

    /* Get device-specific callback registration function */
    void *extra_func = NULL;
    err = esp_board_extra_func_get(dev_desc->type, &extra_func);
    if (err != 0) {
        ESP_LOGE(TAG, "Callback register func for device type %s not found", dev_desc->type);
        return ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED;
    }

    /* Get device handle */
    void *dev_handle = NULL;
    err = esp_board_device_get_handle(name, &dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get device handle for %s", name);
        return ESP_BOARD_ERR_DEVICE_NOT_FOUND;
    }

    /* Cast to appropriate function pointer type and call */
    esp_board_device_callback_register_func register_func = (esp_board_device_callback_register_func)extra_func;
    void *cfg = NULL;
    uint16_t cfg_size = 0;
    err = esp_board_runtime_lock();
    ESP_BOARD_RETURN_ON_ERROR(err, TAG, "Failed to lock runtime for callback config lookup");
    resolve_effective_config(name, dev_desc, &cfg, &cfg_size);
    esp_board_runtime_unlock();

    err = register_func(dev_handle, cfg, cfg_size, call_back_func, user_data);
    if (err != 0) {
        ESP_LOGE(TAG, "Device callback registration failed for %s, ", name);
        return ESP_BOARD_ERR_DEVICE_INIT_FAILED;
    }

    ESP_LOGI(TAG, "Callback registered for device %s", name);
    return ESP_OK;
}
