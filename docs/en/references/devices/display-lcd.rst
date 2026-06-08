LCD Display (display_lcd)
=========================

:link_to_translation:`zh_CN:[中文]`

Overview
--------

The ``display_lcd`` device describes an LCD display and creates an ``esp_lcd_panel_handle_t`` along with an optional panel IO handle during initialization. Applications obtain a ``dev_display_lcd_handles_t`` via :cpp:func:`esp_board_manager_get_device_handle`, then access the screen using ESP-IDF ``esp_lcd_panel_ops`` or a higher-level graphics library.

This type selects the display interface via ``sub_type``. The current device template covers ``dsi``, ``spi``, ``i80``, ``rgb``, ``rgb_3wire_spi``, and ``parlio``. When chip-specific LCD initialization commands are needed, the board registers the corresponding LCD factory function in ``setup_device.c``.

Supported Usage Modes
---------------------

``display_lcd`` is categorized by ``sub_type``:

- :ref:`display-lcd-dsi`
- :ref:`display-lcd-spi`
- :ref:`display-lcd-i80`
- :ref:`display-lcd-rgb`
- :ref:`display-lcd-rgb-3wire-spi`
- :ref:`display-lcd-parlio`

Minimal Configuration
---------------------

.. _display-lcd-dsi:

DSI (``sub_type: dsi``)
^^^^^^^^^^^^^^^^^^^^^^^

The ``dsi`` mode is for MIPI DSI displays. The device depends on the ``dsi`` peripheral and optionally references the ``ldo`` peripheral for MIPI power management.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: ldo_mipi
        type: ldo
        config:
          chan_id: 3                      # [TO_BE_CONFIRMED] LDO channel ID
          voltage_mv: 2500                # [TO_BE_CONFIRMED] Output voltage in millivolts
          adjustable: 1
          owned_by_hw: 0

      - name: dsi_display
        type: dsi
        config:
          bus_id: 0
          data_lanes: 2
          phy_clk_src: 0
          lane_bit_rate_mbps: 1000        # [TO_BE_CONFIRMED] Bit rate per data lane in Mbps

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: display_lcd
        type: display_lcd
        sub_type: dsi
        chip: ek79007                     # [TO_BE_CONFIRMED] LCD chip type
        dependencies:
          espressif/esp_lcd_ek79007: "^1.0.0"
        config:
          reset_gpio_num: 27              # [IO] GPIO pin for reset signal
          bits_per_pixel: 24              # [TO_BE_CONFIRMED]
          dpi_config:
            dpi_clock_freq_mhz: 48        # [TO_BE_CONFIRMED]
            pixel_format: LCD_COLOR_PIXEL_FORMAT_RGB888
            in_color_format: LCD_COLOR_FMT_RGB888
            out_color_format: LCD_COLOR_FMT_RGB888
            video_timing:
              h_size: 1024                # [TO_BE_CONFIRMED]
              v_size: 600                 # [TO_BE_CONFIRMED]
              hsync_back_porch: 120       # [TO_BE_CONFIRMED]
              hsync_pulse_width: 10       # [TO_BE_CONFIRMED]
              hsync_front_porch: 120      # [TO_BE_CONFIRMED]
              vsync_back_porch: 20        # [TO_BE_CONFIRMED]
              vsync_pulse_width: 1        # [TO_BE_CONFIRMED]
              vsync_front_porch: 10       # [TO_BE_CONFIRMED]
        peripherals:
          - name: ldo_mipi
          - name: dsi_display

.. _display-lcd-spi:

SPI (``sub_type: spi``)
^^^^^^^^^^^^^^^^^^^^^^^

The ``spi`` mode is for displays that send commands and pixel data via SPI panel IO. The device depends on the ``spi`` peripheral.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: spi_master
        type: spi
        config:
          spi_bus_config:
            spi_port: SPI3_HOST           # [TO_BE_CONFIRMED] SPI port
            mosi_io_num: 37               # [IO] MOSI pin
            miso_io_num: 35               # [IO] MISO pin
            sclk_io_num: 36               # [IO] SCLK pin
            quadwp_io_num: -1             # [IO]
            quadhd_io_num: -1             # [IO]
            max_transfer_sz: 32000

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: display_lcd
        type: display_lcd
        sub_type: spi
        chip: ili9341                     # [TO_BE_CONFIRMED] LCD chip type
        dependencies:
          espressif/esp_lcd_ili9341: "*"
        config:
          x_max: 320                      # [TO_BE_CONFIRMED]
          y_max: 240                      # [TO_BE_CONFIRMED]
          io_spi_config:
            cs_gpio_num: 3                # [IO]
            dc_gpio_num: 35               # [IO]
            pclk_hz: 40000000             # [TO_BE_CONFIRMED]
          lcd_panel_config:
            reset_gpio_num: -1            # [IO]
            bits_per_pixel: 16            # [TO_BE_CONFIRMED]
        peripherals:
          - name: spi_master

.. _display-lcd-i80:

I80 (``sub_type: i80``)
^^^^^^^^^^^^^^^^^^^^^^^

The ``i80`` mode uses ``esp_lcd_new_i80_bus`` and ``esp_lcd_new_panel_io_i80`` to create the panel IO, and does not depend on the ``spi`` peripheral. The I80 bus is created internally by the device based on ``bus_config``; no new ``board_peripherals.yaml`` entry is required.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: display_lcd
        type: display_lcd
        sub_type: i80
        chip: ili9341                     # [TO_BE_CONFIRMED] LCD chip type
        dependencies:
          espressif/esp_lcd_ili9341: "*"
        config:
          x_max: 240                      # [TO_BE_CONFIRMED]
          y_max: 280                      # [TO_BE_CONFIRMED]
          bus_config:
            dc_gpio_num: 7                # [IO]
            wr_gpio_num: 8                # [IO]
            data_gpio_nums: [9, 10, 11, 12, 13, 14, 15, 16]  # [IO]
            bus_width: 8                  # [TO_BE_CONFIRMED] 8 or 16
            max_transfer_bytes: 4092
          io_config:
            cs_gpio_num: 6                # [IO]
            pclk_hz: 10000000             # [TO_BE_CONFIRMED]
          panel_config:
            reset_gpio_num: -1            # [IO]
            bits_per_pixel: 16            # [TO_BE_CONFIRMED]

.. _display-lcd-rgb:

RGB (``sub_type: rgb``)
^^^^^^^^^^^^^^^^^^^^^^^

The ``rgb`` mode uses the RGB LCD peripheral to output pixel data directly without invoking the generic ``lcd_panel_factory_entry_t``. RGB bus GPIOs are configured in the device's ``rgb_panel_config``; no new ``board_peripherals.yaml`` entry is required.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: display_lcd
        type: display_lcd
        sub_type: rgb
        chip: generic_rgb
        config:
          bits_per_pixel: 16
          rgb_panel_config:
            data_width: 16                # [TO_BE_CONFIRMED]
            hsync_gpio_num: 44            # [IO]
            vsync_gpio_num: 45            # [IO]
            de_gpio_num: 43               # [IO]
            pclk_gpio_num: 40             # [IO]
            data_gpio_nums: [8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 33, 34, 35, 36]  # [IO]
            timings:
              pclk_hz: 18000000           # [TO_BE_CONFIRMED]
              h_res: 800                  # [TO_BE_CONFIRMED]
              v_res: 480                  # [TO_BE_CONFIRMED]

.. _display-lcd-rgb-3wire-spi:

RGB + 3-wire SPI (``sub_type: rgb_3wire_spi``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``rgb_3wire_spi`` mode adds a 3-wire SPI initialization IO alongside the RGB pixel bus, and does not require the ``spi`` peripheral. The 3-wire SPI initialization lines can use SoC GPIOs directly, or be provided by a ``gpio_expander`` device.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: display_lcd
        type: display_lcd
        sub_type: rgb_3wire_spi
        chip: generic_rgb_3wire_spi       # [TO_BE_CONFIRMED] LCD chip type
        dependencies:
          espressif/esp_lcd_panel_io_additions: "*"
          espressif/esp_lcd_generic: "*"  # [TO_BE_CONFIRMED]
        config:
          io_3wire_spi_config:
            io_expander_name: ""
            line_config:
              cs_io_type: IO_TYPE_GPIO
              cs_gpio_num: -1             # [IO]
              scl_io_type: IO_TYPE_GPIO
              scl_gpio_num: -1            # [IO]
              sda_io_type: IO_TYPE_GPIO
              sda_gpio_num: -1            # [IO]
          rgb_panel_config:
            hsync_gpio_num: -1            # [IO]
            vsync_gpio_num: -1            # [IO]
            de_gpio_num: -1               # [IO]
            pclk_gpio_num: -1             # [IO]
            data_gpio_nums: [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1]  # [IO]
            timings:
              pclk_hz: 16000000           # [TO_BE_CONFIRMED]
              h_res: 480                  # [TO_BE_CONFIRMED]
              v_res: 480                  # [TO_BE_CONFIRMED]
          lcd_panel_config:
            reset_gpio_num: -1            # [IO]
            bits_per_pixel: 18            # [TO_BE_CONFIRMED]

RGB User Frame Buffers
^^^^^^^^^^^^^^^^^^^^^^

The ``rgb`` and ``rgb_3wire_spi`` modes support the ``user_fbs_func`` field on ESP-IDF v6.0 and later. This field is not a function pointer. It is the name of a board-level callback registered with ``DEVICE_EXTRA_FUNC_REGISTER`` and is used to provide frame buffers managed by the application or board-level code to the RGB LCD driver.

In ``board_devices.yaml``, place ``user_fbs_func`` under ``rgb_panel_config``:

.. code-block:: yaml

    devices:
      - name: display_lcd
        type: display_lcd
        sub_type: rgb
        config:
          rgb_panel_config:
            num_fbs: 2
            user_fbs_func: lcd_rgb_user_fbs

Board-level code must register a callback with the same name:

.. code-block:: c

   #include "dev_display_lcd.h"
   #include "esp_board_extra_func_entry.h"

   static int lcd_rgb_user_fbs(const dev_display_lcd_config_t *lcd_cfg,
                               dev_display_lcd_rgb_user_fbs_action_t action,
                               void *user_fbs[ESP_RGB_LCD_PANEL_MAX_FB_NUM])
   {
       switch (action) {
       case DEV_DISPLAY_LCD_RGB_USER_FBS_GET:
           /* Allocate or assign board-provided frame buffers, then fill user_fbs[]. */
           return 0;
       case DEV_DISPLAY_LCD_RGB_USER_FBS_RELEASE:
           /* Release resources acquired during GET if needed. */
           return 0;
       default:
           return -1;
       }
   }

   DEVICE_EXTRA_FUNC_REGISTER(lcd_rgb_user_fbs, lcd_rgb_user_fbs);

Before creating the RGB panel, BMGR calls this callback with ``DEV_DISPLAY_LCD_RGB_USER_FBS_GET`` and passes the returned pointers to ``esp_lcd_rgb_panel_config_t.user_fbs``. If initialization fails or the device is deinitialized, BMGR calls the callback again with ``DEV_DISPLAY_LCD_RGB_USER_FBS_RELEASE``. The ``GET`` branch must fill valid frame buffer pointers matching ``num_fbs``. If the callback returns a non-zero value, LCD device initialization fails.

When this field is used, the number, size, alignment, and memory capabilities of the frame buffers must match the resolution, color format, ESP-IDF RGB LCD driver requirements, and the ``num_fbs`` configuration. If user-owned frame buffers are not required, leave ``user_fbs_func`` empty and let the driver manage frame buffers according to ``num_fbs``, ``fb_in_psram``, and related settings.


.. _display-lcd-parlio:

PARLIO (``sub_type: parlio``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``parlio`` mode uses ``esp_lcd_new_panel_io_parl`` to create the panel IO, and does not require the ``spi`` peripheral. PARLIO data, clock, and control lines are all configured in the device's ``io_parl_config``; no new ``board_peripherals.yaml`` entry is required.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: display_lcd
        type: display_lcd
        sub_type: parlio
        chip: ili9341                     # [TO_BE_CONFIRMED]
        dependencies:
          espressif/esp_lcd_ili9341: "*"
        config:
          x_max: 284                      # [TO_BE_CONFIRMED]
          y_max: 240                      # [TO_BE_CONFIRMED]
          io_parl_config:
            dc_gpio_num: 4                # [IO]
            clk_gpio_num: 5               # [IO]
            cs_gpio_num: 6                # [IO]
            data_gpio_nums: [7, -1, -1, -1, -1, -1, -1, -1]  # [IO]
            data_width: 1                 # [TO_BE_CONFIRMED]
            pclk_hz: 40000000             # [TO_BE_CONFIRMED]
            max_transfer_bytes: 11360     # [TO_BE_CONFIRMED]
          lcd_panel_config:
            reset_gpio_num: -1            # [IO]
            bits_per_pixel: 16            # [TO_BE_CONFIRMED]

Full Field Reference
--------------------

DSI Full Fields
^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example board_devices.yaml configuration for LCD Display device
    # This shows how to integrate the LCD display device with DSI and SPI sub device into a board configuration

    # Example LCD Display device with DSI sub device configuration
    - name: display_lcd          # The name of the device, must be unique
      type: display_lcd          # The type of the device, must be unique
      version: 1.0.0
      sub_type: dsi              # The sub type: dsi, spi, or parlio
      chip: generic_lcd          # [TO_BE_CONFIRMED] LCD chip type (generic)
      dependencies:
        espressif/esp_lcd_generic: "*"  # [TO_BE_CONFIRMED] Component dependency for the LCD chip
      config:
        invert_color: false  # Invert color flag (default: false)
        need_reset: true                          # Whether to reset the LCD panel during initialization (default: true)
        reset_gpio_num: 27                        # [IO] GPIO pin for reset signal (use -1 if RST not from SoC)
        rgb_ele_order: LCD_RGB_ELEMENT_ORDER_RGB  # [TO_BE_CONFIRMED] RGB element order (default: RGB)
        # Valid values:
        # - LCD_RGB_ELEMENT_ORDER_RGB
        # - LCD_RGB_ELEMENT_ORDER_BGR
        data_endian: LCD_RGB_DATA_ENDIAN_BIG      # [TO_BE_CONFIRMED] Data endianness (default: BIG)
        # Valid values:
        # - LCD_RGB_DATA_ENDIAN_BIG
        # - LCD_RGB_DATA_ENDIAN_LITTLE
        bits_per_pixel: 24                        # [TO_BE_CONFIRMED] Bits per pixel (24bpp, RGB888)
        reset_active_high: 0                      # Reset pin active level (0 = active low)

        # DBI interface configuration (command/parameter transfer)
        dbi_config:
          virtual_channel: 0  # Virtual channel number (default: 0)
          lcd_cmd_bits: 8     # Bit width of LCD command (default: 8)
          lcd_param_bits: 8   # Bit width of LCD parameter (default: 8)

        # DPI interface configuration (pixel data transfer)
        dpi_config:
          virtual_channel: 0                           # Virtual channel number (default: 0)
          dpi_clk_src: MIPI_DSI_DPI_CLK_SRC_DEFAULT    # DPI clock source (default)
          # Refer to `soc_periph_mipi_dsi_dpi_clk_src_t` for valid values
          dpi_clock_freq_mhz: 48                       # [TO_BE_CONFIRMED] DPI pixel clock MHz from panel budget
          pixel_format: LCD_COLOR_PIXEL_FORMAT_RGB565  # [TO_BE_CONFIRMED] Pixel format that used by the MIPI LCD device (default: RGB565)
          # Valid values
          # - LCD_COLOR_PIXEL_FORMAT_RGB565
          # - LCD_COLOR_PIXEL_FORMAT_RGB666
          # - LCD_COLOR_PIXEL_FORMAT_RGB888
          in_color_format: LCD_COLOR_FMT_RGB565        # [TO_BE_CONFIRMED] Input color format (default: RGB565)
          # Valid values
          # - LCD_COLOR_FMT_RGB565
          # - LCD_COLOR_FMT_RGB666
          # - LCD_COLOR_FMT_RGB888
          # - LCD_COLOR_FMT_YUV422
          # - LCD_COLOR_FMT_GRAY8
          out_color_format: LCD_COLOR_FMT_RGB565       # [TO_BE_CONFIRMED] Output color format (default: RGB565)
          num_fbs: 1                                   # Number of frame buffers (default: 1)
          flags:
            use_dma2d: false                         # Use DMA2D for pixel data transfer (default: false)
            disable_lp: false                        # Disable low power mode (default: false)

          # Video timing parameters (resolution + sync signals)
          video_timing:
            h_size: 1024            # [TO_BE_CONFIRMED] Horizontal resolution
            v_size: 600             # [TO_BE_CONFIRMED] Vertical resolution
            hsync_back_porch: 120   # [TO_BE_CONFIRMED] Horizontal back porch
            hsync_pulse_width: 10   # [TO_BE_CONFIRMED] Horizontal sync pulse width
            hsync_front_porch: 120  # [TO_BE_CONFIRMED] Horizontal front porch
            vsync_back_porch: 20    # [TO_BE_CONFIRMED] Vertical back porch
            vsync_pulse_width: 1    # [TO_BE_CONFIRMED] Vertical sync pulse width
            vsync_front_porch: 20   # [TO_BE_CONFIRMED] Vertical front porch

      peripherals:
        - name: ldo_mipi          # [TO_BE_CONFIRMED] LDO peripheral for dsi power management
        - name: dsi_display       # [TO_BE_CONFIRMED] DSI peripheral instance used for this display

SPI Full Fields
^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example LCD Display device with SPI sub device configuration
    - name: display_lcd          # The name of the device, must be unique
      type: display_lcd          # The type of the device, must be unique
      version: 1.0.0
      sub_type: spi              # The sub type: dsi, spi, or parlio
      chip: generic_lcd          # [TO_BE_CONFIRMED] LCD chip type (e.g., st77916, st7789, ili9341, gc9a01)
      dependencies:
        espressif/esp_lcd_generic: "*"  # [TO_BE_CONFIRMED] Component dependency for the LCD chip
      config:
        swap_xy: false
        mirror_y: false
        mirror_x: false
        x_max: 320                          # [TO_BE_CONFIRMED] Horizontal resolution
        y_max: 240                          # [TO_BE_CONFIRMED] Vertical resolution
        invert_color: false                 # Invert color flag (default: false)
        need_reset: true                    # Whether to reset the LCD panel during initialization (default: true)
        # esp_lcd_panel_io_spi_config_t fields for SPI communication
        io_spi_config:
          cs_gpio_num: -1                   # [IO] GPIO pin for Chip Select (default: -1)
          dc_gpio_num: -1                   # [IO] GPIO pin for Data/Command (default: -1)
          spi_mode: 0                       # Traditional SPI mode (0~3) (default: 0)
          pclk_hz: 40000000                 # [TO_BE_CONFIRMED] Frequency of pixel clock (default: 40MHz)
          trans_queue_depth: 1              # Size of internal transaction queue (default: 1)
          lcd_cmd_bits: 8                   # Bit-width of LCD command (default: 8)
          lcd_param_bits: 8                 # Bit-width of LCD parameter (default: 8)
          cs_ena_pretrans: 0                # CS pre-transmission cycles (default: 0)
          cs_ena_posttrans: 0               # CS post-transmission cycles (default: 0)
          flags:
            dc_high_on_cmd: false           # DC level = 1 indicates command transfer (default: false)
            dc_low_on_data: false           # DC level = 0 indicates color data transfer (default: false)
            dc_low_on_param: false          # DC level = 0 indicates parameter transfer (default: false)
            octal_mode: false               # Octal mode (8 data lines) (default: false)
            quad_mode: false                # Quad mode (4 data lines) (default: false)
            sio_mode: false                 # Single line mode (default: false)
            psram_dma_direct: false         # IDF v6.1+: DMA directly from PSRAM color buffers (default: false)
            lsb_first: false                # false：MSB bit first；true：LSB bit first (default: false)
            cs_high_active: false           # CS line is low active (default: false)

        # esp_lcd_panel_dev_config_t fields for panel configuration
        lcd_panel_config:
          reset_gpio_num: -1                # Reset GPIO pin (default: -1); set to SoC pin [IO] when RST is on GPIO
          rgb_ele_order: LCD_RGB_ELEMENT_ORDER_RGB  # [TO_BE_CONFIRMED] RGB element order (default: RGB)
          # Valid values:
          # - LCD_RGB_ELEMENT_ORDER_RGB
          # - LCD_RGB_ELEMENT_ORDER_BGR
          data_endian: LCD_RGB_DATA_ENDIAN_BIG      # [TO_BE_CONFIRMED] Data endianness (default: BIG)
          # Valid values:
          # - LCD_RGB_DATA_ENDIAN_BIG
          # - LCD_RGB_DATA_ENDIAN_LITTLE
          bits_per_pixel: 16                # [TO_BE_CONFIRMED] Bits per pixel (default: 16)
          flags:
            reset_active_high: false        # Reset pin active level (default: false)

          # Chip-specific configuration
          vendor_config: ""                 # Vendor-specific configuration (default: empty string)

      peripherals:
        - name: spi_master                  # [TO_BE_CONFIRMED] SPI peripheral for LCD communication

I80 Full Fields
^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example LCD Display device with I80 sub device configuration
    - name: display_lcd          # The name of the device, must be unique
      type: display_lcd          # The type of the device, must be unique
      version: default
      sub_type: i80              # The sub type of the device, must be 'dsi', 'spi' or 'i80'
      chip: generic_lcd          # [TO_BE_CONFIRMED] LCD chip type (e.g., st7789, nt35510, ili9341)
      dependencies:
        espressif/esp_lcd_generic: "*"  # [TO_BE_CONFIRMED] Component dependency for the LCD chip
      config:
        swap_xy: false
        mirror_y: false
        mirror_x: false
        x_max: 240                          # [TO_BE_CONFIRMED] Horizontal resolution
        y_max: 280                          # [TO_BE_CONFIRMED] Vertical resolution
        invert_color: true                  # Invert color flag (default: true)
        need_reset: true                    # Whether to reset the LCD panel during initialization (default: true)

        # esp_lcd_i80_bus_config_t fields for I80 bus configuration
        bus_config:
          dc_gpio_num: -1                   # [IO] GPIO used for D/C line
          wr_gpio_num: -1                   # [IO] GPIO used for WR line
          clk_src: 0                        # Clock source for the I80 LCD peripheral (default: 0)
          data_gpio_nums: []                # [IO] GPIOs used for data lines (D0-D15)
          bus_width: 8                      # Number of data lines, 8 or 16 (default: 8)
          max_transfer_bytes: 4092          # Maximum transfer size (default: 4092)
          dma_burst_size: 64                # DMA burst size in bytes (default: 64)
          flags:
            allow_pd: false                 # IDF v6+: allow power domain off in sleep (default: false)

        # esp_lcd_panel_io_i80_config_t fields for Panel IO configuration
        io_config:
          cs_gpio_num: -1                   # [IO] GPIO used for CS line (-1 if exclusive)
          pclk_hz: 10000000                 # Frequency of pixel clock (default: 10MHz)
          trans_queue_depth: 10             # Transaction queue size (default: 10)
          lcd_cmd_bits: 8                   # Bit-width of LCD command (default: 8)
          lcd_param_bits: 8                 # Bit-width of LCD parameter (default: 8)
          dc_levels:
            dc_idle_level: 0                # Level of DC line in IDLE phase
            dc_cmd_level: 0                 # Level of DC line in CMD phase
            dc_dummy_level: 0               # Level of DC line in DUMMY phase
            dc_data_level: 1                # Level of DC line in DATA phase
          flags:
            cs_active_high: false           # CS line is high active (default: false)
            reverse_color_bits: false       # Reverse the data bits
            swap_color_bytes: false         # Swap adjacent two color bytes
            pclk_active_neg: false          # Write data on falling edge of WR
            pclk_idle_low: false            # WR signal stays low in IDLE phase

        # esp_lcd_panel_dev_config_t fields for panel configuration
        panel_config:
          reset_gpio_num: -1                # [IO] Reset GPIO pin (default: -1)
          rgb_ele_order: LCD_RGB_ELEMENT_ORDER_RGB  # [TO_BE_CONFIRMED] RGB element order (default: RGB)
          # Valid values:
          # - LCD_RGB_ELEMENT_ORDER_RGB
          # - LCD_RGB_ELEMENT_ORDER_BGR
          data_endian: LCD_RGB_DATA_ENDIAN_BIG      # [TO_BE_CONFIRMED] Data endianness (default: BIG)
          # Valid values:
          # - LCD_RGB_DATA_ENDIAN_BIG
          # - LCD_RGB_DATA_ENDIAN_LITTLE
          bits_per_pixel: 16                # [TO_BE_CONFIRMED] Bits per pixel (default: 16)
          flags:
            reset_active_high: false        # Reset pin active level (default: false)
          # Chip-specific configuration
          vendor_config: ""                 # Vendor-specific configuration (default: empty string)

RGB Full Fields
^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example LCD over RGB (esp_lcd_new_rgb_panel), sub_type rgb — no panel IO / chip factory.
    - name: display_lcd
      type: display_lcd
      version: 1.0.0
      sub_type: rgb
      chip: generic_rgb
      config:
        swap_xy: false                      # Swap X/Y coordinates in esp_lcd_panel_swap_xy() after panel init
        mirror_y: false                     # Mirror panel output on Y axis after panel init
        mirror_x: false                     # Mirror panel output on X axis after panel init
        invert_color: false                 # Invert panel colors after panel init (if panel driver supports it)
        need_reset: true                    # Whether to call esp_lcd_panel_reset() during initialization
        bits_per_pixel: 16                  # Application-side frame buffer color depth; IDF v5.x emits this into rgb panel config
        rgb_ele_order: LCD_RGB_ELEMENT_ORDER_RGB  # [TO_BE_CONFIRMED] RGB/BGR byte order expected by panel helper logic
        data_endian: LCD_RGB_DATA_ENDIAN_BIG      # [TO_BE_CONFIRMED] Endianness for color units larger than 1 byte
        # esp_lcd_rgb_panel_config_t fields
        rgb_panel_config:
          clk_src: LCD_CLK_SRC_DEFAULT      # RGB LCD peripheral clock source (lcd_clock_source_t)
          data_width: 16                    # Number of RGB data lines wired to the panel, e.g. 8/16/18/24
          # IDF v6+: in/out color formats are emitted into esp_lcd_rgb_panel_config_t.
          # IDF v5.x: bits_per_pixel is emitted instead.
          in_color_format: LCD_COLOR_FMT_RGB565   # IDF v6+: frame buffer storage format consumed by RGB driver
          out_color_format: LCD_COLOR_FMT_RGB565  # IDF v6+: on-wire format expected by LCD panel; default follows input format
          num_fbs: 1                         # Number of full-screen frame buffers allocated by driver; 0/1 means single FB
          bounce_buffer_size_px: 0           # Bounce buffer size in pixels; non-zero enables two internal DMA bounce buffers
          dma_burst_size: 64                 # DMA burst size in bytes; affects memory alignment/bandwidth behavior
          hsync_gpio_num: 44                 # [IO] GPIO used for HSYNC signal
          vsync_gpio_num: 45                 # [IO] GPIO used for VSYNC signal
          de_gpio_num: 43                    # [IO] GPIO used for DE signal; set -1 if panel does not use DE
          pclk_gpio_num: 40                  # [IO] GPIO used for pixel clock (PCLK)
          disp_gpio_num: -1                  # [IO] Optional display enable pin; set -1 if not connected
          data_gpio_nums: [8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 33, 34, 35, 36]  # [IO] RGB data bus GPIOs, ordered from D0 upward
          # Optional IDF v6+ advanced hook. If set, the function must be registered by
          # DEVICE_EXTRA_FUNC_REGISTER(name, func) and handle GET/RELEASE actions.
          user_fbs_func: ""                  # Optional board callback name that provides/reclaims user-owned frame buffers
          timings:
            pclk_hz: 18000000                # [TO_BE_CONFIRMED] Pixel clock frequency in Hz; must satisfy panel timing budget
            h_res: 800                       # [TO_BE_CONFIRMED] Active horizontal resolution in pixels
            v_res: 480                       # [TO_BE_CONFIRMED] Active vertical resolution in lines
            hsync_pulse_width: 1             # [TO_BE_CONFIRMED] HSYNC pulse width, unit: PCLK cycles
            hsync_back_porch: 40             # [TO_BE_CONFIRMED] Horizontal back porch, between HSYNC and active data
            hsync_front_porch: 20            # [TO_BE_CONFIRMED] Horizontal front porch, between active data end and next HSYNC
            vsync_pulse_width: 1             # [TO_BE_CONFIRMED] VSYNC pulse width, unit: lines
            vsync_back_porch: 10             # [TO_BE_CONFIRMED] Vertical back porch, between VSYNC and first active line
            vsync_front_porch: 5             # [TO_BE_CONFIRMED] Vertical front porch, between last active line and next VSYNC
            flags:
              hsync_idle_low: false          # HSYNC idle polarity: false=idle high, true=idle low
              vsync_idle_low: false          # VSYNC idle polarity: false=idle high, true=idle low
              de_idle_high: false            # DE idle polarity: false=idle low, true=idle high
              pclk_active_neg: true          # Sample/launch pixel data on falling edge of PCLK when true
              pclk_idle_high: false          # Keep PCLK at high level during idle when true
          flags:
            disp_active_low: false           # disp_gpio_num active level: false=high turns panel on, true=low turns panel on
            refresh_on_demand: false         # Disable continuous streaming; refresh only on draw_bitmap/explicit refresh
            fb_in_psram: true                # Prefer allocating frame buffers in PSRAM when available
            double_fb: false                 # Convenience flag for two driver-owned frame buffers; conflicts with no_fb
            no_fb: false                     # Driver does not allocate frame buffer; requires bounce buffer callback workflow
            bb_invalidate_cache: false       # In bounce-buffer mode, invalidate cache after DMA reads; can be unsafe with concurrent writers

RGB + 3-wire SPI Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example LCD over RGB with 3-wire SPI initialization IO, sub_type rgb_3wire_spi.
    # The 3-wire SPI IO sends LCD controller commands/parameters only; RGB transfers pixel data.
    - name: display_lcd
      type: display_lcd
      version: 1.0.0
      sub_type: rgb_3wire_spi                # RGB pixel bus plus 3-wire SPI command/init IO
      chip: generic_rgb_3wire_spi            # [TO_BE_CONFIRMED] LCD chip type, e.g., gc9503/st7701/nv3052
      dependencies:
        espressif/esp_lcd_panel_io_additions: "*"  # Provides esp_lcd_new_panel_io_3wire_spi()
        espressif/esp_lcd_generic: "*"             # [TO_BE_CONFIRMED] Replace with the real LCD chip component
      config:
        swap_xy: false                      # Swap X/Y coordinates after panel init if the chip driver supports it
        mirror_y: false                     # Mirror panel output on Y axis after panel init if supported
        mirror_x: false                     # Mirror panel output on X axis after panel init if supported
        invert_color: false                 # Invert panel colors after init if supported
        need_reset: true                    # Whether to call esp_lcd_panel_reset() during device initialization
        bits_per_pixel: 16                  # Application-side frame buffer color depth; IDF v5.x may emit this into RGB config
        rgb_ele_order: LCD_RGB_ELEMENT_ORDER_RGB  # RGB/BGR byte order used by common panel helper logic
        data_endian: LCD_RGB_DATA_ENDIAN_BIG      # Endianness for color units larger than 1 byte

        # esp_lcd_panel_io_3wire_spi_config_t fields for the low-speed LCD command/init interface.
        io_3wire_spi_config:
          io_expander_name: ""              # Optional bmgr gpio_expander device name; empty when CS/SCL/SDA are direct GPIOs
          line_config:
            cs_io_type: IO_TYPE_GPIO        # CS line source: IO_TYPE_GPIO or IO_TYPE_EXPANDER
            cs_gpio_num: -1                 # [IO] SoC GPIO for CS when cs_io_type is IO_TYPE_GPIO
            cs_expander_pin: -1             # IO expander pin for CS when cs_io_type is IO_TYPE_EXPANDER
            scl_io_type: IO_TYPE_GPIO       # SCL/SCK line source: IO_TYPE_GPIO or IO_TYPE_EXPANDER
            scl_gpio_num: -1                # [IO] SoC GPIO for SCL/SCK when scl_io_type is IO_TYPE_GPIO
            scl_expander_pin: -1            # IO expander pin for SCL/SCK when scl_io_type is IO_TYPE_EXPANDER
            sda_io_type: IO_TYPE_GPIO       # SDA/MOSI line source: IO_TYPE_GPIO or IO_TYPE_EXPANDER
            sda_gpio_num: -1                # [IO] SoC GPIO for SDA/MOSI when sda_io_type is IO_TYPE_GPIO
            sda_expander_pin: -1            # IO expander pin for SDA/MOSI when sda_io_type is IO_TYPE_EXPANDER
          expect_clk_speed: 500000          # Expected bit-banged clock in Hz; max is PANEL_IO_3WIRE_SPI_CLK_MAX
          spi_mode: 0                       # SPI mode 0~3; select according to LCD IC sampling edge/strap mode
          lcd_cmd_bytes: 1                  # Bytes per LCD command package, valid range 1~4
          lcd_param_bytes: 1                # Bytes per LCD parameter package, valid range 1~4
          flags:
            use_dc_bit: true                # Prefix each command/data package with an in-band D/C bit
            dc_zero_on_data: false          # false: D/C=0 command and D/C=1 data; true reverses that mapping
            lsb_first: false                # false sends MSB first; true sends LSB first
            cs_high_active: false           # false means CS active low; true means CS active high
            del_keep_cs_inactive: true      # Keep CS inactive on panel IO delete; SCL/SDA are still reset/released

        # esp_lcd_rgb_panel_config_t fields for the RGB pixel data interface.
        rgb_panel_config:
          clk_src: LCD_CLK_SRC_DEFAULT      # RGB LCD peripheral clock source (lcd_clock_source_t)
          data_width: 16                    # Number of RGB data lines wired to the panel, e.g., 8/16/18/24
          in_color_format: LCD_COLOR_FMT_RGB565   # IDF v6+: frame buffer storage format consumed by RGB driver
          out_color_format: LCD_COLOR_FMT_RGB565  # IDF v6+: on-wire RGB format expected by the panel
          num_fbs: 1                        # Number of full-screen frame buffers allocated by driver; 0/1 means single FB
          bounce_buffer_size_px: 0          # Bounce buffer size in pixels; non-zero enables internal DMA bounce buffers
          dma_burst_size: 64                # DMA burst size in bytes; affects memory alignment/bandwidth behavior
          hsync_gpio_num: -1                # [IO] GPIO used for HSYNC signal
          vsync_gpio_num: -1                # [IO] GPIO used for VSYNC signal
          de_gpio_num: -1                   # [IO] GPIO used for DE signal; set -1 if panel does not use DE
          pclk_gpio_num: -1                 # [IO] GPIO used for pixel clock (PCLK)
          disp_gpio_num: -1                 # [IO] Optional display enable pin; set -1 if not connected
          data_gpio_nums: [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1]  # [IO] RGB data bus GPIOs, D0 upward
          user_fbs_func: ""                 # Optional IDF v6+ DEVICE_EXTRA_FUNC_REGISTER callback for user-owned RGB frame buffers
          timings:
            pclk_hz: 16000000               # [TO_BE_CONFIRMED] Pixel clock frequency in Hz; must satisfy panel timing budget
            h_res: 480                      # [TO_BE_CONFIRMED] Active horizontal resolution in pixels
            v_res: 480                      # [TO_BE_CONFIRMED] Active vertical resolution in lines
            hsync_pulse_width: 10           # [TO_BE_CONFIRMED] HSYNC pulse width, unit: PCLK cycles
            hsync_back_porch: 10            # [TO_BE_CONFIRMED] Horizontal back porch, between HSYNC and active data
            hsync_front_porch: 20           # [TO_BE_CONFIRMED] Horizontal front porch, between active data end and next HSYNC
            vsync_pulse_width: 10           # [TO_BE_CONFIRMED] VSYNC pulse width, unit: lines
            vsync_back_porch: 10            # [TO_BE_CONFIRMED] Vertical back porch, between VSYNC and first active line
            vsync_front_porch: 10           # [TO_BE_CONFIRMED] Vertical front porch, between last active line and next VSYNC
            flags:
              hsync_idle_low: false         # HSYNC idle polarity: false=idle high, true=idle low
              vsync_idle_low: false         # VSYNC idle polarity: false=idle high, true=idle low
              de_idle_high: false           # DE idle polarity: false=idle low, true=idle high
              pclk_active_neg: false        # Sample/launch pixel data on falling edge of PCLK when true
              pclk_idle_high: false         # Keep PCLK at high level during idle when true
          flags:
            disp_active_low: false          # disp_gpio_num active level: false=high turns panel on, true=low turns panel on
            refresh_on_demand: false        # Disable continuous streaming; refresh only on draw_bitmap/explicit refresh
            fb_in_psram: true               # Prefer allocating frame buffers in PSRAM when available
            double_fb: false                # Convenience flag for two driver-owned frame buffers; conflicts with no_fb
            no_fb: false                    # Driver does not allocate frame buffer; requires bounce buffer callback workflow
            bb_invalidate_cache: false      # In bounce-buffer mode, invalidate cache after DMA reads

        # esp_lcd_panel_dev_config_t fields passed to the LCD chip factory.
        lcd_panel_config:
          reset_gpio_num: -1                # [IO] Reset GPIO pin; set -1 if reset is not connected to a SoC GPIO
          rgb_ele_order: LCD_RGB_ELEMENT_ORDER_RGB  # RGB/BGR element order for LCD controller command setup
          data_endian: LCD_RGB_DATA_ENDIAN_BIG      # Color data endian for units larger than 1 byte
          bits_per_pixel: 18                # [TO_BE_CONFIRMED] LCD controller pixel width, e.g., GC9503 often uses 18
          flags:
            reset_active_high: false        # Reset pin active level: false=active low, true=active high
          vendor_config: NULL               # NULL lets bmgr pass rgb_panel_config as vendor_config by default
          auto_del_panel_io: false          # If chip driver supports it, delete 3-wire panel IO after init; GPIO expander is not deleted

PARLIO Full Fields
^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example LCD over PARLIO (esp_lcd_new_panel_io_parl), sub_type parlio — no spi_master peripheral.
    # io_parl_config maps to esp_lcd_panel_io_parl_config_t (components/esp_lcd/include/esp_lcd_io_parl.h).
    - name: display_lcd
      type: display_lcd
      version: 1.0.0
      sub_type: parlio
      chip: ili9341                           # [TO_BE_CONFIRMED] must match lcd_panel_factory_entry_t + driver
      dependencies:
        espressif/esp_lcd_ili9341: "*"        # [TO_BE_CONFIRMED]
      config:
        swap_xy: false
        mirror_y: false
        mirror_x: false
        x_max: 284                            # [TO_BE_CONFIRMED] Horizontal resolution
        y_max: 240                            # [TO_BE_CONFIRMED] Vertical resolution
        invert_color: false                   # Invert color flag (default: false)
        need_reset: true                      # Whether to reset the LCD panel during initialization (default: true)
        # dev_display_lcd_parlio_sub_config_t fields for Parlio communication
        io_parl_config:
          dc_gpio_num: 4                      # [IO] GPIO used for D/C line
          clk_gpio_num: 5                     # [IO] GPIO used for CLK line
          cs_gpio_num: 6                      # [IO] GPIO used for CS line
          # GPIOs used for data lines (ESP_PARLIO_LCD_WIDTH_MAX == 8); 1-line SPI uses index 0 only, rest -1
          data_gpio_nums: [7, -1, -1, -1, -1, -1, -1, -1]                      # [IO] GPIOs used for data lines
          data_width: 1                       # [TO_BE_CONFIRMED] 1 (SPI) or 8 (I80); must match wiring
          pclk_hz: 40000000                   # [TO_BE_CONFIRMED] Frequency of pixel clock
          clk_src: PARLIO_CLK_SRC_DEFAULT     # Clock source for the Parlio peripheral (parlio_clock_source_t)
          max_transfer_bytes: 11360           # [TO_BE_CONFIRMED] >= largest color transfer; too small breaks DMA path
          dma_burst_size: 32                  # DMA burst size, in bytes
          trans_queue_depth: 10               # Transaction queue size; larger queue, higher throughput
          lcd_cmd_bits: 8                     # Bit-width of LCD command (default: 8)
          lcd_param_bits: 8                   # Bit-width of LCD parameter (default: 8)
          dc_levels:
            dc_cmd_level: 0                   # Level of DC line in CMD phase (default: 0; typical SPI LCD)
            dc_data_level: 1                  # Level of DC line in DATA phase (default: 1)
          flags:
            # If set, high CS selects device; otherwise CS is low-active
            cs_active_high: false
        lcd_panel_config:
          reset_gpio_num: -1                  # Reset GPIO pin (default: -1); else pin [IO]
          rgb_ele_order: LCD_RGB_ELEMENT_ORDER_RGB  # [TO_BE_CONFIRMED] RGB element order (default: RGB)
          # Valid values:
          # - LCD_RGB_ELEMENT_ORDER_RGB
          # - LCD_RGB_ELEMENT_ORDER_BGR
          data_endian: LCD_RGB_DATA_ENDIAN_LITTLE  # [TO_BE_CONFIRMED] Data endianness (default: LITTLE)
          # Valid values:
          # - LCD_RGB_DATA_ENDIAN_BIG
          # - LCD_RGB_DATA_ENDIAN_LITTLE
          bits_per_pixel: 16                    # [TO_BE_CONFIRMED] Bits per pixel (default: 16)
          flags:
            reset_active_high: false             # Reset pin active level (default: false)
          vendor_config: ""

Component Dependencies
----------------------

A ``display_lcd`` device that uses a chip driver component must declare the LCD chip driver component in ``dependencies``. The template uses ``espressif/esp_lcd_generic``, ``espressif/esp_lcd_ili9341``, and ``espressif/esp_lcd_ek79007`` as placeholders or board-level examples. The actual component name and version must match the ``chip`` value and the factory function registered in the board's ``setup_device.c``.

The ``rgb_3wire_spi`` sub-type additionally depends on ``espressif/esp_lcd_panel_io_additions`` to provide the 3-wire SPI panel IO. The pure ``rgb`` path creates the RGB panel directly and does not require an LCD chip factory component.

Required Peripherals
--------------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - Required
     - Purpose
   * - ``dsi``
     - N/A
     - Required for ``dsi`` mode
     - MIPI DSI data interface
   * - ``ldo``
     - N/A
     - Optional for ``dsi`` mode depending on board power design
     - MIPI power management
   * - ``spi``
     - N/A
     - Required for ``spi`` mode
     - SPI panel IO
   * - ``gpio_expander``
     - device handle
     - Required for ``rgb_3wire_spi`` when using IO expander initialization lines
     - Provides CS, SCL, or SDA lines for 3-wire SPI

Code Reference
--------------

- ``esp_board_manager/test_apps/main/test_dev_lcd_init.c``: Obtains the ``display_lcd`` device handle and initializes the LVGL display path.
- ``esp_board_manager/test_apps/main/test_dev_lcd_lvgl.c``: LVGL menu display example.
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_dsi.c``: DSI sub-type initialization implementation.
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_spi.c``: SPI sub-type initialization implementation.
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_i80.c``: I80 sub-type initialization implementation.
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_rgb.c``: RGB sub-type initialization implementation.
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_rgb_3wire_spi.c``: RGB + 3-wire SPI sub-type initialization implementation.
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_parlio.c``: PARLIO sub-type initialization implementation.

Board-level Reference
---------------------

- ``esp_boards/esp32_p4_function_ev_board/board_devices.yaml``: ``dsi`` display configuration.
- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``: ``dsi`` and ``ldo`` peripheral configuration.
- ``esp_boards/esp32_p4_function_ev_board/setup_device.c``: DSI panel factory function.
- ``esp_boards/esp32_s3_korvo_2_3/board_devices.yaml``: ``spi`` display configuration.
- ``esp_boards/esp32_s3_korvo_2_3/board_peripherals.yaml``: SPI peripheral configuration.
- ``esp_boards/esp32_s3_lcd_ev_board/board_devices.yaml``: ``rgb_3wire_spi`` display configuration.
- ``esp_boards/esp32_s3_lcd_ev_board/sub_board_800_480_lcd/panel_800_480_lcd.yaml``: ``rgb`` display amend configuration.

Notes
-----

- ``chip``, ``dependencies``, and the board-level factory function must all refer to the same LCD controller. Otherwise the device can create the panel IO, but the panel initialization commands will not match the actual screen controller.
- ``x_max``, ``y_max``, ``video_timing``, ``timings``, pixel format, and ``bits_per_pixel`` must come from the panel datasheet or a board-level verified configuration.
- When ``rgb_3wire_spi`` uses ``IO_TYPE_EXPANDER``, ``io_expander_name`` must reference an already-initialized ``gpio_expander`` device.
- After modifying the LCD device, display peripheral, or factory function, re-run ``idf.py bmgr -b <board>``.

Debugging Tips
--------------

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain the device handle. The handle type is ``dev_display_lcd_handles_t``:

.. code-block:: c

   typedef struct {
       esp_lcd_panel_io_handle_t  io_handle;     /*!< LCD panel IO handle */
       esp_lcd_panel_handle_t     panel_handle;  /*!< LCD panel device handle */
       /* RGB/RGB_3WIRE_SPI + IDF v6+ only: */
       /* void *rgb_user_fbs[ESP_RGB_LCD_PANEL_MAX_FB_NUM]; */
   } dev_display_lcd_handles_t;

``io_handle`` is ``NULL`` for the RGB interface (RGB has no independent panel IO). The RGB and RGB_3WIRE_SPI sub-types additionally provide the ``rgb_user_fbs`` array under IDF v6+ to store pointers to board-supplied frame buffers.

Related declarations are in ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd.h``.
