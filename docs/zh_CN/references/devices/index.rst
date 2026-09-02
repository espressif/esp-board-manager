设备参考
============

:link_to_translation:`en:[English]`

设备（device）是 BMGR 中面向功能的板级对象，用于描述音频编解码器、显示屏、触摸屏控制器、按键、摄像头、文件系统等可被应用直接使用的硬件或软件能力。设备条目写在 ``board_devices.yaml`` 中，BMGR 解析后生成设备配置、组件依赖和初始化代码，应用通过 :cpp:func:`esp_board_manager_get_device_handle` 按名称获取设备句柄。

设备参考页用于查询某个 ``type`` 的配置方式。通用字段规则（``name``、``type``、``sub_type``、``chip``、``version``、``[IO]``、``[TO_BE_CONFIRMED]``、``${BOARD_PATH}``、``dependencies`` 等）见 :doc:`/programming-guide/board-directory` 与 :doc:`/programming-guide/yaml-rules`。本节仅说明各设备类型的专属字段、依赖外设、组件依赖与适配限制。

.. toctree::
   :maxdepth: 1
   :caption: 设备列表

   audio-codec
   display-lcd
   lcd-touch
   button
   knob
   led-strip
   ledc-ctrl
   gpio-ctrl
   gpio-expander
   power-ctrl
   camera
   fs-fat
   fs-spiffs
   littlefs
   custom

配置来源
------------

BMGR 设备配置来源于以下几个方面：

- 设备 YAML 模板：``esp_board_manager/devices/dev_<type>/dev_<type>.yaml``。
- 设备头文件与实现：``esp_board_manager/devices/dev_<type>/dev_<type>.h`` 与对应 ``.c``。

每个设备的可配置参数均在 YAML 模板中列出，配置项的默认值和取值范围也在模板中注明。设备头文件中定义了设备句柄类型与配置类型，设备实现文件中给出了初始化与反初始化逻辑；部分设备存在特殊行为，会在对应设备页中说明。

BMGR 设备实现通常基于 IDF 驱动或组件。在设计设备时，BMGR 会尽量复用 IDF 驱动的功能和配置项；因此，设备 YAML 中的字段通常与 IDF 驱动的 API 参数一一对应，设备实现中调用对应的 IDF 驱动 API 完成初始化与反初始化。

注意事项
------------

- 修改设备 YAML 后需重新执行 ``idf.py bmgr -b <board>``，并重新构建工程。
- ``[IO]`` 表示需按原理图替换的管脚或硬件资源；``[TO_BE_CONFIRMED]`` 表示需要板级维护者确认的取值。
- 设备引用的外设 ``name`` 必须与 ``board_peripherals.yaml`` 中的实例名一致。
- 需要自定义初始化、面板工厂函数或运行时注册逻辑时，优先查看对应设备页是否要求配合 ``setup_device.c`` 或 ``custom`` 设备。板级 C 中依赖芯片驱动组件的实现须用 ``__has_include`` 包裹，工厂入口还须声明为弱符号，以便 ``gen_skip`` 与 amend。详见 :doc:`/programming-guide/board-directory`。
