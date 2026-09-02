i2c
============

:link_to_translation:`en:[English]`

.. _i2c-intro:

简介
------

``i2c`` 外设描述 ESP-IDF I2C master bus。BMGR 将 ``board_peripherals.yaml`` 中的 ``i2c`` 条目转换为 ``i2c_master_bus_config_t``，并在初始化时创建 I2C master bus 句柄。

``i2c`` 常用于外接音频 codec 控制、LCD touch、IO expander、摄像头 SCCB 控制等设备。设备侧通过外设实例名引用同一条 I2C 总线，并在设备 ``peripherals`` 引用条目中填写器件地址等设备私有参数。

.. _i2c-working-modes:

支持的工作模式
---------------------

``i2c`` 按控制器端口族区分配置方式。当前外设初始化入口仅创建 master bus，不提供 slave 设备模式。

- :ref:`HP I2C master <i2c-hp-master>`
- :ref:`LP I2C master <i2c-lp-master>`

.. _i2c-min-config:

最小配置
------------

.. _i2c-hp-master:

HP I2C master
^^^^^^^^^^^^^^^^^

完整字段见 :ref:`i2c-full`。

``board_peripherals.yaml``：

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

LP I2C master
^^^^^^^^^^^^^^^^^

完整字段见 :ref:`i2c-full`。

``board_peripherals.yaml``：

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

.. _i2c-mode-notes:

模式说明
------------

HP I2C 使用 ``I2C_NUM_0`` 或 ``I2C_NUM_1`` 等端口宏，或对应的数字索引。``port: -1`` 表示由 ESP-IDF 自动选择 HP I2C 端口，不可与 LP I2C 时钟源组合。

LP I2C 需显式使用 ``LP_I2C_NUM_0`` 一类端口宏，并使用 ``LP_I2C_SCLK_*`` 时钟源。LP I2C 的 GPIO 选择受目标芯片 LP GPIO 能力限制。

I2C 器件地址不写入外设 ``config``：音频 codec 使用 ``address``，LCD touch 与 IO expander 使用 ``i2c_addr``，这些字段属于设备侧引用条目。

.. _i2c-full-fields:

完整字段
------------

.. _i2c-full:

HP I2C master 完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^

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

.. _i2c-devices:

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - ``audio_codec``
     - 设备 ``peripherals`` 引用 ``i2c``，并填写 ``address`` 和 ``frequency``
     - 外接 codec 控制接口；I2C 地址写在设备侧
   * - ``lcd_touch``
     - 设备 ``peripherals`` 引用 ``i2c``，并填写 ``i2c_addr``
     - 触摸控制器通信；触摸坐标和复位/中断 GPIO 属于设备侧配置
   * - ``gpio_expander``
     - 设备 ``peripherals`` 引用 ``i2c``，并填写 ``i2c_addr``
     - IO 扩展芯片控制；扩展 IO 的方向和默认电平属于设备侧配置
   * - ``camera``
     - DVP / SPI 摄像头可引用 ``i2c`` 作为 SCCB 控制总线
     - 摄像头数据总线或 SPI 数据线不写进 ``i2c`` 外设

.. _i2c-code:

参考代码
------------

- ``esp_board_manager/peripherals/periph_i2c/periph_i2c.c``
- ``esp_board_manager/peripherals/periph_i2c/periph_i2c.h``
- ``esp_board_manager/examples/play_embed_music/main/play_embed_music.c``
- ``esp_board_manager/examples/record_to_sdcard/main/record_to_sdcard.c``

.. _i2c-boards:

板级参考
------------

- ``esp_friends_boards/esp32_s3_korvo_2l/board_peripherals.yaml``：音频 codec 使用的 I2C 控制总线。
- ``esp_boards/esp32_s3_lcd_ev_board/board_peripherals.yaml``：音频、触摸和 IO expander 共用 I2C 总线。
- ``m5stack_boards/m5stack_tab5/board_peripherals.yaml``：音频、触摸和多个 IO expander 使用 I2C 总线。

.. _i2c-notes:

注意事项
------------

- 公共 YAML 字段规则见 :doc:`/programming-guide/yaml-rules`。
- ``clk_source`` 必须与端口族匹配。LP I2C 端口不可使用 ``I2C_CLK_SRC_*``，HP I2C 端口不可使用 ``LP_I2C_SCLK_*``。
- 设备地址、I2C 传输频率、触摸控制参数、codec 控制参数属于设备侧配置，不写入 ``i2c`` 外设的 ``config``。
- 修改 I2C 外设配置后，需重新执行 ``idf.py bmgr -b <board>``。

.. _i2c-debug:

调试技巧
------------

.. _i2c-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 I2C 外设句柄，句柄类型为 ESP-IDF 原生的 ``i2c_master_bus_handle_t``，可直接传入 ``i2c_master_*`` 系列 API 或用于挂载 I2C 设备。

相关声明位于 ``esp_board_manager/peripherals/periph_i2c/periph_i2c.h``。

底层 ESP-IDF 驱动文档：`I2C 接口 <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/i2c.html>`__\ 。
