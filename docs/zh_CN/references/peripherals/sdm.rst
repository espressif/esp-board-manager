sdm
=======

:link_to_translation:`en:[English]`

简介
------

``sdm`` 外设类型用于描述片上 Sigma Delta Modulation 输出通道。BMGR 据此生成 ``sdm_config_t``，并调用 ESP-IDF ``driver/sdm.h`` 创建 SDM channel。

BMGR 仅负责创建并保存 SDM 通道句柄。应用获取 ``periph_sdm_handle_t`` 后，可调用 ESP-IDF SDM API 启用通道、设置 pulse density，并在结束时关闭通道。

支持的工作模式
---------------------

``sdm`` 当前不使用 ``role`` 或 ``format`` 拆分模式，配置轴为单个 SDM 输出通道。

- `SDM 输出通道`_

最小配置
------------

SDM 输出通道
^^^^^^^^^^^^^^^^

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: sdm
        type: sdm
        config:
          gpio_num: 4
          clk_src: "SDM_CLK_SRC_DEFAULT"
          sample_rate_hz: 1000000
          invert_out: false
          io_loop_back: false

模式说明
------------

``sdm`` 创建一个输出通道，输出 GPIO 由 ``gpio_num`` 指定。``sample_rate_hz`` 和 ``clk_src`` 决定通道采样时钟配置，输出极性由 ``invert_out`` 控制。

``io_loop_back`` 和 ``allow_pd`` 是随 ESP-IDF 版本变化的 flags：IDF v5.x 生成 ``io_loop_back``，IDF v6.x 及之后生成 ``allow_pd``。在不支持的 ESP-IDF 版本中配置对应字段时，parser 会忽略该字段并打印 warning。

完整字段
------------

SDM 输出通道完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: sdm
      type: sdm
      config:
        # GPIO pin number for SDM output (required, no default - must be >= 0)
        gpio_num: 0                     # [IO] GPIO pin number for SDM output

        # Clock source (default: SDM_CLK_SRC_DEFAULT)
        # Valid values depend on the selected chip,
        # please refer to the enum 'soc_periph_sdm_clk_src_t' in 'soc/clk_tree_defs.h'
        clk_src: "SDM_CLK_SRC_DEFAULT"

        # Sample rate in Hz (default: 1000000)
        sample_rate_hz: 1000000         # [TO_BE_CONFIRMED] Sample rate in Hz

        # Invert output signal (default: false)
        invert_out: false

        # IO loop back for debug/test (IDF < 6.0 only; ignored with a warning on IDF >= 6.0)
        io_loop_back: false

        # Allow SDM power domain to power off during sleep (IDF >= 6.0 only; ignored with a warning on IDF < 6.0)
        allow_pd: false

字段来源：

- YAML 模板：``esp_board_manager/peripherals/periph_sdm/periph_sdm.yml``。
- 头文件：``esp_board_manager/peripherals/periph_sdm/periph_sdm.h``。

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - 直接由应用使用
     - 应用通过 ``esp_board_manager_get_periph_handle`` 获取 SDM 外设句柄
     - 当前仓库未提供引用 ``sdm`` 外设的 device 类型；pulse density 设置由应用或测试代码完成

参考代码
------------

- ``esp_board_manager/test_apps/main/periph/test_periph_sdm.c``

板级参考
------------

- ``esp_boards/esp32_c3_lyra/board_peripherals.yaml``：``sdm`` 输出通道配置。

注意事项
------------

- ``gpio_num`` 必须是非负 GPIO 编号，parser 会拒绝小于 0 的值。
- ``sample_rate_hz`` 必须大于 0。
- ``invert_out``、``io_loop_back`` 和 ``allow_pd`` 必须是布尔值。
- IDF v5.x 下 ``allow_pd`` 不会写入生成配置；IDF v6.x 及之后 ``io_loop_back`` 不会写入生成配置。
- 修改 SDM 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 SDM 外设句柄，句柄类型为 ``periph_sdm_handle_t``：

.. code-block:: c

   typedef struct {
       sdm_channel_handle_t  sdm_chan;  /*!< SDM channel handle */
   } periph_sdm_handle_t;

``sdm_chan`` 可传入 ``sdm_channel_set_pulse_density`` 等 ESP-IDF SDM API 动态调整输出脉冲密度。

相关声明位于 ``esp_board_manager/peripherals/periph_sdm/periph_sdm.h``。

底层 ESP-IDF 驱动文档：`Sigma-Delta 调制器 (SDM) <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/sdm.html>`__\ 。
