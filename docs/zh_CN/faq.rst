FAQ
============

:link_to_translation:`en:[English]`

FAQ 汇总 BMGR 常见报错、排查入口和处理方法。遇到问题时，先判断问题发生在生成、编译还是运行时阶段，再按具体现象定位。

- **生成阶段**：开发板选择错误、YAML 不完整、依赖描述不正确、生成产物缺失。
- **编译阶段**：缺宏、缺组件、缺生成文件、链接失败。
- **运行时阶段**：初始化顺序、供电、回调、配置生效时机异常。

通用排查顺序：先确认当前选定的开发板，以及最近一次 ``idf.py bmgr`` 是否成功执行；再检查 ``components/gen_bmgr_codes`` 下的 ``gen_board_*.c``、``board_manager.defaults``、``Kconfig.projbuild``、``gen_board_metadata.yaml`` 是否齐全；然后确认生成结果是否实际参与编译与链接；最后排查运行时初始化、句柄获取、供电与时序问题。

生成阶段
-----------

idf.py bmgr 或 idf.py gen-bmgr-config 命令未找到
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**可能原因**

- ESP-IDF 未发现 ``esp_board_manager`` 组件中的 ``idf_ext.py``。
- ESP-IDF v6.0 之前的版本中，``IDF_EXTRA_ACTIONS_PATH`` 未指向正确路径。
- 当前 shell 未重新加载 ESP-IDF 环境。

**推荐动作**

1. 推荐使用 ``esp-bmgr-assist``，省去手动配置 ``IDF_EXTRA_ACTIONS_PATH``。在已激活的 ESP-IDF Python 环境中执行 ``pip install esp-bmgr-assist``；当提示需要更新时，执行 ``pip install --upgrade esp-bmgr-assist``。
2. 如果已安装 ``esp-bmgr-assist`` 后仍然报错，先查看命令输出中的具体错误信息。常见原因包括：执行命令的目录不是标准 ESP-IDF 工程目录，或依赖组件存在芯片限制导致依赖解析失败。详细排查方式见 :doc:`/tools/esp-bmgr-assist`。
3. 如需手动设置 ``IDF_EXTRA_ACTIONS_PATH``，可使用以下命令检查路径配置是否正确。输出应包含 ``esp_board_manager`` 组件目录；多个路径使用 ``;`` 分隔。

   .. code-block:: text

      # Linux / macOS
      echo $IDF_EXTRA_ACTIONS_PATH

      # Windows PowerShell
      echo $env:IDF_EXTRA_ACTIONS_PATH

      # Windows CMD
      echo %IDF_EXTRA_ACTIONS_PATH%

找不到 esp_board_manager 组件路径
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**可能原因**

- 主组件清单 ``idf_component.yml`` 未声明依赖。
- 声明依赖后未执行任何能触发组件管理器下载的命令。

**推荐动作**

1. 检查项目主 ``idf_component.yml``，确认包含 ``espressif/esp_board_manager`` 依赖项。
2. 添加依赖后执行 ``idf.py menuconfig`` 或 ``idf.py build``，组件管理器会将 ``esp_board_manager`` 下载到 ``<project>/managed_components/``。

开发板名称找不到或拼写错误
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**可能原因**

- 当前工程扫描到的开发板中不包含该名称。
- 板目录命名包含大写字母、中划线等不允许的字符，扫描阶段被忽略。
- 当前版本（0.5.x）中，官方开发板随 BMGR 内置，无需额外声明依赖。从 0.6 起，官方开发板已迁移至独立组件，部分非官方开发板的使用需手动声明依赖，具体组件见 :doc:`/references/boards/index`。

**推荐动作**

1. 执行 ``idf.py bmgr -l``，查看实际识别到的所有开发板及其来源标记。
2. 开发板名称必须仅包含小写字母、数字、下划线，并与 ``board_info.yaml`` 中的 ``board`` 字段保持一致。
3. 临时验证某个外部开发板目录时，可直接传入路径：``idf.py bmgr -b /abs/path/to/board``。

修改 YAML 后生成结果未变化
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**可能原因**

- 未重新执行 ``idf.py bmgr -b <board>``；BMGR 不会因 YAML 修改自动重新生成。
- 修改的是 amend 目录下的文件，但未在 ``board_amend.yaml`` 的 ``apply:`` 中列出。
- 修改的是其他开发板的 YAML，当前选中的并非该开发板，或修改的文件所在目录被高优先级同名开发板覆盖，优先级关系见 :doc:`/programming-guide/board-path-priority`。

**推荐动作**

1. 重新执行 ``idf.py bmgr -b <board> [-a <amend>]``，查看 ``components/gen_bmgr_codes`` 是否被刷新。
2. 检查 amend 的 ``apply:`` 列表是否包含改动的文件。未列出的文件 BMGR 仅输出 INFO 日志，不会参与合并。
3. 通过 ``idf.py bmgr -b <board>`` 确认当前选中的开发板实际目录，可以查看日志中的 `Board path: <path>` 信息。

SoC 能力校验失败
~~~~~~~~~~~~~~~~~~~

**可能原因**

- 已选开发板配置与 ``board_info.yaml`` 中的芯片不匹配。
- YAML 使用了目标芯片不支持的设备或外设。
- 配置超过 SoC 硬件限制，例如 I2C 或 I2S instance 数量。
- GPIO 超出有效范围，或与其他已配置功能冲突。

**推荐动作**

1. 检查已选开发板 YAML，确认 ``board_info.yaml`` 使用的是预期芯片。
2. 对照 ``private_inc/soc_capability_catalog/`` 下的目标芯片能力 catalog，或核对对应 ESP-IDF ``soc_caps.h`` 中的宏定义。
3. 若板级配置有意使用暂未建模的硬件能力，或者确认错误为误报，可临时跳过该校验：

   .. code-block:: bash

      idf.py bmgr -b <board> --skip-soc-capability-check
      export ESP_BOARD_MANAGER_SKIP_SOC_CAPABILITY_CHECK=1

   该跳过选项适合临时验证。发布前仍应修正开发板 YAML 或能力 catalog。

编译阶段
-----------

undefined reference to g_esp_board_*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

典型符号：``g_esp_board_devices``、``g_esp_board_device_handles``、``g_esp_board_peripherals``。

**可能原因**

1. ``idf.py bmgr -b <board>`` 未完整执行成功，``components/gen_bmgr_codes`` 缺少必要文件。除生成的 ``.c`` 与 ``.h`` 外，还\ **必须**\ 包含 ``CMakeLists.txt``\ （以及通常一同生成的 ``idf_component.yml``、``board_manager.defaults``）。若目录中仅存部分 ``.c`` 或缺少 ``CMakeLists.txt``，ESP-IDF 不会将该目录注册为组件，生成源码不会进入编译，链接阶段会出现该类未定义引用。
2. 工程启用了最小化或裁剪构建，例如 ``idf_build_set_property(MINIMAL_BUILD ON)`` 或 ``set(COMPONENTS main)``。前者仅保留最小公共组件，后者仅构建显式列出的组件及其依赖。在这两种情况下，若 ``gen_bmgr_codes`` 未显式加入构建范围，板级生成代码不会参与编译。

**推荐动作**

1. 首先确认 ``components/gen_bmgr_codes`` 下文件齐全。最少应包含：``gen_board_info.c``、``gen_board_device_config.c``、``gen_board_device_handles.c``、``gen_board_periph_config.c``、``gen_board_periph_handles.c``、``gen_board_device_custom.h``、``CMakeLists.txt``、``idf_component.yml``、``board_manager.defaults``、``Kconfig.projbuild``。
2. 若不齐全，重新执行 ``idf.py bmgr -b <board>``。
3. 在裁剪构建场景下，将 ``gen_bmgr_codes`` 显式加入构建范围（在 ``set(COMPONENTS ...)`` 列表中加入该组件，或确认依赖链可传递到该组件）。

找不到头文件、宏或生成符号
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

许多 BMGR 问题的根因并非在应用层，而在 YAML、生成产物或默认配置未对齐。建议按以下顺序对照检查，避免仅在业务代码中排查：

1. ``components/gen_bmgr_codes`` 是否正确生成且文件齐全（同上一条）。
2. ``gen_board_metadata.yaml``：解析得到的 device 与 peripheral 是否符合预期。
3. ``gen_board_device_config.c`` 与 ``gen_board_periph_config.c``：具体字段值是否符合预期。
4. ``board_manager.defaults``：``CONFIG_ESP_BOARD_DEV_*_SUPPORT``、``CONFIG_ESP_BOARD_PERIPH_*_SUPPORT`` 与 ``CONFIG_ESP_BOARD_DEV_*_SUB_*_SUPPORT`` 等能力宏是否齐全。
5. ``Kconfig.projbuild``：当前选中的开发板对应的 ``CONFIG_ESP_BOARD_<BOARD>`` 是否被声明。
6. ``sdkconfig``：上述能力宏是否实际写入 sdkconfig。

组件依赖解析失败 / 版本求解失败
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

典型报错：

.. code-block:: text

   ERROR: Because project depends on xxxxx which
   doesn't match any versions, version solving failed.

.. code-block:: text

   Failed to resolve component 'esp_board_manager' required by component
     'gen_bmgr_codes': unknown name.

**可能原因**

- 上次执行后 ``gen_bmgr_codes`` 中的 ``idf_component.yml`` 残留了已不再适用的依赖。
- YAML 中 ``dependencies`` 的组件名或版本约束错误。
- 依赖来源（仓库组件、本地组件、私有组件）不一致。例如某个依赖原本是组件仓库包，已替换为本地路径但同名依赖未清理。

**推荐动作**

1. 先使用 ``idf.py bmgr -x``\ （等价于旧命令 ``idf.py gen-bmgr-config -x``）清理生成产物。该命令会删除生成的 ``.c`` 与 ``.h``，重置 ``gen_bmgr_codes/CMakeLists.txt`` 与 ``idf_component.yml``，并移除 ``board_manager.defaults``。
2. 再执行 ``idf.py bmgr -b <board>`` 重新生成。
3. 若仍报错，参照 `IDF Component Manager Manifest 文档 <https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/manifest_file.html#dependencies>`_ 检查 device YAML 中 ``dependencies`` 字段的写法。

板卡组件与 Board Manager 版本不兼容
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**现象**

板卡组件与 Board Manager 版本不匹配时，可能出现以下问题：

- 使用 ``0.6.0`` 版本的板卡组件时，旧版 Board Manager 不识别 ``i2c_name``、``spi_name``、``pa_name`` 等语义化外设引用字段，导致板级 YAML 解析失败。
- 使用已发布的 ``esp_boards 0.5.4``、``esp_friends_boards 0.5.3`` 和 ``m5stack_boards 0.5.4`` 时，旧版 Board Manager 不识别 ``dev_audio_codec`` 的 PA 配置字段 ``pa_active_level`` （现已弃用），导致 PA 配置解析失败。

**推荐动作**

如果工程必须使用旧版 Board Manager，请在工程的 ``idf_component.yml`` 中手动添加实际使用的板卡组件，并指定匹配版本。

**版本关系**

- ``esp_boards 0.6.0``、``esp_friends_boards 0.6.0`` 和 ``m5stack_boards 0.6.0`` 要求配合 ``esp_board_manager >=0.7.1`` 使用。
- ``esp_boards 0.5.4``、``esp_friends_boards 0.5.3`` 和 ``m5stack_boards 0.5.4`` 要求配合 ``esp_board_manager >=0.7.0`` 使用。

运行时阶段
--------------

切换开发板后行为异常或 sdkconfig 不一致
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**可能原因**

- 通过 ``idf.py menuconfig`` 或手工修改 ``sdkconfig`` 完成切换，未经 ``idf.py bmgr -b <board>`` 触发清理与重新生成。
- 上一块开发板的能力宏残留在 ``sdkconfig`` 中。
- 在工程 ``sdkconfig.defaults`` 中手写了 ``CONFIG_ESP_BOARD_DEV_*_SUPPORT``、``CONFIG_ESP_BOARD_PERIPH_*_SUPPORT``、``CONFIG_ESP_BOARD_DEV_*_SUB_*_SUPPORT`` 等 BMGR 管理的能力符号，干扰了切换开发板的逻辑。

**推荐动作**

1. 切换开发板必须使用 ``idf.py bmgr -b <other_board>`` 或等价的脚本入口，不应使用 ``idf.py menuconfig`` 进行切换。BMGR 在切换开发板时会自动将旧 ``sdkconfig`` 备份为 ``components/gen_bmgr_codes/sdkconfig.bmgr_board.old`` 并删除原文件，避免旧开发板的 ``CONFIG_IDF_TARGET`` 与设备使能配置残留。
2. 不要在工程 ``sdkconfig.defaults`` 中手写 BMGR 的设备、外设或设备子类型能力符号；这些符号应仅来自 BMGR 生成的 ``board_manager.defaults``。板级特有的常规 sdkconfig 项（PSRAM、Flash、partition 等）放到板目录下的 ``sdkconfig.defaults.board``。
3. 如需回到当前开发板的默认值，可删除工程 ``sdkconfig``，重新执行 ``idf.py build`` 让 ``board_manager.defaults`` 再次生效。

修改配置后运行结果无变化
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**可能原因**

- 修改的是 YAML ，修改后未执行 ``idf.py bmgr``。

**推荐动作**

1. 如果修改的是 YAML，先重新执行 ``idf.py bmgr -b <board>``，确认 ``components/gen_bmgr_codes`` 已刷新。
2. 如果修改的是 ``gen_board_device_config.c`` 和 ``gen_board_periph_config.c``，请在测试通过后将修改同步回 YAML，避免下次 ``idf.py bmgr`` 覆盖。

运行时无法获取句柄或配置
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**可能原因**

- 当前开发板未启用该设备或外设，YAML 中未声明。
- ``init_skip: true`` 导致 BMGR 自动初始化阶段跳过该设备。
- :cpp:func:`esp_board_manager_init` 未成功执行（先前有报错或返回 ESP_FAIL）。
- ``name`` 拼写错误，与 YAML 中不一致。

**推荐动作**

1. 启动后调用 :cpp:func:`esp_board_manager_print`，确认期望的设备与外设均已正确登记并初始化。
2. 查看启动日志，确认 :cpp:func:`esp_board_manager_init` 是否返回成功；任何 device 初始化失败都会留下错误日志。
3. 对显式声明 ``init_skip: true`` 的设备，需在业务时机调用 :cpp:func:`esp_board_manager_init_device_by_name` 后才能获取句柄。

设备功能异常（电源 / 复位 / 时序）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**可能原因**

- 板级 ``setup_device.c`` 中工厂函数或上电时序实现错误。
- ``power_ctrl_device`` 引用错误，导致目标设备初始化前未实际上电。
- YAML 中关键 ``[IO]`` 或 ``[TO_BE_CONFIRMED]`` 字段沿用模板默认值，未按原理图填写。

**推荐动作**

1. 对照原理图与器件资料，逐项核对该设备相关 YAML 中标记 ``[IO]`` 与 ``[TO_BE_CONFIRMED]`` 的字段。
2. 对于 LCD 显示屏、触摸屏、摄像头等需要芯片专属时序的设备，确认板级 ``setup_device.c`` 中对应工厂函数（``lcd_panel_factory_entry_t``、``lcd_touch_factory_entry_t`` 等）已正确实现。
3. 当设备由其他设备供电时，确认 device 条目中已正确填写 ``power_ctrl_device``，且被引用的 ``power_ctrl`` 设备本身能成功初始化。
