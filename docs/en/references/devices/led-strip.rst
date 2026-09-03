LED Strip (``led_strip``)
=========================

:link_to_translation:`zh_CN:[中文]`

.. _led-strip-intro:

Overview
--------

``led_strip`` is an addressable LED strip device based on the ESP-IDF ``led_strip`` component, used to initialize single-wire protocol LEDs such as WS2812, SK6812, WS2811, and WS2816. This device writes the common strip parameters and specific backend configuration into ``board_devices.yaml``. After obtaining the ``led_strip_handle_t`` via BMGR, the application calls the ``led_strip`` component API to set pixel colors and refresh the output.

``led_strip`` selects the backend with ``sub_type``. The current source code implements two sub-types: ``rmt`` and ``spi``. Both modes have the underlying driver instance created by the ``led_strip`` component during device initialization; no new RMT or SPI peripheral entry is needed in ``board_peripherals.yaml``.

.. _led-strip-usage-modes:

Supported Usage Modes
---------------------

``led_strip`` distinguishes output backends with ``sub_type``:

- `RMT LED Strip`_
- `SPI LED Strip`_

.. _led-strip-min-config:

Minimal Configuration
---------------------

.. _led-strip-rmt:

RMT LED Strip
^^^^^^^^^^^^^

See complete fields: :ref:`led-strip-rmt-full`.

``rmt`` mode calls ``led_strip_new_rmt_device`` to create the strip instance. It is suitable for RMT-backend LED strip output. After successful initialization, ``led_strip_clear`` is called to clear the strip output. No new ``board_peripherals.yaml`` entry is required.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: led_strip
        type: led_strip
        chip: ws2812             # [TO_BE_CONFIRMED]
        sub_type: rmt
        version: 1.0.0
        config:
          strip_gpio_num: 37     # [IO]
          max_leds: 1            # [TO_BE_CONFIRMED]
          led_model: LED_MODEL_WS2812
          color_component_format: LED_STRIP_COLOR_COMPONENT_FMT_GRB
          invert_out: false
          rmt:
            clk_src: RMT_CLK_SRC_DEFAULT
            resolution_hz: 10000000
            mem_block_symbols: 0
            with_dma: false

.. _led-strip-spi:

SPI LED Strip
^^^^^^^^^^^^^

See complete fields: :ref:`led-strip-spi-full`.

``spi`` mode calls ``led_strip_new_spi_device`` to create the strip instance. It is suitable for SPI-backend LED strip output. After successful initialization, ``led_strip_clear`` is called to clear the strip output. No new ``board_peripherals.yaml`` entry is required; the ``led_strip`` component creates the SPI backend based on the device configuration. The selected ``spi_bus`` must not conflict with any already-initialized SPI bus.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: led_strip
        type: led_strip
        chip: ws2812             # [TO_BE_CONFIRMED]
        sub_type: spi
        version: 1.0.0
        config:
          strip_gpio_num: 48     # [IO]
          max_leds: 1            # [TO_BE_CONFIRMED]
          led_model: LED_MODEL_WS2812
          color_component_format: LED_STRIP_COLOR_COMPONENT_FMT_GRB
          invert_out: false
          spi:
            clk_src: SPI_CLK_SRC_DEFAULT
            spi_bus: SPI3_HOST
            with_dma: true

.. _led-strip-full-fields:

All Fields
----------

.. _led-strip-rmt-full:

RMT LED Strip All Fields
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example board_devices.yaml configuration for LED strip device

    - name: led_strip
      type: led_strip                      # The type of the device, must be led_strip
      chip: ws2812                         # [TO_BE_CONFIRMED] LED strip chip/model name
      sub_type: rmt                        # The sub type: rmt or spi
      version: 1.0.0
      config:
        strip_gpio_num: 37                 # [IO] GPIO used to output LED strip data signal
        max_leds: 1                        # [TO_BE_CONFIRMED] Number of LEDs in the strip
        led_model: LED_MODEL_WS2812        # [TO_BE_CONFIRMED] LED model timing profile
        # Valid values include:
        # - LED_MODEL_WS2812
        # - LED_MODEL_SK6812
        # - LED_MODEL_WS2811
        # - LED_MODEL_WS2816
        color_component_format: LED_STRIP_COLOR_COMPONENT_FMT_GRB  # [TO_BE_CONFIRMED] Per-pixel color component order
        # Common values include:
        # - LED_STRIP_COLOR_COMPONENT_FMT_GRB
        # - LED_STRIP_COLOR_COMPONENT_FMT_RGB
        # - LED_STRIP_COLOR_COMPONENT_FMT_GRBW
        # - LED_STRIP_COLOR_COMPONENT_FMT_RGBW
        invert_out: false                  # Invert output signal level (default: false)
        # led_strip_rmt_config_t fields
        rmt:
          clk_src: RMT_CLK_SRC_DEFAULT     # RMT clock source (rmt_clock_source_t)
          resolution_hz: 10000000          # RMT tick resolution in Hz; 0 lets driver use default 10MHz
          mem_block_symbols: 0             # RMT symbols per channel block; 0 lets driver choose default size
          with_dma: false                  # Use DMA for RMT transmission when target supports it

.. _led-strip-spi-full:

SPI LED Strip All Fields
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: ambient_led
      type: led_strip                      # The type of the device, must be led_strip
      chip: ws2812                         # [TO_BE_CONFIRMED] LED strip chip/model name
      sub_type: spi                        # The sub type: rmt or spi
      version: 1.0.0
      config:
        strip_gpio_num: 35                 # [IO] GPIO used as SPI MOSI for LED strip data signal
        max_leds: 8                        # [TO_BE_CONFIRMED] Number of LEDs in the strip
        led_model: LED_MODEL_WS2812        # [TO_BE_CONFIRMED] LED model timing profile
        # Valid values include:
        # - LED_MODEL_WS2812
        # - LED_MODEL_SK6812
        # - LED_MODEL_WS2811
        # - LED_MODEL_WS2816
        color_component_format: LED_STRIP_COLOR_COMPONENT_FMT_GRB  # [TO_BE_CONFIRMED] Per-pixel color component order
        # Common values include:
        # - LED_STRIP_COLOR_COMPONENT_FMT_GRB
        # - LED_STRIP_COLOR_COMPONENT_FMT_RGB
        # - LED_STRIP_COLOR_COMPONENT_FMT_GRBW
        # - LED_STRIP_COLOR_COMPONENT_FMT_RGBW
        invert_out: false                  # Invert output signal level (default: false)
        # led_strip_spi_config_t fields; the led_strip driver initializes this SPI bus internally.
        spi:
          clk_src: SPI_CLK_SRC_DEFAULT     # SPI clock source (spi_clock_source_t)
          spi_bus: SPI2_HOST               # SPI host used by led_strip driver; must not conflict with another initialized SPI bus
          with_dma: true                   # Use DMA for SPI transmission when target supports it

.. _led-strip-deps:

Component Dependencies
----------------------

``led_strip`` introduces ``espressif/led_strip`` (version ``"*"``) via ``esp_board_manager/idf_component.yml`` when ``CONFIG_ESP_BOARD_DEV_LED_STRIP_SUPPORT`` is enabled. Board YAML does not need to re-declare ``dependencies`` for this common component.

.. _led-strip-peripherals:

Required Peripherals
--------------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - Required
     - Purpose
   * - None
     - None
     - No BMGR peripheral required
     - The ``led_strip`` component creates the RMT or SPI backend during device initialization

.. _led-strip-code:

Reference Code
--------------

- ``esp_board_manager/test_apps/main/test_dev_led_strip.c``
- ``esp_board_manager/devices/dev_led_strip/dev_led_strip.c``
- ``esp_board_manager/devices/dev_led_strip/dev_led_strip_sub_rmt.c``
- ``esp_board_manager/devices/dev_led_strip/dev_led_strip_sub_spi.c``

.. _led-strip-boards:

Board Reference
---------------

- ``esp_boards/esp32_s31_korvo_1/board_devices.yaml``: RMT mode on-board WS2812 status LED configuration.
- ``esp_boards/esp32_s31_function_coreboard_1/board_devices.yaml``: RMT mode on-board WS2812 status LED configuration.
- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_s3_devkitc/board_devices.yaml``: SPI mode ``led_strip`` test configuration.

.. _led-strip-notes:

Notes
-----

- ``strip_gpio_num`` must match the schematic; ``max_leds`` must be greater than 0.
- In SPI mode, the ``spi_bus`` is initialized internally by the ``led_strip`` driver and must not conflict with any other initialized SPI bus.
- When signal inversion or level shifting circuits are present, confirm ``invert_out`` according to the hardware connections.
- After modifying YAML, re-run ``idf.py bmgr -b <board>``.

.. _led-strip-debug:

Debugging Tips
--------------

.. _led-strip-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain the device handle. The handle type is ``dev_led_strip_handles_t``:

.. code-block:: c

   typedef struct {
       led_strip_handle_t  strip_handle;  /*!< Native led_strip handle */
   } dev_led_strip_handles_t;

``strip_handle`` can be passed directly to the ``espressif/led_strip`` component control APIs (set color, refresh, clear, etc.).

The related declarations are located in ``esp_board_manager/devices/dev_led_strip/dev_led_strip.h``.
