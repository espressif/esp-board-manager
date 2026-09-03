mcpwm
=========

:link_to_translation:`en:[English]`

.. _mcpwm-intro:

简介
------

``mcpwm`` 外设类型用于描述一组 MCPWM timer、operator、comparator 与 generator。BMGR 根据该配置创建 MCPWM 资源，并返回 ``periph_mcpwm_handle_t``。

该外设仅创建 MCPWM 基础资源。生成器事件动作与 comparator 占空比不在外设初始化中设置，需由使用该句柄的设备或应用代码继续配置。

.. _mcpwm-working-modes:

支持的工作模式
---------------------

``mcpwm`` 当前按单组 MCPWM 资源配置，不通过 ``role`` 进一步拆分工作模式。

- `MCPWM 资源组`_

.. _mcpwm-min-config:

最小配置
------------

.. _mcpwm-group:

MCPWM 资源组
^^^^^^^^^^^^^^^

完整字段见 :ref:`mcpwm-group-idf5`。
完整字段见 :ref:`mcpwm-group-idf6`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: mcpwm_group_0
        type: mcpwm
        config:
          timer_config:
            group_id: 0
            clk_src: MCPWM_TIMER_CLK_SRC_DEFAULT
            resolution_hz: 1000000
            count_mode: MCPWM_TIMER_COUNT_MODE_UP
            period_ticks: 20000
            intr_priority: 0
            flags:
              update_period_on_empty: false
              update_period_on_sync: false
              allow_pd: false
          operator_config:
            group_id: 0
            intr_priority: 0
            flags:
              update_gen_action_on_tez: false
              update_gen_action_on_tep: false
              update_gen_action_on_sync: false
              update_dead_time_on_tez: false
              update_dead_time_on_tep: false
              update_dead_time_on_sync: false
          comparator_configs:
            - comparator: 0
              intr_priority: 0
              flags:
                update_cmp_on_tez: true
                update_cmp_on_tep: false
                update_cmp_on_sync: false
          generator_config:
            gpio_num: 45
            flags:
              invert_pwm: false
              io_od_mode: false
              pull_up: false
              pull_down: false

.. _mcpwm-mode-notes:

模式说明
------------

``timer_config`` 与 ``operator_config`` 的 ``group_id`` 应指向同一个 MCPWM group。``comparator_configs`` 用于创建一个或多个 comparator；也可以使用模板中的 ``comparator_config`` 写法配置单个 comparator。``generator_config`` 只绑定 PWM 输出 GPIO 和 GPIO 相关 flags，不设置 waveform action。

IDF 5 与 IDF 6 的 MCPWM 模板基本一致，差异在 ``generator_config.flags``：IDF 5 模板包含 ``io_loop_back``，IDF 6 模板不包含该字段。

.. _mcpwm-full-fields:

完整字段
------------

.. _mcpwm-group-idf5:

IDF 5 MCPWM 资源组完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # MCPWM Peripheral Default Configuration
    # This file shows the default values used by the MCPWM peripheral parser
    # Based on periph_mcpwm.py parsing script
    # Version: 1.1.0 - Added support for multiple comparators

    - name: mcpwm_group_0
      type: mcpwm
      config:
        # Timer configuration
        timer_config:
          # Group ID (default: 0), must be less than 'SOC_MCPWM_GROUPS'
          # Please check the 'SOC_MCPWM_GROUPS' in 'soc/soc_caps.h' for valid values
          group_id: 0

          # Clock source (default: MCPWM_TIMER_CLK_SRC_DEFAULT)
          # Valid values depend on the selected chip,
          # please refer to the enum 'soc_periph_mcpwm_timer_clk_src_t' in 'soc/clk_tree_defs.h'
          clk_src: MCPWM_TIMER_CLK_SRC_DEFAULT

          # Timer resolution in Hz (default: 1000000, 1MHz = 1us per tick)
          # The step size of each count tick equals to (1 / resolution_hz) seconds
          resolution_hz: 1000000          # [TO_BE_CONFIRMED] Timer resolution in Hz

          # Count mode (default: MCPWM_TIMER_COUNT_MODE_UP)
          count_mode: MCPWM_TIMER_COUNT_MODE_UP
          # Valid values:
          # - MCPWM_TIMER_COUNT_MODE_PAUSE
          # - MCPWM_TIMER_COUNT_MODE_UP
          # - MCPWM_TIMER_COUNT_MODE_DOWN
          # - MCPWM_TIMER_COUNT_MODE_UP_DOWN

          # Number of count ticks within a period (default: 20000, 20ms for 50Hz PWM)
          period_ticks: 20000             # [TO_BE_CONFIRMED] Number of count ticks within a period

          # MCPWM timer interrupt priority
          # If set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)
          intr_priority: 0

          # Timer flags
          flags:
            # Whether to update period when timer counts to zero (default: false)
            update_period_on_empty: false

            # Whether to update period on sync event (default: false)
            update_period_on_sync: false

            # Allow power down (default: false)
            # When this flag set, the driver will backup/restore the MCPWM registers before/after entering/exit sleep mode.
            # By this approach, the system can power off MCPWM's power domain.
            # This can save power, but at the expense of more RAM being consumed.
            allow_pd: false

        # Operator configuration
        operator_config:
          # Group ID, must be less than 'SOC_MCPWM_GROUPS' (default: 0)
          # Please check the 'SOC_MCPWM_GROUPS' in 'soc/soc_caps.h' for valid values
          # Operator and timer should reside in the same group
          group_id: 0

          # Interrupt priority (default: 0)
          intr_priority: 0

          # Operator flags
          flags:
            # Whether to update generator action when timer counts to zero (default: false)
            update_gen_action_on_tez: false

            # Whether to update generator action when timer counts to peak (default: false)
            update_gen_action_on_tep: false

            # Whether to update generator action on sync event (default: false)
            update_gen_action_on_sync: false

            # Whether to update dead time when timer counts to zero (default: false)
            update_dead_time_on_tez: false

            # Whether to update dead time when timer counts to peak (default: false)
            update_dead_time_on_tep: false

            # Whether to update dead time on sync event (default: false)
            update_dead_time_on_sync: false

        # Comparator configurations, support for multiple comparators,
        # the supported number of comparators for each operator can be determined by checking the 'SOC_MCPWM_COMPARATORS_PER_OPERATOR' in soc/soc_caps.h
        # Use either 'comparator_config' for single comparator or 'comparator_configs' for multiple comparators
        comparator_configs:

          # First comparator configuration
          - comparator: 0

            # Interrupt priority (default: 0)
            intr_priority: 0
            flags:
              # Whether to update compare value when timer counts to zero (default: true)
              update_cmp_on_tez: true

              # Whether to update compare value when timer counts to peak (default: false)
              update_cmp_on_tep: false

              # Whether to update compare value on sync event (default: false)
              update_cmp_on_sync: false

          # Second comparator configuration (optional)
          # - comparator: 1
          #   intr_priority: 0
          #   flags:
          #     update_cmp_on_tez: true
          #     update_cmp_on_tep: false
          #     update_cmp_on_sync: false

        # Alternative: single comparator configuration
        # comparator_config:
        #   intr_priority: 0
        #   flags:
        #     update_cmp_on_tez: true
        #     update_cmp_on_tep: false
        #     update_cmp_on_sync: false

        # Generator configuration
        generator_config:
          # GPIO pin number for PWM output (required, no default - must be >= 0)
          gpio_num: 0                     # [IO] GPIO pin number for PWM output

          # Generator flags
          flags:
            # Whether to invert PWM output signal (default: false)
            invert_pwm: false

            # IO loop back for debug/test (default: false)
            io_loop_back: false

            # Configure GPIO as open-drain mode (default: false)
            io_od_mode: false

            # Whether to pull up internally (default: false)
            pull_up: false

            # Whether to pull down internally (default: false)
            pull_down: false

.. _mcpwm-group-idf6:

IDF 6 MCPWM 资源组完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # MCPWM Peripheral Default Configuration
    # This file shows the default values used by the MCPWM peripheral parser
    # Based on periph_mcpwm.py parsing script
    # Version: 1.1.0 - Added support for multiple comparators

    - name: mcpwm_group_0
      type: mcpwm
      config:
        # Timer configuration
        timer_config:
          # Group ID (default: 0), must be less than 'SOC_MCPWM_GROUPS'
          # Please check the 'SOC_MCPWM_GROUPS' in 'soc/soc_caps.h' for valid values
          group_id: 0

          # Clock source (default: MCPWM_TIMER_CLK_SRC_DEFAULT)
          # Valid values depend on the selected chip,
          # please refer to the enum 'soc_periph_mcpwm_timer_clk_src_t' in 'soc/clk_tree_defs.h'
          clk_src: MCPWM_TIMER_CLK_SRC_DEFAULT

          # Timer resolution in Hz (default: 1000000, 1MHz = 1us per tick)
          # The step size of each count tick equals to (1 / resolution_hz) seconds
          resolution_hz: 1000000          # [TO_BE_CONFIRMED] Timer resolution in Hz

          # Count mode (default: MCPWM_TIMER_COUNT_MODE_UP)
          count_mode: MCPWM_TIMER_COUNT_MODE_UP
          # Valid values:
          # - MCPWM_TIMER_COUNT_MODE_PAUSE
          # - MCPWM_TIMER_COUNT_MODE_UP
          # - MCPWM_TIMER_COUNT_MODE_DOWN
          # - MCPWM_TIMER_COUNT_MODE_UP_DOWN

          # Number of count ticks within a period (default: 20000, 20ms for 50Hz PWM)
          period_ticks: 20000             # [TO_BE_CONFIRMED] Number of count ticks within a period

          # MCPWM timer interrupt priority
          # If set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)
          intr_priority: 0

          # Timer flags
          flags:
            # Whether to update period when timer counts to zero (default: false)
            update_period_on_empty: false

            # Whether to update period on sync event (default: false)
            update_period_on_sync: false

            # Allow power down (default: false)
            # When this flag set, the driver will backup/restore the MCPWM registers before/after entering/exit sleep mode.
            # By this approach, the system can power off MCPWM's power domain.
            # This can save power, but at the expense of more RAM being consumed.
            allow_pd: false

        # Operator configuration
        operator_config:
          # Group ID, must be less than 'SOC_MCPWM_GROUPS' (default: 0)
          # Please check the 'SOC_MCPWM_GROUPS' in 'soc/soc_caps.h' for valid values
          # Operator and timer should reside in the same group
          group_id: 0

          # Interrupt priority (default: 0)
          intr_priority: 0

          # Operator flags
          flags:
            # Whether to update generator action when timer counts to zero (default: false)
            update_gen_action_on_tez: false

            # Whether to update generator action when timer counts to peak (default: false)
            update_gen_action_on_tep: false

            # Whether to update generator action on sync event (default: false)
            update_gen_action_on_sync: false

            # Whether to update dead time when timer counts to zero (default: false)
            update_dead_time_on_tez: false

            # Whether to update dead time when timer counts to peak (default: false)
            update_dead_time_on_tep: false

            # Whether to update dead time on sync event (default: false)
            update_dead_time_on_sync: false

        # Comparator configurations, support for multiple comparators,
        # the supported number of comparators for each operator can be determined by checking the 'SOC_MCPWM_COMPARATORS_PER_OPERATOR' in soc/soc_caps.h
        # Use either 'comparator_config' for single comparator or 'comparator_configs' for multiple comparators
        comparator_configs:

          # First comparator configuration
          - comparator: 0

            # Interrupt priority (default: 0)
            intr_priority: 0
            flags:
              # Whether to update compare value when timer counts to zero (default: true)
              update_cmp_on_tez: true

              # Whether to update compare value when timer counts to peak (default: false)
              update_cmp_on_tep: false

              # Whether to update compare value on sync event (default: false)
              update_cmp_on_sync: false

          # Second comparator configuration (optional)
          # - comparator: 1
          #   intr_priority: 0
          #   flags:
          #     update_cmp_on_tez: true
          #     update_cmp_on_tep: false
          #     update_cmp_on_sync: false

        # Alternative: single comparator configuration
        # comparator_config:
        #   intr_priority: 0
        #   flags:
        #     update_cmp_on_tez: true
        #     update_cmp_on_tep: false
        #     update_cmp_on_sync: false

        # Generator configuration
        generator_config:
          # GPIO pin number for PWM output (required, no default - must be >= 0)
          gpio_num: 0                     # [IO] GPIO pin number for PWM output

          # Generator flags
          flags:
            # Whether to invert PWM output signal (default: false)
            invert_pwm: false

            # Configure GPIO as open-drain mode (default: false)
            io_od_mode: false

            # Whether to pull up internally (default: false)
            pull_up: false

            # Whether to pull down internally (default: false)
            pull_down: false

.. _mcpwm-devices:

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - ``custom``
     - 设备侧可引用已定义的 ``mcpwm`` 外设
     - waveform action、duty 更新和电机控制逻辑由自定义设备或应用代码完成

.. _mcpwm-code:

参考代码
------------

- ``esp_board_manager/peripherals/periph_mcpwm/idf5/periph_mcpwm.c``
- ``esp_board_manager/peripherals/periph_mcpwm/idf6/periph_mcpwm.c``
- ``esp_board_manager/test_apps/main/periph/test_periph_mcpwm.c``

.. _mcpwm-boards:

板级参考
------------

- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_s3_devkitc/board_peripherals.yaml``：定义 ``mcpwm_group_0`` 测试外设。

.. _mcpwm-notes:

注意事项
------------

- ``operator_config.group_id`` 需要与 ``timer_config.group_id`` 保持一致。
- comparator 数量受 ``SOC_MCPWM_COMPARATORS_PER_OPERATOR`` 限制；BMGR 初始化时会校验 comparator 数量。
- ``periph_mcpwm_init`` 只创建 timer、operator、comparator 和 generator，不设置 generator event action 和 comparator duty。
- 修改 MCPWM 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

.. _mcpwm-debug:

调试技巧
------------

.. _mcpwm-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 MCPWM 外设句柄，句柄类型为 ``periph_mcpwm_handle_t``：

.. code-block:: c

   typedef struct {
       mcpwm_timer_handle_t  timer;                                            /*!< MCPWM timer handle */
       mcpwm_oper_handle_t   operator;                                         /*!< MCPWM operator handle */
       mcpwm_cmpr_handle_t   comparators[SOC_MCPWM_COMPARATORS_PER_OPERATOR];  /*!< MCPWM comparator handles */
       mcpwm_gen_handle_t    generator;                                        /*!< MCPWM generator handle */
   } periph_mcpwm_handle_t;

各成员可直接传入对应的 ``mcpwm_timer_*``、``mcpwm_operator_*``、``mcpwm_comparator_*``、``mcpwm_generator_*``\ ESP-IDF API，实现动态占空比、周期和故障保护控制。

相关声明位于 ``esp_board_manager/peripherals/periph_mcpwm/periph_mcpwm.h``。

底层 ESP-IDF 驱动文档：`电机控制脉宽调制器 (MCPWM) <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/mcpwm.html>`__\ 。
