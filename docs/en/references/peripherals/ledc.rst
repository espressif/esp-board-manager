ledc
========

:link_to_translation:`zh_CN:[中文]`

.. _ledc-intro:

Overview
--------

The ``ledc`` peripheral type describes a single LEDC PWM output channel. BMGR uses this configuration to initialize the LEDC timer and channel, and returns a ``periph_ledc_handle_t`` containing the channel and speed mode.

This peripheral is suited for board-level backlight control and simple PWM output scenarios. Brightness percentage, default brightness, and other device semantics belong to the ``ledc_ctrl`` device configuration and must not be written into the ``ledc`` peripheral ``config``.

.. _ledc-working-modes:

Supported Operating Modes
--------------------------

``ledc`` is currently configured as a single PWM output channel and does not split operating modes via ``role``.

- `PWM Output`_

.. _ledc-min-config:

Minimal Configuration
---------------------

.. _ledc-pwm:

PWM Output
^^^^^^^^^^

See complete fields: :ref:`ledc-pwm-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: ledc_backlight
        type: ledc
        config:
          gpio_num: 22
          channel: LEDC_CHANNEL_0
          timer_sel: LEDC_TIMER_0
          freq_hz: 5000
          duty: 0
          duty_resolution: LEDC_TIMER_10_BIT
          speed_mode: LEDC_LOW_SPEED_MODE

.. _ledc-mode-notes:

Mode Description
----------------

The ``ledc`` peripheral configures both the LEDC timer and the channel. If multiple LEDC peripherals share the same timer, the timer-related parameters must be consistent; if the frequency or resolution differs, select a different ``timer_sel``.

.. _ledc-full-fields:

Full Field Reference
--------------------

.. _ledc-pwm-full:

PWM Output — Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # LEDC Peripheral Default Configuration
    # This file shows the default values used by the LEDC peripheral parser
    # Based on periph_ledc.py parsing script

    - name: ledc_backlight
      type: ledc
      config:
        # GPIO pin number (required, no default - must be >= 0)
        gpio_num: 0                     # [IO] GPIO pin number

        # LEDC channel (default: LEDC_CHANNEL_0)
        channel: "LEDC_CHANNEL_0"
        # Valid values: 0-7 or "LEDC_CHANNEL_0" to "LEDC_CHANNEL_7"

        # LEDC timer (default: LEDC_TIMER_0)
        timer_sel: "LEDC_TIMER_0"
        # Valid values: 0-3 or "LEDC_TIMER_0" to "LEDC_TIMER_3"

        # PWM frequency in Hz (default: 4000)
        freq_hz: 4000                  # [TO_BE_CONFIRMED] PWM frequency in Hz

        # Initial duty cycle (default: 0)
        duty: 0
        # Valid values: 0 to 2^duty_resolution

        # Duty resolution in bits (default: LEDC_TIMER_13_BIT)
        duty_resolution: "LEDC_TIMER_13_BIT"  # [TO_BE_CONFIRMED] Duty resolution in bits
        # Valid values: 1-20 or "LEDC_TIMER_1_BIT" to "LEDC_TIMER_20_BIT"

        # Speed mode (default: LEDC_LOW_SPEED_MODE)
        speed_mode: "LEDC_LOW_SPEED_MODE"
        # Valid values:
        # - LEDC_LOW_SPEED_MODE
        # - LEDC_HIGH_SPEED_MODE

        # Output inversion (default: false)
        output_invert: false

        # Sleep mode (default: LEDC_SLEEP_MODE_NO_ALIVE_NO_PD)
        sleep_mode: "LEDC_SLEEP_MODE_NO_ALIVE_NO_PD"
        # Valid values:
        # - LEDC_SLEEP_MODE_NO_ALIVE_NO_PD
        # - LEDC_SLEEP_MODE_NO_ALIVE_ALLOW_PD
        # - LEDC_SLEEP_MODE_KEEP_ALIVE

.. _ledc-devices:

Applicable Devices
------------------

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Notes
   * - ``ledc_ctrl``
     - Device-side ``peripherals`` references the ``ledc`` peripheral
     - Brightness control parameters such as ``default_percent`` are written on the device side; PWM GPIO, timer, channel, frequency, and resolution are written on the peripheral side

.. _ledc-code:

Reference Code
--------------

- ``esp_board_manager/peripherals/periph_ledc/periph_ledc.c``
- ``esp_board_manager/devices/dev_ledc_ctrl/dev_ledc_ctrl.c``
- ``esp_board_manager/test_apps/main/test_dev_ledc.c``

.. _ledc-boards:

Board-Level Reference
---------------------

- ``m5stack_boards/m5stack_tab5/board_peripherals.yaml``: defines the LCD backlight ``ledc_backlight``.
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``: ``ledc_ctrl`` device references ``ledc_backlight``.
- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``: defines the LCD backlight ``ledc_backlight``.
- ``esp_boards/esp32_s3_box_3/board_peripherals.yaml``: defines the LCD backlight ``ledc_backlight``.
- ``esp_boards/esp_vocat_1_0/board_peripherals.yaml``: defines the LCD backlight ``ledc_backlight``.

.. _ledc-notes:

Notes
-----

- ``gpio_num`` is the PWM output GPIO and must be confirmed against the target chip and board wiring.
- The valid range of ``duty`` is determined by ``duty_resolution``.
- After a ``ledc_ctrl`` device references the LEDC peripheral, do not duplicate the LEDC channel, timer, frequency, or GPIO in the device-side configuration.
- After modifying the LEDC peripheral configuration, re-run ``idf.py bmgr -b <board>``.

.. _ledc-debug:

Debugging Tips
--------------

.. _ledc-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to retrieve the LEDC peripheral handle. The handle type is ``periph_ledc_handle_t``:

.. code-block:: c

   typedef struct {
       ledc_channel_t  channel;     /*!< LEDC channel number */
       ledc_mode_t     speed_mode;  /*!< LEDC speed mode */
   } periph_ledc_handle_t;

``channel`` and ``speed_mode`` can be passed to ``ledc_set_duty``, ``ledc_update_duty``, and other ESP-IDF LEDC APIs for dynamic duty cycle control.

Related declarations are in ``esp_board_manager/peripherals/periph_ledc/periph_ledc.h``.

Underlying ESP-IDF driver documentation: `LED PWM Controller <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/ledc.html>`__.
