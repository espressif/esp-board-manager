uart
============

:link_to_translation:`en:[English]`

.. _uart-intro:

简介
------

``uart`` 外设描述 ESP-IDF UART 驱动实例。BMGR 将 ``board_peripherals.yaml`` 中的 ``uart`` 条目转换为 ``periph_uart_config_t``，初始化时安装 UART driver、配置串口参数、设置 TX/RX/RTS/CTS 管脚，并按需设置 UART 工作模式。

``uart`` 适用于需由板级配置统一声明的串口资源。当前仓库未发现已有 board 配置引用 ``uart`` 外设的设备示例，因此本页仅列出外设自身的可配置工作模式与源码入口。

.. _uart-working-modes:

支持的工作模式
---------------------

``uart`` 按 ESP-IDF ``uart_mode_t`` 与是否启用事件队列区分配置。模板中的 ``uart_config`` 字段描述串口帧格式与时钟源。

- :ref:`UART 基本模式 <uart-basic>`
- :ref:`RS485 / IrDA 等 UART mode <uart-alt-mode>`
- :ref:`启用事件队列 <uart-queue>`

.. _uart-min-config:

最小配置
------------

.. _uart-basic:

UART 基本模式
^^^^^^^^^^^^^^^^^

完整字段见 :ref:`uart-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: uart_1
        type: uart
        config:
          uart_num: 1
          tx_io_num: 17     # [IO]
          rx_io_num: 18     # [IO]
          uart_config:
            baud_rate: 115200

.. _uart-alt-mode:

RS485 / IrDA 等 UART mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

完整字段见 :ref:`uart-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: uart_rs485
        type: uart
        config:
          uart_num: 1
          tx_io_num: 17     # [IO]
          rx_io_num: 18     # [IO]
          rts_io_num: 19    # [IO]
          uart_mode: UART_MODE_RS485_HALF_DUPLEX
          uart_config:
            baud_rate: 115200

.. _uart-queue:

启用事件队列
^^^^^^^^^^^^^^^^^^

完整字段见 :ref:`uart-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: uart_1
        type: uart
        config:
          uart_num: 1
          tx_io_num: 17     # [IO]
          rx_io_num: 18     # [IO]
          use_queue: true
          queue_size: 10
          uart_config:
            baud_rate: 115200

.. _uart-mode-notes:

模式说明
------------

基本模式使用 ``UART_MODE_UART``。RS485、IrDA 和其他 UART mode 通过 ``uart_mode`` 选择，BMGR 初始化时仅在该字段不是 ``UART_MODE_UART`` 时调用 ``uart_set_mode``。

``use_queue`` 控制 ``uart_driver_install`` 是否创建事件队列。启用队列后，返回的 ``periph_uart_handle_t`` 中包含 ``uart_queue``；不启用队列时该成员不用于接收事件。

``source_clk`` 和 ``lp_source_clk`` 是 union 关系，不能同时配置。使用 LP UART 时，端口和 GPIO 选择需要符合目标芯片 LP UART 能力。

.. _uart-full-fields:

完整字段
------------

.. _uart-full:

UART 基本模式 / 扩展 mode / 事件队列完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # UART Peripheral Configuration
    # This section configures UART peripheral settings

    - name: uart_1
      type: uart
      config:
        # UART port number, valid values depend on SOC
        # Can be configured as UART_NUM_0, UART_NUM_1, UART_NUM_2, ... or 0,1,2,... directly
        # Valid values depend on SOC, please check soc/soc_caps.h
        uart_num: 1

        # RX buffer size in bytes (default: 256)
        rx_buffer_size: 256

        # TX buffer size in bytes (default: 256)
        tx_buffer_size: 256

        # GPIO number for TX pin (default: -1 for not used)
        tx_io_num: -1                     # [IO] GPIO number for TX pin

        # GPIO number for RX pin (default: -1 for not used)
        rx_io_num: -1                     # [IO] GPIO number for RX pin

        # GPIO number for RTS pin (default: -1 for not used)
        rts_io_num: -1                    # [IO] GPIO number for RTS pin

        # GPIO number for CTS pin (default: -1 for not used)
        cts_io_num: -1                    # [IO] GPIO number for CTS pin

        # UART operating mode (default: UART_MODE_UART)
        uart_mode: UART_MODE_UART
        # Valid values:
        # - UART_MODE_UART
        # - UART_MODE_RS485_HALF_DUPLEX
        # - UART_MODE_IRDA
        # - UART_MODE_RS485_COLLISION_DETECT
        # - UART_MODE_RS485_APP_CTRL

        # Flag to enable queue for UART events (default: false)
        use_queue: false

        # Size of the event queue, only used when use_queue is true (default: 10)
        queue_size: 10

        # Interrupt allocation flags (default: 0)
        intr_alloc_flags: 0

        # UART configuration parameters
        uart_config:
          # Baud rate in bits per second (default: 115200)
          baud_rate: 115200               # [TO_BE_CONFIRMED] Baud rate in bits per second

          # Number of data bits (default: UART_DATA_8_BITS)
          data_bits: UART_DATA_8_BITS
          # Valid values:
          # - UART_DATA_5_BITS
          # - UART_DATA_6_BITS
          # - UART_DATA_7_BITS
          # - UART_DATA_8_BITS

          # Parity setting (default: UART_PARITY_DISABLE)
          parity: UART_PARITY_DISABLE
          # Valid values:
          # - UART_PARITY_DISABLE
          # - UART_PARITY_EVEN
          # - UART_PARITY_ODD

          # Number of stop bits (default: UART_STOP_BITS_1)
          stop_bits: UART_STOP_BITS_1
          # Valid values:
          # - UART_STOP_BITS_1
          # - UART_STOP_BITS_1_5
          # - UART_STOP_BITS_2

          # Flow control mode (default: UART_HW_FLOWCTRL_DISABLE)
          flow_ctrl: UART_HW_FLOWCTRL_DISABLE
          # Valid values:
          # - UART_HW_FLOWCTRL_DISABLE
          # - UART_HW_FLOWCTRL_RTS
          # - UART_HW_FLOWCTRL_CTS
          # - UART_HW_FLOWCTRL_CTS_RTS

          # RX flow control threshold (default: 0)
          # This is a optional configuration, used to control the hardware flow control threshold.
          # Should be used together with the configuration flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS
          rx_flow_ctrl_thresh: 0

          # Source clock for UART (default: UART_SCLK_DEFAULT)
          # Valid values depend on SOC, please check soc/clk_tree_defs.h
          source_clk: UART_SCLK_DEFAULT

          # Source clock for LP UART (default: not used)
          # This is a configuration related to the SOC, which requires the chip to have an LP UART controller
          # You can check the ESP-IDF programming Guide for more details, or check
          # the macro definition of SOC_UART_LP_NUM in the file soc/soc_caps.h to see if the chip supports LP UART directly
          # If supported, you need to set the uart_num to a specific port, or directly set uart_num to LP_UART_NUM_0
          # And the GPIO pins of the LP UART controller can only be selected from the LP GPIO pins
          # This configuration and source_clk are mutually exclusive; please do not configure both options at the same time
          # Valid values depend on SOC, please check soc/clk_tree_defs.h
          lp_source_clk: NULL

          flags:
            # If set, driver allows the power domain to be powered off when system enters sleep mode.
            # This can save power, but at the expense of more RAM being consumed to save register context.
            allow_pd: 0

            # @deprecated, same meaning as allow_pd
            backup_before_sleep: 0

.. _uart-devices:

适用设备
------------

当前仓库未发现已有 device YAML 模板要求引用 ``uart`` 外设。自定义设备可以通过 ``custom`` 设备或板级代码获取 UART 外设句柄后使用。

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - ``custom``
     - 设备可在自定义初始化代码中获取 ``uart`` 外设句柄
     - UART 端口、管脚和串口参数写在 ``uart`` 外设 ``config`` 中

.. _uart-code:

参考代码
------------

- ``esp_board_manager/peripherals/periph_uart/periph_uart.c``
- ``esp_board_manager/peripherals/periph_uart/periph_uart.h``

.. _uart-boards:

板级参考
------------

.. _uart-notes:

注意事项
------------

- 公共 YAML 字段规则见 :doc:`/programming-guide/yaml-rules`。
- ``uart_num``、可用 UART 数量、LP UART 能力和可选时钟源取决于目标芯片。
- ``source_clk`` 和 ``lp_source_clk`` 不能同时配置。
- RTS/CTS 管脚为 ``-1`` 时表示不使用对应硬件流控管脚；若开启硬件流控，需要同步配置 ``flow_ctrl``。
- 修改 UART 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

.. _uart-debug:

调试技巧
------------

.. _uart-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 UART 外设句柄，句柄类型为 ``periph_uart_handle_t``：

.. code-block:: c

   typedef struct {
       uart_port_t    uart_num;    /*!< UART port number */
       QueueHandle_t  uart_queue;  /*!< Queue handle for UART events if set use_queue */
   } periph_uart_handle_t;

``uart_num`` 可传入 ``uart_read_bytes``、``uart_write_bytes`` 等 ESP-IDF UART API；若 ``board_peripherals.yaml`` 中启用了 ``use_queue``，则 ``uart_queue`` 有效，可用于监听串口事件。

相关声明位于 ``esp_board_manager/peripherals/periph_uart/periph_uart.h``。

底层 ESP-IDF 驱动文档：`通用异步接收器/发送器 (UART) <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/uart.html>`__\ 。
