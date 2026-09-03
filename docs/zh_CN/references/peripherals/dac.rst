dac
=======

:link_to_translation:`en:[English]`

.. _dac-intro:

简介
------

``dac`` 外设类型用于描述片上 DAC 通道。BMGR 据此生成 ``periph_dac_config_t``，并在初始化阶段创建 ESP-IDF DAC 驱动句柄。该类型覆盖 ``driver/dac_oneshot.h``、``driver/dac_continuous.h`` 与 ``driver/dac_cosine.h`` 中的三类 DAC 驱动入口。

在 ``board_peripherals.yaml`` 中配置 ``dac`` 时，需通过 ``role`` 选择工作模式。BMGR 初始化后，应用通过 :cpp:func:`esp_board_manager_get_periph_handle` 获取 ``periph_dac_handle_t``，再调用对应的 ESP-IDF DAC API 输出电压、电压序列或余弦波。

.. _dac-working-modes:

支持的工作模式
---------------------

``dac`` 的分类轴是 ``role``。每个外设实例仅能选择一种 ``role``，运行时句柄也仅对应该模式。

- `oneshot`_
- `continuous`_
- `cosine`_

.. _dac-min-config:

最小配置
------------

.. _dac-oneshot:

oneshot
^^^^^^^^^^^

完整字段见 :ref:`dac-oneshot-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: dac_channel_0
        type: dac
        role: oneshot
        config:
          channel: 0

.. _dac-continuous:

continuous
^^^^^^^^^^^^^^

完整字段见 :ref:`dac-continuous-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: dac_continuous
        type: dac
        role: continuous
        config:
          chan_mask: DAC_CHANNEL_MASK_CH0
          desc_num: 8
          buf_size: 2048
          freq_hz: 1000000
          offset: 0
          clk_src: DAC_DIGI_CLK_SRC_DEFAULT
          chan_mode: DAC_CHANNEL_MODE_SIMUL

.. _dac-cosine:

cosine
^^^^^^^^^^

完整字段见 :ref:`dac-cosine-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: dac_cosine
        type: dac
        role: cosine
        config:
          channel: 0
          freq_hz: 1000
          clk_src: DAC_COSINE_CLK_SRC_DEFAULT
          atten: DAC_COSINE_ATTEN_DEFAULT
          phase: DAC_COSINE_PHASE_0
          offset: 0
          force_set_freq: false

.. _dac-mode-notes:

模式说明
------------

``oneshot`` 创建单个 DAC 通道，用于应用主动调用 ``dac_oneshot_output_voltage`` 输出离散电压值。``continuous`` 创建连续模式通道，适合应用启用通道后写入循环或流式数据。``cosine`` 创建余弦波发生器通道，适合由 DAC 驱动输出指定频率的余弦波。

BMGR 按 ``role`` 填充 ``periph_dac_config_t`` 的联合体成员。应用获取句柄后，需要根据配置中的 ``role`` 访问 ``periph_dac_handle_t`` 中对应的 ``oneshot``、``continuous`` 或 ``cosine`` 成员。

.. _dac-full-fields:

完整字段
------------

.. _dac-oneshot-full:

oneshot 完整字段
^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # DAC oneshot mode example
    - name: dac_oneshot
      type: dac
      role: oneshot
      config:
        # DAC channel number (default: 0)
        channel: 0                       # [TO_BE_CONFIRMED] DAC channel number
        # Valid values: 0, 1
        # Channel 0: GPIO25 on ESP32, GPIO17 on ESP32S2
        # Channel 1: GPIO26 on ESP32, GPIO18 on ESP32S2

.. _dac-continuous-full:

continuous 完整字段
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # DAC continuous mode example
    - name: dac_continuous
      type: dac
      role: continuous
      config:
        # DAC channels' mask (default: DAC_CHANNEL_MASK_CH0)
        chan_mask: DAC_CHANNEL_MASK_CH0  # [TO_BE_CONFIRMED] DAC channels' mask
        # Valid values:
        # - DAC_CHANNEL_MASK_CH0
        # - DAC_CHANNEL_MASK_CH1
        # - DAC_CHANNEL_MASK_ALL

        # Number of DMA descriptors (default: 8)
        desc_num: 8
        # At least 2 descriptors are required, suggest >5

        # DMA buffer size (default: 2048)
        buf_size: 2048
        # Should be within 32~4092 bytes, typically multiple of 4

        # DAC conversion frequency (default: 1000000 Hz)
        freq_hz: 1000000                 # [TO_BE_CONFIRMED] DAC conversion frequency
        # Range depends on target and clock source

        # Data offset (default: 0)
        offset: 0
        # Range: -128~127

        # Clock source (default: DAC_DIGI_CLK_SRC_DEFAULT)
        # Valid values depend on the selected chip,
        # please refer to the enum 'soc_periph_dac_digi_clk_src_t' in 'soc/clk_tree_defs.h'
        clk_src: DAC_DIGI_CLK_SRC_DEFAULT

        # Assume the data in buffer is 'A B C D E F'
        # DAC_CHANNEL_MODE_SIMUL:
        #     channel 0: A B C D E F
        #     channel 1: A B C D E F
        # DAC_CHANNEL_MODE_ALTER:
        #     channel 0: A C E
        #     channel 1: B D F
        # Channel mode (default: DAC_CHANNEL_MODE_SIMUL)
        chan_mode: DAC_CHANNEL_MODE_SIMUL
        # Valid values:
        # - DAC_CHANNEL_MODE_SIMUL
        # - DAC_CHANNEL_MODE_ALTER

.. _dac-cosine-full:

cosine 完整字段
^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # DAC cosine mode example
    - name: dac_cosine
      type: dac
      role: cosine
      config:
        # DAC channel number (default: 0)
        channel: 0                       # [TO_BE_CONFIRMED] DAC channel number
        # Valid values: 0, 1
        # On ESP32: Channel 0: GPIO25, Channel 1: GPIO26
        # On ESP32S2: Channel 0: GPIO17, Channel 1: GPIO18

        # Cosine wave frequency (default: 1000 Hz)
        freq_hz: 1000                    # [TO_BE_CONFIRMED] Cosine wave frequency

        # Clock source (default: DAC_COSINE_CLK_SRC_DEFAULT)
        # Valid values depend on the selected chip,
        # please refer to the enum 'soc_periph_dac_cosine_clk_src_t' in 'soc/clk_tree_defs.h'
        clk_src: DAC_COSINE_CLK_SRC_DEFAULT

        # Attenuation (default: DAC_COSINE_ATTEN_DEFAULT)
        atten: DAC_COSINE_ATTEN_DEFAULT
        # Valid values:
        # - DAC_COSINE_ATTEN_DEFAULT
        # - DAC_COSINE_ATTEN_DB_0
        # - DAC_COSINE_ATTEN_DB_6
        # - DAC_COSINE_ATTEN_DB_12
        # - DAC_COSINE_ATTEN_DB_18

        # Phase (default: DAC_COSINE_PHASE_0)
        phase: DAC_COSINE_PHASE_0
        # Valid values:
        # - DAC_COSINE_PHASE_0
        # - DAC_COSINE_PHASE_180

        # DC offset (default: 0)
        offset: 0
        # Range: -128~127

        # Force set frequency (default: false)
        # Set true to force update frequency when multiple channels are used
        force_set_freq: false

字段来源：

- YAML 模板：``esp_board_manager/peripherals/periph_dac/periph_dac.yml``。
- 头文件：``esp_board_manager/peripherals/periph_dac/periph_dac.h``。

.. _dac-devices:

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - 直接由应用使用
     - 应用通过 ``esp_board_manager_get_periph_handle`` 获取 DAC 外设句柄
     - 当前仓库未提供引用 ``dac`` 外设的 device 类型；输出数据和启停动作由应用或测试代码完成

.. _dac-code:

参考代码
------------

- ``esp_board_manager/test_apps/main/periph/test_periph_dac.c``

.. _dac-boards:

板级参考
------------

- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_devkitc/board_peripherals.yaml``：``oneshot`` 示例和注释形式的 ``continuous`` / ``cosine`` 示例。

.. _dac-notes:

注意事项
------------

- ``role`` 必须是 ``oneshot``、``continuous`` 或 ``cosine``；缺少或写成其他值会导致 YAML 解析失败。
- ``channel`` 只接受 ``0`` 或 ``1``，BMGR parser 会映射为 ``DAC_CHAN_0`` 或 ``DAC_CHAN_1``。
- ``continuous`` 的 ``chan_mask`` 只接受 ``DAC_CHANNEL_MASK_CH0``、``DAC_CHANNEL_MASK_CH1`` 或 ``DAC_CHANNEL_MASK_ALL``。
- ``cosine`` 的 ``atten`` 和 ``phase`` 需要使用模板列出的枚举值。
- 修改 DAC 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

.. _dac-debug:

调试技巧
------------

.. _dac-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 DAC 外设句柄，句柄类型为 ``periph_dac_handle_t``：

.. code-block:: c

   typedef union {
       dac_oneshot_handle_t     oneshot;     /*!< Oneshot mode handle */
       dac_continuous_handle_t  continuous;  /*!< Continuous mode handle */
       dac_cosine_handle_t      cosine;      /*!< Cosine wave mode handle */
   } periph_dac_handle_t;

三个成员互斥，具体哪个有效取决于 ``board_peripherals.yaml`` 中配置的 ``role`` 字段（``oneshot``、``continuous`` 或 ``cosine``）。

相关声明位于 ``esp_board_manager/peripherals/periph_dac/periph_dac.h``。

底层 ESP-IDF 驱动文档：`数模转换器 (DAC) <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32/api-reference/peripherals/dac.html>`__\ 。
