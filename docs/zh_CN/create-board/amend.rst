使用 -a/--amend
==========================================================================

:link_to_translation:`en:[English]`

在已有开发板上做少量差异（例如修改某个引脚、替换触摸芯片）或是需要追加开发板默认未包含的设备时，无需复制整份开发板目录。准备一个 **amend 目录**，其中放置一份 ``board_amend.yaml`` 清单，再通过 ``-a/--amend <dir>`` 在生成时将变更打补丁到已选开发板之上：

.. code-block:: bash

   # amend 目录是绝对路径或相对路径，其中必须包含 board_amend.yaml
   idf.py bmgr -b esp32_s3_korvo_2_3 -a path/to/my_amend

   # amend 目录放在所选开发板目录下时，可直接传入子目录名
   # 例如：esp_boards/esp32_s3_lcd_ev_board/sub_board_800_480_lcd
   idf.py bmgr -b esp32_s3_lcd_ev_board -a sub_board_800_480_lcd

``-a`` 接受绝对路径或相对路径；相对路径以当前工作目录（``cwd``）为基准。 **若 amend 目录放在所选开发板目录下，也可以只传入子目录名，BMGR 会在该开发板目录内查找对应子目录。**

同一主板的不同子板、屏幕模组或小范围硬件变体，建议统一放在该开发板目录下（例如 ``esp_boards/esp32_s3_lcd_ev_board/sub_board_800_480_lcd/``），使用时只传入子目录名，不受工程所在路径的影响。

.. note::

   除了上面显式的 ``-a``，BMGR 还支持\ **按开发板名自动发现并应用 amend**\ （auto-amend），适合在多板示例工程中让所有开发板共用同一条命令。详见下文 :ref:`auto-amend` 一节。

amend 目录的基本结构：

.. code-block:: text

   my_amend/
     board_amend.yaml          # 必需：清单文件
     tweak.yaml                # YAML 片段，需在 apply: 中列出
     extra_setup.c             # 可选源码，需在 apply: 中列出
     sdkconfig.defaults.board  # 可选，需在 apply: 中列出

清单文件
--------------

``board_amend.yaml`` 格式：

.. code-block:: yaml

   version: "1.0"
   description: "Add external sensor power control"

   apply:                        # 有序列表，后者覆盖前者
     - tweak.yaml
     - extra_setup.c
     - sdkconfig.defaults.board

amend 根下的文件（包括 ``sdkconfig.defaults.board``、``Kconfig.projbuild``）\ **必须显式列在 apply: 中才会生效**。仅放置但未列出的文件会被忽略并输出 info 日志。目录项不被支持，子目录下的文件需写出完整的相对路径，例如 ``pack/extra.yaml``。

``apply:`` 中每一项支持的路径写法：

- **相对路径**：相对 ``board_amend.yaml`` 所在目录解析，例如 ``tweak.yaml``、``pack/extra.yaml``、``../shared/extra_setup.c``。
- **绝对路径**：直接使用，例如 ``/abs/path/to/extra_setup.c``。

无论哪种写法，均不支持目录项。子目录下的文件必须按文件名完整展开列出（``pack/extra.yaml``，而非 ``pack``）。

YAML 片段合并规则
----------------------

每个 YAML 片段顶层必须包含 ``devices:`` 或 ``peripherals:``。合并按 ``apply:`` 顺序进行，同名 device 或 peripheral 做字段级合并（``config`` 采用深度合并），不存在的名称追加到列表末尾。

.. code-block:: yaml

   # tweak.yaml 示例：新增一个外设和对应电源控制设备
   peripherals:
     - name: gpio_sensor_power
       type: gpio
       role: io
       version: default
       config:
         pin: 4                  # [IO]
         mode: GPIO_MODE_OUTPUT

   devices:
     - name: sensor_power
       type: power_ctrl
       sub_type: gpio
       version: default
       peripherals:
         - name: gpio_sensor_power
           active_level: 1

源码文件覆盖
--------------

``apply:`` 中的 ``.c``、``.cpp``、``.cc``、``.cxx``、``.S`` 文件会编译进生成组件。生成组件设置了 ``WHOLE_ARCHIVE``，因此 amend 提供的强符号会覆盖 base 开发板中同名的弱符号函数。典型用法是重写 ``setup_device.c`` 中的初始化钩子。基础开发板（base）的工厂函数、``custom`` 实现以及依赖芯片驱动的 ``DEVICE_EXTRA_FUNC_REGISTER`` 钩子应统一采用 ``__attribute__((weak))``（适用于具有外部链接的工厂入口）与 ``__has_include`` 组合写法，便于 amend 替换，也便于下游用 ``gen_skip`` 关闭设备后仍能编译。详见 :doc:`/programming-guide/board-directory` 的 ``setup_device.c`` 一节。

跨板复用功能模块
------------------

``apply:`` 支持相对路径，以 ``board_amend.yaml`` 所在目录为基准，可以跳出 amend 目录引用工程其他位置的文件。利用这个特性，可以将通用的外设和设备配置拆成独立的 YAML 与源码片段，集中存放在共享目录下，再由不同开发板的 amend 按需引用。这相当于用可复用的功能模块组合出完整的板级配置。

例如将气体传感器的适配拆成独立片段，放在工程共享目录：

.. code-block:: text

   sensors/
     gas_sensor/
       gas_sensor.yaml   # 外设与设备声明
       gas_sensor.c      # 初始化实现（可选）

某块开发板的 ``board_amend.yaml`` 通过相对路径引用：

.. code-block:: yaml

   version: "1.0"
   description: "Board A: base board + gas sensor"

   apply:
     - ../sensors/gas_sensor/gas_sensor.yaml
     - ../sensors/gas_sensor/gas_sensor.c

需要相同传感器的另一块开发板，直接复用同一批文件，无需重新维护：

.. code-block:: yaml

   version: "1.0"
   description: "Board B: base board + gas sensor + extra periph"

   apply:
     - ../sensors/gas_sensor/gas_sensor.yaml
     - ../sensors/gas_sensor/gas_sensor.c
     - extra_periph.yaml   # 本板特有的额外调整

随着共享目录中功能模块的积累，新开发板的适配可以越来越多地依赖已有模块：在 ``apply:`` 中组合所需片段，而非从头编写重复的 YAML 内容。

.. _auto-amend:

自动 amend（auto-amend）
--------------------------

除了显式 ``-a``，BMGR 还支持\ **按开发板名自动发现并应用 amend**\ 。在扫描路径下（包括 ``-c/--customer-path`` 指定的目录），若存在一个\ **目录名与所选开发板同名、包含** ``board_amend.yaml`` **、且本身不是完整开发板目录**\ 的目录，BMGR 会自动把它作为该板的 amend 应用，无需传 ``-a``。

约定的目录结构为 ``<扫描根>/<开发板名>/board_amend.yaml``：

.. code-block:: text

   board_overlays/                       # 用 -c 指向这里
     esp32_s3_box_3/
       board_amend.yaml
       box_tweak.yaml
     esp32_p4_function_ev_board/
       board_amend.yaml
       p4_tweak.yaml

自动 amend 目录内 ``board_amend.yaml`` 的写法、片段合并规则、源码覆盖与跨板复用，与显式 ``-a`` 一致（``apply:`` 仍是唯一真相）。

要点：

- ``-c/--customer-path`` 支持用分号分隔的多个路径，例如 ``-c "overlays_a;overlays_b"``，按顺序扫描，靠后的路径优先级更高。
- 显式 ``-a`` 与自动 amend 可以叠加，显式 ``-a`` 优先级最高（最后应用，覆盖自动 amend）。
- 完整开发板目录即使包含 ``board_amend.yaml``，也会被当作开发板处理，不会被当作 auto-amend 应用到自身。
- 自动发现最多向扫描根下递归 3 层。
- 设置环境变量 ``ESP_BOARD_MANAGER_DISABLE_AUTO_AMEND=1`` 可关闭自动发现，仅保留显式 ``-a``。

在 demo 中使用 amend 的两种方式
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

编写一个支持多块开发板的示例工程时，自动 amend 的主要价值是\ **让所有开发板共用同一条命令**。

**方式一：所有开发板共用同一个 amend**

当所有目标板需要打同一套补丁（例如统一打开某项调试配置、追加同一个外接模块）时，用显式 ``-a`` 指向同一个 amend 目录即可，各板命令中 amend 部分保持一致：

.. code-block:: bash

   idf.py bmgr -b esp32_s3_box_3             -a path/to/common_amend
   idf.py bmgr -b esp32_p4_function_ev_board -a path/to/common_amend

**方式二：不同开发板用不同 amend，但命令保持一致**

当每块开发板需要各自不同的补丁时，过去只能给每块板写不同的 ``-a`` 路径，命令各不相同，容易混乱。改用自动 amend 后，只需把各板的 overlay 统一收在一个根目录下，按板名建子目录：

.. code-block:: text

   board_overlays/
     esp32_s3_box_3/board_amend.yaml                # box 专属补丁
     esp32_p4_function_ev_board/board_amend.yaml    # p4 专属补丁

然后所有开发板使用\ **相同**\ 的命令，只改 ``-b`` 的板名，``-c`` 始终指向同一个 overlay 根，BMGR 会按板名自动匹配并应用对应的 amend：

.. code-block:: bash

   idf.py bmgr -b esp32_s3_box_3             -c board_overlays
   idf.py bmgr -b esp32_p4_function_ev_board -c board_overlays

没有为某块板准备 overlay 时，该命令会正常生成 base 板，不会报错。这样在 CI 或多板演示脚本里，只需对板名列表循环执行同一条命令，无需为每块板维护不同的 amend 参数。
