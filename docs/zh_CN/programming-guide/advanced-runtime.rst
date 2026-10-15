进阶运行时能力
==========================================================================

:link_to_translation:`en:[English]`

除"初始化后获取句柄"这条主路径外，BMGR 还提供以下面向进阶场景的运行时能力。

运行时配置与 Ops 覆盖
------------------------

每个 device 或 peripheral 名称的有效 config 与 init/deinit ops 按以下优先级选择：

**YAML 生成配置 < BOARD < APP**

- **YAML 生成配置**：板级 YAML 生成的 descriptor 配置与类型级 init/deinit 操作。revision apply 或 C 代码修改 descriptor 后，基础配置以当前 descriptor 为准。
- **BOARD**：板级 pre-init hook 调用覆盖 API 写入的按名称覆盖。板级实现 :cpp:func:`esp_board_runtime_pre_init`；manager 在首次创建 handle 前自动调用该 hook。
- **APP**：应用显式调用 ``override_config`` / ``set_ops`` / ``restore_config`` / ``restore_ops``，按名称作用于 device 或 peripheral。

多版本配置（\ ``board_revisions``\ ）在生成期保留基础配置和各版本差异，使它们在同一固件中共存，运行时由板级代码检测硬件并调用生成的 apply 函数。apply 函数更新基础 descriptor 表，不写入 BOARD 覆盖层。仅声明 revision 而未调用 apply 时，BMGR 继续使用基础配置。

相关接口：

- Device：:cpp:func:`esp_board_device_override_config`、:cpp:func:`esp_board_device_restore_config`、:cpp:func:`esp_board_device_set_ops`、:cpp:func:`esp_board_device_restore_ops`
- Peripheral：:cpp:func:`esp_board_periph_override_config`、:cpp:func:`esp_board_periph_restore_config`、:cpp:func:`esp_board_periph_set_ops`、:cpp:func:`esp_board_periph_restore_ops`

``override_config`` 对配置结构做顶层 ``memcpy``。结构体内的嵌套指针仍指向调用方内存；在覆盖有效期间须保持该嵌套内存存活。若要改单个字段，先复制生成配置，再改字段，最后调用 override。

设备 config 覆盖会更新该 device 的有效配置。:cpp:func:`esp_board_device_get_config` 返回这份配置，设备 ``init`` 使用它创建驱动句柄，:cpp:func:`esp_board_device_callback_register` 则在使用已初始化句柄注册回调时，将这份配置传给具体设备实现。回调注册不会重新初始化设备，因此 config 覆盖通常必须在 device init 之前完成。

外设 config 覆盖会更新该 peripheral 的有效配置。:cpp:func:`esp_board_periph_get_config` 和 peripheral ``init`` 会读取这份配置；当 device 初始化过程中使用该 peripheral 时，也会使用对应的有效配置。BMGR 没有与 peripheral 对应的 callback 注册接口。

config 与 ops 分层独立记录。仅设置 ops 时，config 仍使用更低层；仅设置 config 时，ops 仍使用更低层。

目标名称 ``ref_count != 0`` 时，再对该名称调用 ``override_config`` / ``set_ops`` / ``restore_*`` 返回 ``ESP_ERR_INVALID_STATE``。尚未 init 时，同一层重复调用直接替换。``deinit`` 使计数回到 0 后可再次修改。

``deinit`` 只释放已创建的驱动 handle，不清 BOARD 或 APP 覆盖。``restore_config`` / ``restore_ops`` 只清除 APP 层。APP restore 后，config 回退到 BOARD 覆盖；若没有 BOARD config 覆盖，则读取当前 descriptor 的配置。当前 descriptor 可能已由 revision apply 替换或被 C 代码修改；若未修改，其配置就是 YAML 生成的默认配置。ops 则回退到 BOARD ops，或在没有 BOARD ops 时回退到匹配类型/角色的操作。恢复 APP 覆盖不会撤销 revision apply 或 descriptor 修改。pre-init hook 失败时会回滚未被活动资源使用的 BOARD 覆盖；hook 必须自行反初始化临时硬件资源。

没有 YAML 生成配置的资源不能覆盖。共享同一 ``(type, role)`` 但名称不同的资源，按名称隔离 ops 与 config。

板级 hook 约定、prepare 时机与遗留 API 见 :doc:`/programming-guide/runtime-lifecycle`。

运行时限制与配置注意事项：

- 板级 pre-init hook 可以初始化资源并查询 handle。返回失败前须反初始化自己创建的资源，避免活动资源及其配置跨越失败的 prepare。
- 配置多个逻辑 peripheral 时，请确认其物理总线关系。BMGR 不会因为两个 peripheral 使用相同的 ``port`` 值就自动共享同一条物理总线。

手动设备电源控制
--------------------------

相关接口：:cpp:func:`esp_board_device_power_ctrl`。

适用于设备存在独立上电或断电动作、且应用需要在 BMGR 自动流程之外显式控制的场景。该接口根据设备描述中的 ``power_ctrl_device`` 找到对应的控制设备，再调用与该 ``sub_type`` 对应的电源控制扩展函数。能否控制以及如何控制，取决于具体 power control device 的实现。当前 power control device 仅支持 GPIO 子类。对于 IO 扩展芯片或电源芯片等较复杂的电源控制设备，可先通过 ``depends_on`` 声明依赖，确保初始化该设备时所需的电源管理设备已被初始化。

设备回调注册
--------------------------

相关接口：:cpp:func:`esp_board_device_callback_register`。

部分设备的正常使用依赖回调函数注册。BMGR 对注册接口做了统一封装，应用层可根据 device type 查找对应的扩展回调注册函数，再传入当前有效配置和运行时句柄。该能力是否可用以及回调签名取决于具体设备的实现，详见各设备参考页。

按名称初始化或反初始化单个设备
------------------------------------

相关接口：

- :cpp:func:`esp_board_manager_init_device_by_name`
- :cpp:func:`esp_board_manager_deinit_device_by_name`

该类接口常与 ``init_skip``、延迟初始化、局部资源回收等场景配合使用。对于需要先完成整板初始化、再按业务时机打开特定设备的工程，比一次性初始化所有设备更灵活。
