adc
============

:link_to_translation:`en:[English]`

.. _adc-intro:

简介
------

``adc`` 外设描述 ESP-IDF ADC continuous 或 one-shot 驱动实例。BMGR 按 ``role`` 将 ``board_peripherals.yaml`` 中的 ``adc`` 条目转换为 ``periph_adc_config_t``，并创建 continuous 或 one-shot 句柄。

``adc`` 常用于内部 ADC 音频输入与 ADC 按键。continuous 模式面向持续采样数据流；one-shot 模式面向按需读取单个 ADC channel 的场景。

.. _adc-working-modes:

支持的工作模式
---------------------

``adc`` 按 ``role`` 区分工作模式。continuous 模式内部又支持 ``patterns`` 与 ``channel_list`` 两种配置写法。

- :ref:`continuous：patterns 配置 <adc-continuous-patterns>`
- :ref:`continuous：single unit 配置 <adc-continuous-single-unit>`
- :ref:`oneshot <adc-oneshot>`

.. _adc-min-config:

最小配置
------------

.. _adc-continuous-patterns:

continuous：``patterns`` 配置
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

完整字段见 :ref:`adc-continuous-patterns-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: adc_audio_in
        type: adc
        role: continuous
        config:
          sample_freq_hz: 16000
          patterns:
            - unit: ADC_UNIT_1      # [TO_BE_CONFIRMED]
              channel: 0            # [TO_BE_CONFIRMED]
              atten: ADC_ATTEN_DB_12
              bit_width: ADC_BITWIDTH_DEFAULT

.. _adc-continuous-single-unit:

continuous：single unit 配置
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

完整字段见 :ref:`adc-continuous-single-unit-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: adc_audio_in
        type: adc
        role: continuous
        config:
          sample_freq_hz: 16000
          unit_id: ADC_UNIT_1       # [TO_BE_CONFIRMED]
          channel_list: [0]         # [TO_BE_CONFIRMED]

.. _adc-oneshot:

oneshot
^^^^^^^^^^^

完整字段见 :ref:`adc-oneshot-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: adc_oneshot
        type: adc
        role: oneshot
        config:
          unit_id: ADC_UNIT_1       # [TO_BE_CONFIRMED]
          channel_id: 4             # [TO_BE_CONFIRMED]

.. _adc-mode-notes:

模式说明
------------

continuous 模式创建 ``adc_continuous_handle_t``。``patterns`` 写法可描述完整 ``adc_digi_pattern_config_t`` 列表；``channel_list`` 写法用于同一个 ADC unit 的一个或多个 channel。两种写法不能同时使用。

oneshot 模式创建 ``adc_oneshot_unit_handle_t``，并配置一个 ``channel_id``。ADC 按键设备引用 one-shot 外设；内部 ADC 音频输入需要复用外设时引用 continuous 外设。

ADC channel 和 GPIO 的映射依赖目标芯片。模板中 channel 字段保留 ``[TO_BE_CONFIRMED]``，板级配置应按芯片手册或 ESP-IDF ADC 映射 API 确认。

.. _adc-full-fields:

完整字段
------------

.. _adc-continuous-patterns-full:

continuous：``patterns`` 配置完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # ADC Peripheral Default Configuration
    # This file provides commented default configurations used by the ADC peripheral parser.
    # Based on periph_adc.py parsing script

    # -----------------------------------------------------------------------------
    # CONTINUOUS MODE (role: continuous)
    # -----------------------------------------------------------------------------
    - name: adc_continuous
      type: adc
      role: continuous
      config:
        # Max length of the conversion results that driver can store, in bytes. Default: 1024
        max_store_buf_size: 1024

        # Conversion frame size in bytes. Should be a multiple of 'SOC_ADC_DIGI_DATA_BYTES_PER_CONV' for your target. Default: 256
        conv_frame_size: 256

        # Flush internal pool when full. Default: 1
        flush_pool: 1

        # The expected ADC sampling frequency in Hz.
        # Please check the 'soc/soc_caps.h' for the valid values
        sample_freq_hz: 20000

        # ADC DMA output format. (default: ADC_DIGI_OUTPUT_FORMAT_TYPE2)
        format: ADC_DIGI_OUTPUT_FORMAT_TYPE2
        # Valid values:
        # - ADC_DIGI_OUTPUT_FORMAT_TYPE1
        # - ADC_DIGI_OUTPUT_FORMAT_TYPE2

        # Conversion mode for DMA operation.
        # If omitted, parser auto-derives:
        # - single_unit + unit1 => ADC_CONV_SINGLE_UNIT_1
        # - single_unit + unit2 => ADC_CONV_SINGLE_UNIT_2
        # - pattern mixed unit  => ADC_CONV_BOTH_UNIT
        conv_mode: ADC_CONV_SINGLE_UNIT_1
        # Valid values:
        # - ADC_CONV_SINGLE_UNIT_1
        # - ADC_CONV_SINGLE_UNIT_2
        # - ADC_CONV_BOTH_UNIT
        # - ADC_CONV_ALTER_UNIT

        # ADC pattern configuration
        patterns:
          - unit: ADC_UNIT_1  # [TO_BE_CONFIRMED] ADC unit, optional values are ADC_UNIT_1 and ADC_UNIT_2
            channel: 4        # [TO_BE_CONFIRMED] ADC channel used by this audio input path
                              # Use adc_continuous_io_to_channel() / adc_continuous_channel_to_io() to map IO to channel.
            # ADC attenuation. Different parameters determine the input range. (default: ADC_ATTEN_DB_0)
            # Valid values:
            # - ADC_ATTEN_DB_0
            # - ADC_ATTEN_DB_2_5
            # - ADC_ATTEN_DB_6
            # - ADC_ATTEN_DB_12
            # - ADC_ATTEN_DB_11 (deprecated on some targets)
            atten: ADC_ATTEN_DB_0
            # ADC raw output bit-width.
            # Valid values:
            # - ADC_BITWIDTH_9
            # - ADC_BITWIDTH_10
            # - ADC_BITWIDTH_11
            # - ADC_BITWIDTH_12
            # - ADC_BITWIDTH_13
            # - ADC_BITWIDTH_DEFAULT
            # - SOC_ADC_DIGI_MAX_BITWIDTH
            # - positive integer supported by the target ADC driver
            bit_width: SOC_ADC_DIGI_MAX_BITWIDTH

          # Example of a second pattern for another ADC unit (if supported by the target)
          - unit: ADC_UNIT_2
            channel: 0
            atten: ADC_ATTEN_DB_12
            bit_width: ADC_BITWIDTH_DEFAULT

.. _adc-continuous-single-unit-full:

continuous：single unit 配置完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Single Unit configuration mode, simpler mode for single unit ADC path
    - name: adc_continuous
      type: adc
      role: continuous
      config:
        max_store_buf_size: 1024
        conv_frame_size: 256
        flush_pool: 1
        sample_freq_hz: 20000
        format: ADC_DIGI_OUTPUT_FORMAT_TYPE2
        conv_mode: ADC_CONV_SINGLE_UNIT_1

        # Below are the single unit configuration parameters, alternative to patterns[]
        unit_id: ADC_UNIT_1  # [TO_BE_CONFIRMED] ADC unit, optional values are ADC_UNIT_1 and ADC_UNIT_2
        channel_list: [4]    # [TO_BE_CONFIRMED] List of ADC channels used by this audio input path.
        atten: ADC_ATTEN_DB_0
        # ADC raw output bit-width. Use an enum, SoC macro, or positive integer supported by the target ADC driver.
        bit_width: SOC_ADC_DIGI_MAX_BITWIDTH

.. _adc-oneshot-full:

oneshot 完整字段
^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # -----------------------------------------------------------------------------
    # ONE-SHOT MODE (role: oneshot)
    # -----------------------------------------------------------------------------
    - name: adc_oneshot
      type: adc
      role: oneshot
      config:
        # ADC unit, optional values are ADC_UNIT_1 and ADC_UNIT_2, should be less than 'SOC_ADC_PERIPH_NUM'
        # please check the 'SOC_ADC_PERIPH_NUM' in 'soc/soc_caps.h' for the valid values
        unit_id: ADC_UNIT_1  # [TO_BE_CONFIRMED] ADC unit ID

        # ADC attenuation. Different parameters determine the range of the ADC. (default: ADC_ATTEN_DB_0)
        atten: ADC_ATTEN_DB_0
        # Valid values:
        # - ADC_ATTEN_DB_0
        # - ADC_ATTEN_DB_2_5
        # - ADC_ATTEN_DB_6
        # - ADC_ATTEN_DB_12
        # - ADC_ATTEN_DB_11 (deprecated)

        # ADC raw output bitwidth. (default: ADC_BITWIDTH_DEFAULT)
        bit_width: ADC_BITWIDTH_DEFAULT
        # Valid values:
        # - ADC_BITWIDTH_9
        # - ADC_BITWIDTH_10
        # - ADC_BITWIDTH_11
        # - ADC_BITWIDTH_12
        # - ADC_BITWIDTH_13
        # - ADC_BITWIDTH_DEFAULT (Default ADC output bits, max supported width will be selected)
        # - SOC_ADC_DIGI_MAX_BITWIDTH
        # - positive integer supported by the target ADC driver, for example 17

        # ADC channel to be used.
        # Use adc_oneshot_io_to_channel() and adc_oneshot_channel_to_io() to get the corresponding relationship between ADC channels and ADC IO.
        channel_id: 4                      # [TO_BE_CONFIRMED] ADC channel ID

        # Clock source for ADC module.
        # Please refer to soc/clk_tree_defs.h, see 'soc_periph_adc_digi_clk_src_t' for valid values
        clk_src: ADC_RTC_CLK_SRC_DEFAULT

        # ULP mode selection. (default: ADC_ULP_MODE_DISABLE)
        ulp_mode: ADC_ULP_MODE_DISABLE
        # Valid values:
        # - ADC_ULP_MODE_DISABLE
        # - ADC_ULP_MODE_FSM
        # - ADC_ULP_MODE_RISCV
        # - ADC_ULP_MODE_LP_CORE

.. _adc-devices:

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - ``audio_codec``
     - 内部 ADC 音频输入复用 ``role: continuous`` 的 ADC 外设
     - ``audio_codec`` 也支持设备本地 ``adc_local_cfg``，该路径不需要声明可复用 ADC 外设
   * - ``button``
     - ``sub_type: adc_single`` 或 ``sub_type: adc_multi`` 引用 ``role: oneshot`` 的 ADC 外设
     - 按键电压范围、按钮数量和事件配置写在 ``button`` 设备中

.. _adc-code:

参考代码
------------

- ``esp_board_manager/peripherals/periph_adc/periph_adc.c``
- ``esp_board_manager/peripherals/periph_adc/periph_adc.h``
- ``esp_board_manager/examples/record_to_sdcard/main/record_to_sdcard.c``
- ``esp_board_manager/examples/record_and_play/main/record_and_play.c``

.. _adc-boards:

板级参考
------------

- ``esp_boards/esp32_c3_lyra/board_peripherals.yaml``：内部 ADC 音频输入使用 continuous 外设。
- ``esp_friends_boards/esp32_s3_korvo_2l/board_peripherals.yaml``：ADC button 使用 one-shot 外设。
- ``esp_boards/esp32_s3_korvo_2_3/board_peripherals.yaml``：ADC button 使用 one-shot 外设。
- ``esp_boards/esp32_lyrat_mini_1_1/board_peripherals.yaml``：ADC button 使用 one-shot 外设。

.. _adc-notes:

注意事项
------------

- 公共 YAML 字段规则见 :doc:`/programming-guide/yaml-rules`。
- continuous 模式必须配置 ``patterns`` 或 ``channel_list`` 之一，不能同时配置。
- ``pattern_num`` 若显式配置，必须等于 ``patterns`` 或 ``channel_list`` 的有效条目数。
- ``conv_mode`` 需要和 ADC unit 组合匹配。单一 ``ADC_UNIT_1`` 使用 ``ADC_CONV_SINGLE_UNIT_1``，单一 ``ADC_UNIT_2`` 使用 ``ADC_CONV_SINGLE_UNIT_2``，混合 unit 不能使用 single-unit conversion mode。
- ADC channel 到 GPIO 的映射按目标芯片确定；修改 ADC 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

.. _adc-debug:

调试技巧
------------

.. _adc-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 ADC 外设句柄，句柄类型为 ``periph_adc_handle_t``：

.. code-block:: c

   typedef union {
       adc_continuous_handle_t    continuous;  /*!< Continuous-mode handle */
       adc_oneshot_unit_handle_t  oneshot;     /*!< One-shot unit handle */
   } periph_adc_handle_t;

``continuous`` 在连续采样模式下有效，``oneshot`` 在单次采样模式下有效；两者互斥，取决于 ``board_peripherals.yaml`` 中配置的 ``role`` 字段。

相关声明位于 ``esp_board_manager/peripherals/periph_adc/periph_adc.h``。

底层 ESP-IDF 驱动文档：`ADC 单次转换模式 <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/adc_oneshot.html>`__\ 、`ADC 连续转换模式 <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/adc_continuous.html>`__\ 。
