rmt
=======

:link_to_translation:`en:[English]`

简介
------

``rmt`` 外设类型用于描述 ESP-IDF RMT 通道。BMGR 根据 ``role`` 创建 TX 或 RX channel，并返回 ``periph_rmt_handle_t``。

该外设适用于需要直接暴露 RMT 通道句柄的板级配置。LED 灯带设备也可使用 RMT，但 ``led_strip`` 的 ``rmt`` 参数写在设备侧配置中；只有当设备显式引用已定义的 ``rmt`` 外设时，才需在 ``board_peripherals.yaml`` 中创建 ``rmt`` 外设。

支持的工作模式
---------------------

``rmt`` 按 ``role`` 区分发送与接收模式。

- `TX 通道`_
- `RX 通道`_

最小配置
------------

TX 通道
^^^^^^^^^

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: rmt_tx
        type: rmt
        role: tx
        config:
          gpio_num: 18
          clk_src: RMT_CLK_SRC_DEFAULT
          resolution_hz: 10000000
          mem_block_symbols: 64
          trans_queue_depth: 4
          intr_priority: 1
          flags:
            invert_out: false
            with_dma: false
            io_od_mode: false
            allow_pd: false
            init_level: 0

RX 通道
^^^^^^^^^

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: rmt_rx
        type: rmt
        role: rx
        config:
          gpio_num: 19
          clk_src: RMT_CLK_SRC_DEFAULT
          resolution_hz: 1000000
          mem_block_symbols: 64
          intr_priority: 1
          flags:
            invert_in: false
            with_dma: false
            allow_pd: false

模式说明
------------

``role: tx`` 使用 ``rmt_tx_channel_config_t`` 字段，包含 ``trans_queue_depth`` 和 TX 输出相关 flags。``role: rx`` 使用 ``rmt_rx_channel_config_t`` 字段，不包含 TX 队列深度。RMT 通道创建后，编码器、接收缓冲、发送或接收动作由使用该句柄的设备或应用代码配置。

IDF 5 与 IDF 6 的 RMT YAML 模板基本一致，差异在 flags：IDF 5 模板包含 ``io_loop_back``，IDF 6 模板将开漏相关配置放入 BMGR extra flags，并且模板中不再列出 ``io_loop_back``。

完整字段
------------

IDF 5 TX 通道完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # RMT Peripheral Default Configuration
    # This file shows the default values used by the RMT peripheral parser
    # Based on periph_rmt.py parsing script
    # Note: Configuration now uses ESP-IDF native structures via union

    # -----------------------------------------------------------------------------
    # RMT TX example - Using rmt_tx_channel_config_t structure
    # -----------------------------------------------------------------------------
    - name: rmt_tx
      type: rmt
      role: tx
      config:
        # Basic channel configuration
        gpio_num: 18                     # [IO] GPIO number used by RMT channel. Set to -1 if unused
        clk_src: RMT_CLK_SRC_DEFAULT     # Clock source of RMT channel, channels in same group must use same clock source
                                          # Please check the 'rmt_clock_source_t' in 'driver/rmt_types.h' for valid values
        resolution_hz: 10000000          # [TO_BE_CONFIRMED] Channel clock resolution, in Hz (10MHz = 0.1us precision)
        mem_block_symbols: 64            # Size of memory block in number of rmt_symbol_word_t (even number required)
                                          # In DMA mode: controls DMA buffer size
                                          # In normal mode: controls number of RMT memory blocks
        trans_queue_depth: 4             # Depth of internal transfer queue (more transfers pending in background)
        intr_priority: 1                 # RMT interrupt priority, if set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)

        # TX channel flags
        flags:
          invert_out: false              # Whether to invert RMT channel signal before output to GPIO pad
          with_dma: false                # If set, allocate RMT channel with DMA capability
                                          # Please check the macro definition SOC_RMT_SUPPORT_DMA in soc/soc_caps.h
                                          # to determine whether the chip supports
          io_loop_back: false            # For debug: signal output from GPIO fed back to input path
          io_od_mode: false              # Configure GPIO as open-drain mode
          allow_pd: false                # If set, allow power domain to power off during sleep (saves power, uses more RAM)
          init_level: 0                  # Set initial level of RMT channel signal (0=low, 1=high)

IDF 5 RX 通道完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # -----------------------------------------------------------------------------
    # RMT RX example - Using rmt_rx_channel_config_t structure
    # -----------------------------------------------------------------------------
    - name: rmt_rx
      type: rmt
      role: rx
      config:
        # Basic channel configuration
        gpio_num: 19                     # [IO] GPIO number used by RMT channel. Set to -1 if unused
        clk_src: RMT_CLK_SRC_DEFAULT     # Clock source of RMT channel, channels in same group must use same clock source
                                          # Please check the 'rmt_clock_source_t' in 'driver/rmt_types.h' for valid values
        resolution_hz: 1000000           # [TO_BE_CONFIRMED] Channel clock resolution, in Hz (1MHz = 1us precision, good for 38kHz IR)
        mem_block_symbols: 64            # Size of memory block in number of rmt_symbol_word_t (even number required)
                                          # In DMA mode: controls DMA buffer size
                                          # In normal mode: controls number of RMT memory blocks
        intr_priority: 1                 # RMT interrupt priority, if set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)

        # RX channel flags
        flags:
          invert_in: false               # Whether to invert the incoming RMT channel signal
          with_dma: false                # If set, allocate RMT channel with DMA capability
                                          # Please check the macro definition SOC_RMT_SUPPORT_DMA in soc/soc_caps.h
                                          # to determine whether the chip supports
          io_loop_back: false            # For debug: signal output from GPIO fed back to input path
          allow_pd: false                # If set, allow power domain to power off during sleep (saves power, uses more RAM)

IDF 6 TX 通道完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # RMT Peripheral Default Configuration
    # This file shows the default values used by the RMT peripheral parser
    # Based on periph_rmt.py parsing script
    # Note: Configuration now uses ESP-IDF native structures via union

    # -----------------------------------------------------------------------------
    # RMT TX example - Using rmt_tx_channel_config_t structure
    # -----------------------------------------------------------------------------
    - name: rmt_tx
      type: rmt
      role: tx
      config:
        # Basic channel configuration
        gpio_num: 18                     # [IO] GPIO number used by RMT channel. Set to -1 if unused
        clk_src: RMT_CLK_SRC_DEFAULT     # Clock source of RMT channel, channels in same group must use same clock source
                                          # Please check the 'rmt_clock_source_t' in 'driver/rmt_types.h' for valid values
        resolution_hz: 10000000          # [TO_BE_CONFIRMED] Channel clock resolution, in Hz (10MHz = 0.1us precision)
        mem_block_symbols: 64            # Size of memory block in number of rmt_symbol_word_t (even number required)
                                          # In DMA mode: controls DMA buffer size
                                          # In normal mode: controls number of RMT memory blocks
        trans_queue_depth: 4             # Depth of internal transfer queue (more transfers pending in background)
        intr_priority: 1                 # RMT interrupt priority, if set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)

        # TX channel flags
        flags:
          invert_out: false              # Whether to invert RMT channel signal before output to GPIO pad
          with_dma: false                # If set, allocate RMT channel with DMA capability
                                          # Please check the macro definition SOC_RMT_SUPPORT_DMA in soc/soc_caps.h
                                          # to determine whether the chip supports
          io_od_mode: false              # Configure GPIO as open-drain mode
          allow_pd: false                # If set, allow power domain to power off during sleep (saves power, uses more RAM)
          init_level: 0                  # Set initial level of RMT channel signal (0=low, 1=high)

IDF 6 RX 通道完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # -----------------------------------------------------------------------------
    # RMT RX example - Using rmt_rx_channel_config_t structure
    # -----------------------------------------------------------------------------
    - name: rmt_rx
      type: rmt
      role: rx
      config:
        # Basic channel configuration
        gpio_num: 19                     # [IO] GPIO number used by RMT channel. Set to -1 if unused
        clk_src: RMT_CLK_SRC_DEFAULT     # Clock source of RMT channel, channels in same group must use same clock source
                                          # Please check the 'rmt_clock_source_t' in 'driver/rmt_types.h' for valid values
        resolution_hz: 1000000           # [TO_BE_CONFIRMED] Channel clock resolution, in Hz (1MHz = 1us precision, good for 38kHz IR)
        mem_block_symbols: 64            # Size of memory block in number of rmt_symbol_word_t (even number required)
                                          # In DMA mode: controls DMA buffer size
                                          # In normal mode: controls number of RMT memory blocks
        intr_priority: 1                 # RMT interrupt priority, if set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)

        # RX channel flags
        flags:
          invert_in: false               # Whether to invert the incoming RMT channel signal
          with_dma: false                # If set, allocate RMT channel with DMA capability
                                          # Please check the macro definition SOC_RMT_SUPPORT_DMA in soc/soc_caps.h
                                          # to determine whether the chip supports
          allow_pd: false                # If set, allow power domain to power off during sleep (saves power, uses more RAM)

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - ``led_strip``
     - ``sub_type`` 为 ``rmt`` 时使用 RMT 驱动能力
     - 当前 ``led_strip`` 设备模板把 ``rmt`` 参数放在设备侧 ``config.rmt`` 中，不要求引用独立 ``rmt`` 外设
   * - ``custom``
     - 设备侧可引用已定义的 ``rmt`` 外设
     - 编码器、发送队列使用方式和接收缓冲由自定义设备或应用代码决定

参考代码
------------

- ``esp_board_manager/peripherals/periph_rmt/idf5/periph_rmt.c``
- ``esp_board_manager/peripherals/periph_rmt/idf6/periph_rmt.c``
- ``esp_board_manager/test_apps/main/periph/test_periph_rmt.c``

板级参考
------------

- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_s3_devkitc/board_peripherals.yaml``：定义 ``rmt_tx`` 测试外设。
- ``esp_boards/esp32_s31_korvo_1/board_devices.yaml``：``led_strip`` 使用 ``sub_type: rmt``。
- ``esp_boards/esp32_s31_function_coreboard_1/board_devices.yaml``：``led_strip`` 使用 ``sub_type: rmt``。

注意事项
------------

- 同一 RMT group 内 channel 的 ``clk_src`` 需要一致，模板注释中已标出该限制。
- ``with_dma`` 只在目标芯片支持 RMT DMA 时可用。
- ``role`` 决定解析 TX 还是 RX 配置。TX 专用字段不要写入 RX 外设，RX 专用字段不要写入 TX 外设。
- 修改 RMT 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 RMT 外设句柄，句柄类型为 ``periph_rmt_handle_t``：

.. code-block:: c

   typedef struct {
       rmt_channel_handle_t  channel;  /*!< Generic RMT channel handle */
   } periph_rmt_handle_t;

``channel`` 可传入 ``rmt_transmit``、``rmt_receive`` 等 ESP-IDF RMT API；具体可用接口取决于 ``board_peripherals.yaml`` 中 ``role`` 配置为 TX 还是 RX。

相关声明位于 ``esp_board_manager/peripherals/periph_rmt/periph_rmt.h``。

底层 ESP-IDF 驱动文档：`红外遥控 (RMT) <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/rmt.html>`__\ 。
