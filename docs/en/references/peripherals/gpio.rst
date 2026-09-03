gpio
============

:link_to_translation:`zh_CN:[中文]`

.. _gpio-intro:

Overview
--------

The ``gpio`` peripheral describes the direction, pull resistors, interrupt type, and default output level of a single ESP-IDF GPIO pin. BMGR converts a ``gpio`` entry in ``board_peripherals.yaml`` into a ``periph_gpio_config_t``, calls ``gpio_config`` during initialization, and sets the default level in output mode.

``gpio`` is commonly used for board-level signals such as amplifier enable, LCD and SD card power control, backlight control, mute control, and button input. GPIOs that need to be managed by a device should be written as peripheral entries and then referenced by devices such as ``gpio_ctrl``, ``button``, or ``audio_codec``.

.. _gpio-working-modes:

Supported Operating Modes
--------------------------

``gpio`` distinguishes configuration by direction and output type. The current templates cover the input, output, input-output, and open-drain modes supported by the ESP-IDF ``gpio_config_t``.

- :ref:`GPIO Input <gpio-input>`
- :ref:`GPIO Output <gpio-output>`
- :ref:`GPIO Input-Output / Open-Drain <gpio-io-od>`

.. _gpio-min-config:

Minimal Configuration
---------------------

.. _gpio-input:

GPIO Input
^^^^^^^^^^

See complete fields: :ref:`gpio-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: gpio_boot_button
        type: gpio
        config:
          pin: 0       # [IO]
          mode: GPIO_MODE_INPUT
          pull_up: true

.. _gpio-output:

GPIO Output
^^^^^^^^^^^

See complete fields: :ref:`gpio-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: gpio_pa_control
        type: gpio
        config:
          pin: 48      # [IO]
          mode: GPIO_MODE_OUTPUT
          default_level: 0

.. _gpio-io-od:

GPIO Input-Output / Open-Drain
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See complete fields: :ref:`gpio-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: gpio_control
        type: gpio
        config:
          pin: 10      # [IO]
          mode: GPIO_MODE_INPUT_OUTPUT_OD
          pull_up: true
          default_level: 1

.. _gpio-mode-notes:

Mode Description
----------------

Input mode is used for buttons, detection pins, or interrupt inputs. Output mode is used for power, amplifier, backlight, mute, and similar control pins. Input-output and open-drain modes are used for GPIO signals that require bidirectional or open-drain characteristics.

``default_level`` is set by the initialization code only in output-type modes. Device-side ``active_level``, ``gain``, button events, and application semantics are not written into the ``gpio`` peripheral ``config``.

.. _gpio-full-fields:

Full Field Reference
--------------------

.. _gpio-full:

GPIO Input / Output / Input-Output — Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # GPIO Peripheral Default Configuration
    # This file shows the default values used by the GPIO peripheral parser
    # Based on periph_gpio.py parsing script

    - name: gpio
      type: gpio
      config:
        # GPIO pin number (required, no default - must be >= 0)
        pin: 0                   # [IO] GPIO pin number

        # GPIO mode (default: GPIO_MODE_INPUT)
        mode: "GPIO_MODE_INPUT"  # [TO_BE_CONFIRMED] GPIO mode
        # Valid modes:
        # - GPIO_MODE_INPUT
        # - GPIO_MODE_OUTPUT
        # - GPIO_MODE_INPUT_OUTPUT
        # - GPIO_MODE_OUTPUT_OD
        # - GPIO_MODE_INPUT_OUTPUT_OD

        # Pull-up resistor (default: false)
        pull_up: false

        # Pull-down resistor (default: false)
        pull_down: false

        # Interrupt type (default: GPIO_INTR_DISABLE)
        intr_type: "GPIO_INTR_DISABLE"
        # Valid interrupt types:
        # - GPIO_INTR_DISABLE
        # - GPIO_INTR_POSEDGE
        # - GPIO_INTR_NEGEDGE
        # - GPIO_INTR_ANYEDGE
        # - GPIO_INTR_LOW_LEVEL
        # - GPIO_INTR_HIGH_LEVEL

        # Default output level (default: 0)
        default_level: 0
        # Valid values: 0 or 1

.. _gpio-devices:

Applicable Devices
------------------

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Notes
   * - ``gpio_ctrl``
     - Device ``peripherals`` references one ``gpio`` peripheral
     - ``active_level`` and ``default_level`` device semantics are written in the ``gpio_ctrl`` configuration
   * - ``button``
     - Devices with ``sub_type: gpio`` reference the ``gpio`` peripheral
     - Trigger level and event configuration are written in the ``button`` device
   * - ``audio_codec``
     - When an amplifier control pin exists, reference the ``gpio`` peripheral
     - The device-side reference entry includes ``gain`` and ``active_level``
   * - ``display_lcd`` / ``lcd_touch``
     - Reset, interrupt, backlight, or power GPIOs can be expressed via device-side fields or a standalone GPIO control device
     - The LCD panel's ``reset_gpio_num`` and the touch controller's ``rst_gpio_num`` / ``int_gpio_num`` are device-side fields

.. _gpio-code:

Reference Code
--------------

- ``esp_board_manager/peripherals/periph_gpio/periph_gpio.c``
- ``esp_board_manager/peripherals/periph_gpio/periph_gpio.h``
- ``esp_board_manager/examples/play_embed_music/main/play_embed_music.c``
- ``esp_board_manager/examples/play_sdcard_music/main/play_sdcard_music.c``

.. _gpio-boards:

Board-Level Reference
---------------------

- ``esp_boards/esp32_s3_box_3/board_peripherals.yaml``: amplifier, backlight, SD card power, and mute GPIOs.
- ``esp_boards/esp32_lyrat_mini_1_1/board_peripherals.yaml``: SD card power, headphone detection, amplifier, LED, and SD card detect GPIOs.
- ``esp_friends_boards/esp32_c5_spot/board_peripherals.yaml``: amplifier, power control, codec power, and IMU interrupt GPIOs.
- ``esp_boards/esp32_s3_lcd_ev_board/board_peripherals.yaml``: boot button GPIO.

.. _gpio-notes:

Notes
-----

- For common YAML field rules, see :doc:`/programming-guide/yaml-rules`.
- ``pin`` must be a non-negative GPIO number. Whether it can serve as input, output, open-drain, or interrupt source is determined by the target chip and board hardware.
- Output-type GPIOs have ``default_level`` set during initialization; input-type GPIOs do not use that field to change the level.
- The same physical GPIO should not be configured in multiple peripheral entries with conflicting directions or default levels.
- After modifying the GPIO peripheral configuration, re-run ``idf.py bmgr -b <board>``.

.. _gpio-debug:

Debugging Tips
--------------

.. _gpio-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to retrieve the GPIO peripheral handle. The handle type is ``periph_gpio_handle_t``:

.. code-block:: c

   typedef struct {
       gpio_num_t  gpio_num;  /*!< GPIO number */
   } periph_gpio_handle_t;

``gpio_num`` is the GPIO number configured during initialization and can be passed directly to ESP-IDF GPIO APIs such as ``gpio_set_level`` and ``gpio_get_level``.

Related declarations are in ``esp_board_manager/peripherals/periph_gpio/periph_gpio.h``.

Underlying ESP-IDF driver documentation: `GPIO & RTC GPIO <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/gpio.html>`__.
