Knob (knob)
===========

:link_to_translation:`zh_CN:[中文]`

.. _knob-intro:

Overview
--------

The ``knob`` device initializes a low-speed quadrature rotary encoder through the ``espressif/knob`` component. Applications obtain ``dev_knob_handles_t`` from :cpp:func:`esp_board_manager_get_device_handle` and use its ``knob_handle`` with the component APIs to register rotation callbacks or read the count.

The device exposes one input interface, ``sub_type: gpio``. ``use_rtc`` chooses the GPIO HAL used by the upstream component; it does not create another application-facing interface.

.. _knob-usage-modes:

Supported Usage Mode
--------------------

- :ref:`GPIO Quadrature Knob <knob-gpio>`

.. _knob-min-config:

Minimal Configuration
---------------------

.. _knob-gpio:

GPIO Quadrature Knob
^^^^^^^^^^^^^^^^^^^^

See complete fields: :ref:`knob-gpio-full`.

``knob`` owns the encoder GPIOs directly. Do not declare separate BMGR ``gpio`` peripherals for the encoder A/B pins.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: knob
        type: knob
        sub_type: gpio
        config:
          gpio_encoder_a: 41       # [IO]
          gpio_encoder_b: 40       # [IO]
          default_direction: 0
          enable_power_save: false
          use_rtc: false

Set ``use_rtc: true`` only when both encoder pins are RTC-capable. BMGR then calls ``iot_knob_create_rtc()``; otherwise it calls ``iot_knob_create()``.

.. _knob-full-fields:

All Fields
----------

.. _knob-gpio-full:

GPIO Quadrature Knob All Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Knob device configuration example
    - name: knob
      type: knob
      sub_type: gpio
      config:
        gpio_encoder_a: 41       # [TO_BE_CONFIRMED] Encoder phase A GPIO
        gpio_encoder_b: 40       # [TO_BE_CONFIRMED] Encoder phase B GPIO
        default_direction: 0     # 0: positive count for the default direction; 1: negative
        enable_power_save: false # Enable the upstream knob driver's power-save path
        use_rtc: false           # Use RTC GPIO HAL; both encoder GPIOs must be RTC-capable

.. _knob-deps:

Component Dependencies
----------------------

Enabling ``CONFIG_ESP_BOARD_DEV_KNOB_SUPPORT`` adds the public ``espressif/knob`` dependency at version ``^1.1.0``. Board YAML does not need to repeat this dependency.

.. _knob-peripherals:

Required Peripherals
--------------------

No BMGR peripheral is required. The ``espressif/knob`` component initializes and releases both encoder pins.

.. _knob-code:

Reference Code
--------------

- ``esp_board_manager/test_apps/main/test_dev_knob.c``
- ``esp_board_manager/devices/dev_knob/dev_knob.c``

.. _knob-notes:

Notes
-----

- ``gpio_encoder_a`` and ``gpio_encoder_b`` must be different non-negative GPIO numbers.
- GPIO numbers and ``default_direction`` must be YAML integers; floats, booleans, and numeric strings are not converted automatically.
- ``default_direction`` accepts ``0`` or ``1``.
- ``use_rtc`` requires RTC-capable GPIOs. The regular GPIO backend remains suitable when low-power RTC GPIO handling is not needed.
- ``knob`` does not accept non-empty ``peripherals`` bindings; the device owns both encoder pins directly.
- This device uses the upstream software quadrature decoder. Use the independent ``pcnt`` peripheral when an application needs hardware pulse counting.
- After modifying YAML, run ``idf.py bmgr -b <board>`` before building.

.. _knob-debug:

Debugging Tips
--------------

Use the test application command ``case run knob.read`` on a board that defines a knob. The test clears the count, registers left/right callbacks, and reports the count after ten seconds.

.. _knob-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain ``dev_knob_handles_t``:

.. code-block:: c

    typedef struct {
        knob_handle_t  knob_handle;
    } dev_knob_handles_t;

Pass ``knob_handle`` to ``iot_knob_register_cb()``, ``iot_knob_get_count_value()``, and ``iot_knob_clear_count_value()``. The declarations are in ``esp_board_manager/devices/dev_knob/dev_knob.h``.
