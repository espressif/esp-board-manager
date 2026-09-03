pcnt
========

:link_to_translation:`en:[English]`

.. _pcnt-intro:

简介
------

``pcnt`` 外设类型用于描述 ESP-IDF pulse counter 单元、glitch filter、channel 与 watch point。BMGR 根据该配置创建 PCNT unit 与 channel，并返回 ``periph_pcnt_handle_t``。

该外设适用于需要直接使用 PCNT 句柄的计数、编码器或测试场景。计数值读取、事件回调与业务层处理由使用该句柄的设备或应用代码完成。

.. _pcnt-working-modes:

支持的工作模式
---------------------

``pcnt`` 当前按单个 PCNT unit 配置，不通过 ``role`` 进一步拆分工作模式。

- `PCNT 计数单元`_

.. _pcnt-min-config:

最小配置
------------

.. _pcnt-unit:

PCNT 计数单元
^^^^^^^^^^^^^^^^^

完整字段见 :ref:`pcnt-unit-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: pcnt_unit
        type: pcnt
        config:
          unit_config:
            low_limit: -100
            high_limit: 100
            intr_priority: 0
            flags:
              accum_count: false
          filter_config:
            max_glitch_ns: 1000
          channel_count: 1
          channel_list:
            - channel: 0
              channel_config:
                edge_gpio_num: 7
                level_gpio_num: -1
                flags:
                  invert_edge_input: false
                  invert_level_input: false
                  virt_edge_io_level: 0
                  virt_level_io_level: 0
                  io_loop_back: false
              pos_act: PCNT_CHANNEL_EDGE_ACTION_INCREASE
              neg_act: PCNT_CHANNEL_EDGE_ACTION_DECREASE
              high_act: PCNT_CHANNEL_LEVEL_ACTION_KEEP
              low_act: PCNT_CHANNEL_LEVEL_ACTION_KEEP
          watch_point_count: 3
          watch_point_list: [-100, 0, 100]

.. _pcnt-mode-notes:

模式说明
------------

``pcnt`` 外设先创建 unit，再设置 glitch filter，然后按 ``channel_count`` 创建 channel 并设置 edge/level action。``watch_point_list`` 用于向 unit 添加 watch point；事件回调不在外设初始化中注册。

.. _pcnt-full-fields:

完整字段
------------

.. _pcnt-unit-full:

PCNT 计数单元完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # PCNT Peripheral Default Configuration
    # This file shows the default values used by the PCNT peripheral parser
    # Based on periph_pcnt.py parsing script

    - name: pcnt_unit
      type: pcnt
      config:
        unit_config:
          low_limit: -100  # [TO_BE_CONFIRMED] Low count limit (should be lower than 0 and should not less than 'PCNT_LL_MIN_LIM')
          high_limit: 100  # [TO_BE_CONFIRMED] High count limit (should be higher than 0 and should not exceed 'PCNT_LL_MAX_LIM')
          intr_priority: 0  # PCNT interrupt priority,
                            # if set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)

          flags:
            accum_count: false  # Whether to accumulate the count value when overflows at the high/low limit

            # The following two flags require chip support,
            # which can be determined by checking the macro definition SOC_PCNT_SUPPORT_STEP_NOTIFY
            # or by referring to the IDF Programming Guide
            en_step_notify_up: false  # Enable step notify in the positive direction
            en_step_notify_down: false  # Enable step notify in the negative direction

        filter_config:
          max_glitch_ns: 1000  # Pulse width smaller than this threshold will be treated as glitch and ignored, in the unit of ns

        channel_count: 2  # [TO_BE_CONFIRMED] Number of PCNT channels to configure, should be less than SOC_PCNT_CHANNELS_PER_UNIT
        # Configuration for each PCNT channel
        channel_list:
          # Channel 0 configuration
          - channel: 0
            channel_config:
              edge_gpio_num: -1   # [IO] GPIO number used by the edge signal, input mode with pull up enabled. Set to -1 if unused
              level_gpio_num: -1  # [IO] GPIO number used by the level signal, input mode with pull up enabled. Set to -1 if unused
              flags:
                invert_edge_input: false  # Invert the input edge signal
                invert_level_input: false  # Invert the input level signal
                virt_edge_io_level: 0  # Virtual edge IO level, 0: low, 1: high. Only valid when edge_gpio_num is set to -1
                virt_level_io_level: 0  # Virtual level IO level, 0: low, 1: high. Only valid when level_gpio_num is set to -1
                io_loop_back: false  # For debug/test, the signal output from the GPIO will be fed to the input path as well.
                                     # Note that this flag is deprecated, will be removed in IDF v6.0.
                                     # Instead, you can configure the output mode by calling gpio_config() first, and then do PCNT channel configuration.
                                     # Necessary configurations for the IO to be used as the PCNT input will be appended.

            # Edge actions for pcnt channel
            pos_act: PCNT_CHANNEL_EDGE_ACTION_DECREASE
            neg_act: PCNT_CHANNEL_EDGE_ACTION_INCREASE
            # Valid values:
            # - PCNT_CHANNEL_EDGE_ACTION_HOLD  Hold current count value
            # - PCNT_CHANNEL_EDGE_ACTION_INCREASE  Increase count value
            # - PCNT_CHANNEL_EDGE_ACTION_DECREASE  Decrease count value

            # Level actions for pcnt channel
            high_act: PCNT_CHANNEL_LEVEL_ACTION_KEEP
            low_act: PCNT_CHANNEL_LEVEL_ACTION_INVERSE
            # Valid values:
            # - PCNT_CHANNEL_LEVEL_ACTION_KEEP  Keep the current count value
            # - PCNT_CHANNEL_LEVEL_ACTION_INVERSE  Invert current count mode (increase -> decrease, decrease -> increase)
            # - PCNT_CHANNEL_LEVEL_ACTION_HOLD  Hold current count value

          # Channel 1 configuration
          - channel: 1
            channel_config:
              edge_gpio_num: 38          # [IO] GPIO number used by the edge signal
              level_gpio_num: 37         # [IO] GPIO number used by the level signal
              flags:
                invert_edge_input: false
                invert_level_input: false
                virt_edge_io_level: false
                virt_level_io_level: false
                io_loop_back: false
            pos_act: PCNT_CHANNEL_EDGE_ACTION_INCREASE
            neg_act: PCNT_CHANNEL_EDGE_ACTION_DECREASE
            high_act: PCNT_CHANNEL_LEVEL_ACTION_KEEP
            low_act: PCNT_CHANNEL_LEVEL_ACTION_INVERSE

        # Watch point configuration
        watch_point_count: 5  # Number of watch points to configure,
                              # In addition to setting 0 and +/- limit as watch points,
                              # users can also customize SOC_PCNT_THRES_POINT_PER_UNIT watch points
        watch_point_list: [-100, -50, 0, 50, 100]

.. _pcnt-devices:

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - ``custom``
     - 设备侧可引用已定义的 ``pcnt`` 外设
     - 计数读取、watch point 回调和业务语义由自定义设备或应用代码完成

.. _pcnt-code:

参考代码
------------

- ``esp_board_manager/peripherals/periph_pcnt/periph_pcnt.c``
- ``esp_board_manager/test_apps/main/periph/test_periph_pcnt.c``

.. _pcnt-boards:

板级参考
------------

- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_s3_devkitc/board_peripherals.yaml``：定义 ``pcnt_unit`` 测试外设。

.. _pcnt-notes:

注意事项
------------

- ``channel_count`` 不能超过目标芯片支持的 ``SOC_PCNT_CHANNELS_PER_UNIT``；BMGR 初始化时会按上限截断并打印 warning。
- ``watch_point_count`` 不能超过目标芯片支持的 watch point 数量加上边界点容量；BMGR 初始化时会按上限截断并打印 warning。
- ``edge_gpio_num`` 或 ``level_gpio_num`` 设为 ``-1`` 时，对应 virtual IO level 字段才有意义。
- 修改 PCNT 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

.. _pcnt-debug:

调试技巧
------------

.. _pcnt-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 PCNT 外设句柄，句柄类型为 ``periph_pcnt_handle_t``：

.. code-block:: c

   typedef struct {
       pcnt_unit_handle_t     pcnt_unit;                             /*!< Handle to the PCNT unit */
       pcnt_channel_handle_t  channels[SOC_PCNT_CHANNELS_PER_UNIT];  /*!< Array of handles for PCNT channels */
   } periph_pcnt_handle_t;

``pcnt_unit`` 可传入 ``pcnt_unit_*`` 系列 API 进行计数器使能、清零和值读取；``channels`` 数组可按通道索引传入 ``pcnt_channel_*`` API 调整边沿和电平响应动作。

相关声明位于 ``esp_board_manager/peripherals/periph_pcnt/periph_pcnt.h``。

底层 ESP-IDF 驱动文档：`脉冲计数器 (PCNT) <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/pcnt.html>`__\ 。
