代码生成与构建集成
==========================================================================

:link_to_translation:`en:[English]`

BMGR 在编译前先将开发板的 YAML 描述解析为一组明确的配置源码和构建输入，``idf.py bmgr`` 是面向用户的入口命令。

执行 ``idf.py bmgr -b <board>`` 时，BMGR 依次完成以下工作：

1. 扫描开发板目录，收集默认目录、自定义目录和组件目录中的候选开发板。
2. 根据命令行参数（名称或索引）确定当前选中的开发板。
3. 定位该开发板对应的 ``board_peripherals.yaml``、``board_devices.yaml``、``board_info.yaml``、``sdkconfig.defaults.board``、``Kconfig.projbuild``。
4. 解析外设和设备，生成对应的配置结构、句柄表和板级元数据。
5. 生成当前开发板相关的 ``Kconfig.projbuild``，追加板级目录下的 ``Kconfig.projbuild``。
6. 生成 ``board_manager.defaults``，将板级默认配置和当前开发板的能力符号接入构建。
7. 在 ``components/gen_bmgr_codes`` 下输出参与编译的源码、构建文件和工具摘要文件。

在 BMGR 的模型中，开发板的配置代码来自 YAML 文件描述以及脚本的解析与生成流程，而非通过 ``menuconfig`` 手动勾选设备或外设。``components/gen_bmgr_codes`` 不是缓存，也不是仅供查看的中间产物，而是会参与 ESP-IDF 构建的实际组件。

生成产物说明
----------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - 文件
     - 说明
   * - ``gen_board_periph_config.c``
     - 基于 ``board_peripherals.yaml`` 生成的外设配置结构体定义。
   * - ``gen_board_periph_handles.c``
     - 生成的 peripheral 句柄入口、类型映射和初始化函数挂接点。
   * - ``gen_board_device_config.c``
     - 基于 ``board_devices.yaml`` 生成的 device 配置结构体定义。
   * - ``gen_board_device_handles.c``
     - 生成的 device 句柄入口、初始化/反初始化函数映射和设备链表。
   * - ``gen_board_info.c``
     - 生成的板级元数据，例如板名、芯片、版本、描述和厂商。
   * - ``gen_board_device_custom.h``
     - ``type: custom`` 设备的配置 struct 定义，供应用侧 ``init`` / ``deinit`` 使用。
   * - ``board_manager.defaults``
     - 当前板配置的 ``sdkconfig`` 默认项，以及对应的设备/外设能力符号。
   * - ``Kconfig.projbuild``
     - 当前板相关的 Kconfig 符号定义和选择入口，把板级能力投射进工程侧配置系统。
   * - ``idf_component.yml``
     - 当前板对应的组件依赖描述，设备 ``dependencies`` 反映到这里。
   * - ``gen_board_metadata.yaml``
     - 面向工具和排查的统一板级摘要，便于查看当前板有哪些设备、外设、组件依赖和占用 IO。

BMGR 不通过在 ``menuconfig`` 中逐项手动选择设备或外设来组织编译，而是先根据板级 YAML 生成 ``board_manager.defaults``，其中包含的板级能力宏在后续构建过程中生效。执行 ``idf.py`` 命令时，BMGR 将这些配置注入 ``sdkconfig``，驱动 BMGR 的条件编译。

排查时建议按现象分流：

- 当表现为功能与预期不一致时，优先检查 ``gen_board_periph_config.c`` 与 ``gen_board_device_config.c``。
- 当表现为编译失败或依赖求解异常时，优先检查 ``components/gen_bmgr_codes`` 目录下生成物是否完整、``board_manager.defaults`` 中的能力符号是否符合预期，以及 ``sdkconfig`` 与 ``board_manager.defaults`` 是否一致。

board_manager.defaults 与 Kconfig.projbuild 的合并与覆盖
----------------------------------------------------------------------

``board_manager.defaults`` 是 BMGR 接入 ESP-IDF 编译配置的板级 defaults 文件。BMGR 根据当前开发板的 YAML 生成 ``CONFIG_ESP_BOARD_PERIPH_*_SUPPORT``、``CONFIG_ESP_BOARD_DEV_*_SUPPORT``、``CONFIG_ESP_BOARD_DEV_<DEV>_SUB_<SUB>_SUPPORT`` 等能力符号，并通过该文件参与后续构建，控制设备、外设和设备子类型相关代码是否进入编译。因此，不建议用户在工程 ``sdkconfig.defaults`` 中手写这些 BMGR 管理的能力符号；板级差异应放在 ``sdkconfig.defaults.board`` 或 amend 中统一管理。

执行 ``idf.py bmgr -b <board> [-a <amend>]`` 时，BMGR 按以下顺序组装出最终的 ``board_manager.defaults`` 与 ``Kconfig.projbuild``，\ **后写覆盖前写**\ ：

1. BMGR 自动生成段：``CONFIG_IDF_TARGET``、``CONFIG_ESP_BOARD_<BOARD>=y``、``CONFIG_ESP_BOARD_NAME``，以及根据 YAML 解析得到的 ``CONFIG_ESP_BOARD_PERIPH_*_SUPPORT``、``CONFIG_ESP_BOARD_DEV_*_SUPPORT``、``CONFIG_ESP_BOARD_DEV_<DEV>_SUB_<SUB>_SUPPORT`` 能力符号。
2. 板目录下的 ``sdkconfig.defaults.board`` 与 ``Kconfig.projbuild``\ （如存在）。
3. ``board_amend.yaml`` 清单 ``apply:`` 中列出的 ``sdkconfig.defaults.board`` 与 ``Kconfig.projbuild`` 片段，\ **严格按 apply: 中出现的先后顺序**\ 逐一追加。如需让某个片段覆盖其他 amend 片段，请将其写在 ``apply:`` 列表更靠后的位置。

当 ``board_manager.defaults`` 内出现同名 ``CONFIG_*`` 冲突时，BMGR 保留最后一次出现的值，并将更早的同名行改写为形如 ``# BMGR_CONFIG_OVERRIDE by <section>: <原行>`` 的注释，便于排查覆盖关系。``Kconfig.projbuild`` 为纯文本顺序串联，每段前会插入 ``# --- <label>: <path> ---`` 标记说明来源。

.. note::

   ``board_amend.yaml`` 清单中的 ``sdkconfig.defaults.board`` 与 ``Kconfig.projbuild`` 必须显式列在 ``apply:`` 中才会参与合并。放在 amend 目录但未列出的文件会被忽略并输出 INFO 日志。详细规则见 :doc:`/create-board/amend`。

构建集成：SDKCONFIG_DEFAULTS 优先级
----------------------------------------------------------------------

构建时，ESP-IDF 读取一组 ``SDKCONFIG_DEFAULTS``\ 。这组文件由 ``SDKCONFIG_DEFAULTS`` 环境变量或 CMake 变量声明。

当工程 ``sdkconfig`` 不存在时，BMGR 通过 ``idf.py`` 的全局回调（global callback）组装 ``SDKCONFIG_DEFAULTS``\ ，后续的编译行为将按列表顺序读取 defaults，\ **靠后的文件优先级更高**\ ：

1. 工程 ``sdkconfig.defaults``\ （最低）
2. ``components/gen_bmgr_codes/board_manager.defaults``\ （板级，含 amend）
3. 环境变量 ``SDKCONFIG_DEFAULTS``
4. ``-D SDKCONFIG_DEFAULTS``\ （最高）

因此，\ **板级默认值始终高于工程通用** ``sdkconfig.defaults``\ 。工程 defaults 不能覆盖普通板级默认项。如需板级差异化覆盖，请使用 amend（auto-amend 或 ``-a/--amend``）。

.. warning::

   板级硬件或变体覆盖请使用 amend，不要依赖工程 ``sdkconfig.defaults`` 覆盖开发板配置。工程 ``sdkconfig.defaults`` 适合写与开发板不相关的项目级配置；CI 或临时覆盖请使用环境变量或 ``-D SDKCONFIG_DEFAULTS=``。板级特有的常规 sdkconfig 项（PSRAM、Flash、partition、应用层开关等）请放到板目录下的 ``sdkconfig.defaults.board``，随开发板统一管理。

切换开发板时，``idf.py bmgr -b <other_board>`` 会重新生成 ``board_manager.defaults`` 与 ``Kconfig.projbuild``，并备份与清理旧 ``sdkconfig`` 中由上一块开发板写入的能力宏。

.. note::

   除显式 ``-a/--amend`` 外，BMGR 还支持 **auto-amend（自动 amend）**\ ：在与开发板相同的扫描路径（含 ``-c`` 路径）下，自动查找名字等于当前选中板名、且包含 ``board_amend.yaml``\ 、且本身不是完整开发板目录的目录（约定 ``<scan_root>/<board_name>/board_amend.yaml``），并自动应用为该板的 amend。``-c/--customer-path`` 现支持分号分隔的多路径（如 ``-c "overlays_a;overlays_b"``），后者优先级更高。这样把各板的 overlay 放在同一根目录下并统一指定 ``-c``\ ，所有开发板即可共用相同的命令。显式 ``-a/--amend`` 优先级最高，会覆盖所有 auto-amend；设置 ``ESP_BOARD_MANAGER_DISABLE_AUTO_AMEND=1`` 可关闭自动发现。详见 :doc:`/create-board/amend`。
