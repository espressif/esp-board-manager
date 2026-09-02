ldo
=======

:link_to_translation:`en:[English]`

.. _ldo-intro:

简介
------

``ldo`` 外设类型用于描述片上通用 LDO 通道。BMGR 据此生成 ``esp_ldo_channel_config_t``，并调用 ESP-IDF ``esp_ldo_regulator.h`` 申请 LDO channel。该类型用于需由 SoC 片上 LDO 供电的板级资源，例如 MIPI DSI 或 CSI 相关电源。

在 ``board_peripherals.yaml`` 中配置 ``ldo`` 后，支持的 device 可在设备侧 ``peripherals`` 列表中引用该外设。BMGR 初始化设备时通过外设引用获取 LDO 句柄，设备释放时取消引用。

.. _ldo-working-modes:

支持的工作模式
---------------------

``ldo`` 当前不使用 ``role`` 或 ``format`` 拆分模式，配置轴为单个 LDO channel。

- `LDO channel`_

.. _ldo-min-config:

最小配置
------------

.. _ldo-channel:

LDO channel
^^^^^^^^^^^^^^^

完整字段见 :ref:`ldo-channel-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: ldo_mipi
        type: ldo
        config:
          chan_id: 3
          voltage_mv: 2500
          adjustable: 1
          owned_by_hw: 0

.. _ldo-mode-notes:

模式说明
------------

``ldo`` 创建一个片上 LDO channel。``chan_id``、``voltage_mv``、``adjustable`` 和 ``owned_by_hw`` 直接生成到 ``esp_ldo_channel_config_t``，由 ESP-IDF LDO driver 申请通道。

使用该外设时，LDO channel 和输出电压必须来自目标 SoC 数据手册和板级原理图。设备侧只引用 LDO 外设名称，不把 LDO channel、电压或 ownership 配置写进设备侧引用条目。

.. _ldo-full-fields:

完整字段
------------

.. _ldo-channel-full:

LDO channel 完整字段
^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: ldo_mipi
      type: ldo
      config:
        # LDO (Low-Dropout Regulator) configuration for power management
        # This is an hardware-specific configuration, please refer your SoC's datasheet for valid LDO channel IDs
        # You must specify a LDO channel ID manually, based on your board schematic.
        # e.g. For ESP32_P4, the valid values for chan_id are 1-4,
        # where channel 1 and channel 2 are respectively used for powering the internal Flash and PSRAM,
        # channels 3 and 4 can be used to power external devices
        # In P4 Function-EV Board, channel 3 is used for powering the MIPI phy
        chan_id: 3                     # [TO_BE_CONFIRMED] LDO channel ID

        # Output voltage in millivolts (default: 2500 for 2.5V)
        # Common values: equal to 3300 or between 500 to 2700, depending on device requirements
        voltage_mv: 2500                # [TO_BE_CONFIRMED] Output voltage in millivolts

        # Whether voltage is adjustable (default: 1 for true)
        # Valid values: 0 (fixed), 1 (adjustable)
        adjustable: 1

        # Hardware ownership flag (default: 0 for software control)
        # Valid values: 0 (software-controlled), 1 (hardware-controlled)
        owned_by_hw: 0

字段来源：

- YAML 模板：``esp_board_manager/peripherals/periph_ldo/periph_ldo.yml``。
- 头文件：``esp_board_manager/peripherals/periph_ldo/periph_ldo.h``。

.. _ldo-devices:

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - ``display_lcd``
     - ``dsi`` 子类型在设备侧 ``peripherals`` 中引用 ``ldo_mipi``
     - DSI 显示设备通过 LDO 外设管理 MIPI 供电引用
   * - ``camera``
     - ``csi`` 子类型在设备侧 ``peripherals`` 中引用 ``ldo_mipi``
     - CSI 摄像头设备可通过 ``dont_init_ldo`` 与 LDO 外设配合，避免设备内部重复初始化 LDO

.. _ldo-code:

参考代码
------------

- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd_sub_dsi.c``
- ``esp_board_manager/devices/dev_camera/dev_camera_sub_csi.c``

.. _ldo-boards:

板级参考
------------

- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``：``ldo_mipi`` 配置。
- ``esp_boards/esp32_p4_function_ev_board/board_devices.yaml``：``display_lcd`` 和 ``camera`` 引用 ``ldo_mipi``。
- ``m5stack_boards/m5stack_tab5/board_peripherals.yaml``：``ldo_mipi`` 配置。
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``：``display_lcd`` 和 ``camera`` 引用 ``ldo_mipi``。

.. _ldo-notes:

注意事项
------------

- ``chan_id`` 和 ``voltage_mv`` 是板级硬件参数，需要按 SoC 数据手册和原理图填写。
- ``chan_id`` 和 ``voltage_mv`` 必须是非负整数。
- ``adjustable`` 和 ``owned_by_hw`` 只接受 ``0`` 或 ``1``。
- 同一 LDO 供多个设备引用时，设备侧只写同一个外设名称，由 BMGR 引用计数管理句柄。
- 修改 LDO 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

.. _ldo-debug:

调试技巧
------------

.. _ldo-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 LDO 外设句柄，句柄类型为 ESP-IDF 原生的 ``esp_ldo_channel_handle_t``，可传入 ``esp_ldo_channel_*`` 系列 API 进行电压调节。

相关声明位于 ``esp_board_manager/peripherals/periph_ldo/periph_ldo.h``。

底层 ESP-IDF 驱动文档：`低压差线性稳压器 (LDO) <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32p4/api-reference/peripherals/ldo_regulator.html>`__\ 。
