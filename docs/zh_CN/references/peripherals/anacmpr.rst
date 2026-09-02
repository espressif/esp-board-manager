anacmpr
===========

:link_to_translation:`en:[English]`

.. _anacmpr-intro:

简介
------

``anacmpr`` 外设类型用于描述片上模拟比较器单元。BMGR 据此生成 ``ana_cmpr_config_t``，并调用 ESP-IDF ``driver/ana_cmpr.h`` 创建 analog comparator unit。

BMGR 初始化后，应用通过 :cpp:func:`esp_board_manager_get_periph_handle` 获取 ``periph_anacmpr_handle_t``，再使用 ESP-IDF analog comparator API 配置内部参考、电平交叉回调、去抖与启停动作。

.. _anacmpr-working-modes:

支持的工作模式
---------------------

``anacmpr`` 当前不使用 ``role`` 或 ``format`` 拆分模式，配置轴为单个 analog comparator unit。

- `analog comparator unit`_

.. _anacmpr-min-config:

最小配置
------------

.. _anacmpr-unit:

analog comparator unit
^^^^^^^^^^^^^^^^^^^^^^^^^^

完整字段见 :ref:`anacmpr-unit-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: anacmpr_unit_0
        type: anacmpr
        config:
          unit: 0
          ref_src: ANA_CMPR_REF_SRC_INTERNAL
          cross_type: ANA_CMPR_CROSS_ANY
          clk_src: ANA_CMPR_CLK_SRC_DEFAULT
          intr_priority: 0

.. _anacmpr-mode-notes:

模式说明
------------

``anacmpr`` 创建一个 analog comparator unit。``ref_src`` 选择内部参考或外部参考输入，``cross_type`` 选择触发交叉事件的方向，``intr_priority`` 配置中断优先级。

当前 BMGR parser 只接受 ``unit: 0``。内部参考电压、去抖和事件回调不在外设 YAML 中配置，由应用在获取句柄后通过 ESP-IDF analog comparator API 设置。

.. _anacmpr-full-fields:

完整字段
------------

.. _anacmpr-unit-full:

analog comparator unit 完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Analog Comparator example with internal reference
    - name: anacmpr_unit_0
      type: anacmpr
      config:
        # Analog Comparator unit number, must be not less than 0 and less than 'SOC_ANA_CMPR_NUM'  (default: 0)
        # Please check the 'SOC_ANA_CMPR_NUM' in 'soc/soc_caps.h' for valid values
        unit: 0  # [TO_BE_CONFIRMED] Analog Comparator unit

        # Reference source type (default: ANA_CMPR_REF_SRC_INTERNAL)
        ref_src: ANA_CMPR_REF_SRC_INTERNAL  # [TO_BE_CONFIRMED] Reference source type
        # Valid values:
        # - ANA_CMPR_REF_SRC_INTERNAL: Use internal reference voltage (divided from VDD)
        # - ANA_CMPR_REF_SRC_EXTERNAL: Use external reference from GPIO pin

        # Cross type for interrupt triggering (default: ANA_CMPR_CROSS_ANY)
        cross_type: ANA_CMPR_CROSS_ANY
        # Valid values:
        # - ANA_CMPR_CROSS_POS: Trigger on positive crossing (source > reference)
        # - ANA_CMPR_CROSS_NEG: Trigger on negative crossing (source < reference)
        # - ANA_CMPR_CROSS_ANY: Trigger on both positive and negative crossing

        # Clock source (default: ANA_CMPR_CLK_SRC_DEFAULT)
        clk_src: ANA_CMPR_CLK_SRC_DEFAULT
        # Please check the 'soc_periph_ana_cmpr_clk_src_t' in 'soc/clk_tree_defs.h' for valid values

        # Interrupt priority (default: 0)
        # If set to 0, the driver will automatically select a relative low priority (1,2,3)
        intr_priority: 0

字段来源：

- YAML 模板：``esp_board_manager/peripherals/periph_anacmpr/periph_anacmpr.yml``。
- 头文件：``esp_board_manager/peripherals/periph_anacmpr/periph_anacmpr.h``。

.. _anacmpr-devices:

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - 直接由应用使用
     - 应用通过 ``esp_board_manager_get_periph_handle`` 获取 analog comparator 外设句柄
     - 当前仓库未提供引用 ``anacmpr`` 外设的 device 类型；参考电压、去抖、回调和启停由应用或测试代码完成

.. _anacmpr-code:

参考代码
------------

- ``esp_board_manager/test_apps/main/periph/test_periph_anacmpr.c``

.. _anacmpr-boards:

板级参考
------------

- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_p4_core/board_peripherals.yaml``：``anacmpr_unit_0`` 配置，并配置 ``gpio_monitor`` 用于测试中观察交叉事件。

.. _anacmpr-notes:

注意事项
------------

- 当前 parser 只接受 ``unit: 0``。
- ``ref_src`` 只接受 ``ANA_CMPR_REF_SRC_INTERNAL`` 或 ``ANA_CMPR_REF_SRC_EXTERNAL``。
- ``cross_type`` 只接受 ``ANA_CMPR_CROSS_POS``、``ANA_CMPR_CROSS_NEG`` 或 ``ANA_CMPR_CROSS_ANY``。
- ``clk_src`` 当前只接受 ``ANA_CMPR_CLK_SRC_DEFAULT``。
- 内部参考电压、去抖和事件回调需要在应用代码中配置，不写入 ``anacmpr`` 外设 ``config``。
- 修改 analog comparator 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

.. _anacmpr-debug:

调试技巧
------------

.. _anacmpr-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取模拟比较器外设句柄，句柄类型为 ``periph_anacmpr_handle_t``：

.. code-block:: c

   typedef struct {
       ana_cmpr_handle_t  cmpr_handle;  /*!< Analog comparator handle */
       ana_cmpr_unit_t    unit;         /*!< Analog comparator unit */
   } periph_anacmpr_handle_t;

``cmpr_handle`` 可传入 ESP-IDF ``ana_cmpr_*`` 系列 API；``unit`` 标识当前比较器控制器编号。

相关声明位于 ``esp_board_manager/peripherals/periph_anacmpr/periph_anacmpr.h``。

底层 ESP-IDF 驱动文档：`模拟比较器 <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32p4/api-reference/peripherals/ana_cmpr.html>`__\ 。
