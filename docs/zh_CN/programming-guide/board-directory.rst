板级目录结构与文件职责
==========================================================================

:link_to_translation:`en:[English]`

最小开发板目录需要三个文件：

.. code-block:: text

   my_board/
     board_info.yaml          # 板级元数据：板名、芯片、版本、描述、厂商
     board_peripherals.yaml   # 底层外设资源声明
     board_devices.yaml       # 功能设备声明

BMGR 在扫描可用开发板时以这三个文件是否同时存在作为识别条件。其余文件为可选：

.. code-block:: text

   my_board/
     sdkconfig.defaults.board  # 可选：板级默认 sdkconfig 项
     setup_device.c            # 可选：纯 YAML 无法描述的板级初始化逻辑
     Kconfig.projbuild         # 可选：板级 Kconfig 符号扩展
     packages/                 # 可选：板级本地组件

开发板名称以目录名为准，必须与 ``board_info.yaml`` 中的 ``board`` 字段保持一致。开发板名称只允许字母、数字和下划线，不支持中划线或其他特殊字符。不符合规范的开发板在扫描时会被忽略。

board_info.yaml 板级元数据
----------------------------------------

``board_info.yaml`` 描述一块开发板的静态元数据。生成时写入 ``gen_board_info.c``，可通过 :cpp:func:`esp_board_manager_print_board_info` 输出。所有字段均为可读元数据，不影响设备或外设的初始化逻辑。

.. code-block:: yaml

   board: my_board
   chip: esp32s3
   version: "1.0.0"
   description: "My custom board"
   manufacturer: "MyCompany"

.. list-table::
   :header-rows: 1
   :widths: 18 12 12 58

   * - 字段名
     - 类型
     - 是否必填
     - 解释
   * - ``board``
     - string
     - 必填
     - 开发板名称，必须与板目录名一致；只允许小写字母、数字、下划线，不支持中划线或其他特殊字符。
   * - ``chip``
     - string
     - 必填
     - 开发板主控芯片型号，例如 ``esp32s3``、``esp32p4``，对应 ``CONFIG_IDF_TARGET``。
   * - ``version``
     - string
     - 可选
     - YAML 解析契约的 schema 代号；当前为 ``1.0.0``，省略、``default``\ （不区分大小写）与 ``1.0.0`` 三种写法等价。写入 BMGR 不识别的代号时，生成阶段会输出警告。与 IDF 驱动版本和 BMGR 组件版本无关。
   * - ``description``
     - string
     - 可选
     - 开发板简介，自由文本。运行时由 :cpp:func:`esp_board_manager_print_board_info` 通过 ``ESP_LOGI`` 输出，``idf.py bmgr -l`` 不输出此字段。
   * - ``manufacturer``
     - string
     - 可选
     - 开发板厂商名。运行时由 :cpp:func:`esp_board_manager_print_board_info` 与 ``description`` 一同输出，``idf.py bmgr -l`` 不输出此字段。

board_peripherals.yaml
----------------------------------------

``board_peripherals.yaml`` 声明开发板上的底层硬件资源实例。每条记录对应一路底层资源，例如某条 I2C 总线、某个 SPI 控制器、某路 I2S 接口或某个 GPIO。

每个外设条目的基本结构：

.. code-block:: yaml

   peripherals:
     - name: <peripheral_name>
       type: <peripheral_type>
       version: <version>
       role: <role>
       format: <format_string>
       config: <configuration>

.. list-table::
   :header-rows: 1
   :widths: 18 14 14 54

   * - 字段名
     - 类型
     - 是否必填
     - 解释
   * - ``name``
     - string
     - 必填
     - 外设实例名，\ **必须以 type 为前缀**\ ，例如 ``i2c_main``、``spi_lcd``、``gpio_power``。在当前文件中唯一，只允许小写字母、数字、下划线，不能纯数字。设备通过该名字引用外设，名字不一致设备初始化会失败。
   * - ``type``
     - string
     - 必填
     - 外设类别，使用 BMGR 已支持的标准类型名，例如 ``i2c`` / ``spi`` / ``i2s`` / ``gpio``。支持的清单见 :doc:`/references/support-matrix`。
   * - ``version``
     - string
     - 可选
     - 该外设条目使用的 schema 代号，含义同 ``board_info.yaml`` 的 ``version``，省略即用当前代规范。
   * - ``role``
     - string
     - 条件必填
     - 外设工作模式，例如 ``master`` / ``slave``、``tx`` / ``rx``、``continuous`` / ``oneshot``。是否必填以及取值集合由 ``type`` 决定，详见外设参考。
   * - ``format``
     - string
     - 条件必填
     - 数据格式声明，仅在部分外设需要（当前仅 I2S 使用，例如 ``std-out``、``tdm-in``、``pdm-out``）。取值见外设参考。
   * - ``config``
     - mapping
     - 必填
     - 外设专属配置，字段来自 ``peripherals/periph_<type>/periph_<type>.yml``。详细字段说明见外设参考。

board_devices.yaml
----------------------------------------

``board_devices.yaml`` 声明开发板上的功能设备实例。每条记录对应一个功能设备，例如音频编解码器、LCD 屏幕或按键。

每个设备条目的基本结构：

.. code-block:: yaml

   devices:
     - name: <device_name>
       type: <device_type>
       chip: <chip_name>
       version: <version>
       sub_type: <sub_type>
       init_skip: false
       depends_on: other_device
       power_ctrl_device: power_ctrl
       config:
         <configurations>
       peripherals:
         - name: <periph_name>
           # 可附加 device 侧的专属参数
       dependencies:
         <component_name>:
           require: <scope>
           version: <version_spec>

.. list-table::
   :header-rows: 1
   :widths: 20 16 14 50

   * - 字段名
     - 类型
     - 是否必填
     - 解释
   * - ``name``
     - string
     - 必填
     - 设备实例名，在当前文件中唯一；只允许小写字母、数字、下划线，不能纯数字。\ **不要求以 type 为前缀**\ ，建议贴近功能语义，例如 ``audio_codec_0``、``display_main``、``button_boot``。
   * - ``type``
     - string
     - 必填
     - 设备类别，使用 BMGR 已支持的标准类型名，例如 ``audio_codec`` / ``display_lcd`` / ``button`` / ``power_ctrl`` / ``custom``。支持的清单见 :doc:`/references/support-matrix`。
   * - ``chip``
     - string
     - 条件必填
     - 设备外接芯片型号（与 ``board_info.yaml.chip`` 含义不同），适用于需要识别具体器件型号的设备，例如 LCD 控制器、触摸屏控制器、IO 扩展芯片。具体哪些 type 必填见设备参考。
   * - ``version``
     - string
     - 可选
     - 该外设条目使用的 schema 代号，含义同 ``board_info.yaml`` 的 ``version``，省略即用当前代规范。
   * - ``sub_type``
     - string
     - 条件必填
     - 同一 type 下的子实现路径，例如 ``display_lcd`` 分 ``spi`` / ``i80`` / ``dsi`` / ``parlio`` / ``rgb`` / ``rgb_3wire_spi``。是否必填和取值集合由 ``type`` 决定，详见设备参考。
   * - ``init_skip``
     - bool
     - 可选
     - 默认 ``false``。``true`` 时 :cpp:func:`esp_board_manager_init` 不会自动初始化该设备，需要应用按业务时机自行调用 :cpp:func:`esp_board_manager_init_device_by_name`。详见 :doc:`/programming-guide/runtime-lifecycle`。
   * - ``depends_on``
     - string / list
     - 可选
     - 声明该设备依赖的其他设备实例名。BMGR 在初始化该设备前会先递归初始化依赖。详见 :doc:`/programming-guide/runtime-lifecycle`。
   * - ``power_ctrl_device``
     - string
     - 可选
     - 引用一个 ``type: power_ctrl`` 的设备实例。BMGR 在该设备 init 之前自动触发其上电、deinit 之后触发下电。详见 :doc:`/programming-guide/runtime-lifecycle`。
   * - ``config``
     - mapping
     - 必填
     - 设备专属配置，字段来自 ``devices/dev_<type>/dev_<type>.yml``。详细字段说明见设备参考。
   * - ``peripherals``
     - list of mappings
     - 条件必填
     - 声明该设备运行依赖的 peripheral 实例（例如 codec 依赖 ``i2c`` + ``i2s``，按键依赖 ``gpio`` 或 ``adc``）。是否必填以及最少需要哪些外设由 type 决定。每个引用条目的 ``name`` 必须与 ``board_peripherals.yaml`` 中的外设实例名完全一致。除 ``name`` 外，还可能包含设备侧的专属参数（例如 I2C 地址、PA 有效电平等），由设备解析器读取，\ **不会修改外设自身的 config**\ 。
   * - ``dependencies``
     - mapping
     - 条件必填
     - 声明设备运行所需的额外 ESP-IDF 组件。生成时合并写入 ``components/gen_bmgr_codes/idf_component.yml``，同名依赖按 YAML 顺序保留最后一个。字段含义与组件管理器一致，详见 `IDF Component Manager Manifest 文档 <https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/manifest_file.html#dependencies>`_。

``peripherals`` 中引用外设并附加设备专属参数的典型写法（例如编解码器在设备侧补 I2C 地址）：

.. code-block:: yaml

   devices:
     - name: audio_codec_0
       type: audio_codec
       peripherals:
         - name: i2c_main
           addr: 0x18        # device 侧的专属参数

sdkconfig.defaults.board 与 Kconfig.projbuild
------------------------------------------------------

每块开发板可在板目录下放置以下两份可选文件，用于扩展工程的配置系统：

- ``sdkconfig.defaults.board``：声明开发板相关的 sdkconfig 默认项（PSRAM、Flash、partition、应用层 ``CONFIG_*`` 等），由 BMGR 在生成阶段合并到 ``components/gen_bmgr_codes/board_manager.defaults``。
- ``Kconfig.projbuild``：开发板需要在 ``menuconfig`` 中暴露的额外 Kconfig 符号（通常是开发板特有的功能开关、子板枚举等），由 BMGR 追加到 ``components/gen_bmgr_codes/Kconfig.projbuild``。

.. code-block:: ini

   # sdkconfig.defaults.board 示例
   CONFIG_SPIRAM_MODE_OCT=y
   CONFIG_SPIRAM_SPEED_80M=y

这两份文件在生成阶段如何被合并、覆盖优先级，以及与 amend 的关系，见 :doc:`/programming-guide/generation-flow`。

board_amend.yaml
----------------------------------------

``board_amend.yaml`` 是 amend（overlay）目录的清单文件。它不属于板目录本身，而是放在独立的 overlay 目录中，用于在不修改原开发板的前提下，为目标开发板叠加或覆盖配置。

清单通过 ``apply:`` 显式列出要叠加的片段，可包含 ``board_devices.yaml`` / ``board_peripherals.yaml`` 片段、``sdkconfig.defaults.board``、``Kconfig.projbuild`` 以及 ``.c`` 源文件；只有列在 ``apply:`` 中的文件才会参与合并。

该 overlay 目录通过 ``-a/--amend`` 显式指定，或由 auto-amend 按板名自动发现。amend 的目录结构、``apply:`` 语义、覆盖规则与用法示例见 :doc:`/create-board/amend`。

setup_device.c 与 custom 自定义设备
----------------------------------------------

``setup_device.c`` 适用于纯 YAML 无法完整表达的板级初始化逻辑，例如 LCD 屏幕的特定复位时序、触摸芯片的工厂函数注册。该文件放在开发板目录下，BMGR 生成时会将其编译进 ``gen_bmgr_codes`` 组件。

为便于下游工程通过 ``-a/--amend`` 替换开发板的默认行为，建议遵循以下两条写法：

- 将对外暴露的工厂或钩子函数（例如 ``lcd_panel_factory_entry_t``、``lcd_touch_factory_entry_t``、``io_expander_factory_entry_t``）声明为弱符号 ``__attribute__((weak))``。amend 目录中提供同名强符号实现，即可在链接阶段替换开发板默认实现，无需修改原开发板源码。覆盖机制见 :doc:`/create-board/amend`。
- 将对芯片驱动头文件的 ``#include`` 用 ``__has_include`` 包裹，并在弱符号函数实现的外层做同样的判断。这样 amend 不仅可以替换函数本身，还可以通过移除对应组件依赖（或替换为其他芯片组件）让开发板默认实现自动消失，避免出现 amend 已接管某条工厂入口、而开发板内的旧实现因找不到头文件而编译报错的情况。

.. code-block:: c

   // setup_device.c：弱符号 + __has_include 的常见组合，参考 esp32_s3_korvo_2_3/setup_device.c
   #if __has_include(<esp_lcd_ili9341.h>)
   #include "esp_lcd_ili9341.h"

   __attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                                            const esp_lcd_panel_dev_config_t *cfg,
                                                            esp_lcd_panel_handle_t *ret_panel)
   {
       return esp_lcd_new_panel_ili9341(io, cfg, ret_panel);
   }
   #endif  /* __has_include(<esp_lcd_ili9341.h>) */

``custom`` 类型设备适合 BMGR 尚未内置支持的硬件，例如特定电源管理芯片或传感器。在 ``board_devices.yaml`` 中将 ``type`` 设为 ``custom``，BMGR 生成阶段会将 ``config:`` 下的字段展开为专属配置结构体，写入 ``components/gen_bmgr_codes/gen_board_device_custom.h``。若需要自定义初始化逻辑，在板目录下提供初始化/反初始化函数实现并用 ``CUSTOM_DEVICE_IMPLEMENT`` 宏注册，例如：

.. code-block:: c

   CUSTOM_DEVICE_IMPLEMENT(axp2101_power_manager,
                             cores3_power_manager_init,
                             cores3_power_manager_deinit);

配置结构体命名规则、类型推断表、注册宏用法及应用侧访问方式详见 :doc:`/references/devices/custom`；板级完整示例参考 ``boards/m5stack_cores3/power_manager.c``。
