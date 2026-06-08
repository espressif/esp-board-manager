Touch Controller (``lcd_touch``)
=================================

:link_to_translation:`zh_CN:[中文]`

Overview
--------

``lcd_touch`` is a generic touch controller device that wraps a touch controller chip, the touch bus, and an ``esp_lcd_touch`` driver instance into a single BMGR device. This device is suitable for board configurations that have migrated to the generic touch model: the specific driver component is selected via ``chip`` and ``dependencies``, and the bus implementation is selected via ``sub_type``.

In the current source code, ``lcd_touch`` only implements ``sub_type: i2c``. ``sub_type: spi`` is reserved in the header file and Kconfig, but the parser will reject this configuration and the SPI sub-type source file is not included in the component build.

Supported Usage Modes
---------------------

``lcd_touch`` distinguishes touch bus modes with ``sub_type``:

- `I2C Touch`_
- `SPI Touch (Reserved)`_

Minimal Configuration
---------------------

I2C Touch
^^^^^^^^^^

In I2C mode, ``lcd_touch`` uses the ``i2c`` peripheral to provide the bus handle. During initialization it creates an LCD panel IO I2C handle, then calls the ``lcd_touch_factory_entry_t`` linked in the project to create the specific touch driver. The touch chip dependency component must be declared in ``dependencies``. The ``i2c_addr`` in the device-side ``peripherals`` reference entry uses 8-bit left-shifted format; after a successful probe at runtime it is shifted right for use by the ESP-IDF I2C API. Up to 4 candidate addresses can be specified; the parser rejects odd addresses, addresses above ``0xfe``, and empty lists.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: i2c_master
        type: i2c
        role: master
        config:
          port: 0
          pins:
            sda: 7               # [IO]
            scl: 8               # [IO]

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: lcd_touch
        chip: gt911              # [TO_BE_CONFIRMED]
        type: lcd_touch
        sub_type: i2c
        version: default
        dependencies:
          espressif/esp_lcd_touch_gt911: "*"
        config:
          io_i2c_config:
            lcd_cmd_bits: 16
          touch_config:
            x_max: 1024          # [TO_BE_CONFIRMED]
            y_max: 600           # [TO_BE_CONFIRMED]
        peripherals:
          - name: i2c_master
            i2c_addr: [0xBA, 0x28]

SPI Touch (Reserved)
^^^^^^^^^^^^^^^^^^^^

``sub_type: spi`` cannot currently be used in ``board_devices.yaml``. The parser will report an error indicating this sub-type is reserved but not yet implemented.

All Fields
----------

I2C Touch All Fields
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example board_devices.yaml configuration for LCD Touch device
    # This shows how to integrate the generic LCD touch device into a board configuration.
    # The generic type uses sub_type to select the bus implementation. Only sub_type: i2c
    # is implemented now; sub_type: spi is reserved for a future implementation.
    # I2C addresses are 8-bit / left-shifted values, matching audio_codec and gpio_expander.
    # ESP-IDF I2C runtime APIs receive the selected address after shifting right by one.

    - name: lcd_touch          # The name of the device, must be unique
      chip: generic_touch      # [TO_BE_CONFIRMED] Touch chip type (e.g., cst816s, ft5x06, gt911, tt21100)
      type: lcd_touch          # Generic LCD touch device type
      sub_type: i2c            # Touch bus sub-type; only i2c is supported in this phase
      version: 1.0.0
      dependencies:
        espressif/esp_lcd_touch_generic: "*"  # [TO_BE_CONFIRMED] Component dependency for the touch chip
      config:
        # esp_lcd_panel_io_i2c_config_t fields for I2C communication
        io_i2c_config:
          control_phase_bytes: 1            # Control phase bytes (default: 1)
          dc_bit_offset: 0                  # DC bit offset in control phase (default: 0)
          lcd_cmd_bits: 8                   # [TO_BE_CONFIRMED] Bit-width of LCD command (default: 8)
          lcd_param_bits: 0                 # Bit-width of LCD parameter (default: 0)
          scl_speed_hz: 100000              # I2C SCL frequency (default: 100kHz)
          transaction_timeout_ms: 0         # IDF v6.1+: transfer timeout in ms; 0/-1 waits forever (default: 0)
          flags:
            dc_low_on_data: false           # DC level for data transfer (default: false)
            disable_control_phase: true     # Disable control phase for touch (default: true)

        # esp_lcd_touch_config_t fields for touch configuration
        touch_config:
          x_max: 320                        # [TO_BE_CONFIRMED] Maximum X coordinate (default: 320)
          y_max: 240                        # [TO_BE_CONFIRMED] Maximum Y coordinate (default: 240)
          rst_gpio_num: -1                  # [IO] Reset GPIO (default: -1, GPIO_NUM_NC)
          int_gpio_num: -1                  # [IO] Interrupt GPIO (default: -1, GPIO_NUM_NC)
          levels:
            reset: 0                        # Reset pin active level (default: 0)
            interrupt: 0                    # Interrupt pin active level (default: 0)
          flags:
            swap_xy: false                  # Swap X and Y coordinates (default: false)
            mirror_x: false                 # Mirror X coordinates (default: false)
            mirror_y: false                 # Mirror Y coordinates (default: false)
      peripherals:
        - name: i2c_master              # I2C peripheral for touch communication
          i2c_addr: [0xba]              # [TO_BE_CONFIRMED] I2C address candidates, 8-bit / left-shifted values, up to 4 entries

SPI Touch All Fields
^^^^^^^^^^^^^^^^^^^^^

``sub_type: spi`` currently has no available YAML template. Only the ``dev_lcd_touch_spi_sub_config_t`` and the unbuilt ``dev_lcd_touch_sub_spi.c`` reserved implementation exist in the source code.

Component Dependencies
----------------------

``lcd_touch`` introduces the common component ``espressif/esp_lcd_touch`` (version ``"*"``) via ``esp_board_manager/idf_component.yml`` when ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT`` is enabled.

The specific touch chip driver must be declared in the device entry's ``dependencies``. In existing board configurations, GT911 touch uses ``espressif/esp_lcd_touch_gt911: "*"``. The ``espressif/esp_lcd_touch_generic: "*"`` in the YAML template is a placeholder for the touch chip component to be confirmed; board maintainers should replace it with the component corresponding to the actual touch chip.

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
     - Required for ``sub_type: i2c``
     - Provides the touch controller communication bus; ``i2c_addr`` is filled in the device-side reference entry

Reference Code
--------------

- ``esp_board_manager/test_apps/main/test_dev_lcd_init.c``
- ``esp_board_manager/devices/dev_lcd_touch/dev_lcd_touch.c``
- ``esp_board_manager/devices/dev_lcd_touch/dev_lcd_touch_sub_i2c.c``

Board Reference
---------------

- ``esp_boards/esp32_p4_function_ev_board/board_devices.yaml``: GT911 ``lcd_touch`` I2C configuration.
- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``: ``i2c_master`` configuration referenced by the touch device.
- ``esp_boards/esp32_s3_box_3/board_devices.yaml``: I2C touch configuration.
- ``m5stack_boards/m5stack_cores3/board_devices.yaml``: I2C touch configuration.

Notes
-----

- New configurations use ``type: lcd_touch`` with ``sub_type: i2c``.
- ``sub_type: spi`` is currently unavailable; do not configure this sub-type in board YAML.
- ``i2c_addr`` uses 8-bit left-shifted addresses; up to 4 candidate values. The source code probes each address in turn and records the valid address found.
- The project must provide an ``lcd_touch_factory_entry_t`` to create an ``esp_lcd_touch_handle_t`` from the touch chip component.
- After modifying YAML, re-run ``idf.py bmgr -b <board>``.

Factory function and multi-address selection
--------------------------------------------

The project must provide an ``lcd_touch_factory_entry_t``, which BMGR calls back when creating the touch device to build an ``esp_lcd_touch_handle_t`` from the touch chip component. The signature is:

.. code-block:: c

   esp_err_t lcd_touch_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                       const esp_lcd_touch_config_t *touch_dev_config,
                                       esp_lcd_touch_handle_t *ret_touch)

If the same board may carry different touch chips (that is, ``i2c_addr`` lists several candidate addresses), the factory function can read the actually probed address with ``esp_board_device_get_i2c_effective_addr()`` and select the matching driver:

.. code-block:: c

   uint16_t touch_addr = 0;
   esp_err_t ret = esp_board_device_get_i2c_effective_addr("lcd_touch", &touch_addr);
   if (ret != ESP_OK) {
       return ret;
   }

   if (touch_addr == 0xba) {
       return esp_lcd_touch_new_i2c_gt911(io, touch_dev_config, ret_touch);
   }

   if (touch_addr == 0x48) {
       return esp_lcd_touch_new_i2c_tt21100(io, touch_dev_config, ret_touch);
   }

``esp_board_device_get_i2c_effective_addr()`` returns the 8-bit left-shifted address, matching the YAML ``i2c_addr`` semantics.

Debugging Tips
--------------

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain the device handle. The handle type is ``dev_lcd_touch_handles_t``:

.. code-block:: c

   typedef struct {
       esp_lcd_touch_handle_t     touch_handle;  /*!< LCD touch driver handle */
       esp_lcd_panel_io_handle_t  io_handle;     /*!< LCD panel IO handle */
   } dev_lcd_touch_handles_t;

The related declarations are located in ``esp_board_manager/devices/dev_lcd_touch/dev_lcd_touch.h``.
