GPIO Control (``gpio_ctrl``)
============================

:link_to_translation:`zh_CN:[中文]`

.. _gpio-ctrl-intro:

Overview
--------

``gpio_ctrl`` is a GPIO control device based on the ``gpio`` peripheral. It wraps a single board-level GPIO output as a device handle that can be retrieved by name. This device is suitable for describing board-level control signals that only require setting a high or low level, such as LCD power, audio power, and module enable signals.

When BMGR initializes ``gpio_ctrl``, it references the ``gpio`` peripheral, reads the GPIO pin number, and sets the initial output according to ``init_level``. If ``init_level`` is omitted or set to ``-1``, initialization uses ``active_level``. The application retrieves ``periph_gpio_handle_t`` via :cpp:func:`esp_board_manager_get_device_handle` and toggles the state according to the ``active_level`` and ``default_level`` in the device configuration.

.. _gpio-ctrl-usage-modes:

Supported Usage Modes
---------------------

``gpio_ctrl`` does not use ``sub_type`` to distinguish modes; the classification axis is the referenced ``gpio`` peripheral. One mode is currently supported:

- `GPIO Output Control`_

.. _gpio-ctrl-min-config:

Minimal Configuration
---------------------

.. _gpio-ctrl-output:

GPIO Output Control
^^^^^^^^^^^^^^^^^^^

See complete fields: :ref:`gpio-ctrl-output-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: gpio_power
        type: gpio
        role: io
        config:
          pin: 4                  # [IO]
          mode: GPIO_MODE_OUTPUT
          default_level: 0

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: power_control
        type: gpio_ctrl
        version: default
        config:
          active_level: 1
          default_level: 0
          init_level: 0
        peripherals:
          - gpio_name: gpio_power

``gpio_ctrl`` binds to a single ``gpio`` peripheral through ``gpio_name``. The selected name must exist in ``board_peripherals.yaml`` and reference a peripheral of type ``gpio``.

``active_level`` is the level that activates the control function, and ``default_level`` is the inactive level. The application uses these fields when calling the GPIO driver to activate or deactivate the function. Deinitialization does not automatically restore ``default_level``.

``init_level`` is an optional integer accepting ``-1``, ``0``, or ``1``. When omitted or set to ``-1``, device initialization writes ``active_level``; when set to ``0`` or ``1``, it writes the specified level. The example above leaves the function inactive after initialization; the application must write ``active_level`` to activate it.

.. _gpio-ctrl-full-fields:

All Fields
----------

.. _gpio-ctrl-output-full:

GPIO Output Control All Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example board_devices.yaml configuration for GPIO control device
    # This shows how to integrate the GPIO control device into a board configuration

    # Example GPIO control device configuration
    - name: power_control         # The name of the device, must be unique
      type: gpio_ctrl             # The type of the device, must be unique
      version: 1.0.0
      config:
        active_level: 1           # [TO_BE_CONFIRMED] GPIO level (0 or 1) when the control function is active
        default_level: 0          # Inactive GPIO level (0 or 1); not automatically applied on deinit
        init_level: -1            # Optional initial level (-1, 0, or 1); omitted or -1 uses active_level
      peripherals:
        - gpio_name: gpio          # [TO_BE_CONFIRMED] GPIO peripheral name (must reference a GPIO peripheral)

.. _gpio-ctrl-deps:

Component Dependencies
----------------------

``gpio_ctrl`` does not require additional component declarations in ``dependencies``. It uses the ESP-IDF GPIO driver and the BMGR ``gpio`` peripheral.

.. _gpio-ctrl-peripherals:

Required Peripherals
--------------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - Required
     - Purpose
   * - ``gpio``
     - ``io``
     - Required
     - Provides the actual GPIO pin and GPIO peripheral handle

.. _gpio-ctrl-code:

Reference Code
--------------

- ``esp_board_manager/test_apps/main/test_dev_pwr_ctrl.c``
- ``esp_board_manager/devices/dev_gpio_ctrl/dev_gpio_ctrl.c``

.. _gpio-ctrl-boards:

Board Reference
---------------

- ``esp_friends_boards/esp32_c5_spot/board_devices.yaml``: configures a ``gpio_ctrl`` device.
- ``esp_friends_boards/esp32_c5_spot/board_peripherals.yaml``: configures the ``gpio`` peripheral referenced by ``gpio_ctrl``.
- ``esp_board_manager/test_apps/test_single_board/board_devices.yaml``: ``gpio_ctrl`` configuration in the test board.
- ``esp_board_manager/test_apps/test_single_board/board_peripherals.yaml``: ``gpio`` peripheral configuration in the test board.

.. _gpio-ctrl-notes:

Notes
-----

- The ``gpio`` peripheral name referenced by the device must match the instance name in ``board_peripherals.yaml`` and must start with ``gpio`` or ``gpio_``.
- The native GPIO peripheral first writes its own ``default_level``, and ``gpio_ctrl`` then writes the device's initial level. ``init_level`` only determines the level when this device initialization completes. For signals sensitive to startup levels, align the peripheral's ``default_level`` with the device's initial level.
- Initializing an already initialized device only adds a reference; it does not write the initial level again. Initialization after the device has been fully released reapplies the initial level.
- To deactivate the control signal, the application must actively write ``default_level`` using the retrieved ``periph_gpio_handle_t`` and the device configuration.
- After upgrading to a version that supports ``init_level`` or modifying YAML, re-run ``idf.py bmgr -b <board>`` and rebuild. Older generated C files omit this field, so recompiling them initializes it to ``0`` rather than the YAML default of ``-1``.

.. _gpio-ctrl-debug:

Debugging Tips
--------------

.. _gpio-ctrl-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain the device handle. The return type is ``periph_gpio_handle_t``, defined by the underlying GPIO peripheral driver. Set the runtime level with ``gpio_set_level(handle->gpio_num, level)``, using the device configuration's ``active_level`` or ``default_level`` for ``level``.

For hand-written C configurations, explicitly set ``init_level = -1`` to initialize at ``active_level``. Omitting the C struct field or zero-initializing the struct sets ``init_level`` to ``0``, which selects a low initial output.

The device configuration struct ``dev_gpio_ctrl_config_t`` and initialization functions are located in ``esp_board_manager/devices/dev_gpio_ctrl/dev_gpio_ctrl.h``.
