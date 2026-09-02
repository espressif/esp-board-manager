LEDC Control (``ledc_ctrl``)
============================

:link_to_translation:`zh_CN:[中文]`

.. _ledc-ctrl-intro:

Overview
--------

``ledc_ctrl`` is a PWM control device based on the ``ledc`` peripheral. It wraps a single LEDC channel as a device handle that can be retrieved by name. This device is suitable for describing board-level signals that require duty-cycle control, such as LCD backlight brightness and single-channel PWM output.

When BMGR initializes ``ledc_ctrl``, it references the already-initialized ``ledc`` peripheral, calculates the initial duty from ``default_percent`` and the peripheral's ``duty_resolution``, and writes it to the channel by calling the LEDC driver. The application retrieves ``periph_ledc_handle_t`` via :cpp:func:`esp_board_manager_get_device_handle` and adjusts the duty cycle directly through the LEDC driver.

.. _ledc-ctrl-usage-modes:

Supported Usage Modes
---------------------

``ledc_ctrl`` does not use ``sub_type`` to distinguish modes; the classification axis is the referenced ``ledc`` peripheral. One mode is currently supported:

- `LEDC PWM Control`_

.. _ledc-ctrl-min-config:

Minimal Configuration
---------------------

.. _ledc-ctrl-pwm:

LEDC PWM Control
^^^^^^^^^^^^^^^^

See complete fields: :ref:`ledc-ctrl-pwm-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: ledc_backlight
        type: ledc
        version: default
        config:
          gpio_num: 26             # [IO]
          speed_mode: LEDC_LOW_SPEED_MODE
          channel: 1
          timer_sel: 1
          duty: 0

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: lcd_brightness
        type: ledc_ctrl
        version: default
        config:
          default_percent: 100
        peripherals:
          - ledc_name: ledc_backlight

``ledc_ctrl`` binds to a single ``ledc`` peripheral through ``ledc_name``. The selected name must exist in ``board_peripherals.yaml`` and reference a peripheral of type ``ledc``.

``default_percent`` is the percentage written during initialization. The driver calculates the initial duty as ``duty = default_percent * (2^duty_resolution - 1) / 100`` and applies it by calling ``ledc_set_duty`` and ``ledc_update_duty``. Subsequent brightness or PWM changes are made by the application directly using the LEDC driver.

.. _ledc-ctrl-full-fields:

All Fields
----------

.. _ledc-ctrl-pwm-full:

LEDC PWM Control All Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example board_devices.yaml configuration for LEDC control device
    # This shows how to integrate the LEDC control device into a board configuration

    # Example LEDC control device configuration for LCD brightness
    - name: lcd_brightness          # The name of the device, must be unique
      type: ledc_ctrl               # The type of the device, must be unique
      version: 1.0.0
      config:
        default_percent: 100        # [TO_BE_CONFIRMED] Default brightness percentage (0-100)
      peripherals:
        - ledc_name: ledc_backlight  # LEDC peripheral name (must reference an LEDC peripheral)

.. _ledc-ctrl-deps:

Component Dependencies
----------------------

``ledc_ctrl`` does not require additional component declarations in ``dependencies``. It uses the ESP-IDF LEDC driver and the BMGR ``ledc`` peripheral.

.. _ledc-ctrl-peripherals:

Required Peripherals
--------------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - Required
     - Purpose
   * - ``ledc``
     - No ``role`` / ``format`` used
     - Required
     - Provides the LEDC channel, timer, resolution, and output GPIO

.. _ledc-ctrl-code:

Reference Code
--------------

- ``esp_board_manager/test_apps/main/test_dev_ledc.c``
- ``esp_board_manager/devices/dev_ledc_ctrl/dev_ledc_ctrl.c``

.. _ledc-ctrl-boards:

Board Reference
---------------

- ``esp_boards/esp32_p4_function_ev_board/board_devices.yaml``: configures the ``lcd_brightness`` device.
- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``: configures the ``ledc_backlight`` peripheral.
- ``esp_boards/esp32_s3_box_3/board_devices.yaml``: configures a ``ledc_ctrl`` backlight device.
- ``esp_boards/esp32_s3_box_3/board_peripherals.yaml``: configures the ``ledc`` peripheral referenced by the backlight device.

.. _ledc-ctrl-notes:

Notes
-----

- The ``ledc`` peripheral name referenced by the device must match the instance name in ``board_peripherals.yaml`` and must start with ``ledc`` or ``ledc_``.
- ``default_percent`` is used only for setting the initial duty during initialization; changing brightness at runtime requires the application to call the LEDC driver again to set the duty and update the channel.
- During ``ledc_ctrl`` deinitialization, ``ledc_stop`` is called with an idle level of ``0``.
- After modifying YAML, re-run ``idf.py bmgr -b <board>``.

.. _ledc-ctrl-debug:

Debugging Tips
--------------

.. _ledc-ctrl-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain the device handle. The return type is ``periph_ledc_handle_t``, defined by the underlying LEDC peripheral driver. This handle can be passed to ``esp_board_device_power_ctrl()`` or used directly through the LEDC peripheral API.

The device configuration struct ``dev_ledc_ctrl_config_t`` and initialization functions are located in ``esp_board_manager/devices/dev_ledc_ctrl/dev_ledc_ctrl.h``.
