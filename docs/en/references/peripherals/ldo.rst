ldo
=======

:link_to_translation:`zh_CN:[中文]`

.. _ldo-intro:

Overview
--------

The ``ldo`` peripheral type describes an on-chip general-purpose LDO channel. BMGR generates an ``esp_ldo_channel_config_t`` from this configuration and calls the ESP-IDF ``esp_ldo_regulator.h`` to acquire the LDO channel. This type is used for board-level resources that need to be powered by the SoC on-chip LDO, such as MIPI DSI or CSI related power supplies.

After configuring ``ldo`` in ``board_peripherals.yaml``, supported devices can reference the peripheral by name in their device-side ``peripherals`` list. BMGR obtains the LDO handle via the peripheral reference when initializing the device and releases the reference when the device is destroyed.

.. _ldo-working-modes:

Supported Operating Modes
--------------------------

``ldo`` does not use ``role`` or ``format`` to split modes. The configuration axis is a single LDO channel.

- `LDO Channel`_

.. _ldo-min-config:

Minimal Configuration
---------------------

.. _ldo-channel:

LDO Channel
^^^^^^^^^^^

See complete fields: :ref:`ldo-channel-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: ldo_mipi
        type: ldo
        config:
          chan_id: 3
          voltage_mv: 2500
          adjustable: 1
          owned_by_hw: 0

.. _ldo-mode-notes:

Mode Description
----------------

``ldo`` creates one on-chip LDO channel. ``chan_id``, ``voltage_mv``, ``adjustable``, and ``owned_by_hw`` are generated directly into ``esp_ldo_channel_config_t`` for the ESP-IDF LDO driver to acquire the channel.

When using this peripheral, the LDO channel and output voltage must come from the target SoC datasheet and board schematic. The device side only references the LDO peripheral name; LDO channel, voltage, and ownership configuration must not be written into device-side reference entries.

.. _ldo-full-fields:

Full Field Reference
--------------------

.. _ldo-channel-full:

LDO Channel — Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: ldo_mipi
      type: ldo
      config:
        # LDO (Low-Dropout Regulator) configuration for power management
        # This is an hardware-specific configuration, please refer your SoC's datasheet for valid LDO channel IDs
        # You must specify a LDO channel ID manually, based on your board schematic.
        # e.g. For ESP32_P4, the valid values for chan_id are 1-4,
        # where channel 1 and channel 2 are respectively used for powering the internal Flash and PSRAM,
        # channels 3 and 4 can be used to power external devices
        # In P4 Function-EV Board, channel 3 is used for powering the MIPI phy
        chan_id: 3                     # [TO_BE_CONFIRMED] LDO channel ID

        # Output voltage in millivolts (default: 2500 for 2.5V)
        # Common values: equal to 3300 or between 500 to 2700, depending on device requirements
        voltage_mv: 2500                # [TO_BE_CONFIRMED] Output voltage in millivolts

        # Whether voltage is adjustable (default: 1 for true)
        # Valid values: 0 (fixed), 1 (adjustable)
        adjustable: 1

        # Hardware ownership flag (default: 0 for software control)
        # Valid values: 0 (software-controlled), 1 (hardware-controlled)
        owned_by_hw: 0

Field sources:

- YAML template: ``esp_board_manager/peripherals/periph_ldo/periph_ldo.yml``.
- Header file: ``esp_board_manager/peripherals/periph_ldo/periph_ldo.h``.

.. _ldo-devices:

Applicable Devices
------------------

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Notes
   * - ``display_lcd``
     - The ``dsi`` sub-type references ``ldo_mipi`` in the device-side ``peripherals``
     - DSI display devices manage MIPI power supply references via the LDO peripheral
   * - ``camera``
     - The ``csi`` sub-type references ``ldo_mipi`` in the device-side ``peripherals``
     - CSI camera devices can use ``dont_init_ldo`` together with the LDO peripheral to avoid duplicate LDO initialization inside the device

.. _ldo-code:

Reference Code
--------------

- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_dsi.c``
- ``esp_board_manager/devices/dev_camera/dev_camera_sub_csi.c``

.. _ldo-boards:

Board-Level Reference
---------------------

- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``: ``ldo_mipi`` configuration.
- ``esp_boards/esp32_p4_function_ev_board/board_devices.yaml``: ``display_lcd`` and ``camera`` reference ``ldo_mipi``.
- ``m5stack_boards/m5stack_tab5/board_peripherals.yaml``: ``ldo_mipi`` configuration.
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``: ``display_lcd`` and ``camera`` reference ``ldo_mipi``.

.. _ldo-notes:

Notes
-----

- ``chan_id`` and ``voltage_mv`` are board-level hardware parameters that must be filled in according to the SoC datasheet and schematic.
- ``chan_id`` and ``voltage_mv`` must be non-negative integers.
- ``adjustable`` and ``owned_by_hw`` only accept ``0`` or ``1``.
- When the same LDO is referenced by multiple devices, each device-side entry uses the same peripheral name; BMGR manages the handle with reference counting.
- After modifying the LDO peripheral configuration, re-run ``idf.py bmgr -b <board>``.

.. _ldo-debug:

Debugging Tips
--------------

.. _ldo-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to retrieve the LDO peripheral handle. The handle type is the ESP-IDF native ``esp_ldo_channel_handle_t``, which can be passed to the ``esp_ldo_channel_*`` APIs for voltage adjustment.

Related declarations are in ``esp_board_manager/peripherals/periph_ldo/periph_ldo.h``.

Underlying ESP-IDF driver documentation: `Low-Dropout Regulator (LDO) <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32p4/api-reference/peripherals/ldo_regulator.html>`__.
