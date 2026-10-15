/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_board_manager.h"
#include "esp_board_manager_err.h"
#include "esp_board_find_utils.h"
#include "esp_board_runtime_state.h"

static const char *TAG = "BOARD_RUNTIME";

typedef struct esp_board_runtime_init_mark {
    struct esp_board_runtime_init_mark *next;
    const char                         *name;          /*!< Points at descriptor name (flash) */
    void                               *bound_deinit;  /*!< Deinit snapshotted after successful 0->1 init */
} esp_board_runtime_init_mark_t;

static esp_board_runtime_prepare_state_t s_prepare_state = ESP_BOARD_RUNTIME_UNPREPARED;
static TaskHandle_t s_prepare_owner_task = NULL;
static StaticSemaphore_t s_runtime_mutex_buf;
static SemaphoreHandle_t s_runtime_mutex = NULL;
static portMUX_TYPE s_runtime_mutex_init_mux = portMUX_INITIALIZER_UNLOCKED;
static esp_board_runtime_node_t *s_runtime_nodes = NULL;
static esp_board_runtime_init_mark_t *s_init_marks = NULL;

#ifndef CONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM
#define CONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM  256
#endif  /* CONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM */
#ifndef CONFIG_ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM
#define CONFIG_ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM  256
#endif  /* CONFIG_ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM */

extern esp_board_device_handle_t g_esp_board_device_handles[];
extern esp_board_periph_entry_t g_esp_board_periph_handles[];

typedef struct {
    esp_board_device_desc_t *entry;
    esp_board_device_desc_t  value;
} device_table_snapshot_entry_t;

typedef struct {
    esp_board_periph_desc_t *entry;
    esp_board_periph_desc_t  value;
} periph_table_snapshot_entry_t;

typedef struct {
    device_table_snapshot_entry_t *entries;
    size_t                         count;
} device_table_snapshot_t;

typedef struct {
    periph_table_snapshot_entry_t *entries;
    size_t                         count;
} periph_table_snapshot_t;

typedef struct {
    esp_board_device_handle_t *entry;
    esp_board_device_handle_t  value;
} device_handle_snapshot_entry_t;

typedef struct {
    esp_board_periph_entry_t *entry;
    esp_board_periph_entry_t  value;
} periph_ops_snapshot_entry_t;

typedef struct {
    device_handle_snapshot_entry_t *entries;
    size_t                          count;
} device_handle_snapshot_t;

typedef struct {
    periph_ops_snapshot_entry_t *entries;
    size_t                       count;
} periph_ops_snapshot_t;

static esp_err_t snapshot_device_handle_links(device_handle_snapshot_t *snapshot)
{
    size_t capacity = (size_t)CONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM + 1;
    snapshot->entries = calloc(capacity, sizeof(*snapshot->entries));
    if (!snapshot->entries) {
        return ESP_ERR_NO_MEM;
    }
    esp_board_device_handle_t *entry = g_esp_board_device_handles;
    while (entry) {
        for (size_t i = 0; i < snapshot->count; ++i) {
            if (snapshot->entries[i].entry == entry) {
                free(snapshot->entries);
                snapshot->entries = NULL;
                snapshot->count = 0;
                return ESP_ERR_INVALID_STATE;
            }
        }
        if (snapshot->count >= capacity) {
            size_t new_capacity = capacity * 2;
            void *new_entries = realloc(snapshot->entries, new_capacity * sizeof(*snapshot->entries));
            if (!new_entries) {
                free(snapshot->entries);
                snapshot->entries = NULL;
                snapshot->count = 0;
                return ESP_ERR_NO_MEM;
            }
            snapshot->entries = new_entries;
            capacity = new_capacity;
        }
        snapshot->entries[snapshot->count++] =
            (device_handle_snapshot_entry_t) {.entry = entry, .value = *entry};
        if (!entry->name) {
            break;
        }
        entry = entry->next;
    }
    return ESP_OK;
}

static esp_err_t snapshot_periph_ops_links(periph_ops_snapshot_t *snapshot)
{
    size_t capacity = (size_t)CONFIG_ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM + 1;
    snapshot->entries = calloc(capacity, sizeof(*snapshot->entries));
    if (!snapshot->entries) {
        return ESP_ERR_NO_MEM;
    }
    esp_board_periph_entry_t *entry = g_esp_board_periph_handles;
    while (entry) {
        for (size_t i = 0; i < snapshot->count; ++i) {
            if (snapshot->entries[i].entry == entry) {
                free(snapshot->entries);
                snapshot->entries = NULL;
                snapshot->count = 0;
                return ESP_ERR_INVALID_STATE;
            }
        }
        if (snapshot->count >= capacity) {
            size_t new_capacity = capacity * 2;
            void *new_entries = realloc(snapshot->entries, new_capacity * sizeof(*snapshot->entries));
            if (!new_entries) {
                free(snapshot->entries);
                snapshot->entries = NULL;
                snapshot->count = 0;
                return ESP_ERR_NO_MEM;
            }
            snapshot->entries = new_entries;
            capacity = new_capacity;
        }
        snapshot->entries[snapshot->count++] =
            (periph_ops_snapshot_entry_t) {.entry = entry, .value = *entry};
        if (!entry->type) {
            break;
        }
        entry = entry->next;
    }
    return ESP_OK;
}

static void discard_handle_snapshots(device_handle_snapshot_t *devices,
                                     periph_ops_snapshot_t *peripherals, bool restore)
{
    if (restore) {
        for (size_t i = 0; i < devices->count; ++i) {
            esp_board_device_handle_t *entry = devices->entries[i].entry;
            bool active = devices->entries[i].value.name && esp_board_runtime_is_initialized_unlocked(devices->entries[i].value.name);
            void *active_handle = entry->device_handle;
            uint8_t active_ref_count = entry->ref_count;
            *entry = devices->entries[i].value;
            if (active) {
                entry->device_handle = active_handle;
                entry->ref_count = active_ref_count;
            }
        }
        for (size_t i = 0; i < peripherals->count; ++i) {
            *peripherals->entries[i].entry = peripherals->entries[i].value;
        }
    }
    free(devices->entries);
    free(peripherals->entries);
}

static esp_err_t snapshot_device_table(device_table_snapshot_t *snapshot)
{
    size_t capacity = (size_t)CONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM + 1;
    snapshot->entries = calloc(capacity, sizeof(*snapshot->entries));
    if (snapshot->entries == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_board_device_desc_t *entry = (esp_board_device_desc_t *)g_esp_board_devices;
    while (entry != NULL) {
        if (snapshot->count >= capacity) {
            free(snapshot->entries);
            snapshot->entries = NULL;
            snapshot->count = 0;
            return ESP_ERR_INVALID_STATE;
        }
        snapshot->entries[snapshot->count].entry = entry;
        snapshot->entries[snapshot->count].value = *entry;
        snapshot->count++;
        if (entry->name == NULL) {
            return ESP_OK;
        }
        if (snapshot->count > CONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM) {
            free(snapshot->entries);
            snapshot->entries = NULL;
            snapshot->count = 0;
            return ESP_ERR_INVALID_STATE;
        }
        entry = (esp_board_device_desc_t *)entry->next;
    }
    return ESP_OK;
}

static esp_err_t snapshot_periph_table(periph_table_snapshot_t *snapshot)
{
    size_t capacity = (size_t)CONFIG_ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM + 1;
    snapshot->entries = calloc(capacity, sizeof(*snapshot->entries));
    if (snapshot->entries == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_board_periph_desc_t *entry = (esp_board_periph_desc_t *)g_esp_board_peripherals;
    while (entry != NULL) {
        if (snapshot->count >= capacity) {
            free(snapshot->entries);
            snapshot->entries = NULL;
            snapshot->count = 0;
            return ESP_ERR_INVALID_STATE;
        }
        snapshot->entries[snapshot->count].entry = entry;
        snapshot->entries[snapshot->count].value = *entry;
        snapshot->count++;
        if (entry->name == NULL) {
            return ESP_OK;
        }
        if (snapshot->count > CONFIG_ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM) {
            free(snapshot->entries);
            snapshot->entries = NULL;
            snapshot->count = 0;
            return ESP_ERR_INVALID_STATE;
        }
        entry = (esp_board_periph_desc_t *)entry->next;
    }
    return ESP_OK;
}

static void restore_device_table(device_table_snapshot_t *snapshot)
{
    for (size_t i = 0; i < snapshot->count; ++i) {
        if (memcmp(snapshot->entries[i].entry, &snapshot->entries[i].value,
                   sizeof(snapshot->entries[i].value)) != 0) {
            *snapshot->entries[i].entry = snapshot->entries[i].value;
        }
    }
    free(snapshot->entries);
    snapshot->entries = NULL;
    snapshot->count = 0;
}

static void restore_periph_table(periph_table_snapshot_t *snapshot)
{
    for (size_t i = 0; i < snapshot->count; ++i) {
        if (memcmp(snapshot->entries[i].entry, &snapshot->entries[i].value,
                   sizeof(snapshot->entries[i].value)) != 0) {
            *snapshot->entries[i].entry = snapshot->entries[i].value;
        }
    }
    free(snapshot->entries);
    snapshot->entries = NULL;
    snapshot->count = 0;
}

static void discard_table_snapshots(device_table_snapshot_t *devices,
                                    periph_table_snapshot_t *peripherals)
{
    free(devices->entries);
    devices->entries = NULL;
    devices->count = 0;
    free(peripherals->entries);
    peripherals->entries = NULL;
    peripherals->count = 0;
}

static void clear_layer(esp_board_runtime_layer_t *layer)
{
    if (layer == NULL) {
        return;
    }
    free(layer->cfg);
    layer->cfg = NULL;
    layer->cfg_size = 0;
    layer->has_cfg = false;
    layer->init_fn = NULL;
    layer->deinit_fn = NULL;
    layer->has_ops = false;
}

static bool layer_is_empty(const esp_board_runtime_layer_t *layer)
{
    return layer != NULL && !layer->has_cfg && !layer->has_ops;
}

static void free_node(esp_board_runtime_node_t *node)
{
    if (node == NULL) {
        return;
    }
    clear_layer(&node->board);
    clear_layer(&node->app);
    free(node);
}

static void remove_node_if_empty(esp_board_runtime_node_t **link)
{
    esp_board_runtime_node_t *node = *link;
    if (node == NULL) {
        return;
    }
    if (layer_is_empty(&node->board) && layer_is_empty(&node->app)) {
        *link = node->next;
        free_node(node);
    }
}

static esp_board_runtime_node_t **find_node_link(const char *name)
{
    esp_board_runtime_node_t **link = &s_runtime_nodes;
    while (*link) {
        if (strcmp((*link)->name, name) == 0) {
            return link;
        }
        link = &(*link)->next;
    }
    return NULL;
}

static esp_board_runtime_init_mark_t **find_init_mark_link(const char *name)
{
    esp_board_runtime_init_mark_t **link = &s_init_marks;
    while (*link) {
        if (strcmp((*link)->name, name) == 0) {
            return link;
        }
        link = &(*link)->next;
    }
    return NULL;
}

static esp_err_t resolve_descriptor_name(const char *name, const char **out_name,
                                         bool *is_device, esp_err_t *not_found_err)
{
    const esp_board_device_desc_t *dev = esp_board_find_device_desc(name);
    if (dev != NULL) {
        *out_name = dev->name;
        *is_device = true;
        *not_found_err = ESP_BOARD_ERR_DEVICE_NOT_FOUND;
        return ESP_OK;
    }

    const esp_board_periph_desc_t *periph = esp_board_find_periph_desc(name);
    if (periph != NULL) {
        *out_name = periph->name;
        *is_device = false;
        *not_found_err = ESP_BOARD_ERR_PERIPH_NOT_FOUND;
        return ESP_OK;
    }

    *out_name = NULL;
    *is_device = false;
    *not_found_err = ESP_BOARD_ERR_DEVICE_NOT_FOUND;
    return ESP_BOARD_ERR_DEVICE_NOT_FOUND;
}

static esp_err_t check_has_generated_config(const char *name, bool is_device)
{
    if (is_device) {
        const esp_board_device_desc_t *desc = esp_board_find_device_desc(name);
        if (desc == NULL) {
            return ESP_BOARD_ERR_DEVICE_NOT_FOUND;
        }
        if (desc->cfg == NULL || desc->cfg_size == 0) {
            return ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED;
        }
        return ESP_OK;
    }

    const esp_board_periph_desc_t *desc = esp_board_find_periph_desc(name);
    if (desc == NULL) {
        return ESP_BOARD_ERR_PERIPH_NOT_FOUND;
    }
    if (desc->cfg == NULL || desc->cfg_size == 0) {
        return ESP_BOARD_ERR_PERIPH_NOT_SUPPORTED;
    }
    return ESP_OK;
}

static esp_board_runtime_layer_t *layer_for_source(esp_board_runtime_node_t *node,
                                                   esp_board_runtime_source_t source)
{
    if (source == ESP_BOARD_RUNTIME_SOURCE_BOARD) {
        return &node->board;
    }
    return &node->app;
}

esp_board_runtime_source_t esp_board_runtime_get_override_source(void)
{
    /* BOARD only while PREPARING and the current task is the prepare owner. */
    if (s_prepare_state == ESP_BOARD_RUNTIME_PREPARING &&
        xTaskGetCurrentTaskHandle() == s_prepare_owner_task) {
        return ESP_BOARD_RUNTIME_SOURCE_BOARD;
    }
    return ESP_BOARD_RUNTIME_SOURCE_APP;
}

esp_board_runtime_prepare_state_t esp_board_runtime_get_prepare_state(void)
{
    return s_prepare_state;
}

/**
 * @brief  Take the process-wide runtime mutex
 *
 *         The mutex is created once via StaticSemaphore_t under a port critical
 *         section so two cores cannot publish different handles. Not recursive.
 *         Never hold this lock across esp_board_runtime_pre_init() or driver init.
 */
esp_err_t esp_board_runtime_lock(void)
{
    if (s_runtime_mutex == NULL) {
        portENTER_CRITICAL(&s_runtime_mutex_init_mux);
        if (s_runtime_mutex == NULL) {
            s_runtime_mutex = xSemaphoreCreateMutexStatic(&s_runtime_mutex_buf);
        }
        portEXIT_CRITICAL(&s_runtime_mutex_init_mux);
        if (s_runtime_mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }
    if (xSemaphoreTake(s_runtime_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

void esp_board_runtime_unlock(void)
{
    (void)xSemaphoreGive(s_runtime_mutex);
}

/**
 * @brief  During PREPARING, only the prepare owner may write override/set_ops/restore
 *
 *         Caller must hold s_runtime_mutex. Outside PREPARING every caller is allowed
 *         (subject to initialized checks).
 */
static esp_err_t ensure_override_writable(void)
{
    if (s_prepare_state == ESP_BOARD_RUNTIME_PREPARING) {
        if (xTaskGetCurrentTaskHandle() != s_prepare_owner_task) {
            return ESP_ERR_INVALID_STATE;
        }
    }
    return ESP_OK;
}

esp_err_t esp_board_runtime_check_unique_names(const esp_board_device_desc_t *devices,
                                               const esp_board_periph_desc_t *peripherals)
{
    size_t count = 0;
    for (const esp_board_device_desc_t *outer = devices; outer && outer->name; outer = outer->next) {
        if (++count > CONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM) {
            ESP_LOGE(TAG, "Device descriptor list exceeds its generated limit or contains a cycle");
            return ESP_ERR_INVALID_STATE;
        }
        size_t inner_count = 0;
        for (const esp_board_device_desc_t *inner = outer->next; inner && inner->name; inner = inner->next) {
            if (inner == outer || ++inner_count > CONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM) {
                ESP_LOGE(TAG, "Device descriptor list contains a cycle");
                return ESP_ERR_INVALID_STATE;
            }
            if (strcmp(outer->name, inner->name) == 0) {
                ESP_LOGE(TAG, "Duplicate device descriptor name: %s", outer->name);
                return ESP_ERR_INVALID_STATE;
            }
        }
    }

    count = 0;
    for (const esp_board_periph_desc_t *outer = peripherals; outer && outer->name; outer = outer->next) {
        if (++count > CONFIG_ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM) {
            ESP_LOGE(TAG, "Peripheral descriptor list exceeds its generated limit or contains a cycle");
            return ESP_ERR_INVALID_STATE;
        }
        size_t inner_count = 0;
        for (const esp_board_periph_desc_t *inner = outer->next; inner && inner->name; inner = inner->next) {
            if (inner == outer || ++inner_count > CONFIG_ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM) {
                ESP_LOGE(TAG, "Peripheral descriptor list contains a cycle");
                return ESP_ERR_INVALID_STATE;
            }
            if (strcmp(outer->name, inner->name) == 0) {
                ESP_LOGE(TAG, "Duplicate peripheral descriptor name: %s", outer->name);
                return ESP_ERR_INVALID_STATE;
            }
        }
    }

    return ESP_OK;
}

static const esp_board_device_desc_t *find_device_desc_bounded(const char *name)
{
    const esp_board_device_desc_t *desc = g_esp_board_devices;
    for (size_t i = 0; desc && desc->name && i < CONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM; ++i) {
        if (strcmp(desc->name, name) == 0) {
            return desc;
        }
        desc = desc->next;
    }
    return NULL;
}

static const esp_board_device_handle_t *find_device_handle_bounded(const char *name)
{
    const esp_board_device_handle_t *handle = g_esp_board_device_handles;
    const esp_board_device_handle_t *slow = handle;
    const esp_board_device_handle_t *fast = handle;
    while (handle && handle->name) {
        if (strcmp(handle->name, name) == 0) {
            return handle;
        }
        handle = handle->next;
        slow = slow ? slow->next : NULL;
        fast = fast && fast->next ? fast->next->next : NULL;
        if (slow && fast && slow == fast) {
            return NULL;
        }
    }
    return NULL;
}

static const esp_board_periph_entry_t *find_periph_handle_bounded(const char *type,
                                                                  esp_board_periph_role_t role)
{
    const esp_board_periph_entry_t *entry = g_esp_board_periph_handles;
    const esp_board_periph_entry_t *slow = entry;
    const esp_board_periph_entry_t *fast = entry;
    while (entry && entry->type) {
        if (entry->role == role && strcmp(entry->type, type) == 0) {
            return entry;
        }
        entry = entry->next;
        slow = slow ? slow->next : NULL;
        fast = fast && fast->next ? fast->next->next : NULL;
        if (slow && fast && slow == fast) {
            return NULL;
        }
    }
    return NULL;
}

static esp_err_t validate_handle_tables(void)
{
    const esp_board_device_handle_t *slow = g_esp_board_device_handles;
    const esp_board_device_handle_t *fast = g_esp_board_device_handles;
    while (fast && fast->name && fast->next && fast->next->name) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) {
            ESP_LOGE(TAG, "Device handle list contains a cycle");
            return ESP_ERR_INVALID_STATE;
        }
    }
    for (const esp_board_device_handle_t *outer = g_esp_board_device_handles;
         outer && outer->name; outer = outer->next) {
        for (const esp_board_device_handle_t *inner = outer->next;
             inner && inner->name; inner = inner->next) {
            if (strcmp(outer->name, inner->name) == 0) {
                ESP_LOGE(TAG, "Device handle list contains a cycle or duplicate name");
                return ESP_ERR_INVALID_STATE;
            }
        }
    }

    const esp_board_periph_entry_t *periph_slow = g_esp_board_periph_handles;
    const esp_board_periph_entry_t *periph_fast = g_esp_board_periph_handles;
    while (periph_fast && periph_fast->type && periph_fast->next && periph_fast->next->type) {
        periph_slow = periph_slow->next;
        periph_fast = periph_fast->next->next;
        if (periph_slow == periph_fast) {
            ESP_LOGE(TAG, "Peripheral ops list contains a cycle");
            return ESP_ERR_INVALID_STATE;
        }
    }
    for (const esp_board_periph_entry_t *outer = g_esp_board_periph_handles;
         outer && outer->type; outer = outer->next) {
        for (const esp_board_periph_entry_t *inner = outer->next;
             inner && inner->type; inner = inner->next) {
            if (outer->role == inner->role && strcmp(outer->type, inner->type) == 0) {
                ESP_LOGE(TAG, "Peripheral ops list contains a cycle or duplicate type/role");
                return ESP_ERR_INVALID_STATE;
            }
        }
    }
    return ESP_OK;
}

static bool device_name_exists_bounded(const char *name)
{
    return find_device_desc_bounded(name) != NULL;
}

static esp_err_t validate_device_dependency_graph(size_t device_count)
{
    if (device_count == 0) {
        return ESP_OK;
    }

    const esp_board_device_desc_t **devices = calloc(device_count, sizeof(*devices));
    uint8_t *states = calloc(device_count, sizeof(*states));
    size_t *stack = calloc(device_count, sizeof(*stack));
    size_t *next_dependencies = calloc(device_count, sizeof(*next_dependencies));
    if (devices == NULL || states == NULL || stack == NULL || next_dependencies == NULL) {
        free(devices);
        free(states);
        free(stack);
        free(next_dependencies);
        return ESP_ERR_NO_MEM;
    }

    size_t index = 0;
    for (const esp_board_device_desc_t *desc = g_esp_board_devices;
         desc && desc->name && index < device_count; desc = desc->next) {
        devices[index++] = desc;
    }
    if (index != device_count) {
        free(devices);
        free(states);
        free(stack);
        free(next_dependencies);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = ESP_OK;
    for (size_t root = 0; root < device_count && ret == ESP_OK; ++root) {
        if (states[root] != 0) {
            continue;
        }

        size_t depth = 0;
        stack[depth] = root;
        next_dependencies[depth] = 0;
        states[root] = 1;
        while (true) {
            size_t current_index = stack[depth];
            const esp_board_device_desc_t *current = devices[current_index];
            if (next_dependencies[depth] >= current->depends_on_num) {
                states[current_index] = 2;
                if (depth == 0) {
                    break;
                }
                depth--;
                continue;
            }

            const char *dependency_name = current->depends_on[next_dependencies[depth]++];
            size_t dependency_index = 0;
            while (dependency_index < device_count &&
                   strcmp(devices[dependency_index]->name, dependency_name) != 0) {
                dependency_index++;
            }
            if (dependency_index == device_count || states[dependency_index] == 1) {
                ESP_LOGE(TAG, "Device dependency cycle detected at %s -> %s",
                         current->name, dependency_name);
                ret = ESP_ERR_INVALID_STATE;
                break;
            }
            if (states[dependency_index] == 0) {
                depth++;
                stack[depth] = dependency_index;
                next_dependencies[depth] = 0;
                states[dependency_index] = 1;
            }
        }
    }

    free(devices);
    free(states);
    free(stack);
    free(next_dependencies);
    return ret;
}

esp_err_t esp_board_runtime_preflight(void)
{
    esp_err_t ret = esp_board_runtime_check_unique_names(g_esp_board_devices, g_esp_board_peripherals);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = validate_handle_tables();
    if (ret != ESP_OK) {
        return ret;
    }

    size_t device_count = 0;
    for (const esp_board_device_desc_t *desc = g_esp_board_devices;
         desc && desc->name && device_count < CONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM;
         desc = desc->next, ++device_count) {
        const esp_board_device_handle_t *handle = find_device_handle_bounded(desc->name);
        if (!desc->type || !handle || !handle->type || strcmp(desc->type, handle->type) != 0) {
            ESP_LOGE(TAG, "Device descriptor %s has no matching handle", desc->name);
            return ESP_ERR_INVALID_STATE;
        }
        if (desc->power_ctrl_device && !device_name_exists_bounded(desc->power_ctrl_device)) {
            ESP_LOGE(TAG, "Device %s references missing power control device %s",
                     desc->name, desc->power_ctrl_device);
            return ESP_ERR_INVALID_STATE;
        }
        if (desc->depends_on_num > 0 && !desc->depends_on) {
            ESP_LOGE(TAG, "Device %s has an invalid dependency list", desc->name);
            return ESP_ERR_INVALID_STATE;
        }
        for (size_t i = 0; i < desc->depends_on_num; ++i) {
            if (!desc->depends_on[i] || !device_name_exists_bounded(desc->depends_on[i])) {
                ESP_LOGE(TAG, "Device %s references a missing dependency", desc->name);
                return ESP_ERR_INVALID_STATE;
            }
        }
    }
    ret = validate_device_dependency_graph(device_count);
    if (ret != ESP_OK) {
        return ret;
    }
    size_t periph_count = 0;
    for (const esp_board_periph_desc_t *desc = g_esp_board_peripherals;
         desc && desc->name && periph_count < CONFIG_ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM;
         desc = desc->next, ++periph_count) {
        const esp_board_periph_entry_t *shared = desc->type                                                    ?
                                                            find_periph_handle_bounded(desc->type, desc->role) : NULL;
        const esp_board_runtime_node_t *node = esp_board_runtime_find_node(desc->name);
        bool complete_ops = shared && shared->init && shared->deinit;
        if (node && node->board.has_ops) {
            complete_ops = node->board.init_fn && node->board.deinit_fn;
        }
        if (node && node->app.has_ops) {
            complete_ops = node->app.init_fn && node->app.deinit_fn;
        }
        if (!desc->type || !complete_ops) {
            ESP_LOGE(TAG, "Peripheral descriptor %s has no matching type/role handle", desc->name);
            return ESP_ERR_INVALID_STATE;
        }
    }
    return ESP_OK;
}

esp_err_t esp_board_runtime_prepare(void)
{
    esp_err_t lock_ret = esp_board_runtime_lock();
    if (lock_ret != ESP_OK) {
        return lock_ret;
    }

    if (s_prepare_state == ESP_BOARD_RUNTIME_PREPARED) {
        esp_board_runtime_unlock();
        return ESP_OK;
    }
    if (s_prepare_state == ESP_BOARD_RUNTIME_PREPARING) {
        bool is_owner = xTaskGetCurrentTaskHandle() == s_prepare_owner_task;
        esp_board_runtime_unlock();
        return is_owner ? ESP_OK : ESP_ERR_INVALID_STATE;
    }

    TaskHandle_t self = xTaskGetCurrentTaskHandle();
    if (self == NULL) {
        esp_board_runtime_unlock();
        return ESP_ERR_INVALID_STATE;
    }

    s_prepare_state = ESP_BOARD_RUNTIME_PREPARING;
    s_prepare_owner_task = self;
    esp_board_runtime_unlock();

    device_table_snapshot_t device_snapshot = {0};
    periph_table_snapshot_t periph_snapshot = {0};
    device_handle_snapshot_t device_handles_snapshot = {0};
    periph_ops_snapshot_t periph_ops_snapshot = {0};
    esp_err_t ret = ESP_OK;
    ret = snapshot_device_table(&device_snapshot);
    if (ret == ESP_OK) {
        ret = snapshot_periph_table(&periph_snapshot);
    }
    if (ret == ESP_OK) {
        ret = snapshot_device_handle_links(&device_handles_snapshot);
    }
    if (ret == ESP_OK) {
        ret = snapshot_periph_ops_links(&periph_ops_snapshot);
    }
    if (ret != ESP_OK) {
        discard_table_snapshots(&device_snapshot, &periph_snapshot);
        discard_handle_snapshots(&device_handles_snapshot, &periph_ops_snapshot, false);
        if (esp_board_runtime_lock() == ESP_OK) {
            s_prepare_state = ESP_BOARD_RUNTIME_UNPREPARED;
            s_prepare_owner_task = NULL;
            esp_board_runtime_unlock();
        }
        return ret;
    }

    ret = esp_board_runtime_pre_init();

    lock_ret = esp_board_runtime_lock();
    if (lock_ret != ESP_OK) {
        /* Unexpected with a static mutex; leave a consistent terminal state. */
        if (ret == ESP_OK) {
            ret = lock_ret;
        }
        esp_board_runtime_rollback_board();
        restore_device_table(&device_snapshot);
        restore_periph_table(&periph_snapshot);
        discard_handle_snapshots(&device_handles_snapshot, &periph_ops_snapshot, true);
        s_prepare_state = ESP_BOARD_RUNTIME_UNPREPARED;
        s_prepare_owner_task = NULL;
        ESP_LOGE(TAG, "Board runtime prepare exit lock failed: %s", esp_err_to_name(lock_ret));
        return ret;
    }

    if (ret != ESP_OK) {
        /* Rollback ignores owner. Leave PREPARING only while owner is still set. */
        esp_board_runtime_rollback_board();
        restore_device_table(&device_snapshot);
        restore_periph_table(&periph_snapshot);
        discard_handle_snapshots(&device_handles_snapshot, &periph_ops_snapshot, true);
        s_prepare_state = ESP_BOARD_RUNTIME_UNPREPARED;
        s_prepare_owner_task = NULL;
        esp_board_runtime_unlock();
        ESP_LOGE(TAG, "Board runtime pre-init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_board_runtime_preflight();
    if (ret != ESP_OK) {
        esp_board_runtime_rollback_board();
        restore_device_table(&device_snapshot);
        restore_periph_table(&periph_snapshot);
        discard_handle_snapshots(&device_handles_snapshot, &periph_ops_snapshot, true);
        s_prepare_state = ESP_BOARD_RUNTIME_UNPREPARED;
        s_prepare_owner_task = NULL;
        esp_board_runtime_unlock();
        ESP_LOGE(TAG, "Board runtime table preflight failed: %s", esp_err_to_name(ret));
        return ret;
    }

    s_prepare_state = ESP_BOARD_RUNTIME_PREPARED;
    s_prepare_owner_task = NULL;
    discard_table_snapshots(&device_snapshot, &periph_snapshot);
    discard_handle_snapshots(&device_handles_snapshot, &periph_ops_snapshot, false);
    esp_board_runtime_unlock();
    return ESP_OK;
}

esp_board_runtime_node_t *esp_board_runtime_find_node(const char *name)
{
    if (name == NULL) {
        return NULL;
    }
    esp_board_runtime_node_t **link = find_node_link(name);
    return link ? *link : NULL;
}

esp_err_t esp_board_runtime_find_or_create_node(const char *name, esp_board_runtime_node_t **out_node)
{
    ESP_BOARD_RETURN_ON_FALSE(name && out_node, ESP_BOARD_ERR_MANAGER_INVALID_ARG, TAG, "Invalid args");

    const char *stable_name = NULL;
    bool is_device = false;
    esp_err_t not_found_err = ESP_BOARD_ERR_DEVICE_NOT_FOUND;
    esp_err_t resolve_ret = resolve_descriptor_name(name, &stable_name, &is_device, &not_found_err);
    if (resolve_ret != ESP_OK) {
        ESP_LOGE(TAG, "Resource %s not found in generated descriptors", name);
        return not_found_err;
    }

    esp_board_runtime_node_t **link = find_node_link(stable_name);
    if (link != NULL) {
        *out_node = *link;
        return ESP_OK;
    }

    esp_board_runtime_node_t *node = calloc(1, sizeof(*node));
    ESP_BOARD_RETURN_ON_FALSE(node, ESP_ERR_NO_MEM, TAG, "Failed to allocate runtime node for %s", name);
    node->name = stable_name;
    node->next = s_runtime_nodes;
    s_runtime_nodes = node;
    *out_node = node;
    return ESP_OK;
}

esp_err_t esp_board_runtime_copy_config(const char *name, const void *config, uint16_t config_size)
{
    ESP_BOARD_RETURN_ON_FALSE(name && config && config_size, ESP_BOARD_ERR_MANAGER_INVALID_ARG, TAG, "Invalid args");

    esp_err_t ret = esp_board_runtime_lock();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = ensure_override_writable();
    if (ret != ESP_OK) {
        goto out;
    }

    if (esp_board_runtime_is_initialized_unlocked(name)) {
        ret = ESP_ERR_INVALID_STATE;
        goto out;
    }

    const char *stable_name = NULL;
    bool is_device = false;
    esp_err_t not_found_err = ESP_BOARD_ERR_DEVICE_NOT_FOUND;
    esp_err_t resolve_ret = resolve_descriptor_name(name, &stable_name, &is_device, &not_found_err);
    if (resolve_ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot override unknown resource %s", name);
        ret = not_found_err;
        goto out;
    }

    esp_err_t cfg_ret = check_has_generated_config(stable_name, is_device);
    if (cfg_ret != ESP_OK) {
        ret = cfg_ret;
        goto out;
    }

    uint16_t generated_size = 0;
    if (is_device) {
        const esp_board_device_desc_t *desc = esp_board_find_device_desc(stable_name);
        generated_size = desc->cfg_size;
    } else {
        const esp_board_periph_desc_t *desc = esp_board_find_periph_desc(stable_name);
        generated_size = (uint16_t)desc->cfg_size;
    }
    if (config_size != generated_size) {
        ESP_LOGW(TAG, "%s %s override size differs from generated config size, default: %u, override: %u",
                 is_device ? "Device" : "Peripheral", name, generated_size, config_size);
    }

    esp_board_runtime_node_t *node = NULL;
    ret = esp_board_runtime_find_or_create_node(stable_name, &node);
    if (ret != ESP_OK) {
        goto out;
    }

    void *cfg_copy = malloc(config_size);
    if (cfg_copy == NULL) {
        esp_board_runtime_node_t **link = find_node_link(stable_name);
        if (link != NULL) {
            remove_node_if_empty(link);
        }
        ESP_LOGE(TAG, "Failed to allocate config copy for %s", name);
        ret = ESP_ERR_NO_MEM;
        goto out;
    }
    memcpy(cfg_copy, config, config_size);

    esp_board_runtime_layer_t *layer = layer_for_source(node, esp_board_runtime_get_override_source());
    free(layer->cfg);
    layer->cfg = cfg_copy;
    layer->cfg_size = config_size;
    layer->has_cfg = true;
    ret = ESP_OK;

out:
    esp_board_runtime_unlock();
    return ret;
}

esp_err_t esp_board_runtime_set_ops(const char *name, void *init_fn, void *deinit_fn)
{
    ESP_BOARD_RETURN_ON_FALSE(name && init_fn && deinit_fn, ESP_BOARD_ERR_MANAGER_INVALID_ARG, TAG, "Invalid args");

    esp_err_t ret = esp_board_runtime_lock();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = ensure_override_writable();
    if (ret != ESP_OK) {
        goto out;
    }

    if (esp_board_runtime_is_initialized_unlocked(name)) {
        ret = ESP_ERR_INVALID_STATE;
        goto out;
    }

    esp_board_runtime_node_t *node = NULL;
    ret = esp_board_runtime_find_or_create_node(name, &node);
    if (ret != ESP_OK) {
        goto out;
    }

    esp_board_runtime_layer_t *layer = layer_for_source(node, esp_board_runtime_get_override_source());
    layer->init_fn = init_fn;
    layer->deinit_fn = deinit_fn;
    layer->has_ops = true;
    ret = ESP_OK;

out:
    esp_board_runtime_unlock();
    return ret;
}

esp_err_t esp_board_runtime_clear_layer(const char *name, esp_board_runtime_source_t layer)
{
    ESP_BOARD_RETURN_ON_FALSE(name, ESP_BOARD_ERR_MANAGER_INVALID_ARG, TAG, "Invalid args");

    esp_board_runtime_node_t **link = find_node_link(name);
    if (link == NULL) {
        return ESP_OK;
    }

    clear_layer(layer_for_source(*link, layer));
    remove_node_if_empty(link);
    return ESP_OK;
}

esp_err_t esp_board_runtime_clear_app_config(const char *name)
{
    ESP_BOARD_RETURN_ON_FALSE(name, ESP_BOARD_ERR_MANAGER_INVALID_ARG, TAG, "Invalid args");

    esp_err_t ret = esp_board_runtime_lock();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = ensure_override_writable();
    if (ret != ESP_OK) {
        goto out;
    }

    if (esp_board_runtime_is_initialized_unlocked(name)) {
        ret = ESP_ERR_INVALID_STATE;
        goto out;
    }

    esp_board_runtime_node_t **link = find_node_link(name);
    if (link == NULL || !(*link)->app.has_cfg) {
        ret = ESP_ERR_NOT_FOUND;
        goto out;
    }

    free((*link)->app.cfg);
    (*link)->app.cfg = NULL;
    (*link)->app.cfg_size = 0;
    (*link)->app.has_cfg = false;
    remove_node_if_empty(link);
    ret = ESP_OK;

out:
    esp_board_runtime_unlock();
    return ret;
}

esp_err_t esp_board_runtime_clear_app_ops(const char *name)
{
    ESP_BOARD_RETURN_ON_FALSE(name, ESP_BOARD_ERR_MANAGER_INVALID_ARG, TAG, "Invalid args");

    esp_err_t ret = esp_board_runtime_lock();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = ensure_override_writable();
    if (ret != ESP_OK) {
        goto out;
    }

    if (esp_board_runtime_is_initialized_unlocked(name)) {
        ret = ESP_ERR_INVALID_STATE;
        goto out;
    }

    esp_board_runtime_node_t **link = find_node_link(name);
    if (link == NULL || !(*link)->app.has_ops) {
        ret = ESP_ERR_NOT_FOUND;
        goto out;
    }

    (*link)->app.init_fn = NULL;
    (*link)->app.deinit_fn = NULL;
    (*link)->app.has_ops = false;
    remove_node_if_empty(link);
    ret = ESP_OK;

out:
    esp_board_runtime_unlock();
    return ret;
}

void esp_board_runtime_rollback_board(void)
{
    esp_board_runtime_node_t **link = &s_runtime_nodes;
    while (*link) {
        /* A hook that failed to deinit a resource may still be using BOARD cfg. */
        if (esp_board_runtime_is_initialized_unlocked((*link)->name)) {
            link = &(*link)->next;
            continue;
        }
        clear_layer(&(*link)->board);
        if (layer_is_empty(&(*link)->board) && layer_is_empty(&(*link)->app)) {
            esp_board_runtime_node_t *dead = *link;
            *link = dead->next;
            free_node(dead);
            continue;
        }
        link = &(*link)->next;
    }
}

bool esp_board_runtime_is_initialized_unlocked(const char *name)
{
    if (name == NULL) {
        return false;
    }
    return find_init_mark_link(name) != NULL;
}

bool esp_board_runtime_is_initialized(const char *name)
{
    if (name == NULL) {
        return false;
    }
    if (esp_board_runtime_lock() != ESP_OK) {
        return false;
    }
    bool marked = esp_board_runtime_is_initialized_unlocked(name);
    esp_board_runtime_unlock();
    return marked;
}

esp_err_t esp_board_runtime_set_initialized_unlocked(const char *name, bool initialized)
{
    ESP_BOARD_RETURN_ON_FALSE(name, ESP_BOARD_ERR_MANAGER_INVALID_ARG, TAG, "Invalid args");

    const char *stable_name = NULL;
    bool is_device = false;
    esp_err_t not_found_err = ESP_BOARD_ERR_DEVICE_NOT_FOUND;
    esp_err_t resolve_ret = resolve_descriptor_name(name, &stable_name, &is_device, &not_found_err);
    if (resolve_ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot mark unknown resource %s", name);
        return not_found_err;
    }

    esp_board_runtime_init_mark_t **link = find_init_mark_link(stable_name);
    if (!initialized) {
        if (link != NULL) {
            esp_board_runtime_init_mark_t *dead = *link;
            *link = dead->next;
            free(dead);
        }
        return ESP_OK;
    }

    if (link != NULL) {
        return ESP_OK;
    }

    esp_board_runtime_init_mark_t *mark = calloc(1, sizeof(*mark));
    ESP_BOARD_RETURN_ON_FALSE(mark, ESP_ERR_NO_MEM, TAG, "Failed to allocate init mark for %s", name);
    mark->name = stable_name;
    mark->bound_deinit = NULL;
    mark->next = s_init_marks;
    s_init_marks = mark;
    return ESP_OK;
}

esp_err_t esp_board_runtime_set_initialized(const char *name, bool initialized)
{
    esp_err_t ret = esp_board_runtime_lock();
    if (ret != ESP_OK) {
        return ret;
    }
    ret = esp_board_runtime_set_initialized_unlocked(name, initialized);
    esp_board_runtime_unlock();
    return ret;
}

esp_err_t esp_board_runtime_set_bound_deinit(const char *name, void *deinit_fn)
{
    ESP_BOARD_RETURN_ON_FALSE(name && deinit_fn, ESP_BOARD_ERR_MANAGER_INVALID_ARG, TAG, "Invalid args");

    esp_err_t ret = esp_board_runtime_lock();
    if (ret != ESP_OK) {
        return ret;
    }

    esp_board_runtime_init_mark_t **link = find_init_mark_link(name);
    if (link == NULL) {
        ret = ESP_ERR_INVALID_STATE;
        goto out;
    }
    (*link)->bound_deinit = deinit_fn;
    ret = ESP_OK;

out:
    esp_board_runtime_unlock();
    return ret;
}

void *esp_board_runtime_get_bound_deinit(const char *name)
{
    if (name == NULL) {
        return NULL;
    }
    if (esp_board_runtime_lock() != ESP_OK) {
        return NULL;
    }
    void *fn = NULL;
    esp_board_runtime_init_mark_t **link = find_init_mark_link(name);
    if (link != NULL) {
        fn = (*link)->bound_deinit;
    }
    esp_board_runtime_unlock();
    return fn;
}

void esp_board_runtime_test_reset(void)
{
    esp_board_runtime_node_t *node = s_runtime_nodes;
    while (node) {
        esp_board_runtime_node_t *next = node->next;
        free_node(node);
        node = next;
    }
    s_runtime_nodes = NULL;

    esp_board_runtime_init_mark_t *mark = s_init_marks;
    while (mark) {
        esp_board_runtime_init_mark_t *next = mark->next;
        free(mark);
        mark = next;
    }
    s_init_marks = NULL;

    s_prepare_state = ESP_BOARD_RUNTIME_UNPREPARED;
    s_prepare_owner_task = NULL;
}
