/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "esp_board_extra_func_entry.h"
#include "esp_board_manager_defs.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define PERIPHERAL_EXTRA_FUNC_REGISTER(name, extra_func)  EXTRA_FUNC_IMPLEMENT(name, extra_func)

/**
 * @brief  Function pointer type for peripheral initialization
 */
typedef esp_err_t (*esp_board_periph_init_func)(void *cfg, int cfg_size, void **periph_handle);

/**
 * @brief  Function pointer type for peripheral deinitialization
 */
typedef esp_err_t (*esp_board_periph_deinit_func)(void *periph_handle);

/**
 * @brief  Structure representing a peripheral descriptor
 */
typedef struct esp_board_periph_desc {
    const struct esp_board_periph_desc *next;           /*!< Pointer to next peripheral descriptor */
    const char                         *name;           /*!< Peripheral name */
    const char                         *type;           /*!< Peripheral type */
    esp_board_periph_role_t             role;           /*!< Peripheral role */
    const char                         *format;         /*!< Peripheral format */
    const void                         *cfg;            /*!< Peripheral configuration data */
    int                                 cfg_size;       /*!< Size of configuration data */
    int                                 id;             /*!< Peripheral ID extracted from name (e.g., 48 from gpio48, 0 from iic0) */
    uint8_t                             init_skip : 1;  /*!< Skip initialization when manager initializes all peripherals */
} esp_board_periph_desc_t;

/**
 * @brief  Structure representing a peripheral entry
 */
typedef struct esp_board_periph_entry {
    struct esp_board_periph_entry *next;    /*!< Pointer to next peripheral entry */
    const char                    *type;    /*!< Peripheral type */
    esp_board_periph_role_t        role;    /*!< Peripheral role */
    esp_board_periph_init_func     init;    /*!< Peripheral initialization function */
    esp_board_periph_deinit_func   deinit;  /*!< Peripheral deinitialization function */
} esp_board_periph_entry_t;

/**
 * @brief  Structure representing a peripheral list entry
 */
typedef struct esp_board_periph_list {
    struct esp_board_periph_list *next;           /*!< Pointer to next peripheral list entry */
    const char                   *name;           /*!< Peripheral name */
    const char                   *type;           /*!< Peripheral type */
    esp_board_periph_role_t       role;           /*!< Peripheral role */
    void                         *periph_handle;  /*!< Peripheral-specific handle */
    uint8_t                       ref_count;      /*!< Reference count */
} esp_board_periph_list_t;

/**
 * @brief  Initialize a peripheral by name
 *
 *         This function initializes a peripheral with reference counting support. If the peripheral
 *         is already initialized, it increments the reference count instead of reinitializing
 *         The peripheral is only actually initialized when the reference count reaches 1
 *         Peripherals are found by matching type and role from the handle list.
 *         The initializer must roll back resources acquired by a failed call.
 *         Applications do not need deinit after an ordinary initialization failure.
 *         An error with a non-NULL handle retains the record and configuration to
 *         prevent reuse and requires a restart; deinit does not recover that instance.
 *
 * @param[in]  name  Peripheral name to initialize
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_ERR_NO_MEM                    If memory allocation fails for list entry
 *       - ESP_BOARD_ERR_PERIPH_INVALID_ARG  If name is NULL
 *       - ESP_BOARD_ERR_PERIPH_NOT_FOUND    If peripheral descriptor not found
 *       - ESP_BOARD_ERR_PERIPH_NO_HANDLE    If no matching handle found for type/role
 *       - ESP_BOARD_ERR_PERIPH_NO_INIT      If no init function is available
 *       - ESP_ERR_INVALID_STATE             If initialization or board prepare requires a restart, or init is in progress
 *       - ESP_ERR_INVALID_SIZE              If effective config size differs from the descriptor
 *       - Others                            Error codes from peripheral-specific initialization
 */
esp_err_t esp_board_periph_init(const char *name);

/**
 * @brief  Get peripheral handle by name
 *
 *         Retrieves the peripheral handle for a given peripheral name
 *         The peripheral is automatically initialized if it is not initialized
 *
 * @param[in]   name           Peripheral name
 * @param[out]  periph_handle  Pointer to store the peripheral handle
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_BOARD_ERR_PERIPH_INVALID_ARG  If name or periph_handle is NULL
 *       - ESP_BOARD_ERR_PERIPH_NO_HANDLE    If peripheral initialized failed or no handle found
 */
esp_err_t esp_board_periph_get_handle(const char *name, void **periph_handle);

/**
 * @brief  Get peripheral name by handle
 *
 *         Retrieves the peripheral name for a given peripheral handle
 *         Searches through all initialized peripherals to find the matching handle
 *
 * @param[in]   periph_handle  Peripheral handle to search for
 * @param[out]  name           Pointer to store the peripheral name
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_BOARD_ERR_PERIPH_INVALID_ARG  If periph_handle or name is NULL
 *       - ESP_BOARD_ERR_PERIPH_NOT_FOUND    If no peripheral found with the given handle
 */
esp_err_t esp_board_periph_get_name_by_handle(void *periph_handle, const char **name);

/**
 * @brief  Get reference peripheral handle by name
 *
 *         Retrieves the peripheral handle for a given peripheral name
 *         The peripheral is automatically initialized if it is not initialized
 *         Increases the reference count for the peripheral
 *
 * @param[in]   name           Peripheral name
 * @param[out]  periph_handle  Pointer to store the peripheral handle
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_BOARD_ERR_PERIPH_INVALID_ARG  If name or periph_handle is NULL
 *       - ESP_BOARD_ERR_PERIPH_NO_HANDLE    If peripheral initialized failed or no handle found
 */
esp_err_t esp_board_periph_ref_handle(const char *name, void **periph_handle);

/**
 * @brief  Unreference peripheral handle by name
 *
 *         Decreases the reference count for the peripheral
 *         The peripheral is only actually deinitialized when the reference count reaches 0
 *
 * @param[in]  name  Peripheral name
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_BOARD_ERR_PERIPH_INVALID_ARG  If name is NULL
 *       - ESP_BOARD_ERR_PERIPH_NOT_FOUND    If peripheral list entry not found
 *       - ESP_BOARD_ERR_PERIPH_NO_INIT      If no deinit function is available
 *       - Others                            Error codes from peripheral-specific deinitialization
 */
esp_err_t esp_board_periph_unref_handle(const char *name);

/**
 * @brief  Get peripheral configuration by name
 *
 *         Returns the effective configuration pointer: APP override, else BOARD
 *         override, else the generated descriptor config. Sidecar layers hold a
 *         top-level byte copy only; nested pointers inside the struct still
 *         alias the caller's memory. The returned pointer may be invalidated by
 *         the next override or restore operation for this peripheral; copy it
 *         if it must be retained across those operations.
 *
 * @param[in]   name    Peripheral name
 * @param[out]  config  Pointer to store the configuration
 *
 * @return
 *       - ESP_OK                              On success
 *       - ESP_BOARD_ERR_PERIPH_INVALID_ARG    If name or config is NULL
 *       - ESP_BOARD_ERR_PERIPH_NOT_FOUND      If peripheral configuration not found
 *       - ESP_BOARD_ERR_PERIPH_NOT_SUPPORTED  If peripheral has no configuration
 */
esp_err_t esp_board_periph_get_config(const char *name, void **config);

/**
 * @brief  Override a peripheral configuration at runtime (APP layer)
 *
 *         Stores a top-level byte copy of config for future get_config and init.
 *         Nested pointers inside the struct are not deep-copied; the caller must
 *         keep referenced memory valid until restore_config. Size need not match
 *         the generated descriptor; a mismatch logs a warning and still copies
 *         config_size bytes, but esp_board_periph_init() rejects a mismatched
 *         effective size with ESP_ERR_INVALID_SIZE. Repeated calls replace the
 *         previous APP copy. Rejected while this peripheral is initialized.
 *
 * @param[in]  name         Peripheral name
 * @param[in]  config       Pointer to the new configuration
 * @param[in]  config_size  Size of the configuration to copy
 *
 * @return
 *       - ESP_OK                              On success
 *       - ESP_ERR_NO_MEM                      If memory allocation fails
 *       - ESP_ERR_INVALID_STATE               If the peripheral is already initialized
 *       - ESP_BOARD_ERR_PERIPH_INVALID_ARG    If name/config is NULL or config_size is 0
 *       - ESP_BOARD_ERR_PERIPH_NOT_FOUND      If peripheral is not found
 *       - ESP_BOARD_ERR_PERIPH_NOT_SUPPORTED  If peripheral has no generated configuration
 */
esp_err_t esp_board_periph_override_config(const char *name, const void *config, uint16_t config_size);

/**
 * @brief  Restore APP-layer configuration for a peripheral
 *
 *         Clears only the APP config copy. BOARD override remains if present;
 *         otherwise effective config falls back to generated. Does not change ops.
 *         Rejected while this peripheral's ref_count is non-zero.
 *
 * @param[in]  name  Peripheral name
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_ERR_NOT_FOUND                 If no APP config override exists
 *       - ESP_ERR_INVALID_STATE             If the peripheral is already initialized
 *       - ESP_BOARD_ERR_PERIPH_INVALID_ARG  If name is NULL
 */
esp_err_t esp_board_periph_restore_config(const char *name);

/**
 * @brief  Set per-name peripheral init/deinit ops (APP layer)
 *
 *         Does not modify the shared type/role handler table. Repeated calls
 *         replace the previous APP ops. Rejected while ref_count is non-zero.
 *
 * @param[in]  name    Peripheral name
 * @param[in]  init    Initialization function pointer
 * @param[in]  deinit  Deinitialization function pointer
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_ERR_NO_MEM                    If memory allocation fails
 *       - ESP_ERR_INVALID_STATE             If the peripheral is already initialized
 *       - ESP_BOARD_ERR_PERIPH_INVALID_ARG  If any parameter is NULL
 *       - ESP_BOARD_ERR_PERIPH_NOT_FOUND    If peripheral is not found
 */
esp_err_t esp_board_periph_set_ops(const char *name, esp_board_periph_init_func init,
                                   esp_board_periph_deinit_func deinit);

/**
 * @brief  Restore APP-layer ops for a peripheral
 *
 *         Clears only APP ops. BOARD ops remain if present; otherwise effective
 *         ops fall back to the shared type/role handler. Does not change config.
 *         Rejected while this peripheral's ref_count is non-zero.
 *
 * @param[in]  name  Peripheral name
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_ERR_NOT_FOUND                 If no APP ops override exists
 *       - ESP_ERR_INVALID_STATE             If the peripheral is already initialized
 *       - ESP_BOARD_ERR_PERIPH_INVALID_ARG  If name is NULL
 */
esp_err_t esp_board_periph_restore_ops(const char *name);

/**
 * @brief  Initialize peripheral with custom functions (legacy)
 *
 *         Legacy ABI: updates the shared (type, role) handler table and initializes
 *         the named peripheral immediately. It is not the per-name variant mechanism
 *         and does not write APP/BOARD sidecar ops or use effective config priority;
 *         init uses the generated descriptor config. New code should use the board
 *         pre-init hook (BOARD ``set_ops``) or APP :c:func:`esp_board_periph_set_ops`.
 *         If the peripheral is already initialized, only the shared handler is updated.
 *
 * @param[in]  name    Peripheral name
 * @param[in]  init    Initialization function pointer
 * @param[in]  deinit  Deinitialization function pointer
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_ERR_NO_MEM                    If memory allocation fails for list entry
 *       - ESP_BOARD_ERR_PERIPH_INVALID_ARG  If any parameter is NULL
 *       - ESP_BOARD_ERR_PERIPH_NOT_FOUND    If peripheral descriptor not found
 *       - ESP_BOARD_ERR_PERIPH_NO_HANDLE    If no matching handle found for type/role
 *       - ESP_BOARD_ERR_PERIPH_INIT_FAILED  If initialization fails
 */
esp_err_t esp_board_periph_init_custom(const char *name, esp_board_periph_init_func init, esp_board_periph_deinit_func deinit)
    __attribute__((deprecated("Use esp_board_periph_set_ops or the board pre-init hook")));

/**
 * @brief  Deinitialize a peripheral by name
 *
 *         Decrements the reference count for a peripheral. The peripheral is only actually
 *         deinitialized when the reference count reaches 0. If the peripheral is not
 *         initialized, this function returns success
 *
 *         A successful instance remains available for another cleanup attempt if
 *         its deinit callback fails. An instance left by failed initialization, or
 *         a failed board prepare with unreleased resources, requires a restart.
 *
 * @param[in]  name  Peripheral name to deinitialize
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_ERR_INVALID_STATE             If initialization or prepare requires a restart
 *       - ESP_BOARD_ERR_PERIPH_INVALID_ARG  If name is NULL
 *       - ESP_BOARD_ERR_PERIPH_NOT_FOUND    If peripheral list entry not found
 *       - ESP_BOARD_ERR_PERIPH_NO_INIT      If no deinit function is available
 *       - Others                            Error codes from peripheral-specific deinitialization
 */
esp_err_t esp_board_periph_deinit(const char *name);

/**
 * @brief  Show peripheral information
 *
 *         Displays detailed information about peripherals. If name is NULL, shows
 *         information for all peripherals. Otherwise, shows information for the
 *         specified peripheral including type, role, handle status, and reference count
 *
 * @param[in]  name  Peripheral name (NULL for all peripherals)
 *
 * @return
 *       - ESP_OK                          On success
 *       - ESP_BOARD_ERR_PERIPH_NOT_FOUND  If specific peripheral not found (when name is not NULL)
 */
esp_err_t esp_board_periph_show(const char *name);

/**
 * @brief  Initialize all peripherals
 *
 *         Prepares the board before iterating through peripheral descriptors and
 *         initializing each peripheral. Returns immediately if prepare fails.
 *         Continues initializing even if some peripherals fail, but logs errors
 *         Descriptors with init_skip set are left uninitialized; call
 *         esp_board_periph_init() later if that peripheral is needed
 *
 * @note  Peripheral initialization follows the order defined in board_peripherals.yaml
 *
 * @return
 *       - ESP_OK  Prepare succeeded; individual peripheral init errors are logged
 *       - Others  Board prepare failed
 */
esp_err_t esp_board_periph_init_all(void);

/**
 * @brief  Deinitialize all peripherals
 *
 *         Iterates through all peripheral list entries and deinitializes each peripheral
 *         Continues deinitializing even if some peripherals fail, but logs errors
 *
 * @return
 *       - ESP_OK  On success (always returns ESP_OK, errors are logged)
 */
esp_err_t esp_board_periph_deinit_all(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
