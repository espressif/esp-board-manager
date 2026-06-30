Power Control (``power_ctrl``)
==============================

:link_to_translation:`zh_CN:[中文]`

Overview
--------

The ``power_ctrl`` device wraps a board-level power enable signal as a reusable device. Other devices can reference it via the ``power_ctrl_device`` field to trigger power-on or power-off control during device initialization and shutdown.

The current template and implementation support ``sub_type: gpio``. In this mode, a single ``gpio`` peripheral sets the power control pin level; ``active_level`` represents the active-high or active-low sense for power-on.

Supported Usage Modes
---------------------

``power_ctrl`` distinguishes usage modes with ``sub_type``:

- `GPIO Power Control`_

Minimal Configuration
---------------------

GPIO Power Control
^^^^^^^^^^^^^^^^^^

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: gpio_power_audio
        type: gpio
        role: io
        config:
          pin: 46
          mode: GPIO_MODE_OUTPUT

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: audio_power_ctrl
        type: power_ctrl
        sub_type: gpio
        peripherals:
          - name: gpio_power_audio
            active_level: 1

      - name: audio_dac
        chip: es8311
        type: audio_codec
        power_ctrl_device: audio_power_ctrl
        config:
          adc_enabled: false
          dac_enabled: true
        peripherals:
          - name: i2s_audio_out
          - name: i2c_master
            address: 0x30
            frequency: 400000

In ``gpio`` mode, initialization references the ``gpio`` peripheral from the configuration and saves the peripheral handle. When a power-on request is received, the GPIO is set to ``active_level``; when a power-off request is received, it is set to the opposite level. The ``power_ctrl`` device itself only defines the power control resource; devices that need to be controlled reference this device name via the ``power_ctrl_device`` field, for example in board configurations for ``audio_codec``, ``fs_fat``, or ``display_lcd``.

All Fields
----------

GPIO Power Control All Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example Power Control device with GPIO sub type configuration
    - name: audio_power_ctrl          # The name of the device, must be unique
      type: power_ctrl                # The type of the device, must be unique
      sub_type: gpio                  # The sub type of the device, must be 'gpio'
      peripherals:
        - name: gpio                  # [TO_BE_CONFIRMED] GPIO peripheral name (must reference a GPIO peripheral)
          active_level: 1             # [TO_BE_CONFIRMED] Active level (0-low, 1-high) when power is on

    # Example usage in devices, add the power_ctrl_device attribute to the device configuration
    # - name: audio_dac
    #   chip: es8311
    #   type: audio_codec
    #   version: default
    #   power_ctrl_device: audio_power_ctrl  # Reference to power control device
    #   config:
    #     adc_enabled: false
    #     dac_enabled: true
    #     dac_max_channel: 1
    #     dac_channel_mask: "1"
    #     mclk_enabled: true
    #   peripherals:
    #     - name: i2s_audio_out
    #     - name: i2c_master
    #       address: 0x30
    #       frequency: 400000

Component Dependencies
----------------------

The ``gpio`` mode of ``power_ctrl`` uses the ESP-IDF GPIO driver and the BMGR ``gpio`` peripheral. The current device template does not require additional ``dependencies`` declarations in ``board_devices.yaml``.

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
     - Required for ``sub_type: gpio``
     - Provides the power enable GPIO

Reference Code
--------------

- ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl.c``
- ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl_sub_gpio.c``
- Board customization workflow: :doc:`/create-board/index`

Board Reference
---------------

- ``esp_boards/esp_vocat_1_2/board_devices.yaml``
- ``esp_boards/esp_vocat_1_0/board_devices.yaml``
- ``esp_boards/esp32_lyrat_mini_1_1/board_devices.yaml``
- ``esp_boards/esp32_s3_box_3/board_devices.yaml``
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``
- ``esp_friends_boards/esp32_c5_spot/board_devices.yaml``

Notes
-----

- For common YAML field rules, see :doc:`/programming-guide/yaml-rules`.
- The ``power_ctrl_device`` field of the controlled device must reference a defined ``power_ctrl`` device name.
- ``active_level`` must match the board power switch circuit; the driver outputs the opposite level when powering off.
- The GPIO peripheral referenced by ``power_ctrl`` should be configured as output mode.
- After modifying YAML, re-run ``idf.py bmgr -b <board>``.

Debugging Tips
--------------

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain the device handle. The handle type is ``dev_power_ctrl_handle_t``:

.. code-block:: c

   typedef struct {
       void *periph_handle;  /*!< Peripheral handle */
   } dev_power_ctrl_handle_t;

``periph_handle`` points to the underlying peripheral driver handle (for the GPIO sub-type, this is the corresponding GPIO peripheral handle). It is generally invoked indirectly by ``esp_board_device_power_ctrl()`` through the ``power_ctrl_device`` reference and does not need to be operated directly.

The related declarations are located in ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl.h``.
