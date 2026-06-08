i2c
============

:link_to_translation:`zh_CN:[中文]`

Overview
--------

The ``i2c`` peripheral describes an ESP-IDF I2C master bus. BMGR converts an ``i2c`` entry in ``board_peripherals.yaml`` into an ``i2c_master_bus_config_t`` and creates an I2C master bus handle during initialization.

``i2c`` is commonly used for external audio codec control, LCD touch controllers, IO expanders, and camera SCCB control. Device-side entries reference the same I2C bus by peripheral instance name, and device-private parameters such as device addresses are filled in the device ``peripherals`` reference entry.

Supported Operating Modes
--------------------------

``i2c`` distinguishes configuration by controller port family. The current peripheral initialization entry only creates a master bus and does not provide slave device mode.

- :ref:`HP I2C master <i2c-hp-master>`
- :ref:`LP I2C master <i2c-lp-master>`

Minimal Configuration
---------------------

.. _i2c-hp-master:

HP I2C Master
^^^^^^^^^^^^^

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: i2c_master
        type: i2c
        config:
          port: 0
          pins:
            sda: 17     # [IO]
            scl: 18     # [IO]

.. _i2c-lp-master:

LP I2C Master
^^^^^^^^^^^^^

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: lp_i2c_master
        type: i2c
        config:
          port: LP_I2C_NUM_0
          clk_source: LP_I2C_SCLK_DEFAULT
          pins:
            sda: 6      # [IO]
            scl: 7      # [IO]

Mode Description
----------------

HP I2C uses port macros such as ``I2C_NUM_0`` or ``I2C_NUM_1``, or their corresponding numeric indexes. ``port: -1`` means ESP-IDF automatically selects the HP I2C port and must not be combined with an LP I2C clock source.

LP I2C requires explicit use of port macros such as ``LP_I2C_NUM_0`` and ``LP_I2C_SCLK_*`` clock sources. LP I2C GPIO selection is constrained by the LP GPIO capabilities of the target chip.

I2C device addresses are not written into the peripheral ``config``: audio codecs use ``address``, and LCD touch controllers and IO expanders use ``i2c_addr``. These fields belong to device-side reference entries.

Full Field Reference
--------------------

HP I2C Master — Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # I2C Peripheral Default Configuration
    # This file shows the default values used by the I2C peripheral parser
    # Based on periph_i2c.py parsing script

    - name: i2c_master
      type: i2c
      config:
        # I2C port number (default: -1 for auto selection)
        port: -1
        # Valid values:
        # - -1 (auto select, HP I2C only)
        # - numeric index: 0 / 1 (will map to I2C_NUM_0 / I2C_NUM_1)
        # - macro: I2C_NUM_0 / I2C_NUM_1 / LP_I2C_NUM_0

        # I2C clock source macro (optional)
        # If omitted:
        # - HP I2C uses I2C_CLK_SRC_DEFAULT
        # - LP I2C uses LP_I2C_SCLK_DEFAULT
        # Example values:
        #   I2C_CLK_SRC_DEFAULT, I2C_CLK_SRC_APB, I2C_CLK_SRC_XTAL,
        #   I2C_CLK_SRC_RC_FAST, I2C_CLK_SRC_REF_TICK,
        #   LP_I2C_SCLK_DEFAULT, LP_I2C_SCLK_LP_FAST, LP_I2C_SCLK_XTAL_D2
        clk_source: I2C_CLK_SRC_DEFAULT

        # I2C pins configuration
        pins:
          # SDA pin (default: -1, not set)
          sda: -1                     # [IO] SDA pin
          # SCL pin (default: -1, not set)
          scl: -1                     # [IO] SCL pin

        # Enable internal pullup resistors (default: true)
        enable_internal_pullup: true

        # Glitch filter count (default: 7)
        glitch_count: 7

        # Interrupt priority (default: 1)
        intr_priority: 1

Applicable Devices
------------------

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Notes
   * - ``audio_codec``
     - Device ``peripherals`` references ``i2c`` and provides ``address`` and ``frequency``
     - External codec control interface; I2C address is written on the device side
   * - ``lcd_touch``
     - Device ``peripherals`` references ``i2c`` and provides ``i2c_addr``
     - Touch controller communication; touch coordinates and reset/interrupt GPIOs belong to device-side configuration
   * - ``gpio_expander``
     - Device ``peripherals`` references ``i2c`` and provides ``i2c_addr``
     - IO expander chip control; expanded IO direction and default level belong to device-side configuration
   * - ``camera``
     - DVP / SPI cameras can reference ``i2c`` as the SCCB control bus
     - Camera data bus or SPI data lines are not written into the ``i2c`` peripheral

Reference Code
--------------

- ``esp_board_manager/peripherals/periph_i2c/periph_i2c.c``
- ``esp_board_manager/peripherals/periph_i2c/periph_i2c.h``
- ``esp_board_manager/examples/play_embed_music/main/play_embed_music.c``
- ``esp_board_manager/examples/record_to_sdcard/main/record_to_sdcard.c``

Board-Level Reference
---------------------

- ``esp_friends_boards/esp32_s3_korvo_2l/board_peripherals.yaml``: I2C control bus used by the audio codec.
- ``esp_boards/esp32_s3_lcd_ev_board/board_peripherals.yaml``: shared I2C bus for audio, touch, and IO expander.
- ``m5stack_boards/m5stack_tab5/board_peripherals.yaml``: I2C bus used by audio, touch, and multiple IO expanders.

Notes
-----

- For common YAML field rules, see :doc:`/programming-guide/yaml-rules`.
- ``clk_source`` must match the port family. LP I2C ports cannot use ``I2C_CLK_SRC_*``; HP I2C ports cannot use ``LP_I2C_SCLK_*``.
- Device address, I2C transfer frequency, touch control parameters, and codec control parameters belong to device-side configuration and must not be written into the ``i2c`` peripheral ``config``.
- After modifying the I2C peripheral configuration, re-run ``idf.py bmgr -b <board>``.

Debugging Tips
--------------

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to retrieve the I2C peripheral handle. The handle type is the ESP-IDF native ``i2c_master_bus_handle_t``, which can be passed directly to ``i2c_master_*`` APIs or used to attach I2C devices.

Related declarations are in ``esp_board_manager/peripherals/periph_i2c/periph_i2c.h``.

Underlying ESP-IDF driver documentation: `I2C Driver <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/i2c.html>`__.
