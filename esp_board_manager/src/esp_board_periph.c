/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_board_manager.h"
#include "esp_board_periph.h"
#include "esp_board_manager_err.h"
#include "esp_board_find_utils.h"
#include "esp_board_runtime_state.h"

static const char *TAG = "BOARD_PERIPH";

static esp_err_t ensure_board_prepared(void)
{
    return esp_board_runtime_prepare();
}

#ifndef ESP_BOARD_LEGACY_API_WARN
#define ESP_BOARD_LEGACY_API_WARN  1
#endif  /* ESP_BOARD_LEGACY_API_WARN */

extern ESP_BOARD_TABLE_CONST esp_board_periph_desc_t g_esp_board_peripherals[];
extern esp_board_periph_entry_t g_esp_board_periph_handles[];

/* Peripheral handle list */
static esp_board_periph_list_t *periph_list = NULL;

/* Defined in board_periph_handles.c */
extern esp_board_periph_entry_t g_esp_board_periph_handles[];

/* Find peripheral list entry by name */
static esp_board_periph_list_t *find_periph_list(const char *name)
{
    esp_board_periph_list_t *list = periph_list;
    while (list) {
        if (strcmp(list->name, name) == 0) {
            return list;
        }
        list = list->next;
    }
    return NULL;
}

/* Add new peripheral list entry */
static esp_board_periph_list_t *add_periph_list(const char *name, const char *type, esp_board_periph_role_t role)
{
    esp_board_periph_list_t *list = calloc(1, sizeof(esp_board_periph_list_t));
    if (!list) {
        return NULL;
    }

    list->name = strdup(name);
    list->type = strdup(type);
    list->role = role;
    list->ref_count = 0;
    list->periph_handle = NULL;
    list->next = periph_list;
    periph_list = list;

    return list;
}

static bool periph_is_active(const char *name)
{
    if (esp_board_runtime_is_initialized(name)) {
        return true;
    }
    esp_board_periph_list_t *list = find_periph_list(name);
    return list != NULL && list->ref_count > 0;
}

static esp_err_t map_runtime_err_to_periph(esp_err_t err)
{
    if (err == ESP_BOARD_ERR_DEVICE_NOT_FOUND) {
        return ESP_BOARD_ERR_PERIPH_NOT_FOUND;
    }
    if (err == ESP_BOARD_ERR_MANAGER_INVALID_ARG) {
        return ESP_BOARD_ERR_PERIPH_INVALID_ARG;
    }
    return err;
}

static void resolve_effective_config(const char *name, const esp_board_periph_desc_t *desc,
                                     void **out_cfg, int *out_cfg_size)
{
    esp_board_runtime_node_t *node = esp_board_runtime_find_node(name);
    if (node != NULL) {
        if (node->app.has_cfg) {
            *out_cfg = node->app.cfg;
            *out_cfg_size = (int)node->app.cfg_size;
            return;
        }
        if (node->board.has_cfg) {
            *out_cfg = node->board.cfg;
            *out_cfg_size = (int)node->board.cfg_size;
            return;
        }
    }
    *out_cfg = (void *)desc->cfg;
    *out_cfg_size = desc->cfg_size;
}

static void resolve_effective_ops(const char *name, const esp_board_periph_entry_t *shared,
                                  esp_board_periph_init_func *out_init,
                                  esp_board_periph_deinit_func *out_deinit)
{
    esp_board_runtime_node_t *node = esp_board_runtime_find_node(name);
    if (node != NULL) {
        if (node->app.has_ops) {
            *out_init = (esp_board_periph_init_func)node->app.init_fn;
            *out_deinit = (esp_board_periph_deinit_func)node->app.deinit_fn;
            return;
        }
        if (node->board.has_ops) {
            *out_init = (esp_board_periph_init_func)node->board.init_fn;
            *out_deinit = (esp_board_periph_deinit_func)node->board.deinit_fn;
            return;
        }
    }
    *out_init = shared ? shared->init : NULL;
    *out_deinit = shared ? shared->deinit : NULL;
}

esp_err_t esp_board_periph_init(const char *name)
{
    ESP_BOARD_RETURN_ON_FALSE(name, ESP_BOARD_ERR_PERIPH_INVALID_ARG, TAG, "name is null");

    esp_err_t prep = esp_board_manager_prepare();
    ESP_BOARD_RETURN_ON_ERROR(prep, TAG, "Board prepare failed before periph init");

    /* Find peripheral descriptor */
    const esp_board_periph_desc_t *desc = esp_board_find_periph_desc(name);
    ESP_BOARD_RETURN_ON_PERIPH_NOT_FOUND(desc, name, TAG, "Peripheral %s not found", name);

    /* Shared type/role handler (fallback when no per-name ops override) */
    esp_board_periph_entry_t *shared = esp_board_find_periph_handle(desc->type, desc->role);
    esp_board_periph_init_func init_fn = NULL;
    esp_board_periph_deinit_func deinit_fn = NULL;
    esp_err_t ret = esp_board_runtime_lock();
    ESP_BOARD_RETURN_ON_ERROR(ret, TAG, "Failed to lock runtime for peripheral init: %s", name);
    resolve_effective_ops(name, shared, &init_fn, &deinit_fn);
    esp_board_runtime_unlock();
    if (init_fn == NULL && shared == NULL) {
        ESP_LOGE(TAG, "No handle found for %s (type=%s, role=%d)", name, desc->type, desc->role);
        return ESP_BOARD_ERR_PERIPH_NO_HANDLE;
    }

    /* Find or create list entry */
    esp_board_periph_list_t *list = find_periph_list(name);
    if (!list) {
        list = add_periph_list(name, desc->type, desc->role);
        ESP_BOARD_RETURN_ON_FALSE(list, ESP_ERR_NO_MEM, TAG, "Failed to create list entry for %s", name);
    }

    /* Existing reference fast path; concurrent init/deinit of this peripheral is not supported. */
    if (list->ref_count > 0) {
        list->ref_count++;
        ESP_LOGI(TAG, "Reuse periph: %s, ref_count=%d", name, list->ref_count);
        return ESP_OK;
    }

    ESP_BOARD_RETURN_ON_FALSE(init_fn, ESP_BOARD_ERR_PERIPH_NO_INIT, TAG, "No init function for periph: %s", name);

    ret = esp_board_runtime_lock();
    ESP_BOARD_RETURN_ON_ERROR(ret, TAG, "Failed to lock runtime for periph init: %s", name);

    if (list->ref_count > 0) {
        list->ref_count++;
        esp_board_runtime_unlock();
        ESP_LOGI(TAG, "Reuse periph: %s, ref_count=%d", name, list->ref_count);
        return ESP_OK;
    }
    if (esp_board_runtime_is_initialized_unlocked(name)) {
        esp_board_runtime_unlock();
        return ESP_ERR_INVALID_STATE;
    }

    void *cfg = NULL;
    int cfg_size = 0;
    init_fn = NULL;
    deinit_fn = NULL;
    resolve_effective_ops(name, shared, &init_fn, &deinit_fn);
    resolve_effective_config(name, desc, &cfg, &cfg_size);
    if (init_fn == NULL && shared == NULL) {
        esp_board_runtime_unlock();
        ESP_LOGE(TAG, "No handle found for %s (type=%s, role=%d)", name, desc->type, desc->role);
        return ESP_BOARD_ERR_PERIPH_NO_HANDLE;
    }
    if (cfg != NULL && cfg_size != desc->cfg_size) {
        esp_board_runtime_unlock();
        ESP_LOGE(TAG, "Peripheral %s effective config size %d does not match descriptor size %d",
                 name, cfg_size, desc->cfg_size);
        return ESP_ERR_INVALID_SIZE;
    }
    if (init_fn == NULL) {
        esp_board_runtime_unlock();
        return ESP_BOARD_ERR_PERIPH_NO_INIT;
    }

    /**
     * Mark initialized under the same mutex as override/restore before calling the
     * driver, so another task cannot free the config pointer we just resolved.
     * Do not hold the mutex across driver init.
     */
    ret = esp_board_runtime_set_initialized_unlocked(name, true);
    if (ret != ESP_OK) {
        esp_board_runtime_unlock();
        ESP_LOGE(TAG, "Failed to mark periph initialized: %s", name);
        return ret;
    }
    esp_board_runtime_unlock();

    ret = init_fn(cfg, cfg_size, &list->periph_handle);
    if (ret != ESP_OK) {
        (void)esp_board_runtime_set_initialized(name, false);
        list->periph_handle = NULL;
        ESP_LOGE(TAG, "Failed to init periph: %s", name);
        return ret;
    }

    ret = esp_board_runtime_set_bound_deinit(name, (void *)deinit_fn);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to snapshot deinit for periph: %s", name);
        if (deinit_fn) {
            (void)deinit_fn(list->periph_handle);
        }
        list->periph_handle = NULL;
        (void)esp_board_runtime_set_initialized(name, false);
        return ret;
    }

    list->ref_count = 1;
    ESP_LOGD(TAG, "Initialized periph: %s, handle: %p", name, list->periph_handle);
    return ESP_OK;
}

esp_err_t esp_board_periph_get_handle(const char *name, void **periph_handle)
{
    ESP_BOARD_RETURN_ON_FALSE(name && periph_handle, ESP_BOARD_ERR_PERIPH_INVALID_ARG, TAG, "Invalid args");
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before peripheral handle lookup");

    /* Find list entry */
    esp_board_periph_list_t *list = find_periph_list(name);
    ESP_BOARD_RETURN_ON_FALSE(list && list->periph_handle, ESP_BOARD_ERR_PERIPH_NO_HANDLE, TAG,
                              "No handle found for %s on get handle", name);
    *periph_handle = list->periph_handle;
    return ESP_OK;
}

esp_err_t esp_board_periph_get_name_by_handle(void *periph_handle, const char **name)
{
    ESP_BOARD_RETURN_ON_FALSE(periph_handle && name, ESP_BOARD_ERR_PERIPH_INVALID_ARG, TAG, "Invalid args");
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before peripheral handle lookup");
    /* Search through all peripheral list entries */
    esp_board_periph_list_t *list = periph_list;
    while (list) {
        if (list->periph_handle == periph_handle) {
            *name = list->name;
            ESP_LOGD(TAG, "Found peripheral name: %s for handle: %p", list->name, periph_handle);
            return ESP_OK;
        }
        list = list->next;
    }
    ESP_LOGE(TAG, "Not found name for peripheral with handle: %p", periph_handle);
    return ESP_BOARD_ERR_PERIPH_NOT_FOUND;
}

esp_err_t esp_board_periph_ref_handle(const char *name, void **periph_handle)
{
    ESP_BOARD_RETURN_ON_FALSE(name && periph_handle, ESP_BOARD_ERR_PERIPH_INVALID_ARG, TAG, "Invalid args");

    esp_err_t ret = esp_board_periph_init(name);
    ESP_BOARD_RETURN_ON_ERROR(ret, TAG, "Failed to init periph: %s on ref handle", name);

    esp_board_periph_list_t *list = find_periph_list(name);
    ESP_BOARD_RETURN_ON_FALSE(list, ESP_BOARD_ERR_PERIPH_NO_HANDLE, TAG, "No list entry found for %s", name);

    if (list->periph_handle == NULL) {
        ESP_LOGE(TAG, "Peripheral %s handle is NULL", name);
        return ESP_BOARD_ERR_PERIPH_NO_HANDLE;
    }
    *periph_handle = list->periph_handle;
    return ESP_OK;
}

esp_err_t esp_board_periph_unref_handle(const char *name)
{
    ESP_BOARD_RETURN_ON_FALSE(name, ESP_BOARD_ERR_PERIPH_INVALID_ARG, TAG, "The name is NULL");
    return esp_board_periph_deinit(name);
}

esp_err_t esp_board_periph_get_config(const char *name, void **config)
{
    if (name == NULL || config == NULL) {
        ESP_LOGE(TAG, "Invalid arguments: name or config is NULL");
        return ESP_BOARD_ERR_PERIPH_INVALID_ARG;
    }
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before peripheral config lookup");
    const esp_board_periph_desc_t *desc = esp_board_find_periph_desc(name);
    if (desc == NULL) {
        ESP_LOGE(TAG, "Peripheral descriptor not found for %s", name);
        return ESP_BOARD_ERR_PERIPH_NOT_FOUND;
    }

    void *cfg = NULL;
    int cfg_size = 0;
    esp_err_t lock_ret = esp_board_runtime_lock();
    ESP_BOARD_RETURN_ON_ERROR(lock_ret, TAG, "Failed to lock runtime for peripheral config lookup");
    resolve_effective_config(name, desc, &cfg, &cfg_size);
    esp_board_runtime_unlock();
    if (cfg == NULL) {
        ESP_LOGW(TAG, "Peripheral %s has no configuration", name);
        *config = NULL;
        return ESP_BOARD_ERR_PERIPH_NOT_SUPPORTED;
    }
    *config = cfg;
    ESP_LOGI(TAG, "Peripheral %s config found: %p (size: %d)", name, cfg, cfg_size);
    return ESP_OK;
}

esp_err_t esp_board_periph_override_config(const char *name, const void *config, uint16_t config_size)
{
    ESP_BOARD_RETURN_ON_FALSE(name && config && config_size, ESP_BOARD_ERR_PERIPH_INVALID_ARG, TAG, "Invalid args");
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before peripheral config update");

    const esp_board_periph_desc_t *desc = esp_board_find_periph_desc(name);
    ESP_BOARD_RETURN_ON_PERIPH_NOT_FOUND(desc, name, TAG, "Peripheral %s not found", name);

    if (periph_is_active(name)) {
        return ESP_ERR_INVALID_STATE;
    }

    return map_runtime_err_to_periph(esp_board_runtime_copy_config(name, config, config_size));
}

esp_err_t esp_board_periph_restore_config(const char *name)
{
    ESP_BOARD_RETURN_ON_FALSE(name, ESP_BOARD_ERR_PERIPH_INVALID_ARG, TAG, "name is null");
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before peripheral config restore");

    const esp_board_periph_desc_t *desc = esp_board_find_periph_desc(name);
    ESP_BOARD_RETURN_ON_PERIPH_NOT_FOUND(desc, name, TAG, "Peripheral %s not found", name);

    if (periph_is_active(name)) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_board_runtime_clear_app_config(name);
    if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "Config override for peripheral %s not found", name);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_BOARD_RETURN_ON_ERROR(ret, TAG, "Failed to restore config for peripheral: %s", name);
    ESP_LOGI(TAG, "Config override reset for peripheral %s", name);
    return ESP_OK;
}

esp_err_t esp_board_periph_set_ops(const char *name, esp_board_periph_init_func init,
                                   esp_board_periph_deinit_func deinit)
{
    ESP_BOARD_RETURN_ON_FALSE(name && init && deinit, ESP_BOARD_ERR_PERIPH_INVALID_ARG, TAG, "Invalid args");
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before peripheral ops update");

    const esp_board_periph_desc_t *desc = esp_board_find_periph_desc(name);
    ESP_BOARD_RETURN_ON_PERIPH_NOT_FOUND(desc, name, TAG, "Peripheral %s not found", name);

    if (periph_is_active(name)) {
        return ESP_ERR_INVALID_STATE;
    }

    return map_runtime_err_to_periph(esp_board_runtime_set_ops(name, (void *)init, (void *)deinit));
}

esp_err_t esp_board_periph_restore_ops(const char *name)
{
    ESP_BOARD_RETURN_ON_FALSE(name, ESP_BOARD_ERR_PERIPH_INVALID_ARG, TAG, "name is null");
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before peripheral ops restore");

    const esp_board_periph_desc_t *desc = esp_board_find_periph_desc(name);
    ESP_BOARD_RETURN_ON_PERIPH_NOT_FOUND(desc, name, TAG, "Peripheral %s not found", name);

    if (periph_is_active(name)) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_board_runtime_clear_app_ops(name);
    if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "Ops override for peripheral %s not found", name);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_BOARD_RETURN_ON_ERROR(ret, TAG, "Failed to restore ops for peripheral: %s", name);
    ESP_LOGI(TAG, "Ops override reset for peripheral %s", name);
    return ESP_OK;
}

#if ESP_BOARD_LEGACY_API_WARN
static void warn_legacy_init_custom_once(void)
{
    static bool s_warned;
    if (!s_warned) {
        s_warned = true;
        ESP_LOGW(TAG, "esp_board_periph_init_custom is legacy: updates shared (type,role) "
                      "handler and inits immediately; prefer board pre-init hook or "
                      "esp_board_periph_set_ops");
    }
}
#endif  /* ESP_BOARD_LEGACY_API_WARN */

esp_err_t esp_board_periph_init_custom(const char *name, esp_board_periph_init_func init, esp_board_periph_deinit_func deinit)
{
    ESP_BOARD_RETURN_ON_FALSE(name && init && deinit, ESP_BOARD_ERR_PERIPH_INVALID_ARG, TAG, "Invalid args");

#if ESP_BOARD_LEGACY_API_WARN
    warn_legacy_init_custom_once();
#endif  /* ESP_BOARD_LEGACY_API_WARN */

    esp_err_t prep = esp_board_manager_prepare();
    ESP_BOARD_RETURN_ON_ERROR(prep, TAG, "Board prepare failed before custom periph init");

    /* Find peripheral descriptor */
    const esp_board_periph_desc_t *desc = esp_board_find_periph_desc(name);
    ESP_BOARD_RETURN_ON_PERIPH_NOT_FOUND(desc, name, TAG, "Peripheral %s not found", name);

    /* Find handle */
    esp_board_periph_entry_t *handle = esp_board_find_periph_handle(desc->type, desc->role);
    ESP_BOARD_RETURN_ON_FALSE(handle, ESP_BOARD_ERR_PERIPH_NO_HANDLE, TAG,
                              "No handle found for %s (type=%s, role=%d)", name, desc->type, desc->role);

    /* Find or create list entry */
    esp_board_periph_list_t *list = find_periph_list(name);
    if (!list) {
        list = add_periph_list(name, desc->type, desc->role);
        ESP_BOARD_RETURN_ON_FALSE(list, ESP_ERR_NO_MEM, TAG, "Failed to create list entry for %s", name);
    }

    /* Set functions (legacy: still writes the shared type/role handler) */
    handle->init = init;
    handle->deinit = deinit;

    /* Initialize the peripheral if not already initialized */
    if (list->ref_count == 0) {
        esp_err_t ret = esp_board_runtime_lock();
        if (ret != ESP_OK) {
            handle->init = NULL;
            handle->deinit = NULL;
            return ret;
        }
        if (list->ref_count > 0) {
            list->ref_count++;
            esp_board_runtime_unlock();
            return ESP_OK;
        }
        if (esp_board_runtime_is_initialized_unlocked(name)) {
            esp_board_runtime_unlock();
            handle->init = NULL;
            handle->deinit = NULL;
            return ESP_ERR_INVALID_STATE;
        }

        /**
         * Mark initialized under the same mutex as override/restore before calling the
         * driver, so another task cannot free an override config for this name.
         * Do not hold the mutex across driver init.
         */
        ret = esp_board_runtime_set_initialized_unlocked(name, true);
        if (ret != ESP_OK) {
            esp_board_runtime_unlock();
            handle->init = NULL;
            handle->deinit = NULL;
            ESP_LOGE(TAG, "Failed to mark custom periph initialized: %s", name);
            return ret;
        }
        esp_board_runtime_unlock();

        ret = handle->init((void *)desc->cfg, desc->cfg_size, &list->periph_handle);
        if (ret != ESP_OK) {
            (void)esp_board_runtime_set_initialized(name, false);
            list->periph_handle = NULL;
            ESP_LOGE(TAG, "Failed to init periph: %s", name);
            /* Reset functions on failure */
            handle->init = NULL;
            handle->deinit = NULL;
            return ESP_BOARD_ERR_PERIPH_INIT_FAILED;
        }

        ret = esp_board_runtime_set_bound_deinit(name, (void *)deinit);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to snapshot deinit for custom periph: %s", name);
            (void)deinit(list->periph_handle);
            list->periph_handle = NULL;
            (void)esp_board_runtime_set_initialized(name, false);
            handle->init = NULL;
            handle->deinit = NULL;
            return ret;
        }

        list->ref_count = 1;
        ESP_LOGI(TAG, "Custom initialized periph: %s, handle: %p", name, list->periph_handle);
    }

    return ESP_OK;
}

esp_err_t esp_board_periph_deinit(const char *name)
{
    ESP_BOARD_RETURN_ON_FALSE(name, ESP_BOARD_ERR_PERIPH_INVALID_ARG, TAG, "name is null");

    /* Find peripheral descriptor */
    const esp_board_periph_desc_t *desc = esp_board_find_periph_desc(name);
    ESP_BOARD_RETURN_ON_PERIPH_NOT_FOUND(desc, name, TAG, "Peripheral %s not found", name);

    /* Find list entry */
    esp_board_periph_list_t *list = find_periph_list(name);
    ESP_BOARD_RETURN_ON_FALSE(list, ESP_BOARD_ERR_PERIPH_NO_HANDLE, TAG, "No list entry found for %s", name);

    /* Check if peripheral is initialized */
    if (!list->periph_handle) {
        ESP_LOGW(TAG, "Peripheral %s not initialized", name);
        return ESP_OK;
    }

    uint8_t old_ref_count = list->ref_count;

    /* Decrement reference count */
    list->ref_count--;
    ESP_LOGI(TAG, "Deinit peripheral %s ref_count: %d", name, list->ref_count);

    /* Only deinitialize if ref_count reaches 0 */
    if (list->ref_count == 0) {
        /* Use deinit snapshotted at 0->1 init; do not re-resolve ops */
        esp_board_periph_deinit_func bound_deinit =
            (esp_board_periph_deinit_func)esp_board_runtime_get_bound_deinit(name);
        if (!bound_deinit) {
            list->ref_count = old_ref_count;
            ESP_LOGE(TAG, "No deinit function for periph: %s", name);
            return ESP_BOARD_ERR_PERIPH_NO_INIT;
        }

        esp_err_t ret = bound_deinit(list->periph_handle);
        if (ret != ESP_OK) {
            list->ref_count = old_ref_count;
            ESP_LOGE(TAG, "Failed to deinit periph: %s", name);
            return ret;
        }

        list->periph_handle = NULL;
        (void)esp_board_runtime_set_initialized(name, false);
    } else {
        ESP_LOGW(TAG, "Peripheral %s still has %d references, not deinitializing", name, list->ref_count);
    }

    return ESP_OK;
}

esp_err_t esp_board_periph_show(const char *name)
{
    esp_err_t prepare_ret = ensure_board_prepared();
    ESP_BOARD_RETURN_ON_ERROR(prepare_ret, TAG, "Board prepare failed before peripheral show");
    if (name) {
        /* Show information for specific peripheral */
        const esp_board_periph_desc_t *desc = esp_board_find_periph_desc(name);
        ESP_BOARD_RETURN_ON_PERIPH_NOT_FOUND(desc, name, TAG, "Peripheral %s not found", name);

        esp_board_periph_list_t *list = find_periph_list(name);
        ESP_LOGI(TAG, "Peripheral %s:", name);
        ESP_LOGI(TAG, "  Type: %s", desc->type);
        ESP_LOGI(TAG, "  Role: %s", role_names_map[desc->role]);
        ESP_LOGI(TAG, "  Format: %s", desc->format ? desc->format : "N/A");
        ESP_LOGI(TAG, "  Config size: %d", desc->cfg_size);
        ESP_LOGI(TAG, "  ID: %d", desc->id);
        if (list) {
            ESP_LOGI(TAG, "  Handle: %p", list->periph_handle);
            ESP_LOGI(TAG, "  Status: %s", list->periph_handle ? "Initialized" : "Not initialized");
            ESP_LOGI(TAG, "  Ref count: %d", list->ref_count);
        } else {
            ESP_LOGI(TAG, "  No list entry found");
        }
    } else {
        /* Show information for all peripherals */
        const esp_board_periph_desc_t *desc = g_esp_board_peripherals;
        while (desc && desc->name) {
            esp_board_periph_list_t *list = find_periph_list(desc->name);
            ESP_LOGI(TAG, "Peripheral %s:", desc->name);
            ESP_LOGI(TAG, "  Type: %s", desc->type);
            ESP_LOGI(TAG, "  Role: %s", role_names_map[desc->role]);
            ESP_LOGI(TAG, "  Format: %s", desc->format ? desc->format : "N/A");
            ESP_LOGI(TAG, "  Config size: %d", desc->cfg_size);
            ESP_LOGI(TAG, "  ID: %d", desc->id);
            if (list) {
                ESP_LOGI(TAG, "  Handle: %p", list->periph_handle);
                ESP_LOGI(TAG, "  Status: %s", list->periph_handle ? "Initialized" : "Not initialized");
                ESP_LOGI(TAG, "  Ref count: %d", list->ref_count);
            } else {
                ESP_LOGI(TAG, "  No list entry found");
            }
            desc = desc->next;
        }
    }
    return ESP_OK;
}

esp_err_t esp_board_periph_init_all(void)
{
    const esp_board_periph_desc_t *desc = g_esp_board_peripherals;
    esp_err_t ret;

    /* Initialize all peripherals in the list */
    while (desc && desc->name) {
        if (desc->init_skip) {
            ESP_LOGD(TAG, "Skipping initialization of peripheral: %s (init_skip=true)", desc->name);
            desc = desc->next;
            continue;
        }

        ret = esp_board_periph_init(desc->name);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize peripheral: %s", desc->name);
        }
        desc = desc->next;
    }
    return ESP_OK;
}

esp_err_t esp_board_periph_deinit_all(void)
{
    const esp_board_periph_desc_t *desc = g_esp_board_peripherals;
    esp_err_t ret;

    /* Deinitialize all peripherals in the list */
    while (desc && desc->name) {
        if (desc->init_skip && find_periph_list(desc->name) == NULL) {
            desc = desc->next;
            continue;
        }

        ret = esp_board_periph_deinit(desc->name);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to deinitialize peripheral: %s", desc->name);
        }
        desc = desc->next;
    }
    return ESP_OK;
}
