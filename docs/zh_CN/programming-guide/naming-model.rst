命名与关键概念
==========================================================================

:link_to_translation:`en:[English]`

理解 BMGR 时，最容易混淆的并非 API，而是几层命名和配置对象各自代表的含义。

- ``peripherals``：开发板上的底层资源实例，定义在 ``board_peripherals.yaml`` 中，例如某一路 ``i2c``、``spi``、``i2s``、``gpio``。
- ``devices``：面向功能的设备实例，定义在 ``board_devices.yaml`` 中，例如 ``audio_codec``、``display_lcd``、``button``。
- ``name``：当前开发板中实际使用的实例名，是板级配置中的主键。应用通过该名称获取句柄，例如 ``esp_board_manager_get_device_handle("display_lcd", ...)``；设备也通过该名称引用特定外设。
- ``type``：设备或外设的类别，决定匹配的解析规则和运行时实现，例如 ``audio_codec``、``button``、``i2c``。
- ``sub_type``：部分 device 的子类型，用于区分同一类设备下的不同实现路径，例如 ``display_lcd`` 按接口分为 ``spi``、``i80``、``dsi``、``rgb``、``rgb_3wire_spi``、``parlio``。
- ``role``：部分 peripheral 的工作模式，例如 ``adc`` 区分 ``oneshot`` 与 ``continuous`` 两种模式。
- ``format``：部分 peripheral 的数据格式，典型场景为 ``i2s`` 的 ``std-out``、``std-in``。
- ``dependencies``：设备额外依赖的组件信息，生成时写入 ``components/gen_bmgr_codes/idf_component.yml``。
- ``power_ctrl_device``：用于需要受控供电的设备。通过配置 ``power_ctrl_device`` 引用 ``power_ctrl`` 类型设备，可在初始化时自动上电。
- ``depends_on``：用于声明设备间的初始化依赖。初始化时 BMGR 自动检查并先初始化所声明的依赖设备；类型不限，一个设备可声明多个依赖。
- ``init_skip``：标记该 device 不参与 :cpp:func:`esp_board_manager_init` 的自动初始化，需要应用在合适时机手动初始化。
- ``gen_skip``：通常用于覆盖开发板默认配置，表示当前工程不使用该设备或外设，在解析和生成阶段跳过。

整体可以分为两层关系：``peripherals`` 描述底层资源的接入方式，``devices`` 描述由底层资源组合而成的功能能力。``name`` 是板内实例名，``type``、``sub_type``、``role``、``format`` 决定具体走哪条实现路径。``depends_on`` 与 ``power_ctrl_device`` 描述设备之间的依赖关系，影响运行时初始化顺序，详见 :doc:`runtime-lifecycle`。
