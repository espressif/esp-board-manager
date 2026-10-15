/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_board_device.h"
#include "esp_board_periph.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Writable override source for the current call context
 *
 *         Default is APP. BOARD only while PREPARING and the current task is the
 *         prepare owner. Derived by esp_board_runtime_get_override_source().
 */
typedef enum {
    ESP_BOARD_RUNTIME_SOURCE_APP = 0,
    ESP_BOARD_RUNTIME_SOURCE_BOARD,
} esp_board_runtime_source_t;

/**
 * @brief  Manager prepare lifecycle state
 */
typedef enum {
    ESP_BOARD_RUNTIME_UNPREPARED = 0,
    ESP_BOARD_RUNTIME_PREPARING,
    ESP_BOARD_RUNTIME_PREPARED,
} esp_board_runtime_prepare_state_t;

/**
 * @brief  One config/ops layer (BOARD or APP) inside a sparse sidecar
 */
typedef struct {
    void     *cfg;        /*!< Heap copy of override config, or NULL */
    uint16_t  cfg_size;   /*!< Size of cfg in bytes */
    bool      has_cfg;    /*!< True when cfg is set for this layer */
    void     *init_fn;    /*!< Override init function pointer, or NULL */
    void     *deinit_fn;  /*!< Override deinit function pointer, or NULL */
    bool      has_ops;    /*!< True when ops are set for this layer */
} esp_board_runtime_layer_t;

/**
 * @brief  Sparse per-name runtime override sidecar
 *
 *         Allocated only when a BOARD/APP override or set_ops occurs.
 *         Initialized marks live in a separate sparse set and never create
 *         this sidecar or allocate config copies. Generated descriptors stay const.
 */
typedef struct esp_board_runtime_node {
    struct esp_board_runtime_node *next;   /*!< Next sparse sidecar node */
    const char                    *name;   /*!< Points at descriptor name (flash) */
    esp_board_runtime_layer_t      board;  /*!< BOARD layer */
    esp_board_runtime_layer_t      app;    /*!< APP layer */
} esp_board_runtime_node_t;

/**
 * @brief  Get the override layer selected for the current call context
 *
 *         Returns BOARD only for the prepare owner while the hook is running;
 *         all other calls use APP.
 */
esp_board_runtime_source_t esp_board_runtime_get_override_source(void);

/**
 * @brief  Get the current board prepare lifecycle state
 */
esp_board_runtime_prepare_state_t esp_board_runtime_get_prepare_state(void);

/**
 * @brief  Run prepare: hook under BOARD source for the owner task, then PREPARED
 *
 *         Idempotent when already PREPARED and for nested calls by the prepare
 *         owner while PREPARING. Other tasks get ESP_ERR_INVALID_STATE. Only the
 *         owner may write BOARD during the hook. Manager deinit does not return
 *         to UNPREPARED.
 */
esp_err_t esp_board_runtime_prepare(void);

/**
 * @brief  Reject duplicate names within each generated descriptor list
 *
 *         This is the generated-table wrapper around
 *         esp_board_runtime_check_unique_names().
 */
esp_err_t esp_board_runtime_preflight(void);

/**
 * @brief  Check names for duplicates within each descriptor list
 *
 *         Device and peripheral namespaces are checked independently. A NULL
 *         list is treated as empty; cross-list duplicates are not checked.
 *
 * @param[in]  devices      Device descriptor list, or NULL
 * @param[in]  peripherals  Peripheral descriptor list, or NULL
 *
 * @return
 *       - ESP_OK                 No duplicates found in either list.
 *       - ESP_ERR_INVALID_STATE  A name is repeated within one list.
 */
esp_err_t esp_board_runtime_check_unique_names(const esp_board_device_desc_t *devices,
                                               const esp_board_periph_desc_t *peripherals);

/**
 * @brief  Find a sparse runtime sidecar by resource name
 *
 *         The returned node is owned by the runtime state and must not be freed
 *         by the caller. The caller must hold the runtime lock when concurrent
 *         access is possible.
 *
 * @return
 *       - Runtime-owned  node, or NULL when name is NULL or no sidecar exists
 */
esp_board_runtime_node_t *esp_board_runtime_find_node(const char *name);

/**
 * @brief  Find or allocate a sparse runtime sidecar for a resource name
 *
 *         The caller must hold the runtime lock when concurrent access is possible.
 *
 * @param[in]   name      Device or peripheral name
 * @param[out]  out_node  Receives the existing or newly allocated node
 *
 * @return
 *       - ESP_OK                             Node returned in out_node.
 *       - ESP_ERR_NO_MEM                     Allocation failed.
 *       - ESP_BOARD_ERR_MANAGER_INVALID_ARG  name or out_node is NULL.
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND     Device name not found.
 *       - ESP_BOARD_ERR_PERIPH_NOT_FOUND     Peripheral name not found.
 */
esp_err_t esp_board_runtime_find_or_create_node(const char *name, esp_board_runtime_node_t **out_node);

/**
 * @brief  Copy config into the current writable source layer for an existing name
 *
 *         The config struct is copied at the top level. Nested pointers remain
 *         owned by the caller and must remain valid until the layer is cleared.
 */
esp_err_t esp_board_runtime_copy_config(const char *name, const void *config, uint16_t config_size);

/**
 * @brief  Store ops into the current writable source layer for an existing name
 */
esp_err_t esp_board_runtime_set_ops(const char *name, void *init_fn, void *deinit_fn);

/**
 * @brief  Clear one runtime override layer for a resource name
 *
 *         The caller must hold the runtime lock. This helper is used for
 *         prepare rollback and does not change the generated descriptors.
 *
 * @param[in]  name   Device or peripheral name
 * @param[in]  layer  BOARD or APP layer to clear
 */
esp_err_t esp_board_runtime_clear_layer(const char *name, esp_board_runtime_source_t layer);

/**
 * @brief  Clear only the APP config slot for a name (ops on that layer stay)
 *
 * @return
 *       - ESP_OK                             On success
 *       - ESP_ERR_NOT_FOUND                  If no APP config is present
 *       - ESP_BOARD_ERR_MANAGER_INVALID_ARG  If name is NULL
 */
esp_err_t esp_board_runtime_clear_app_config(const char *name);

/**
 * @brief  Clear only the APP ops slot for a name (config on that layer stays)
 *
 * @return
 *       - ESP_OK                             On success
 *       - ESP_ERR_NOT_FOUND                  If no APP ops are present
 *       - ESP_BOARD_ERR_MANAGER_INVALID_ARG  If name is NULL
 */
esp_err_t esp_board_runtime_clear_app_ops(const char *name);

/**
 * @brief  Roll back all BOARD-layer overrides written during prepare
 *
 *         The caller must hold the runtime lock. APP-layer overrides and
 *         generated descriptors are preserved.
 */
void esp_board_runtime_rollback_board(void);

/**
 * @brief  Check whether a resource is marked initialized
 */
bool esp_board_runtime_is_initialized(const char *name);

/**
 * @brief  Set or clear a resource's initialized mark
 *
 *         This locking wrapper is for callers that do not already hold the
 *         runtime mutex. Use the *_unlocked variant when the mutex is held.
 */
esp_err_t esp_board_runtime_set_initialized(const char *name, bool initialized);

/**
 * @brief  Take / release the process-wide runtime mutex
 *
 *         Same mutex as copy_config / clear_app_* / set_initialized.
 *         Not recursive: do not call another locking runtime API while held.
 *         Never hold across driver init or esp_board_runtime_pre_init().
 */
esp_err_t esp_board_runtime_lock(void);

/**
 * @brief  Release the process-wide runtime mutex held by the current task
 */
void esp_board_runtime_unlock(void);

/**
 * @brief  Check an initialized mark without taking the runtime mutex
 *
 *         The caller must already hold the runtime lock.
 */
bool esp_board_runtime_is_initialized_unlocked(const char *name);

/**
 * @brief  Set or clear an initialized mark without taking the runtime mutex
 *
 *         The caller must already hold the runtime lock.
 */
esp_err_t esp_board_runtime_set_initialized_unlocked(const char *name, bool initialized);

/**
 * @brief  Store the deinit function for the active resource instance
 *
 *         Capture after a successful 0->1 driver init. The snapshot is removed
 *         when the initialized mark is cleared on last-ref deinit or rollback.
 */
esp_err_t esp_board_runtime_set_bound_deinit(const char *name, void *deinit_fn);

/**
 * @brief  Get the deinit function captured for the active resource instance
 *
 * @return
 *       - Captured  deinit function, or NULL when no active snapshot exists
 */
void *esp_board_runtime_get_bound_deinit(const char *name);

/**
 * @brief  Test-only: clear sidecars and return prepare state to UNPREPARED
 *
 *         This helper is only for host tests and must not be called by product
 *         runtime code.
 */
void esp_board_runtime_test_reset(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
