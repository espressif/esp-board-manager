显示屏 display_lcd
==================

:link_to_translation:`en:[English]`

简介
------

``display_lcd`` 设备用于描述 LCD 显示屏，并在初始化阶段创建 ``esp_lcd_panel_handle_t`` 与可选的 panel IO 句柄。应用通过 :cpp:func:`esp_board_manager_get_device_handle` 获取 ``dev_display_lcd_handles_t``，再使用 ESP-IDF ``esp_lcd_panel_ops`` 或上层图形库访问屏幕。

该类型按 ``sub_type`` 选择显示接口。当前设备模板覆盖 ``dsi``、``spi``、``i80``、``rgb``、``rgb_3wire_spi`` 与 ``parlio``。除纯 ``rgb`` 外，各子类型都必须由开发板提供 LCD panel 工厂函数；仅声明芯片驱动组件不足以创建 panel。

支持的使用模式
---------------------

``display_lcd`` 以 ``sub_type`` 作为分类轴：

- :ref:`display-lcd-dsi`
- :ref:`display-lcd-spi`
- :ref:`display-lcd-i80`
- :ref:`display-lcd-rgb`
- :ref:`display-lcd-rgb-3wire-spi`
- :ref:`display-lcd-parlio`

帧格式
------

``dev_display_lcd`` 会在设备配置中生成 ``frame_format``，供需要选择像素缓冲区或转换格式的应用使用。解析器能够确定像素表示格式时，取值为 ``RGB565_LE``、``RGB565_BE``、``BGR888`` 或 ``RGB888``。无法确定或不支持的布局使用 ``UNKNOWN``。

可以通过 ``config.frame_format`` 覆盖自动推导结果：

.. code-block:: yaml

    config:
      frame_format: RGB565_LE

解析阶段会校验覆盖值。如果显式值与 BMGR 能够自动推导的格式不同，解析器会发出告警。生成的值由应用代码使用；LCD 测试应用根据该值选择 LVGL 字节交换或图像转换输出格式。

未配置 ``config.frame_format`` 时，BMGR 按以下规则自动推导：

.. list-table::
   :header-rows: 1

   * - ``sub_type``
     - 推导依据
   * - ``dsi``
     - ESP-IDF 5.x 使用 ``dpi_config.pixel_format``；ESP-IDF 6.x 及以上使用 ``dpi_config.in_color_format``
   * - ``rgb``、``rgb_3wire_spi``
     - ESP-IDF 5.x 使用 ``bits_per_pixel``；ESP-IDF 6.x 及以上使用 ``rgb_panel_config.in_color_format``
   * - ``spi``
     - 使用 ``lcd_panel_config.data_endian``；未配置时默认为 ``RGB565_BE``
   * - ``i80``
     - 使用 ``panel_config.data_endian``，并根据 ``io_config.flags.swap_color_bytes`` 调整结果
   * - ``parlio``
     - 当前无法自动确定，建议显式配置 ``frame_format``

.. note::

   配置 ``data_endian`` 前，需要核对对应的屏幕芯片实现是否读取 ``esp_lcd_panel_dev_config_t.data_endian``。

   即使屏幕芯片实现不读取 ``data_endian``，SPI 和 I80 模式仍可能使用该字段推导应用侧的 ``frame_format``。因此，删除或修改 ``data_endian`` 可能改变 ``frame_format`` 的推导结果。

主要映射关系如下：

- RGB565 色彩格式映射为 ``RGB565_LE``。
- RGB888 色彩格式映射为 ``BGR888``。
- 16-bit RGB 配置映射为 ``RGB565_LE``。
- ``LCD_RGB_DATA_ENDIAN_BIG`` 映射为 ``RGB565_BE``。
- ``LCD_RGB_DATA_ENDIAN_LITTLE`` 映射为 ``RGB565_LE``。

当配置不足或组合不受支持时，自动推导结果为 ``UNKNOWN``，并输出告警。

板级 panel 工厂函数
--------------------------

BMGR 负责创建显示总线和 panel IO，开发板负责调用实际 LCD 控制器的构造函数并返回 ``esp_lcd_panel_handle_t``。

.. list-table::
   :header-rows: 1

   * - ``sub_type``
     - 是否需要板级函数
     - 必需符号
   * - ``dsi``
     - 是
     - ``lcd_dsi_panel_factory_entry_t``
   * - ``spi``、``i80``、``parlio``、``rgb_3wire_spi``
     - 是
     - ``lcd_panel_factory_entry_t``
   * - ``rgb``
     - 否
     - BMGR 直接调用 ``esp_lcd_new_rgb_panel``

DSI 工厂函数必须使用以下签名。函数负责根据 ``lcd_cfg`` 创建控制器专用 panel，并写入 ``lcd_handles->panel_handle``：

.. code-block:: c

    esp_err_t lcd_dsi_panel_factory_entry_t(esp_lcd_dsi_bus_handle_t dsi_handle,
                                            dev_display_lcd_config_t *lcd_cfg,
                                            dev_display_lcd_handles_t *lcd_handles);

SPI、I80、PARLIO 和 RGB + 3-wire SPI 共享以下签名。函数应调用与 LCD 控制器匹配的 ``esp_lcd_new_panel_*`` API：

.. code-block:: c

    esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                         const esp_lcd_panel_dev_config_t *panel_dev_config,
                                         esp_lcd_panel_handle_t *ret_panel);

基础开发板的实现建议声明为 ``__attribute__((weak))``，使 amend 可以提供同名强符号替换默认面板初始化。工厂函数使用的芯片驱动头文件和 API 必须与设备的 ``chip`` 及 ``dependencies`` 指向同一控制器。源文件放置与弱符号覆盖规则见 :doc:`/programming-guide/board-directory`\ 。

最小配置
------------

.. _display-lcd-dsi:

DSI（``sub_type: dsi``）
^^^^^^^^^^^^^^^^^^^^^^^^^^

``dsi`` 模式用于 MIPI DSI 屏，设备必须引用 ``dsi`` 与 ``ldo`` 外设；运行时会先获取 LDO 句柄，再创建 DSI panel IO。该模式必须实现 ``lcd_dsi_panel_factory_entry_t``。

``board_peripherals.yaml``：

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

``board_devices.yaml``：

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
          - ldo_name: ldo_mipi
          - dsi_name: dsi_display

.. _display-lcd-spi:

SPI（``sub_type: spi``）
^^^^^^^^^^^^^^^^^^^^^^^^^^

``spi`` 模式用于通过 SPI panel IO 发送命令与像素数据的屏，设备依赖 ``spi`` 外设。该模式必须实现 ``lcd_panel_factory_entry_t``。

``board_peripherals.yaml``：

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

``board_devices.yaml``：

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
          - spi_name: spi_master

.. _display-lcd-i80:

I80（``sub_type: i80``）
^^^^^^^^^^^^^^^^^^^^^^^^^^

``i80`` 模式使用 ``esp_lcd_new_i80_bus`` 与 ``esp_lcd_new_panel_io_i80`` 创建 panel IO，不依赖 ``spi`` 外设；I80 总线由设备内部基于 ``bus_config`` 创建，无需新增 ``board_peripherals.yaml`` 条目。该模式必须实现 ``lcd_panel_factory_entry_t``。

``board_devices.yaml``：

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

RGB（``sub_type: rgb``）
^^^^^^^^^^^^^^^^^^^^^^^^^^

``rgb`` 模式使用 RGB LCD 外设直接输出像素数据，不调用通用 ``lcd_panel_factory_entry_t``；RGB 总线 GPIO 在设备 ``rgb_panel_config`` 中配置，无需新增 ``board_peripherals.yaml`` 条目。

``board_devices.yaml``：

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

RGB + 3-wire SPI（``sub_type: rgb_3wire_spi``）
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``rgb_3wire_spi`` 模式在 RGB 像素总线之外增加 3-wire SPI 初始化 IO，无需 ``spi`` 外设；3-wire SPI 初始化线可直接使用 SoC GPIO，或通过 ``gpio_expander`` 设备提供。该模式必须实现 ``lcd_panel_factory_entry_t``，用于执行控制器的命令初始化。

``board_devices.yaml``：

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

RGB 自定义用户帧缓冲区
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``rgb`` 与 ``rgb_3wire_spi`` 类型的 LCD 在 ESP-IDF v6.0 及以上版本支持 ``user_fbs_func`` 字段。该字段不是函数指针，而是通过 ``DEVICE_EXTRA_FUNC_REGISTER`` 注册的板级回调名称，用于向 RGB LCD 驱动提供应用或板级代码管理的帧缓冲区。

在 ``board_devices.yaml`` 中，将 ``user_fbs_func`` 写在 ``rgb_panel_config`` 下：

.. code-block:: yaml

    devices:
      - name: display_lcd
        type: display_lcd
        sub_type: rgb
        config:
          rgb_panel_config:
            num_fbs: 2
            user_fbs_func: lcd_rgb_user_fbs

板级代码需要注册同名回调：

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

BMGR 在创建 RGB panel 前调用该回调，并将返回的指针传入 ``esp_lcd_rgb_panel_config_t.user_fbs`` 字段。初始化失败或设备反初始化时，BMGR 再次调用该回调。``GET`` 分支需填入与 ``num_fbs`` 匹配的有效帧缓冲区指针；回调返回非 0 时当作内存申请失败。

使用该字段时，帧缓冲区数量、大小、对齐方式和内存能力需与分辨率、颜色格式、ESP-IDF RGB LCD 驱动要求以及 ``num_fbs`` 配置一致。未使用用户帧缓冲区时，将 ``user_fbs_func`` 留空，由驱动按 ``num_fbs`` 和 ``fb_in_psram`` 等配置管理帧缓冲区。

.. _display-lcd-parlio:

PARLIO（``sub_type: parlio``）
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``parlio`` 模式使用 ``esp_lcd_new_panel_io_parl`` 创建 panel IO，无需 ``spi`` 外设；PARLIO 数据线、时钟线与控制线均在设备 ``io_parl_config`` 中配置，无需新增 ``board_peripherals.yaml`` 条目。该模式必须实现 ``lcd_panel_factory_entry_t``。

``board_devices.yaml``：

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

完整字段
------------

DSI 完整字段
^^^^^^^^^^^^^^^^

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
        data_endian: LCD_RGB_DATA_ENDIAN_BIG      # 传递给 DSI panel factory 的面板数据字节序
        # Valid values:
        # - LCD_RGB_DATA_ENDIAN_BIG
        # - LCD_RGB_DATA_ENDIAN_LITTLE
        bits_per_pixel: 24                        # [TO_BE_CONFIRMED] Bits per pixel (24bpp, RGB888)
        frame_format: RGB565_LE                   # 应用帧缓冲格式，应与 dpi_config 的像素格式保持一致
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
        - ldo_name: ldo_mipi          # [TO_BE_CONFIRMED] LDO peripheral for dsi power management
        - dsi_name: dsi_display       # [TO_BE_CONFIRMED] DSI peripheral instance used for this display

SPI 完整字段
^^^^^^^^^^^^^^^^

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
          data_endian: LCD_RGB_DATA_ENDIAN_BIG      # Verify the selected LCD driver consumes this field; if frame_format is omitted, BMGR may derive RGB565_LE/RGB565_BE from it
          # Valid values:
          # - LCD_RGB_DATA_ENDIAN_BIG
          # - LCD_RGB_DATA_ENDIAN_LITTLE
          bits_per_pixel: 16                # [TO_BE_CONFIRMED] Bits per pixel (default: 16)
          flags:
            reset_active_high: false        # Reset pin active level (default: false)

          # Chip-specific configuration
          vendor_config: ""                 # Vendor-specific configuration (default: empty string)

      peripherals:
        - spi_name: spi_master                  # [TO_BE_CONFIRMED] SPI peripheral for LCD communication

I80 完整字段
^^^^^^^^^^^^^^^^

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
          data_endian: LCD_RGB_DATA_ENDIAN_BIG      # Verify the selected LCD driver consumes this field; if frame_format is omitted, BMGR may derive RGB565_LE/RGB565_BE from it
          # Valid values:
          # - LCD_RGB_DATA_ENDIAN_BIG
          # - LCD_RGB_DATA_ENDIAN_LITTLE
          bits_per_pixel: 16                # [TO_BE_CONFIRMED] Bits per pixel (default: 16)
          flags:
            reset_active_high: false        # Reset pin active level (default: false)
          # Chip-specific configuration
          vendor_config: ""                 # Vendor-specific configuration (default: empty string)

RGB 完整字段
^^^^^^^^^^^^^^^^

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

RGB + 3-wire SPI 完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

PARLIO 完整字段
^^^^^^^^^^^^^^^^^^^

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

组件依赖
------------

使用芯片驱动组件的 ``display_lcd`` 设备需要在 ``dependencies`` 中声明 LCD 芯片驱动组件。模板使用 ``espressif/esp_lcd_generic``、``espressif/esp_lcd_ili9341``、``espressif/esp_lcd_ek79007`` 作为占位或板级示例，实际组件名与版本应与 ``chip`` 及板级 ``setup_device.c`` 中注册的工厂函数一致。

``rgb_3wire_spi`` 额外依赖 ``espressif/esp_lcd_panel_io_additions`` 提供 3-wire SPI panel IO。纯 ``rgb`` 路径直接创建 RGB panel，无需 LCD 芯片 factory 组件。

依赖外设
------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - 必选性
     - 用途
   * - ``dsi``
     - 无
     - ``dsi`` 模式必选
     - MIPI DSI 数据接口
   * - ``ldo``
     - 无
     - ``dsi`` 模式必选
     - MIPI 供电管理
   * - ``spi``
     - 无
     - ``spi`` 模式必选
     - SPI panel IO
   * - ``gpio_expander``
     - 设备句柄
     - ``rgb_3wire_spi`` 使用扩展 IO 初始化线时需要
     - 提供 3-wire SPI 的 CS、SCL 或 SDA 线

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_lcd_init.c``：获取 ``display_lcd`` 设备句柄并初始化 LVGL 显示路径。
- ``esp_board_manager/test_apps/main/test_dev_lcd_lvgl.c``：LVGL 菜单显示示例。
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_dsi.c``：DSI 子类型初始化实现。
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_spi.c``：SPI 子类型初始化实现。
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_i80.c``：I80 子类型初始化实现。
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_rgb.c``：RGB 子类型初始化实现。
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_rgb_3wire_spi.c``：RGB + 3-wire SPI 子类型初始化实现。
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_parlio.c``：PARLIO 子类型初始化实现。

板级参考
------------

- ``esp_boards/esp32_p4_function_ev_board/board_devices.yaml``：``dsi`` 屏配置。
- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``：``dsi`` 和 ``ldo`` 外设配置。
- ``esp_boards/esp32_p4_function_ev_board/setup_device.c``：DSI panel 工厂函数。
- ``esp_boards/esp32_s3_korvo_2_3/board_devices.yaml``：``spi`` 屏配置。
- ``esp_boards/esp32_s3_korvo_2_3/board_peripherals.yaml``：SPI 外设配置。
- ``esp_boards/esp32_s3_lcd_ev_board/board_devices.yaml``：``rgb_3wire_spi`` 屏配置。
- ``esp_boards/esp32_s3_lcd_ev_board/sub_board_800_480_lcd/panel_800_480_lcd.yaml``：``rgb`` 屏 amend 配置。

注意事项
------------

- ``chip``、``dependencies`` 与板级工厂函数必须匹配同一颗 LCD 控制器；否则设备虽可创建 panel IO，但 panel 初始化命令与实际屏幕控制器不一致。
- ``x_max``、``y_max``、``video_timing``、``timings``、像素格式与 ``bits_per_pixel`` 应来自屏幕规格书或板级已验证配置。
- ``rgb_3wire_spi`` 使用 ``IO_TYPE_EXPANDER`` 时，``io_expander_name`` 必须引用已初始化的 ``gpio_expander`` 设备。
- 修改 LCD 设备、显示外设或工厂函数后，需重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，句柄类型为 ``dev_display_lcd_handles_t``：

.. code-block:: c

   typedef struct {
       esp_lcd_panel_io_handle_t  io_handle;     /*!< LCD panel IO handle */
       esp_lcd_panel_handle_t     panel_handle;  /*!< LCD panel device handle */
       /* RGB/RGB_3WIRE_SPI + IDF v6+ only: */
       /* void *rgb_user_fbs[ESP_RGB_LCD_PANEL_MAX_FB_NUM]; */
   } dev_display_lcd_handles_t;

``io_handle`` 在 RGB 接口下为 ``NULL``（RGB 无独立 panel IO）。RGB 和 RGB_3WIRE_SPI 子类型在 IDF v6+ 下额外提供 ``rgb_user_fbs`` 数组，用于存放板级提供的帧缓冲区指针。

相关声明位于 ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd.h``。
