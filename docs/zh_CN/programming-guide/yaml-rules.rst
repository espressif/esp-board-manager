YAML 标记与写法
==========================================================================

:link_to_translation:`en:[English]`

本节集中说明 BMGR 在标准 YAML 之上额外约定的几种写法，覆盖填写标记、枚举书写、``${BOARD_PATH}`` 变量与锚点合并语法。三份板级 YAML 文件均适用。

[IO] 标记
--------------

字段注释带有 ``[IO]`` 时，表示该字段对应真实硬件引脚，必须按实际原理图填写，不能照搬示例值或模板默认值（通常为 ``-1``）。

.. code-block:: yaml

   - name: i2c_main
     type: i2c
     config:
       sda_io_num: 10   # [IO]
       scl_io_num: 11   # [IO]

[TO_BE_CONFIRMED] 标记
----------------------------

字段注释带有 ``[TO_BE_CONFIRMED]`` 时，表示当前值是占位值或通用默认值，适配开发板时必须逐项确认并替换为真实值。常见于屏幕分辨率、时序参数、电压档位、I2C 地址等需要查阅器件资料才能确定的字段。

枚举值
--------------

YAML 中的枚举字段直接使用 ESP-IDF 驱动或相关组件头文件中的\ **原始枚举名**\ ，不使用自定义字符串或数值替代。例如 GPIO 方向字段填 ``GPIO_MODE_OUTPUT``，不填 ``1`` 或 ``"output"``；MIPI DPI 时钟源填 ``MIPI_DSI_DPI_CLK_SRC_DEFAULT``，不使用数值或缩写。生成器会按字符串原样输出到 C 源码，枚举名拼写错误会在编译阶段暴露。

${BOARD_PATH} 变量
----------------------

``${BOARD_PATH}`` 在开发板 YAML 中指向\ **当前开发板定义的根目录**\ （即包含 ``board_devices.yaml`` 的目录）。引用开发板目录下的本地组件时，建议使用该变量代替手写相对路径。``gen_bmgr_codes/idf_component.yml`` 由 BMGR 生成，相对路径以该目录为基准展开，容易出错。``${BOARD_PATH}`` 由 BMGR 在生成阶段统一替换，写在开发板 YAML 中语义稳定：

.. code-block:: yaml

   dependencies:
     my_company/my_driver:
       version: "*"
       override_path: ${BOARD_PATH}/packages/my_driver

.. note::

   变量名仅识别 ``${BOARD_PATH}`` 这一种写法。``{{BOARD_PATH}}``、``$BOARD_PATH`` 等写法均不会被替换。

YAML 锚点与合并（标准 YAML）
----------------------------------------

BMGR 不限制 YAML 解析行为，因此 ``&`` 锚点、``*`` 引用、``<<:`` 合并等标准语法均可直接使用，常用于消除同型外设的重复配置：

.. code-block:: yaml

   peripherals:
     - name: spi_lcd
       type: spi
       version: "1.0.0"
       role: master
       config: &spi_master_default
         mosi_io_num: 11        # [IO]
         miso_io_num: -1        # 仅写入设备时不需要
         sclk_io_num: 12        # [IO]
         max_transfer_sz: 32768

     - name: spi_touch
       type: spi
       version: "1.0.0"
       role: master
       config:
         <<: *spi_master_default
         mosi_io_num: 35        # [IO]
         sclk_io_num: 36        # [IO]

语义化外设绑定
------------------------

设备条目按语义角色引用外设。请使用与设备类型和子类型匹配的 ``*_name`` 字段，不要依赖外设名称前缀或列表顺序：

.. code-block:: yaml

   devices:
     - name: display_lcd
       type: display_lcd
       sub_type: spi
       peripherals:
         - spi_name: spi_lcd

选择器由设备类型和子类型决定。例如，编解码器 GPIO 使用 ``pa_name`` 和 ``reset_name``，I2C 控制总线使用 ``i2c_name``，SPI 总线使用 ``spi_name``，DSI LCD 使用 ``dsi_name`` 和 ``ldo_name``，GPIO 控制线使用 ``gpio_name``。每个外设条目最多只能包含一个角色选择器。

为保持兼容，仍接受字符串引用以及只包含 ``name`` 的映射。BMGR 会根据设备上下文推断角色并发出告警。未知选择器、重复角色、同时包含 ``name`` 和 ``*_name`` 的引用，或引用外设类型不匹配时，解析阶段会拒绝该配置。
