/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_board_manager_err.h"
#include "esp_board_manager_defs.h"
#include "esp_board_extra_func_entry.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define DEVICE_EXTRA_FUNC_REGISTER(name, extra_func)  EXTRA_FUNC_IMPLEMENT(name, extra_func)

/**
 * @brief  Function pointer type for device initialization
 *
 *         Publish a usable handle only on success. Roll back resources acquired by
 *         a failed initialization; do not free objects still used by the driver.
 */
typedef int (*esp_board_device_init_func)(void *cfg, int cfg_size, void **device_handle);

/**
 * @brief  Function pointer type for device deinitialization
 */
typedef int (*esp_board_device_deinit_func)(void *device_handle);

/**
 * @brief  Function pointer type for device power control
 */
typedef int (*esp_board_device_power_ctrl_func)(void *dev_handle, const char *device_name, bool power_on);

/**
 * @brief  Function pointer type for device callback_register
 */
typedef int (*esp_board_device_callback_register_func)(void *dev_handle, const void *cfg, int cfg_size, void *call_back_func, void *user_data);
/**
 * @brief  Structure representing a device descriptor
 */
typedef struct esp_board_device_desc {
    const struct esp_board_device_desc  *next;               /*!< Pointer to next device descriptor */
    const char                          *name;               /*!< Device name */
    const char                          *chip;               /*!< Device chip type */
    const char                          *type;               /*!< Device type */
    const char                          *sub_type;           /*!< Device sub-type */
    const void                          *cfg;                /*!< Device configuration data */
    uint16_t                             cfg_size;           /*!< Size of configuration data */
    uint8_t                              init_skip : 1;      /*!< Skip initialization when manager initializes all devices */
    const char                          *power_ctrl_device;  /*!< Power control device name for this device */
    const char * const                  *depends_on;         /*!< Array of device names this device depends on */
    uint8_t                              depends_on_num;     /*!< Number of dependencies */
} esp_board_device_desc_t;

/**
 * @brief  Structure representing a device handle
 */
typedef struct esp_board_device_handle {
    struct esp_board_device_handle *next;           /*!< Pointer to next device handle */
    const char                     *name;           /*!< Device name */
    const char                     *chip;           /*!< Device chip type */
    const char                     *type;           /*!< Device type */
    void                           *device_handle;  /*!< Device-specific handle */
    uint8_t                         ref_count;      /*!< Reference count */
    esp_board_device_init_func      init;           /*!< Generated default init (fallback ops) */
    esp_board_device_deinit_func    deinit;         /*!< Generated default deinit (fallback ops) */
} esp_board_device_handle_t;

/**
 * @brief  Initialize a device by name
 *
 *         This function initializes a device with reference counting support. If the device
 *         is already initialized, it increments the reference count instead of reinitializing
 *         The device is only actually initialized when the reference count reaches 1
 *         The initializer must roll back resources acquired by a failed call. Applications
 *         do not need to call deinit after an ordinary initialization failure. If the
 *         initializer returns an error with a non-NULL handle, BMGR retains the record,
 *         configuration and dependencies to prevent reuse; a restart is required.
 *         Such a handle is not exposed for use or for caller-driven deinit recovery.
 *
 * @param[in]  name  Device name to initialize
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_BOARD_ERR_DEVICE_INVALID_ARG  If name is NULL
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND    If device descriptor or handle not found
 *       - ESP_BOARD_ERR_DEVICE_NO_INIT      If no init function is available
 *       - ESP_ERR_INVALID_STATE             If init left unreleased resources or board prepare requires a restart
 *       - ESP_ERR_INVALID_SIZE              If effective config size differs from the descriptor
 *       - Others                            Error codes from device-specific initialization
 */
esp_err_t esp_board_device_init(const char *name);

/**
 * @brief  Get device handle by name
 *
 *         Retrieves the device handle for a given device name. For GPIO devices,
 *         the handle is dereferenced to get the actual pin number
 *
 * @param[in]   name           Device name
 * @param[out]  device_handle  Pointer to store the device handle
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_BOARD_ERR_DEVICE_INVALID_ARG  If name or device_handle is NULL
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND    If device handle not found
 */
esp_err_t esp_board_device_get_handle(const char *name, void **device_handle);

/**
 * @brief  Get device configuration by name
 *
 *         Retrieves the effective configuration for a given device name. The
 *         returned pointer is owned by BMGR and may be invalidated by the next
 *         override or restore operation for this device; copy it if it must be
 *         retained across those operations.
 *
 * @param[in]   name    Device name
 * @param[out]  config  Pointer to store the configuration
 *
 * @return
 *       - ESP_OK                              On success
 *       - ESP_BOARD_ERR_DEVICE_INVALID_ARG    If name or config is NULL
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND      If device configuration not found
 *       - ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED  If device has no configuration
 */
esp_err_t esp_board_device_get_config(const char *name, void **config);

/**
 * @brief  Get device configuration by handle
 *
 *         Retrieves the configuration for a given device handle
 *
 * @param[in]   device_handle  Device handle
 * @param[out]  config         Pointer to store the configuration
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_BOARD_ERR_DEVICE_INVALID_ARG  If given handle is NULL
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND    If device handle or configuration not found
 */
esp_err_t esp_board_device_get_config_by_handle(void *device_handle, void **config);

/**
 * @brief  Override a device configuration at runtime (APP layer)
 *
 *         Stores a top-level byte copy of config for future get_config, init, and
 *         callback_register. Nested pointers inside the struct are not deep-copied;
 *         the caller must keep referenced memory valid until restore_config. Size
 *         need not match the generated descriptor; a mismatch logs a warning and
 *         still copies config_size bytes, but esp_board_device_init() rejects a
 *         mismatched effective size with ESP_ERR_INVALID_SIZE. Repeated calls
 *         replace the previous APP copy. Rejected while this device's ref_count
 *         is non-zero or it is marked initialized.
 *
 * @param[in]  name         Device name
 * @param[in]  config       Pointer to the new configuration
 * @param[in]  config_size  Size of the configuration to copy
 *
 * @return
 *       - ESP_OK                              On success
 *       - ESP_ERR_NO_MEM                      If memory allocation fails
 *       - ESP_ERR_INVALID_STATE               If the device is already initialized
 *       - ESP_BOARD_ERR_DEVICE_INVALID_ARG    If name/config is NULL or config_size is 0
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND      If device is not found
 *       - ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED  If device has no generated configuration
 */
esp_err_t esp_board_device_override_config(const char *name, const void *config, uint16_t config_size);

/**
 * @brief  Restore APP-layer configuration for a device
 *
 *         Clears only the APP config copy. BOARD override remains if present;
 *         otherwise effective config falls back to generated. Does not change ops.
 *         Rejected while this device's ref_count is non-zero or it is marked
 *         initialized.
 *
 * @param[in]  name  Device name
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_ERR_NOT_FOUND                 If no APP config override exists
 *       - ESP_ERR_INVALID_STATE             If the device is already initialized
 *       - ESP_BOARD_ERR_DEVICE_INVALID_ARG  If name is NULL
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND    If device is not found
 */
esp_err_t esp_board_device_restore_config(const char *name);

/**
 * @brief  Get the effective 8-bit I2C address published by an initialized device
 *
 *         Some I2C devices can probe multiple candidate addresses at runtime.
 *         This function returns the final 8-bit (left-shifted) I2C address that
 *         the device published after successful initialization. If the returned
 *         address is passed to ESP-IDF I2C master APIs, shift it right by one.
 *
 * @param[in]   device_name  Device name
 * @param[out]  addr         Pointer to store the 8-bit I2C address
 *
 * @return
 *       - ESP_OK                              On success
 *       - ESP_BOARD_ERR_DEVICE_INVALID_ARG    If device_name or addr is NULL
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND      If device does not exist
 *       - ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED  If the device has not published an I2C address
 */
esp_err_t esp_board_device_get_i2c_effective_addr(const char *device_name, uint16_t *addr);

/**
 * @brief  Set per-name device init/deinit ops (APP layer)
 *
 *         Does not modify the generated handle init/deinit table. Repeated calls
 *         replace the previous APP ops. Rejected while ref_count is non-zero or
 *         the device is marked initialized.
 *
 * @param[in]  name    Device name
 * @param[in]  init    Initialization function pointer
 * @param[in]  deinit  Deinitialization function pointer
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_ERR_NO_MEM                    If memory allocation fails
 *       - ESP_ERR_INVALID_STATE             If the device is already initialized
 *       - ESP_BOARD_ERR_DEVICE_INVALID_ARG  If any parameter is NULL
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND    If device is not found
 */
esp_err_t esp_board_device_set_ops(const char *name, esp_board_device_init_func init, esp_board_device_deinit_func deinit);

/**
 * @brief  Restore APP-layer ops for a device
 *
 *         Clears only APP ops. BOARD ops remain if present; otherwise effective
 *         ops fall back to the generated handle init/deinit. Does not change
 *         config. Rejected while this device's ref_count is non-zero or it is
 *         marked initialized.
 *
 * @param[in]  name  Device name
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_ERR_NOT_FOUND                 If no APP ops override exists
 *       - ESP_ERR_INVALID_STATE             If the device is already initialized
 *       - ESP_BOARD_ERR_DEVICE_INVALID_ARG  If name is NULL
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND    If device is not found
 */
esp_err_t esp_board_device_restore_ops(const char *name);

/**
 * @brief  Deinitialize a device by name
 *
 *         Decrements the reference count for a device. The device is only actually
 *         deinitialized when the reference count reaches 0. If the device is not
 *         initialized, this function returns success
 *
 *         A successful instance remains available for another cleanup attempt if
 *         its deinit callback fails. An instance left by failed initialization, or
 *         a failed board prepare with unreleased resources, requires a restart.
 *
 * @param[in]  name  Device name to deinitialize
 *
 * @return
 *       - ESP_ERR_INVALID_STATE             If initialization or prepare requires a restart
 *       - ESP_OK                            On success
 *       - ESP_BOARD_ERR_DEVICE_INVALID_ARG  If name is NULL
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND    If device handle not found
 *       - ESP_BOARD_ERR_DEVICE_NO_INIT      If no deinit function is available
 *       - Others                            Error codes from device-specific deinitialization
 */
esp_err_t esp_board_device_deinit(const char *name);

/**
 * @brief  Show device information
 *
 *         Displays detailed information about devices. If name is NULL, shows
 *         information for all devices. Otherwise, shows information for the
 *         specified device including type, configuration size, handle status,
 *         and reference count
 *
 * @param[in]  name  Device name (NULL for all devices)
 *
 * @return
 *       - ESP_OK                          On success
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND  If specific device not found (when name is not NULL)
 */
esp_err_t esp_board_device_show(const char *name);

/**
 * @brief  Iterate device names by type
 *
 *         Iterates over registered device descriptors that match @p type.
 *         Set *cursor to NULL before the first call. On success, this
 *         function updates *cursor to the matched descriptor and outputs
 *         the corresponding device name in *name.
 *
 * @param[in]      type    Device type key (for example ESP_BOARD_DEVICE_TYPE_BUTTON)
 * @param[in,out]  cursor  Opaque iteration cursor (set to NULL to start)
 * @param[out]     name    Next matching device name
 *
 * @return
 *       - ESP_OK                            On success
 *       - ESP_BOARD_ERR_DEVICE_INVALID_ARG  If type/cursor/name is NULL
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND    No more matching devices
 */
esp_err_t esp_board_device_iterate_name_by_type(const char *type, void **cursor, const char **name);

/**
 * @brief  Initialize all devices
 *
 *         Prepares the board before iterating through device descriptors and
 *         initializing each device. Returns immediately if prepare fails.
 *         Continues initializing even if some devices fail, but logs errors
 *
 * @note  Device initialization strictly follows the order defined in board_devices.yaml
 *         If a device depends on a peripheral for power-on, it must be initialized
 *         after that peripheral. For example, the LCD power control device should be
 *         listed before the Display LCD device in board_devices.yaml
 *         If initialization fails due to unresolved dependencies, either reorder the
 *         YAML entries or manually call the device initialization function
 *
 * @return
 *       - ESP_OK  Prepare succeeded; individual device init errors are logged
 *       - Others  Board prepare failed
 */
esp_err_t esp_board_device_init_all(void);

/**
 * @brief  Deinitialize all devices
 *
 *         Iterates through all device handles and deinitializes each device
 *         Continues deinitializing even if some devices fail, but logs errors
 *
 * @return
 *       - ESP_OK  On success (always returns ESP_OK, errors are logged)
 */
esp_err_t esp_board_device_deinit_all(void);

/**
 * @brief  Find device handle structure by device-specific handle
 *
 *         Searches through the device handle linked list to find the device
 *         handle structure that contains the specified device-specific handle
 *
 * @param[in]  device_handle  Device-specific handle to search for
 *
 * @return
 *       - NULL    If not found or if device_handle is NULL
 *       - Others  Pointer to device handle structure if found
 */
const esp_board_device_handle_t *esp_board_device_find_by_handle(void *device_handle);

/**
 * @brief  Control power for a device
 *
 *         This function controls the power state for a device using its associated
 *         power control device. If the device has a power_ctrl_device configured,
 *         this function will call the power control function to enable or disable
 *         power for the device.
 *
 * @param[in]  name      Device name to control power for
 * @param[in]  power_on  True to enable power, false to disable power
 *
 * @return
 *       - ESP_OK                              On success
 *       - ESP_BOARD_ERR_DEVICE_INVALID_ARG    If name is NULL
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND      If device or power control device not found
 *       - ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED  If device has no power control device configured
 *       - Others                              Error codes from power control device
 */
esp_err_t esp_board_device_power_ctrl(const char *name, bool power_on);

/**
 * @brief  Register a callback for a device
 *
 *         This function registers a callback function for a specific device.
 *         It looks up the device descriptor and the device‑specific callback
 *         registration function based on the device type, then invokes that
 *         function with the device handle, configuration, and user data.
 *
 *         For the detailed callback definitions and actual callback behaviors,
 *         refer to the corresponding device's header file.
 *
 *         Whether the device supports re-registering (updating) an existing
 *         callback also depends on the device implementation. Refer to the
 *         device-specific documentation for details.
 *
 * @param[in]  name       Device name
 * @param[in]  callback   Callback function pointer (device‑specific)
 * @param[in]  user_data  User data to pass to the callback (optional)
 *
 * @return
 *       - ESP_OK                              On success
 *       - ESP_BOARD_ERR_DEVICE_INVALID_ARG    If name or call_back_func is NULL
 *       - ESP_BOARD_ERR_DEVICE_NOT_FOUND      If device not found
 *       - ESP_BOARD_ERR_DEVICE_NOT_SUPPORTED  If device has no callback register func
 *       - ESP_BOARD_ERR_DEVICE_INIT_FAILED    If callback registration failed
 *       - Others                              Error codes from power control device
 */
esp_err_t esp_board_device_callback_register(const char *name, void *callback, void *user_data);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
