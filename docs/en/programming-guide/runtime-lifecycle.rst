Runtime Lifecycle
=================

:link_to_translation:`zh_CN:[中文]`

From a runtime perspective, BMGR enforces a fixed sequence: which peripherals are initialized first, which devices are initialized next, and how to obtain the corresponding handles and configurations.

Initialization Entry Points and Runtime Access
----------------------------------------------

:cpp:func:`esp_board_manager_init` is the full-board initialization entry point. It performs automatic initialization according to the generated board description. Correspondingly, :cpp:func:`esp_board_manager_deinit` releases runtime objects in the opposite direction.

:cpp:func:`esp_board_manager_init_device_by_name` initializes a single device by device name. It is commonly used for ``init_skip``, deferred initialization, or cases where an application opens a device only when a business condition is met.

Applications can use :cpp:func:`esp_board_manager_get_device_config` and :cpp:func:`esp_board_manager_get_periph_config` to obtain effective configuration structures. Without APP or BOARD overrides, these come from the current descriptor. If the descriptor has not been changed by a board revision or C code, this is the C structure generated from YAML. With overrides, they follow the priority described in :doc:`/programming-guide/advanced-runtime`.

Applications can use :cpp:func:`esp_board_manager_get_device_handle` and :cpp:func:`esp_board_manager_get_periph_handle` to obtain runtime handles. These APIs only return handles for initialized objects; they do not initialize devices or peripherals.

Key Behaviors
-------------

- **Peripherals initialize first, devices initialize second**: When :cpp:func:`esp_board_manager_init` is called, BMGR always executes in peripheral-first, device-second order.
- **Overall traversal order follows YAML declaration order**: :cpp:func:`esp_board_device_init_all` traverses devices in the order they appear in ``board_devices.yaml``.
- ``depends_on`` **declares initialization dependencies between devices**: When a device is configured with ``depends_on``, :cpp:func:`esp_board_device_init` recursively initializes the listed dependencies before initializing that device, regardless of declaration order in YAML. There is no need to manually order those devices in ``board_devices.yaml``. If a dependency has already been initialized through another path (``ref_count > 0``), it is not re-created. A device may declare multiple dependencies of any type.
- **Deinitialization converges by reference count**: Both devices and peripherals maintain an internal reference count (``ref_count``). Reinitializing the same object does not create a new instance; it increments the reference count. The resource is only actually released when the count drops to zero.
- ``init_skip`` **skips automatic initialization**: For a device or peripheral with ``init_skip: true``, :cpp:func:`esp_board_manager_init` does not automatically create its runtime handle. Initialize a skipped device with :cpp:func:`esp_board_manager_init_device_by_name`. Initialize a skipped peripheral with :cpp:func:`esp_board_periph_init`, or by initializing a device that references it. Then obtain the handle with :cpp:func:`esp_board_manager_get_device_handle` or :cpp:func:`esp_board_manager_get_periph_handle`.
- ``power_ctrl_device`` **controls device power-on sequencing**: When a device declares ``power_ctrl_device``, BMGR triggers a power-on action through the corresponding ``power_ctrl`` device before initializing this device; a power-off action is triggered during deinitialization. ``power_ctrl_device`` is a device-to-device reference specifically for power supply control; the referenced device must be of type ``power_ctrl``. Compared to ``depends_on``, ``power_ctrl_device`` additionally triggers power-on and power-off actions and provides the runtime power control API :cpp:func:`esp_board_device_power_ctrl`, so its effect is not limited to ensuring initialization order.
- Using ``depends_on`` and ``power_ctrl_device`` together ensures that even when a device is initialized individually via :cpp:func:`esp_board_manager_init_device_by_name`, initialization will not fail due to power supply or other dependency issues.

BMGR's runtime model does not compress all initialization logic into a single ``init()`` call; instead, it organizes the initialization order according to the board description, while preserving runtime behaviors such as reference counting, deferred initialization, inter-device dependencies, and power control.

Board Pre-init Hook
-------------------

:cpp:func:`esp_board_runtime_pre_init` is a fixed weak symbol. The library provides a default that returns ``ESP_OK``. A final application may supply exactly one strong definition; multiple strong definitions fail at link time. There is no ``board_info.yaml`` field and no generated function-pointer registry. Place the strong definition in board sources such as ``setup_device.c``.

The hook runs once per process the first time prepare executes. :cpp:func:`esp_board_manager_init`, :cpp:func:`esp_board_periph_init`, :cpp:func:`esp_board_device_init`, and :cpp:func:`esp_board_periph_init_custom` all call prepare before creating a handle, so the hook cannot be skipped. :cpp:func:`esp_board_manager_prepare` is an optional, idempotent public entry if the application must read board-overridden effective config before any init.

Prepare and init must run from a FreeRTOS task (a valid current-task handle is required). The hook may edit writable descriptor/handle tables and call ``override_config`` / ``set_ops``; override calls write the BOARD layer. The prepare owner may initialize a resource and query its handle inside the hook. Recursive prepare in the same task does not run the hook again; other tasks cannot query intermediate state. On hook failure, the manager restores table links and BOARD overrides not used by active resources, then returns to ``UNPREPARED``. The hook must deinitialize hardware it created before returning an error; prepare rollback does not release hardware.

Use ``idf.py bmgr -b <board> --writable`` to generate writable descriptor tables. When adding C-defined nodes, raise ``ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM`` or ``ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM`` in project configuration to include the extra nodes. The default limit equals the number of generated nodes. A new device needs descriptor and handle nodes with the same name and type. A new peripheral needs a descriptor plus matching shared ``type + role`` ops or a per-name ``esp_board_periph_set_ops`` call. Nodes and config storage must remain valid for the runtime lifetime.

``manager deinit`` does not clear BOARD or APP overrides and does not move prepare back to ``UNPREPARED``. Override layer semantics, restore rules, and ``ref_count`` gating are documented in :doc:`/programming-guide/advanced-runtime`.

Runtime constraints and configuration considerations:

- A board pre-init hook may initialize resources and query their handles. It must deinitialize resources it created before returning an error.
- When configuring multiple logical peripherals, verify their physical bus relationship. BMGR does not automatically share one physical bus merely because two peripherals use the same ``port`` value.

Legacy APIs
-----------

Only the following remain as legacy / deprecated ABI:

- :cpp:func:`esp_board_periph_init_custom` — updates the shared ``(type, role)`` handler and initializes immediately; it is not the per-name variant path. Prefer the board pre-init hook or APP :cpp:func:`esp_board_periph_set_ops`.
- :cpp:func:`esp_board_manager_register_device_handle` — splices a caller-owned handle into the generated list and bypasses descriptor and APP/BOARD layers. For an existing device name, use :cpp:func:`esp_board_device_set_ops`; do not inject unknown devices through this function.

Other APIs retain their existing status and are not considered legacy or deprecated APIs.
