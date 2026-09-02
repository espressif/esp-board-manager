spi
============

:link_to_translation:`en:[English]`

.. _spi-intro:

简介
------

``spi`` 外设描述 ESP-IDF SPI master bus。BMGR 将 ``board_peripherals.yaml`` 中的 ``spi`` 条目转换为 ``periph_spi_config_t``，并在初始化时调用 ``spi_bus_initialize`` 创建 SPI 总线。

``spi`` 常用于 SPI LCD、部分 SPI 摄像头或其他需要共享 SPI host 的设备。片选、D/C、LCD 像素时钟、设备 SPI mode 等设备私有参数写在设备侧配置中；外设侧仅描述 SPI bus 的 host、数据线与传输能力。

.. _spi-working-modes:

支持的工作模式
---------------------

``spi`` 按总线数据线格式区分配置方式。BMGR 的 SPI 外设初始化入口仅创建 SPI master bus，不创建具体 SPI device。

- :ref:`标准 SPI bus <spi-standard-bus>`
- :ref:`Quad / Octal SPI bus <spi-wide-bus>`

.. _spi-min-config:

最小配置
------------

.. _spi-standard-bus:

标准 SPI bus
^^^^^^^^^^^^^^

完整字段见 :ref:`spi-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: spi_display
        type: spi
        config:
          spi_bus_config:
            spi_port: 1
            mosi_io_num: 11     # [IO]
            miso_io_num: -1     # [IO]
            sclk_io_num: 12     # [IO]

.. _spi-wide-bus:

Quad / Octal SPI bus
^^^^^^^^^^^^^^^^^^^^^^^^

完整字段见 :ref:`spi-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: spi_display
        type: spi
        config:
          spi_bus_config:
            spi_port: SPI2_HOST
            data0_io_num: 11    # [IO]
            data1_io_num: 13    # [IO]
            sclk_io_num: 12     # [IO]
            data2_io_num: 14    # [IO]
            data3_io_num: 15    # [IO]

.. _spi-mode-notes:

模式说明
------------

标准 SPI bus 使用 ``mosi_io_num``、``miso_io_num`` 和 ``sclk_io_num``。只写显示输出时，``miso_io_num`` 可以保持 ``-1``。

Quad / Octal SPI bus 使用 ESP-IDF ``spi_bus_config_t`` 的 data 线字段。``data0_io_num`` 与 ``mosi_io_num`` 是同一 union 字段，``data1_io_num`` 与 ``miso_io_num`` 是同一 union 字段，``data2_io_num`` 与 ``quadwp_io_num`` 是同一 union 字段，``data3_io_num`` 与 ``quadhd_io_num`` 是同一 union 字段，同一组 union 字段不能同时填写。

SPI LCD 的 ``cs_gpio_num``、``dc_gpio_num``、``spi_mode``、``pclk_hz`` 和 transaction queue 深度属于 ``display_lcd`` 设备侧 ``io_spi_config``，不写进 ``spi`` 外设 ``config``。

.. _spi-full-fields:

完整字段
------------

.. _spi-full:

标准 SPI bus / Quad / Octal SPI bus 完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # SPI Peripheral Default Configuration
    # This file shows the default values used by the SPI peripheral parser
    # Based on periph_spi.py parsing script

    - name: spi_master
      type: spi
      config:
        # SPI bus configuration
        spi_bus_config:
          # SPI port (default: 0, maps to SPI1_HOST)
          spi_port: 0
          # Valid values: 0 (SPI1_HOST), 1 (SPI2_HOST), 2 (SPI3_HOST)
          # Or use string: "SPI1_HOST", "SPI2_HOST", "SPI3_HOST"

          # Standard SPI pins (union fields - use either dataX_io_num or traditional names)
          # MOSI pin (default: -1, not set)
          mosi_io_num: -1                     # [IO] MOSI pin
          # Alternative: data0_io_num: -1

          # MISO pin (default: -1, not set)
          miso_io_num: -1                     # [IO] MISO pin
          # Alternative: data1_io_num: -1

          # SCLK pin (default: -1, not set)
          sclk_io_num: -1                     # [IO] SCLK pin

          # Quad SPI pins (union fields)
          # Quad Write Protect pin (default: -1, not set)
          quadwp_io_num: -1                   # [IO] Quad Write Protect pin
          # Alternative: data2_io_num: -1

          # Quad Hold pin (default: -1, not set)
          quadhd_io_num: -1                   # [IO] Quad Hold pin
          # Alternative: data3_io_num: -1

          # Octal mode pins (optional)
          # Data pin 4 (default: -1, not set)
          data4_io_num: -1                    # [IO] Data pin 4
          # Data pin 5 (default: -1, not set)
          data5_io_num: -1                    # [IO] Data pin 5
          # Data pin 6 (default: -1, not set)
          data6_io_num: -1                    # [IO] Data pin 6
          # Data pin 7 (default: -1, not set)
          data7_io_num: -1                    # [IO] Data pin 7

          # Data IO default level (default: false)
          data_io_default_level: false

          # Maximum transfer size (default: 4092)
          max_transfer_sz: 4092

          # DMA burst size in bytes (IDF v6.1+; 0 uses driver default)
          dma_burst_size: 0

          # Flags (default: 0)
          flags: 0

          # ISR CPU affinity (default: ESP_INTR_CPU_AFFINITY_AUTO)
          isr_cpu_id: "ESP_INTR_CPU_AFFINITY_AUTO"
          # Valid values:
          # - ESP_INTR_CPU_AFFINITY_AUTO
          # - ESP_INTR_CPU_AFFINITY_0
          # - ESP_INTR_CPU_AFFINITY_1

          # Interrupt flags (default: 0)
          intr_flags: 0

.. _spi-devices:

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - ``display_lcd``
     - ``sub_type: spi`` 的设备 ``peripherals`` 引用 ``spi``
     - LCD 的 CS、D/C、reset、像素时钟和 panel 参数属于设备侧配置
   * - ``camera``
     - SPI 摄像头设备在设备侧配置 ``spi_config``
     - 当前已有板级配置中 SPI 摄像头参数直接在设备侧描述，不复用 ``spi`` 外设实例

.. _spi-code:

参考代码
------------

- ``esp_board_manager/peripherals/periph_spi/periph_spi.c``
- ``esp_board_manager/peripherals/periph_spi/periph_spi.h``

.. _spi-boards:

板级参考
------------

- ``esp_boards/esp32_s3_box_3/board_peripherals.yaml``：SPI LCD bus 配置。
- ``esp_boards/esp_vocat_1_2/board_peripherals.yaml``：SPI LCD bus 配置。
- ``esp_boards/esp32_s3_korvo_2_3/board_peripherals.yaml``：SPI LCD bus 配置。
- ``m5stack_boards/m5stack_cores3/board_peripherals.yaml``：SPI master bus 配置。

.. _spi-notes:

注意事项
------------

- 公共 YAML 字段规则见 :doc:`/programming-guide/yaml-rules`。
- 数字 ``spi_port`` 会映射为 ``SPI1_HOST``、``SPI2_HOST``、``SPI3_HOST``。目标芯片可用 host 数量以 ESP-IDF SoC 能力为准。
- 同一 union 字段只能选择传统名称或 dataX 名称之一。例如不要同时填写 ``mosi_io_num`` 和 ``data0_io_num``。
- SPI 外设初始化只创建总线；具体设备的片选、命令/数据脚和设备时钟写在对应 device 配置中。
- 修改 SPI 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

.. _spi-debug:

调试技巧
------------

.. _spi-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 SPI 外设句柄，句柄类型为 ``periph_spi_handle_t``：

.. code-block:: c

   typedef struct {
       spi_host_device_t  spi_port;  /*!< SPI port number */
   } periph_spi_handle_t;

``spi_port`` 为 SPI 总线编号，可传入 ``spi_bus_add_device`` 挂载 SPI 设备，或传入 ``esp_lcd_*`` 等使用 SPI 总线的驱动。

相关声明位于 ``esp_board_manager/peripherals/periph_spi/periph_spi.h``。

底层 ESP-IDF 驱动文档：`SPI 主机驱动程序 <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/spi_master.html>`__\ 。
