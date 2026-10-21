配置扩展与运行时覆盖
==========================================================================

:link_to_translation:`en:[English]`

BMGR 支持修改开发板默认配置、让同一固件适配多个硬件版本，以及在初始化前调整设备或外设配置。各机制的使用时机和配置方法如下。

扩展机制
----------------

按需要修改的内容和修改时机选择机制：

.. list-table:: 按需求选择机制
   :header-rows: 1

   * - 使用场景
     - 推荐机制
     - 配置方式与生效时机
   * - 需要直接修改并覆盖默认配置，或是接入已知子板以及其他可复用的功能模块。
     - :ref:`生成期配置修改（amend） <board-amend>`
     - 将差异写入配置片段，生成时合并为一套配置，再构建固件。
   * - 同一开发板有多个硬件版本，需要同一固件兼容这些版本，且各版本差异能提前写入 YAML。
     - :ref:`运行时配置切换（board_revisions） <board-revisions-runtime>`
     - 保留基础配置并生成各版本差异；资源初始化前检测硬件，调用对应版本的 apply 函数。
   * - 需要在应用代码运行时调整设备或外设的配置，或使用自己的设备初始化函数。
     - :ref:`应用运行时覆盖 <app-runtime-overrides>`
     - 在目标资源初始化前，按名称调用 ``override_config`` 或 ``set_ops``。
   * - 设备种类或连接关系需要在运行时动态确定，构造新设备节点及其总线绑定，或移除已有资源，难以通过 YAML 预先描述。
     - :ref:`可写资源表与 C 节点 <writable-tables>`
     - 使用 ``--writable`` 生成可写表，由 C 代码维护资源节点、链接和所需容量。
   * - 需要添加所有使用该开发板的工程都需要在初始化硬件资源前执行的预初始化行为。
     - :ref:`板级 pre-init hook <board-pre-init-runtime>`
     - 将逻辑放在板级 ``esp_board_runtime_pre_init()`` 中，BMGR 在常规资源初始化前自动执行。

这些机制可以组合使用。``board_revisions`` 描述预先确定的版本差异，pre-init hook 是板级检测与适配的执行入口。可以在 hook 中选择版本，也可以调用覆盖 API 调整已有资源。

.. _board-amend:

生成期配置修改（\ ``-a/--amend``\ ）
--------------------------------------------------

amend 通过 ``board_amend.yaml`` 的 ``apply:`` 列表，将配置片段按顺序合并到所选开发板的基础配置中。它可以修改已有字段、追加设备或外设，也可以引入源码、Kconfig 和 sdkconfig 默认值。

通过 ``-a/--amend`` 指定 amend 目录，或按 :ref:`auto-amend` 的目录约定自动发现。合并在生成代码时完成，后续构建使用合并后的配置，不需要直接修改开发板目录中的原始 YAML 配置。目录结构和合并规则见 :doc:`/create-board/amend`。

使用方法参考如下：
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- 项目需要调整 LCD 参数、跳过某个设备的自动初始化，或增加项目专用的 sdkconfig 默认值时，可把差异写入 amend 片段，由项目的 ``board_amend.yaml`` 引用。
- 项目需要增加一个设备或外设，且生成代码前就能确定需要哪些资源时，可在 amend 中追加对应的 ``devices:`` 或 ``peripherals:`` 条目。
- 同一主板需要连接不同子板时，可为每个子板准备一个 amend 目录，生成时选择对应目录。`ESP32-S3-LCD-EV-Board 的 800 × 480 屏幕子板 <https://github.com/espressif/esp-board-manager/tree/main/esp_boards/esp32_s3_lcd_ev_board/sub_board_800_480_lcd>`_ 提供了配置片段和面板初始化源码。
- 多个开发板需要复用同一个功能模块时，可把模块配置和源码放在共享目录，由各开发板的 amend 通过 ``apply:`` 引用。`MAX30102 模块 <https://github.com/espressif/esp-health/blob/main/esp_health/examples/heart_rate_detect/components/amend/max30102/README_CN.md>`_ 复用目标板已有的 I2C 总线，并追加 INT GPIO 和自定义设备；接到其他板时，需要按实际接线修改 GPIO 和总线绑定。

示例工程同时支持多个开发板时，可为每个板名准备自动发现的 amend 目录。这样仍使用 ``idf.py bmgr -b <board>`` 选择开发板，无需每次传入 ``-a``，具体约定见 :ref:`auto-amend`。

.. _board-revisions-runtime:

运行时配置切换（\ ``board_revisions``\ ）
--------------------------------------------------

同一开发板有多个硬件版本，且需要由同一份板卡配置及固件识别并适配时，可在板卡根目录下的 ``board_amend.yaml`` 中声明 ``board_revisions``。基础配置和各版本差异都会参与生成与构建，运行时由板级代码选择要使用的版本。

``board_revisions`` 支持同时声明多个版本。各版本独立从基础配置（base）派生，并按列表顺序合并差异文件，后面的文件覆盖前面的同名字段。例如：

.. code-block:: yaml

   board_revisions:
     v1_2:
       - board_revision_diff_v1_2.yaml
     v1_3:
       - board_revision_diff_v1_2.yaml
       - board_revision_diff_v1_3.yaml

若新版本依赖前一版本的差异，必须显式列出所需的差异文件。上例中，v1_3 表示先应用 v1_2 的差异，再应用 v1_3 的差异；若只列出 ``board_revision_diff_v1_3.yaml``，则仅在 base 上应用该文件，不会自动继承 v1_2 的配置。

生成器为每个版本生成以下 C 文件，其中 ``<name>`` 是版本名，例如 ``v1_2``：

* ``gen_board_device_config_<name>.c``
* ``gen_board_periph_config_<name>.c``
* ``gen_board_revision_<name>.c``

生成的 ``gen_board_revisions.h`` 声明了 ``gen_board_revision_apply_<name>()`` 函数。板级代码检测硬件版本后，调用对应函数更新设备及外设配置表。仅声明 ``board_revisions`` 不会自动切换配置；未调用 apply 函数时，BMGR 使用基础配置。

revision 差异文件沿用 amend 的 YAML 合并规则，见 :doc:`/create-board/amend`。它还支持 ``unused: true``：表示该版本不使用某个资源，调用对应 apply 函数时会把该资源从有效资源表中移除。

``board_revisions`` 可与生成期 amend 组合使用。生成期 amend 在 revision 片段之后合并，对基础配置和所有版本生效，同一字段以应用 amend 的值为准。应用 amend 可以修改基础配置中已有的资源，但不能修改仅在 revision 中新增的资源。应用 amend 中的 ``gen_skip: true`` 会在生成时从基础配置和所有版本中排除该资源。

使用方法参考如下：
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

例如，开发板 v1.2 修改了 I2C 接线，或增加、移除了设备，可把这些差异写进 ``board_revision_diff_v1_2.yaml``，在 pre-init hook 中识别版本并调用 apply 函数。`ESP-Mosaico 的版本差异文件 <https://github.com/espressif/esp-board-manager/blob/main/esp_boards/esp_mosaico/board_revision_diff_v1_2.yaml>`_ 展示了这类配置。

板级代码的调用方式如下。``board_is_v1_2()`` 代表板级硬件检测逻辑，需要由板级代码实现：

.. code-block:: c

   #include "gen_board_revisions.h"

   esp_err_t esp_board_runtime_pre_init(void)
   {
       if (board_is_v1_2()) {
           return gen_board_revision_apply_v1_2();
       }
       return ESP_OK;
   }

.. _app-runtime-overrides:

应用运行时覆盖
--------------

应用可以在设备或外设初始化前，按资源名称修改配置，或替换初始化、反初始化函数。目标资源需要已经存在于 BMGR 的资源表中。

* 修改配置：``esp_board_device_override_config``、``esp_board_periph_override_config``。
* 替换初始化和反初始化函数：``esp_board_device_set_ops``、``esp_board_periph_set_ops``。
* 恢复配置或函数：对应的 ``restore_config``、``restore_ops`` API。

资源正在初始化或尚未释放时，修改和恢复操作会返回 ``ESP_ERR_INVALID_STATE``。init 失败且资源已清理时，可以重试 init。init 失败并留下非空句柄时，该资源后续 init/deinit 都会被拒绝；正常初始化成功后 deinit 失败，则可重试 deinit。具体场景见 :ref:`bmgr-init-failure`。

``override_config`` 会复制配置结构体，但不会复制结构体指针字段所引用的数据。这些数据在覆盖有效期间必须保持有效。完整 API 规则见 :doc:`/programming-guide/advanced-runtime`。

覆盖优先级与恢复
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

同一个资源可以有以下配置来源，BMGR 按优先级选择当前使用的配置：

* 基础配置：生成代码中的设备或外设配置。revision apply 或代码直接修改配置表后，以修改后的表为准。
* 板级覆盖（BOARD）：在 pre-init hook 中通过 ``override_config`` 或 ``set_ops`` 设置。
* 应用覆盖（APP）：应用通过同类 API 设置。

优先级为：基础配置 < BOARD < APP。初始化和反初始化函数也按这个顺序选择，与配置分别记录。因此，只修改配置时，初始化函数保持原来的选择；只替换函数时，配置保持原来的选择。

``restore_config`` 只清除 APP 配置覆盖。清除后，如果有 BOARD 配置覆盖，就使用 BOARD 配置；否则使用当前基础配置表中的配置。``restore_ops`` 对初始化和反初始化函数采用相同的恢复顺序。

例如，板级 hook 已经修改了某个设备的配置，应用又覆盖了同一设备。应用调用 ``restore_config`` 后会回到板级 hook 设置的配置。restore 不会撤销 revision apply 的修改，也不会撤销 C 代码对基础配置表的修改。反初始化设备或 BMGR 也不会清除已设置的覆盖。

使用方法参考如下：
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

如果参数需要在程序运行后根据应用需求计算，可在初始化目标资源前：

#. 调用配置查询 API，获取当前有效配置。
#. 复制配置结构体，修改需要调整的字段。
#. 调用对应的 ``override_config`` API，再初始化该资源。

如果应用需要用自己的初始化函数创建某个已有设备，则在初始化前调用 ``set_ops``，同时提供初始化和反初始化函数。

只影响当前应用的修改放在应用代码中。所有使用该开发板的工程都需要在硬件检测后应用的修改，放在 :ref:`board-pre-init-runtime` 中。

.. _writable-tables:

生成可写 descriptor 表与 C 节点
----------------------------------------

descriptor 是生成代码中的设备或外设描述，记录名称、类型和配置等信息。默认生成的 descriptor 表是只读的。需要在运行时用 C 代码新增、移除或重新链接资源节点时，使用 ``--writable`` 生成可写表：

.. code-block:: bash

   idf.py bmgr -b <board> --writable

``--writable`` 的短选项是 ``-w``。启用后，``g_esp_board_devices[]`` 和 ``g_esp_board_peripherals[]`` 会变为可写，C 代码可以修改节点中的 ``next`` 指针、名称、类型、``cfg``、``cfg_size`` 等字段。

应在 prepare 所属任务中、资源初始化前修改资源表；其他修改入口需要与初始化和清理操作串行执行。生成的 revision apply 函数会在内部检查资源状态，资源正在使用、初始化遗留资源或准备失败需要重启时返回 ``ESP_ERR_INVALID_STATE``，且不修改资源表。

单独使用 ``--writable`` 时，``ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM`` 和 ``ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM`` 默认分别等于生成配置中的设备和外设数量，没有为新增资源预留空间。需要新增节点时，应在项目 Kconfig 中把相应容量调高到“生成节点数 + 最大新增节点数”。

新增设备需要提供名称和类型匹配的 descriptor 节点与 handle 节点。新增外设需要 descriptor 节点，并通过 ``esp_board_periph_set_ops`` 为该外设设置操作。节点、配置和配置引用的数据都必须在使用期间保持有效。节点链接和初始化要求见 :doc:`/programming-guide/runtime-lifecycle`。

声明 ``board_revisions`` 时，生成器也会生成可写表，并按各版本所需的资源数量设置表容量下限。版本差异能写进 YAML 时，推荐通过 ``board_revisions`` 来描述并管理硬件差异，见 :ref:`board-revisions-runtime`。

.. _board-pre-init-runtime:

板级 pre-init hook
------------------

板级代码可以在 ``setup_device.c`` 等源文件中定义 ``esp_board_runtime_pre_init()``，替换 BMGR 的默认实现，在其中检测硬件并调整初始化所需的配置。

BMGR 在整体初始化、单个设备或外设初始化、配置或句柄查询，以及覆盖的设置与恢复等入口中，自动检查板级运行时准备状态。处于未准备状态时，BMGR 会先执行 hook 和运行时校验，成功后再继续调用方的操作。应用可直接使用这些 API，无需主动调用 ``esp_board_manager_prepare()`` 来触发 hook。

配置查询可以在资源初始化前使用。句柄查询也会触发准备，但不会自动初始化目标资源；目标资源尚未创建句柄时，查询会返回错误。

BMGR 会记录准备状态，避免重复调用 ``esp_board_runtime_pre_init()``。如果 hook 或运行时校验失败且没有遗留资源，BMGR 会立即恢复保存的资源表和 BOARD 层覆盖，回到未准备状态，后续调用可以重试。

Hook 可以初始化硬件资源，若 hook 返回失败，必须先自行反初始化它创建的硬件资源；BMGR 的运行时状态回滚不会代替硬件清理。Hook 中通过 ``override_config`` 或 ``set_ops`` 设置的覆盖记为 BOARD 层，优先级和恢复规则见 :ref:`app-runtime-overrides`。

如果 prepare 失败时仍有资源未释放，BMGR 会进入 ``FAILED``。此后所有资源的正常运行时操作都会被拒绝，包括之前初始化成功的资源，直到系统重启。当前表和所需配置仍保留，旧表备份被释放。框架会打印 ``restart required`` 日志，不会自动重启，由应用决定是否重启。场景表和示例见 :ref:`bmgr-prepare-failure`。板级 revision 应在资源初始化前应用。
