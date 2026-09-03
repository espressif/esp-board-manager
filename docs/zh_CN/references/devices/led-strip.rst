LED 灯带 led_strip
==================

:link_to_translation:`en:[English]`

.. _led-strip-intro:

简介
------

``led_strip`` 是基于 ESP-IDF ``led_strip`` 组件的可寻址灯带 device，用于初始化 WS2812、SK6812、WS2811、WS2816 等单线协议 LED。该设备将公共灯带参数与具体后端配置写入 ``board_devices.yaml``；应用通过 BMGR 获取 ``led_strip_handle_t`` 后，调用 ``led_strip`` 组件 API 设置像素并刷新输出。

``led_strip`` 按 ``sub_type`` 选择后端。当前源码实现 ``rmt`` 与 ``spi`` 两种子类型；两种模式均由 ``led_strip`` 组件在设备初始化时创建底层驱动实例，无需在 ``board_peripherals.yaml`` 中新增 RMT 或 SPI 外设条目。

.. _led-strip-usage-modes:

支持的使用模式
---------------------

``led_strip`` 按 ``sub_type`` 区分输出后端：

- `RMT 灯带`_
- `SPI 灯带`_

.. _led-strip-min-config:

最小配置
------------

.. _led-strip-rmt:

RMT 灯带
^^^^^^^^^^

完整字段见 :ref:`led-strip-rmt-full`。

``rmt`` 模式调用 ``led_strip_new_rmt_device`` 创建灯带实例，适合使用 RMT 后端输出灯带，初始化成功后调用 ``led_strip_clear`` 清空灯带输出，无需新增 ``board_peripherals.yaml`` 条目。

``board_devices.yaml``：

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

SPI 灯带
^^^^^^^^^^

完整字段见 :ref:`led-strip-spi-full`。

``spi`` 模式调用 ``led_strip_new_spi_device`` 创建灯带实例，适合使用 SPI 后端输出灯带，初始化成功后调用 ``led_strip_clear`` 清空灯带输出，无需新增 ``board_peripherals.yaml`` 条目；``led_strip`` 组件会按设备配置创建 SPI 后端，所选 ``spi_bus`` 不能与其他已经初始化的 SPI 总线冲突。

``board_devices.yaml``：

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

完整字段
------------

.. _led-strip-rmt-full:

RMT 灯带完整字段
^^^^^^^^^^^^^^^^^^^^^^

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

SPI 灯带完整字段
^^^^^^^^^^^^^^^^^^^^^^

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

组件依赖
------------

``led_strip`` 会通过 ``esp_board_manager/idf_component.yml`` 在启用 ``CONFIG_ESP_BOARD_DEV_LED_STRIP_SUPPORT`` 时引入 ``espressif/led_strip``，版本为 ``"*"``。板级 YAML 不需要为该通用组件重复声明 ``dependencies``。

.. _led-strip-peripherals:

依赖外设
------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - 必选性
     - 用途
   * - 无
     - 无
     - 不需要 BMGR peripheral
     - ``led_strip`` 组件在设备初始化时创建 RMT 或 SPI 后端

.. _led-strip-code:

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_led_strip.c``
- ``esp_board_manager/devices/dev_led_strip/dev_led_strip.c``
- ``esp_board_manager/devices/dev_led_strip/dev_led_strip_sub_rmt.c``
- ``esp_board_manager/devices/dev_led_strip/dev_led_strip_sub_spi.c``

.. _led-strip-boards:

板级参考
------------

- ``esp_boards/esp32_s31_korvo_1/board_devices.yaml``：RMT 模式板载 WS2812 状态灯配置。
- ``esp_boards/esp32_s31_function_coreboard_1/board_devices.yaml``：RMT 模式板载 WS2812 状态灯配置。
- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_s3_devkitc/board_devices.yaml``：SPI 模式 ``led_strip`` 测试配置。

.. _led-strip-notes:

注意事项
------------

- ``strip_gpio_num`` 必须按原理图填写， ``max_leds`` 必须大于 0。
- SPI 模式的 ``spi_bus`` 由 ``led_strip`` 驱动内部初始化，不能与其他已初始化 SPI 总线冲突。
- 存在电平反相或电平转换电路时，需要按硬件连接确认 ``invert_out``。
- 修改 YAML 后需要重新执行 ``idf.py bmgr -b <board>``。

.. _led-strip-debug:

调试技巧
------------

.. _led-strip-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，句柄类型为 ``dev_led_strip_handles_t``：

.. code-block:: c

   typedef struct {
       led_strip_handle_t  strip_handle;  /*!< Native led_strip handle */
   } dev_led_strip_handles_t;

``strip_handle`` 可直接传入 ``espressif/led_strip`` 组件的控制接口（设置颜色、刷新、清除等）。

相关声明位于 ``esp_board_manager/devices/dev_led_strip/dev_led_strip.h``。
