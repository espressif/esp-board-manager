dsi
=======

:link_to_translation:`en:[English]`

简介
------

``dsi`` 外设类型用于描述 MIPI DSI 总线资源。BMGR 根据该配置创建 ``esp_lcd_dsi_bus_handle_t``，供 ``display_lcd`` 设备的 ``dsi`` 子类型引用。

该外设仅描述 DSI 总线本身，例如总线编号、数据 lane 数、PHY 时钟源与每 lane 比特率。面板复位、DBI 与 DPI 参数、像素格式、分辨率与时序属于 ``display_lcd`` 设备配置，不写入 ``dsi`` 外设的 ``config``。

支持的工作模式
---------------------

``dsi`` 当前按 ESP-IDF MIPI DSI bus 模式配置，不通过 ``role`` 或 ``sub_type`` 进一步拆分工作模式。

- `DSI 总线`_

最小配置
------------

DSI 总线
^^^^^^^^^^

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: dsi_display
        type: dsi
        config:
          bus_id: 0
          data_lanes: 2
          phy_clk_src: 0
          lane_bit_rate_mbps: 1000

模式说明
------------

``dsi`` 外设创建的是 DSI bus 句柄。使用 DSI 屏时，``display_lcd`` 设备的 ``sub_type`` 设为 ``dsi``，并在设备侧 ``peripherals`` 中引用该外设名称。若板级配置还需要 MIPI 供电控制，应额外定义 ``ldo`` 或其他供电外设，并由设备侧按名称引用。

完整字段
------------

DSI 总线完整字段
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # DSI Display Configuration
    # This configuration sets up a MIPI DSI display interface
    # Based on common DSI display parameters

    - name: dsi_display
      type: dsi
      config:
        # DSI bus identifier (default: 0 for primary bus)
        # Valid values depending on available DSI controllers, esp32p4 support only 1 MIPI DSI bus
        bus_id: 0

        # Number of data lanes (default: 2 for dual-lane mode)
        # esp32p4 support up to 2 data lanes
        data_lanes: 2

        # Physical layer clock source (default: MIPI_DSI_PHY_CLK_SRC_DEFAULT)
        phy_clk_src: 0
        # Valid values:
        # - 0 (Set it to 0 and then let the driver configure it)
        # - MIPI_DSI_PHY_CLK_SRC_DEFAULT
        # - MIPI_DSI_PHY_CLK_SRC_PLL_F20M
        # - MIPI_DSI_PHY_CLK_SRC_PLL_F25M
        # - MIPI_DSI_PHY_CLK_SRC_RC_FAST

        # Bit rate per data lane in Mbps (default: 1000 for 1Gbps per lane)
        # Common values between 80 to 1500 Mbps depending on display capabilities
        lane_bit_rate_mbps: 1000          # [TO_BE_CONFIRMED] Bit rate per data lane in Mbps

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - ``display_lcd``
     - ``sub_type`` 为 ``dsi`` 时，在设备侧 ``peripherals`` 中引用 ``dsi`` 外设
     - DBI/DPI 配置、色彩格式、面板时序和 LCD 组件依赖写在设备配置中

参考代码
------------

- ``esp_board_manager/peripherals/periph_dsi/periph_dsi.c``
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd.c``

板级参考
------------

- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``：定义 ``dsi_display``，并配合 ``ldo_mipi`` 和 DSI LCD 使用。
- ``esp_boards/esp32_p4_function_ev_board/board_devices.yaml``：``display_lcd`` 以 ``sub_type: dsi`` 引用 ``dsi_display``。
- ``m5stack_boards/m5stack_tab5/board_peripherals.yaml``：定义 ``dsi_display`` 和 LCD 背光 LEDC 外设。
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``：``display_lcd`` 以 ``sub_type: dsi`` 引用 ``dsi_display``。

注意事项
------------

- ``dsi`` 外设配置只覆盖 DSI bus。面板驱动组件依赖、初始化命令和 DPI 时序属于 ``display_lcd`` 设备。
- ESP32-P4 板级配置中的 MIPI 供电通过 ``ldo`` 外设单独描述，``dsi`` 外设不会配置供电电压。
- ``lane_bit_rate_mbps`` 需要与屏幕、lane 数和像素时钟预算匹配，模板中保留 ``[TO_BE_CONFIRMED]``。
- 修改 DSI 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 DSI 外设句柄，句柄类型为 ESP-IDF 原生的 ``esp_lcd_dsi_bus_handle_t``，可直接传入 ``esp_lcd_*`` 系列 API。

相关声明位于 ``esp_board_manager/peripherals/periph_dsi/periph_dsi.h``。

底层 ESP-IDF 驱动文档：`MIPI DSI 接口的 LCD <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32p4/api-reference/peripherals/lcd/dsi_lcd.html>`__\ 。
