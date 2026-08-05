GPIO Control (``gpio_ctrl``)
============================

:link_to_translation:`zh_CN:[中文]`

Overview
--------

``gpio_ctrl`` is a GPIO control device based on the ``gpio`` peripheral. It wraps a single board-level GPIO output as a device handle that can be retrieved by name. This device is suitable for describing board-level control signals that only require setting a high or low level, such as LCD power, audio power, and module enable signals.

When BMGR initializes ``gpio_ctrl``, it references the already-initialized ``gpio`` peripheral, reads the GPIO pin number, and sets the output level to ``active_level``. The application retrieves ``periph_gpio_handle_t`` via :cpp:func:`esp_board_manager_get_device_handle` and toggles the state according to the ``active_level`` and ``default_level`` in the device configuration.

Supported Usage Modes
---------------------

``gpio_ctrl`` does not use ``sub_type`` to distinguish modes; the classification axis is the referenced ``gpio`` peripheral. One mode is currently supported:

- `GPIO Output Control`_

Minimal Configuration
---------------------

GPIO Output Control
^^^^^^^^^^^^^^^^^^^

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
        peripherals:
          - gpio_name: gpio_power

``gpio_ctrl`` binds to a single ``gpio`` peripheral through ``gpio_name``. The selected name must exist in ``board_peripherals.yaml`` and reference a peripheral of type ``gpio``.

``active_level`` is the level written during device initialization and is also the level the application should write when enabling the control signal. ``default_level`` is not written back automatically during deinitialization; the application must actively call the GPIO driver to set this level when powering off or disabling the signal.

All Fields
----------

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
        active_level: 1           # [TO_BE_CONFIRMED] Active level (0 or 1) - GPIO level when device is active
        default_level: 0          # Default level (0 or 1) - GPIO level when device is inactive
      peripherals:
        - gpio_name: gpio         # [TO_BE_CONFIRMED] GPIO peripheral name (must reference a GPIO peripheral)

Component Dependencies
----------------------

``gpio_ctrl`` does not require additional component declarations in ``dependencies``. It uses the ESP-IDF GPIO driver and the BMGR ``gpio`` peripheral.

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

Reference Code
--------------

- ``esp_board_manager/test_apps/main/test_dev_pwr_ctrl.c``
- ``esp_board_manager/devices/dev_gpio_ctrl/dev_gpio_ctrl.c``

Board Reference
---------------

- ``esp_friends_boards/esp32_c5_spot/board_devices.yaml``: configures a ``gpio_ctrl`` device.
- ``esp_friends_boards/esp32_c5_spot/board_peripherals.yaml``: configures the ``gpio`` peripheral referenced by ``gpio_ctrl``.
- ``esp_board_manager/test_apps/test_single_board/board_devices.yaml``: ``gpio_ctrl`` configuration in the test board.
- ``esp_board_manager/test_apps/test_single_board/board_peripherals.yaml``: ``gpio`` peripheral configuration in the test board.

Notes
-----

- The device uses ``gpio_name`` to select the ``gpio`` peripheral instance.
- During ``gpio_ctrl`` initialization, the GPIO is set to ``active_level``. If the board power should be off by default, verify that the initialization order and ``active_level`` setting are consistent with the hardware design.
- To deactivate the control signal, the application must actively write ``default_level`` using the retrieved ``periph_gpio_handle_t`` and the device configuration.
- After modifying YAML, re-run ``idf.py bmgr -b <board>``.

Debugging Tips
--------------

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain the device handle. The return type is ``periph_gpio_handle_t``, defined by the underlying GPIO peripheral driver. This handle can be passed to ``esp_board_device_power_ctrl()`` for runtime level control, or used directly through the GPIO peripheral API.

The device configuration struct ``dev_gpio_ctrl_config_t`` and initialization functions are located in ``esp_board_manager/devices/dev_gpio_ctrl/dev_gpio_ctrl.h``.
