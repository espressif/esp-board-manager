GPIO Expander (``gpio_expander``)
==================================

:link_to_translation:`zh_CN:[中文]`

Overview
--------

The ``gpio_expander`` device describes an I2C-connected IO expander chip. After initialization it returns an ``esp_io_expander_handle_t`` handle. Board code or other devices can retrieve this handle via :cpp:func:`esp_board_manager_get_device_handle` and then set the direction, level, or pull capability of the expanded IOs.

This type is suitable for boards where the native GPIO count is insufficient, or where LCD initialization signals, buttons, or power control pins are connected to an IO expander chip.

Supported Usage Modes
---------------------

``gpio_expander`` currently supports one usage mode — I2C IO expansion:

- :ref:`gpio-expander-i2c`

Minimal Configuration
---------------------

.. _gpio-expander-i2c:

I2C IO Expansion
^^^^^^^^^^^^^^^^

``board_peripherals.yaml`` requires at least one ``i2c`` master peripheral.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: gpio_expander
        chip: tca9554                     # [TO_BE_CONFIRMED] IO expander chip
        type: gpio_expander
        dependencies:
          espressif/esp_io_expander_tca9554: "*"  # [TO_BE_CONFIRMED]
        config:
          max_pins: 8
          output_io_mask: [1, 2, 3]
          output_io_level_mask: [1, 1, 0]
          input_io_mask: NULL
        peripherals:
          - i2c_name: i2c_master
            i2c_addr: [0x70, 0x7A]        # [TO_BE_CONFIRMED]

During initialization, ``gpio_expander`` references the ``i2c`` peripheral handle and probes the addresses in the ``i2c_addr`` list for a responding device. After the driver is created successfully, the device sets the output pins, input pins, default output levels, optional output modes, and optional pull configurations according to the config. After initialization, BMGR records the detected valid I2C address for use by other logic on the same I2C bus.

Board-Level IO Expander Factory Function
----------------------------------------

``gpio_expander`` has no built-in chip constructor. Every board that uses this device type must provide ``io_expander_factory_entry_t`` in a board source file. BMGR calls it with the detected I2C address to create the ``esp_io_expander_handle_t``.

The function signature is:

.. code-block:: c

    esp_err_t io_expander_factory_entry_t(i2c_master_bus_handle_t i2c_bus,
                                          const uint16_t dev_addr,
                                          esp_io_expander_handle_t *handle_ret);

For example, TCA9554 uses the constructor provided by its component:

.. code-block:: c

    #include "esp_io_expander_tca9554.h"

    __attribute__((weak)) esp_err_t io_expander_factory_entry_t(
            i2c_master_bus_handle_t i2c_bus,
            const uint16_t dev_addr,
            esp_io_expander_handle_t *handle_ret)
    {
        return esp_io_expander_new_i2c_tca9554(i2c_bus, dev_addr, handle_ret);
    }

The component in ``dependencies`` must provide the constructor called by the factory function and match the YAML ``chip`` field. If a board configures multiple ``gpio_expander`` devices with different chips, the factory function must select the appropriate chip constructor by ``dev_addr``. See :doc:`/programming-guide/board-directory` for board source placement and weak-symbol override rules.

All Fields
----------

I2C IO Expansion All Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example board_devices.yaml configuration for IO expander device
    # This shows how to integrate the IO expander device into a board configuration

    # Example IO expander device configuration
    - name: gpio_expander                 # The name of the device, must be unique
      chip: tca9554                       # [TO_BE_CONFIRMED] The chip of the IO expander
      type: gpio_expander                 # The type of the device, must be unique
      dependencies:
        espressif/esp_io_expander_generic: "*"  # [TO_BE_CONFIRMED] Component dependency for the IO expander
      config:
        max_pins: 8                       # Maximum number of IO pins supported
        output_io_mask: [1, 2, 3]         # List of pins configured as output, maximum number is 32
        output_io_level_mask : [1, 1, 0]  # List of output levels for output pins (eg. set io 1,2 to high level, 3 to low level)
        output_io_mode_mask: [0, 1, 1]    # Optional configuration, need to determine if the IO expansion chip is supported
                                          # List of output modes for output pins (0: push-pull, 1: open-drain),
        io_pullup_list: [1, 2]            # Optional configuration, need to determine if the IO expansion chip is supported
                                          # List of pins configured with pull-up resistors
        io_pulldown_list: [3, 4]          # Optional configuration, need to determine if the IO expansion chip is supported
                                          # List of pins configured with pull-down resistors
        input_io_mask: NULL               # List of pins configured as input (NULL if unused), maximum number is 32
      peripherals:
        - i2c_name: i2c_master             # I2C peripheral used by the IO expander
          i2c_addr: [0x70, 0x7A]          # [TO_BE_CONFIRMED] I2C address of the IO expander

Component Dependencies
----------------------

``gpio_expander`` requires the IO expander chip driver component to be declared in ``dependencies``. The template uses ``espressif/esp_io_expander_generic`` as a placeholder; board configurations should replace it with the component matching the ``chip`` and ``io_expander_factory_entry_t``.

Required Peripherals
--------------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - Required
     - Purpose
   * - ``i2c``
     - ``master``
     - Required
     - IO expander chip communication

Reference Code
--------------

- ``esp_board_manager/test_apps/main/test_dev_gpio_expander.c``: retrieves the ``gpio_expander`` config and handle, and prints the IO expander state.
- ``esp_board_manager/devices/dev_gpio_expander/dev_gpio_expander.c``: I2C address probing, IO direction, and level initialization implementation.

Board Reference
---------------

- ``esp_boards/esp32_s3_korvo_2_3/board_devices.yaml``: ``gpio_expander`` configuration.
- ``esp_boards/esp32_s3_korvo_2_3/board_peripherals.yaml``: I2C peripheral configuration.
- ``esp_boards/esp32_s3_korvo_2_3/setup_device.c``: IO expander factory function.
- ``esp_boards/esp32_s3_lcd_ev_board/board_devices.yaml``: ``gpio_expander`` and ``rgb_3wire_spi`` LCD on the same board.
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``: two ``gpio_expander`` device configurations.
- ``m5stack_boards/m5stack_cores3/board_devices.yaml``: ``gpio_expander`` configuration.

Notes
-----

- The template uses an address list for ``i2c_addr``; initialization probes each address in the list and selects the responding one.
- ``output_io_mask``, ``output_io_level_mask``, ``output_io_mode_mask``, ``io_pullup_list``, and ``io_pulldown_list`` must match the specific chip's capabilities; unsupported capabilities must not be written into the board configuration.
- Expanded IOs referenced by other devices must be available before those devices are initialized.
- Without ``io_expander_factory_entry_t``, ``gpio_expander`` cannot complete initialization; adding only the chip component dependency does not replace this function.
- After modifying the IO expander device or I2C peripheral configuration, re-run ``idf.py bmgr -b <board>``.

Debugging Tips
--------------

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain the device handle. The return type is ``esp_io_expander_handle_t``, defined by the ``espressif/esp_io_expander`` component. This handle can be passed directly to the IO expander component APIs (read/write IO levels, configure direction, etc.).

The device configuration struct ``dev_io_expander_config_t`` and initialization functions are located in ``esp_board_manager/devices/dev_gpio_expander/dev_gpio_expander.h``.
