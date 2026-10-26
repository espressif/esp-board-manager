Advanced Runtime Capabilities
==============================

:link_to_translation:`zh_CN:[中文]`

In addition to the primary path of obtaining a handle after initialization, BMGR provides the following runtime capabilities for advanced scenarios.

Runtime Configuration and Ops Override
--------------------------------------

Effective config and init/deinit ops for each device or peripheral name follow this priority:

**YAML-generated configuration < BOARD < APP**

- **YAML-generated configuration**: descriptor configuration and type-level init/deinit operations generated from board YAML. After a revision apply or C code changes a descriptor, the current descriptor provides the base configuration.
- **BOARD**: per-name overrides installed by the board pre-init hook through override APIs. The board implements :cpp:func:`esp_board_runtime_pre_init`; the manager calls it automatically before creating the first handle.
- **APP**: the application explicitly calls ``override_config`` / ``set_ops`` / ``restore_config`` / ``restore_ops`` for device or peripheral names.

Multi-revision configuration (``board_revisions``) generates the base configuration and revision-specific differences so they coexist in one firmware image. At runtime, board code detects the hardware and calls the generated apply function. That function updates the base descriptor tables; it does not write the BOARD override layer. If a revision is declared but its apply function is not called, BMGR keeps using the base configuration.

Related APIs:

- Device: :cpp:func:`esp_board_device_override_config`, :cpp:func:`esp_board_device_restore_config`, :cpp:func:`esp_board_device_set_ops`, :cpp:func:`esp_board_device_restore_ops`
- Peripheral: :cpp:func:`esp_board_periph_override_config`, :cpp:func:`esp_board_periph_restore_config`, :cpp:func:`esp_board_periph_set_ops`, :cpp:func:`esp_board_periph_restore_ops`

``override_config`` performs a top-level ``memcpy`` of the config struct. Nested pointers inside the struct still alias the caller's memory; keep that nested memory alive for as long as the override remains in effect. To change one field, copy the generated config first, change the field, then override.

A device config override updates the device's effective config. :cpp:func:`esp_board_device_get_config` returns this config, device ``init`` uses it to create the driver handle, and :cpp:func:`esp_board_device_callback_register` passes it to the concrete device implementation when registering callbacks on the initialized handle. Callback registration does not reinitialize the device, so the config override normally has to be applied before device init.

A peripheral config override updates the peripheral's effective config. :cpp:func:`esp_board_periph_get_config` and peripheral ``init`` read this config; when device initialization consumes that peripheral, it also uses the corresponding effective config. BMGR does not provide a callback-registration API for peripherals.

Config and ops layers are independent. Setting only ops leaves config on the lower layer; setting only config leaves ops on the lower layer.

While a resource is initializing or remains held, ``override_config`` / ``set_ops`` / ``restore_*`` on that name return ``ESP_ERR_INVALID_STATE``. Before init, repeats on the same layer replace the previous value. Edits are allowed after a normal deinit successfully releases the resource. An init failure with a non-NULL handle blocks edits even when ``ref_count`` is 0; see :ref:`bmgr-init-failure`.

``deinit`` releases driver handles but does not clear BOARD or APP overrides. ``restore_config`` / ``restore_ops`` clear the APP layer only. After APP restore, config falls back to a BOARD config override; if none exists, BMGR reads the current descriptor configuration. The descriptor may have been replaced by revision apply or changed by C code. If it has not been modified, its config is the YAML-generated default. Restoring APP overrides does not undo a revision apply or descriptor edits. Ops fall back to BOARD ops, or to the matching type/role operations when no BOARD ops exists. The hook must deinitialize temporary hardware itself. If prepare fails without retained resources, BMGR clears the BOARD overrides installed by the hook. If resources remain, BMGR keeps the current config and rejects subsequent normal runtime operations; see :ref:`bmgr-prepare-failure`.

Names without a YAML-generated config cannot receive an override. Resources that share a ``(type, role)`` but have different names keep isolated per-name ops and config.

For the board hook contract, prepare timing, and legacy APIs, see :doc:`/programming-guide/runtime-lifecycle`.

Runtime constraints and configuration considerations:

- A board pre-init hook may initialize resources and query their handles. It must deinitialize resources it created before returning an error.
- When configuring multiple logical peripherals, verify their physical bus relationship. BMGR does not automatically share one physical bus merely because two peripherals use the same ``port`` value.

Manual Device Power Control
----------------------------

Related API: :cpp:func:`esp_board_device_power_ctrl`.

This applies when a device has an independent power-on or power-off action and the application needs to explicitly control it outside the automatic BMGR flow. The API locates the controlling device based on ``power_ctrl_device`` in the device description, then calls the power-control extension function corresponding to that ``sub_type``. Whether and how control is possible depends on the specific power control device implementation. Currently, only the GPIO sub-type is supported for power control devices. For more complex power control devices such as I/O expander chips or power management ICs, you can declare a dependency using ``depends_on`` to ensure the required power management device is initialized before this device.

Device Callback Registration
-----------------------------

Related API: :cpp:func:`esp_board_device_callback_register`.

Some devices require callback registration for normal operation. BMGR provides a unified wrapper for the registration interface; the application can look up the corresponding extension callback registration function by device type, then pass in the current effective configuration and the runtime handle. Whether this capability is available and the callback signature depend on the specific device implementation; see the individual device reference pages for details.

Initialize or Deinitialize a Device by Name
--------------------------------------------

Related APIs:

- :cpp:func:`esp_board_manager_init_device_by_name`
- :cpp:func:`esp_board_manager_deinit_device_by_name`

These APIs are commonly used with ``init_skip``, deferred initialization, and partial resource reclamation scenarios. For projects that need to complete full-board initialization first and then enable specific devices at the appropriate time, they offer more flexibility than initializing all devices at once.
