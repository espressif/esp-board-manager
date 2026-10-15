运行时生命周期
==========================================================================

:link_to_translation:`en:[English]`

从运行时角度看，BMGR 固定了一组顺序：先初始化哪些外设、再初始化哪些设备、如何获取对应的句柄与配置。

初始化入口与运行时访问
----------------------------

:cpp:func:`esp_board_manager_init` 是整板初始化入口，用于按生成的板级描述完成自动初始化。对应地，:cpp:func:`esp_board_manager_deinit` 按相反方向释放运行时对象。

:cpp:func:`esp_board_manager_init_device_by_name` 用于按 device 名称初始化单个设备，常用于 ``init_skip``、延迟初始化或按业务条件打开某个设备的场景。

应用可通过 :cpp:func:`esp_board_manager_get_device_config` 和 :cpp:func:`esp_board_manager_get_periph_config` 获取有效配置结构体。没有 APP 或 BOARD 覆盖时，配置来自当前 descriptor；如果 descriptor 未被 board revision 或 C 代码修改，该值就是 YAML 解析生成的 C 结构。存在覆盖时，优先级见 :doc:`/programming-guide/advanced-runtime`。

应用可通过 :cpp:func:`esp_board_manager_get_device_handle` 和 :cpp:func:`esp_board_manager_get_periph_handle` 获取运行时句柄。这两个 API 只返回已初始化对象的句柄，不会主动初始化设备或外设。

关键行为
--------------

- **外设先初始化，设备后初始化**：调用 :cpp:func:`esp_board_manager_init` 时，BMGR 固定按先外设、后设备的顺序执行。
- **整体遍历顺序与 YAML 书写顺序相关**：:cpp:func:`esp_board_device_init_all` 按 ``board_devices.yaml`` 中的条目顺序遍历设备。
- ``depends_on``\ **声明设备间的初始化依赖**：当 device 配置了 ``depends_on`` 时，:cpp:func:`esp_board_device_init` 在初始化该 device 之前会递归先初始化所列依赖设备，与 YAML 中条目的前后顺序无关，无需在 ``board_devices.yaml`` 中手动保证顺序。依赖设备若已由其他路径初始化（``ref_count > 0``），不会重复创建实例。依赖的设备类型不限，一个设备可以声明多个依赖项。
- **反初始化按引用计数收敛**：设备和外设内部均维护引用计数（``ref_count``）。重复初始化同一对象时，不会重复创建实例，而是增加引用计数。计数降至 0 时才真正释放。
- ``init_skip``\ **跳过自动初始化**：对于 ``init_skip: true`` 的 device 或 peripheral，:cpp:func:`esp_board_manager_init` 不会自动创建运行时句柄。跳过的设备需通过 :cpp:func:`esp_board_manager_init_device_by_name` 初始化；跳过的外设需调用 :cpp:func:`esp_board_periph_init`，或初始化引用它的设备。之后再调用 :cpp:func:`esp_board_manager_get_device_handle` 或 :cpp:func:`esp_board_manager_get_periph_handle` 获取句柄。
- ``power_ctrl_device``\ **控制设备上电时序**：当 device 声明了 ``power_ctrl_device`` 时，BMGR 在初始化该 device 之前先通过对应的 ``power_ctrl`` 设备执行上电动作；反初始化时也会触发下电。``power_ctrl_device`` 是专门针对供电控制的设备间引用，被引用的设备类型必须为 ``power_ctrl``。与 ``depends_on`` 相比，``power_ctrl_device`` 额外触发上电与下电动作，并提供运行时的电源控制 API :cpp:func:`esp_board_device_power_ctrl`，作用不局限于保证初始化顺序。
- 结合使用 ``depends_on`` 与 ``power_ctrl_device``，可以保证即使通过 :cpp:func:`esp_board_manager_init_device_by_name` 单独初始化某个设备，也不会因供电或其他依赖问题导致初始化失败。

BMGR 的运行时模型不会将所有初始化逻辑压缩到一次 ``init()`` 调用中，而是按板级描述组织初始化顺序，同时保留引用计数、延迟初始化、设备间依赖和电源控制等运行时行为。

板级 Pre-init Hook
------------------

:cpp:func:`esp_board_runtime_pre_init` 是固定的 weak 符号。库提供默认实现并返回 ``ESP_OK``。最终应用可提供且仅可提供一个同名强实现；多个强定义会在链接阶段失败。不在 ``board_info.yaml`` 增加字段，也不生成函数指针注册表。强实现通常放在 ``setup_device.c`` 等板级源文件中。

Hook 在进程内首次 prepare 时执行一次。:cpp:func:`esp_board_manager_init`、:cpp:func:`esp_board_periph_init`、:cpp:func:`esp_board_device_init` 与 :cpp:func:`esp_board_periph_init_custom` 都会在创建 handle 前调用 prepare，因此无法绕过 hook。若应用需要在任何 init 之前读取已应用 BOARD 覆盖后的有效配置，可显式调用幂等的公开接口 :cpp:func:`esp_board_manager_prepare`。

prepare 与 init 必须在 FreeRTOS 任务中运行（需要有效的当前任务句柄）。Hook 可以修改可写的 descriptor/handle 表，并调用 ``override_config`` / ``set_ops``；覆盖 API 在 hook 中写入 BOARD 层。prepare owner 在 hook 中也可以初始化资源并查询其 handle。同一任务递归进入 prepare 时不会再次运行 hook；其他任务在 prepare 期间不能查询中间状态。Hook 失败时，manager 恢复表链接及未被活动资源使用的 BOARD 层覆盖，并回到 ``UNPREPARED``。Hook 必须在返回失败前反初始化自己创建的硬件资源；prepare 回滚不负责硬件资源的反初始化。

使用 ``idf.py bmgr -b <board> --writable`` 生成可写 descriptor 表。若 hook 要新增 C 节点，在项目配置中将 ``ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM`` 或 ``ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM`` 调高到足以容纳新增节点；默认上限等于生成的节点数。新增 device 需同时挂接同名、同 ``type`` 的 descriptor 与 handle 节点；新增 peripheral 需挂接 descriptor，并提供匹配的公共 ``type + role`` ops 或调用 ``esp_board_periph_set_ops``。节点与配置内存必须在运行期间保持有效。

``manager deinit`` 不会清除 BOARD 或 APP 覆盖，也不会把 prepare 打回 ``UNPREPARED``。覆盖分层、restore 规则与 ``ref_count`` 门禁见 :doc:`/programming-guide/advanced-runtime`。

运行时限制与配置注意事项：

- 板级 pre-init hook 可以初始化资源并查询 handle。返回失败前须反初始化自己创建的资源，避免活动资源及其配置跨越失败的 prepare。
- 配置多个逻辑 peripheral 时，请确认其物理总线关系。BMGR 不会因为两个 peripheral 使用相同的 ``port`` 值就自动共享同一条物理总线。

遗留 API
--------

仅以下接口作为遗留 / 弃用 ABI 保留：

- :cpp:func:`esp_board_periph_init_custom` — 更新共享的 ``(type, role)`` handler 并立即初始化，并非按名称的 variant 机制。新代码应使用板级 pre-init hook 或 APP 侧的 :cpp:func:`esp_board_periph_set_ops`。
- :cpp:func:`esp_board_manager_register_device_handle` — 将调用者拥有的 handle 节点插入生成列表，并绕过 descriptor 与 APP/BOARD 层。已有设备名称请使用 :cpp:func:`esp_board_device_set_ops`；不要通过该接口注入未知设备。

除上述接口外，其他 API 仍按现有状态使用，不视为遗留或弃用 API。
