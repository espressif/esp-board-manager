运行时生命周期
==========================================================================

:link_to_translation:`en:[English]`

从运行时角度看，BMGR 固定了一组顺序：先初始化哪些外设、再初始化哪些设备、如何获取对应的句柄与配置。

初始化入口与运行时访问
----------------------------

:cpp:func:`esp_board_manager_init` 是整板初始化入口，用于按生成的板级描述完成自动初始化。对应地，:cpp:func:`esp_board_manager_deinit` 按相反方向释放运行时对象。

:cpp:func:`esp_board_manager_init_device_by_name` 用于按 device 名称初始化单个设备，常用于 ``init_skip``、延迟初始化或按业务条件打开某个设备的场景。

应用可通过 :cpp:func:`esp_board_manager_get_device_config` 和 :cpp:func:`esp_board_manager_get_periph_config` 获取生成后的配置结构体。配置来自 YAML 解析生成的 C 结构，用于读取板级参数。

应用可通过 :cpp:func:`esp_board_manager_get_device_handle` 和 :cpp:func:`esp_board_manager_get_periph_handle` 获取运行时句柄。这两个 API 只返回已初始化对象的句柄，不会主动初始化设备或外设。

关键行为
--------------

- **外设先初始化，设备后初始化**：调用 :cpp:func:`esp_board_manager_init` 时，BMGR 固定按先外设、后设备的顺序执行。
- **整体遍历顺序与 YAML 书写顺序相关**：:cpp:func:`esp_board_device_init_all` 按 ``board_devices.yaml`` 中的条目顺序遍历设备。
- ``depends_on``\ **声明设备间的初始化依赖**：当 device 配置了 ``depends_on`` 时，:cpp:func:`esp_board_device_init` 在初始化该 device 之前会递归先初始化所列依赖设备，与 YAML 中条目的前后顺序无关，无需在 ``board_devices.yaml`` 中手动保证顺序。依赖设备若已由其他路径初始化（``ref_count > 0``），不会重复创建实例。依赖的设备类型不限，一个设备可以声明多个依赖项。
- **反初始化按引用计数收敛**：设备和外设内部均维护引用计数（``ref_count``）。重复初始化同一对象时，不会重复创建实例，而是增加引用计数。计数降至 0 时才真正释放。
- ``init_skip``\ **跳过自动初始化**：对于 ``init_skip: true`` 的 device，:cpp:func:`esp_board_manager_init` 不会自动创建运行时句柄。应用需通过 :cpp:func:`esp_board_manager_init_device_by_name` 初始化该设备，或确保它已被其他设备的依赖链初始化成功，再调用 :cpp:func:`esp_board_manager_get_device_handle` 获取句柄。
- ``power_ctrl_device``\ **控制设备上电时序**：当 device 声明了 ``power_ctrl_device`` 时，BMGR 在初始化该 device 之前先通过对应的 ``power_ctrl`` 设备执行上电动作；反初始化时也会触发下电。``power_ctrl_device`` 是专门针对供电控制的设备间引用，被引用的设备类型必须为 ``power_ctrl``。与 ``depends_on`` 相比，``power_ctrl_device`` 额外触发上电与下电动作，并提供运行时的电源控制 API :cpp:func:`esp_board_device_power_ctrl`，作用不局限于保证初始化顺序。
- 结合使用 ``depends_on`` 与 ``power_ctrl_device``，可以保证即使通过 :cpp:func:`esp_board_manager_init_device_by_name` 单独初始化某个设备，也不会因供电或其他依赖问题导致初始化失败。

BMGR 的运行时模型不会将所有初始化逻辑压缩到一次 ``init()`` 调用中，而是按板级描述组织初始化顺序，同时保留引用计数、延迟初始化、设备间依赖和电源控制等运行时行为。
