Runtime Lifecycle
=================

:link_to_translation:`zh_CN:[中文]`

From a runtime perspective, BMGR enforces a fixed sequence: which peripherals are initialized first, which devices are initialized next, and how to obtain the corresponding handles and configurations.

Initialization Entry Points and Runtime Access
----------------------------------------------

:cpp:func:`esp_board_manager_init` is the full-board initialization entry point. It performs automatic initialization according to the generated board description. Correspondingly, :cpp:func:`esp_board_manager_deinit` releases runtime objects in the opposite direction.

:cpp:func:`esp_board_manager_init_device_by_name` initializes a single device by device name. It is commonly used for ``init_skip``, deferred initialization, or cases where an application opens a device only when a business condition is met.

Applications can use :cpp:func:`esp_board_manager_get_device_config` and :cpp:func:`esp_board_manager_get_periph_config` to obtain generated configuration structures. These configurations come from the C structures generated from YAML and are used to read board-level parameters.

Applications can use :cpp:func:`esp_board_manager_get_device_handle` and :cpp:func:`esp_board_manager_get_periph_handle` to obtain runtime handles. These APIs only return handles for initialized objects; they do not initialize devices or peripherals.

Key Behaviors
-------------

- **Peripherals initialize first, devices initialize second**: When :cpp:func:`esp_board_manager_init` is called, BMGR always executes in peripheral-first, device-second order.
- **Overall traversal order follows YAML declaration order**: :cpp:func:`esp_board_device_init_all` traverses devices in the order they appear in ``board_devices.yaml``.
- ``depends_on`` **declares initialization dependencies between devices**: When a device is configured with ``depends_on``, :cpp:func:`esp_board_device_init` recursively initializes the listed dependencies before initializing that device, regardless of declaration order in YAML. There is no need to manually order those devices in ``board_devices.yaml``. If a dependency has already been initialized through another path (``ref_count > 0``), it is not re-created. A device may declare multiple dependencies of any type.
- **Deinitialization converges by reference count**: Both devices and peripherals maintain an internal reference count (``ref_count``). Reinitializing the same object does not create a new instance; it increments the reference count. The resource is only actually released when the count drops to zero.
- ``init_skip`` **skips automatic initialization**: For a device with ``init_skip: true``, :cpp:func:`esp_board_manager_init` does not automatically create its runtime handle. The application must initialize the device with :cpp:func:`esp_board_manager_init_device_by_name`, or ensure it has been initialized successfully through another device's dependency chain, before calling :cpp:func:`esp_board_manager_get_device_handle` to obtain the handle.
- ``power_ctrl_device`` **controls device power-on sequencing**: When a device declares ``power_ctrl_device``, BMGR triggers a power-on action through the corresponding ``power_ctrl`` device before initializing this device; a power-off action is triggered during deinitialization. ``power_ctrl_device`` is a device-to-device reference specifically for power supply control; the referenced device must be of type ``power_ctrl``. Compared to ``depends_on``, ``power_ctrl_device`` additionally triggers power-on and power-off actions and provides the runtime power control API :cpp:func:`esp_board_device_power_ctrl`, so its effect is not limited to ensuring initialization order.
- Using ``depends_on`` and ``power_ctrl_device`` together ensures that even when a device is initialized individually via :cpp:func:`esp_board_manager_init_device_by_name`, initialization will not fail due to power supply or other dependency issues.

BMGR's runtime model does not compress all initialization logic into a single ``init()`` call; instead, it organizes the initialization order according to the board description, while preserving runtime behaviors such as reference counting, deferred initialization, inter-device dependencies, and power control.
