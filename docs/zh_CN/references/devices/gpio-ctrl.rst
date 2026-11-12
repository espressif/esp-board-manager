GPIO 控制 gpio_ctrl
===================

:link_to_translation:`en:[English]`

.. _gpio-ctrl-intro:

简介
------

``gpio_ctrl`` 是基于 ``gpio`` peripheral 的 GPIO 控制型 device，用于将一个板级 GPIO 输出封装为可按名称获取的设备句柄。该设备适合描述 LCD 电源、音频电源、模块使能等仅需设置高低电平的板级控制信号。

BMGR 初始化 ``gpio_ctrl`` 时引用 ``gpio`` peripheral，读取该 GPIO 的管脚号，并按 ``init_level`` 设置初始化电平。未配置 ``init_level`` 或设为 ``-1`` 时，初始化电平使用 ``active_level``。应用通过 :cpp:func:`esp_board_manager_get_device_handle` 获取 ``periph_gpio_handle_t``，再按设备配置中的 ``active_level`` 与 ``default_level`` 切换状态。

.. _gpio-ctrl-usage-modes:

支持的使用模式
---------------------

``gpio_ctrl`` 不使用 ``sub_type`` 区分模式，分类轴是被引用的 ``gpio`` peripheral。当前支持一种模式：

- `GPIO 输出控制`_

.. _gpio-ctrl-min-config:

最小配置
------------

.. _gpio-ctrl-output:

GPIO 输出控制
^^^^^^^^^^^^^^^^^

完整字段见 :ref:`gpio-ctrl-output-full`。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: gpio_power
        type: gpio
        role: io
        config:
          pin: 4                  # [IO]
          mode: GPIO_MODE_OUTPUT
          default_level: 0

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: power_control
        type: gpio_ctrl
        version: default
        config:
          active_level: 1
          default_level: 0
          init_level: 0
        peripherals:
          - gpio_name: gpio_power

``gpio_ctrl`` 通过 ``gpio_name`` 绑定一个 ``gpio`` 外设。所选名称必须存在于 ``board_peripherals.yaml``，并且引用的外设类型必须为 ``gpio``。

``active_level`` 表示该控制功能有效时的电平，``default_level`` 表示非工作电平。应用启用或关闭该控制功能时，根据这两个字段调用 GPIO 驱动。去初始化不会自动写回 ``default_level``。

``init_level`` 是可选整数，取值为 ``-1``、``0`` 或 ``1``。未配置或设为 ``-1`` 时，设备初始化写入 ``active_level``；设为 ``0`` 或 ``1`` 时，写入指定电平。上述示例在初始化后保持非工作电平，应用需要主动写入 ``active_level`` 才会启用该控制功能。

.. _gpio-ctrl-full-fields:

完整字段
------------

.. _gpio-ctrl-output-full:

GPIO 输出控制完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example board_devices.yaml configuration for GPIO control device
    # This shows how to integrate the GPIO control device into a board configuration

    # Example GPIO control device configuration
    - name: power_control         # The name of the device, must be unique
      type: gpio_ctrl             # The type of the device, must be unique
      version: 1.0.0
      config:
        active_level: 1           # [TO_BE_CONFIRMED] GPIO level (0 or 1) when the control function is active
        default_level: 0          # Inactive GPIO level (0 or 1); not automatically applied on deinit
        init_level: -1            # Optional initial level (-1, 0, or 1); omitted or -1 uses active_level
      peripherals:
        - gpio_name: gpio          # [TO_BE_CONFIRMED] GPIO peripheral name (must reference a GPIO peripheral)

.. _gpio-ctrl-deps:

组件依赖
------------

``gpio_ctrl`` 不需要在 ``dependencies`` 中声明额外组件。它使用 ESP-IDF GPIO 驱动和 BMGR 的 ``gpio`` peripheral。

.. _gpio-ctrl-peripherals:

依赖外设
------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - 必选性
     - 用途
   * - ``gpio``
     - ``io``
     - 必选
     - 提供实际 GPIO 管脚和 GPIO peripheral 句柄

.. _gpio-ctrl-code:

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_pwr_ctrl.c``
- ``esp_board_manager/devices/dev_gpio_ctrl/dev_gpio_ctrl.c``

.. _gpio-ctrl-boards:

板级参考
------------

- ``esp_friends_boards/esp32_c5_spot/board_devices.yaml``：配置 ``gpio_ctrl`` 设备。
- ``esp_friends_boards/esp32_c5_spot/board_peripherals.yaml``：配置被 ``gpio_ctrl`` 引用的 ``gpio`` peripheral。
- ``esp_board_manager/test_apps/test_single_board/board_devices.yaml``：测试板中的 ``gpio_ctrl`` 配置。
- ``esp_board_manager/test_apps/test_single_board/board_peripherals.yaml``：测试板中的 ``gpio`` peripheral 配置。

.. _gpio-ctrl-notes:

注意事项
------------

- 设备引用的 ``gpio`` 外设名称必须与 ``board_peripherals.yaml`` 中的实例名一致，并且名称需以 ``gpio`` 或 ``gpio_`` 开头。
- 原生 GPIO 外设先写入外设配置中的 ``default_level``，随后 ``gpio_ctrl`` 写入设备的初始化电平。``init_level`` 只决定本次设备初始化完成时的电平；对启动电平敏感的控制信号，需要将外设 ``default_level`` 与设备初始化电平对齐。
- 对已初始化设备再次调用初始化接口只增加引用，不会重新写入初始化电平。完整释放设备后再次初始化，会重新应用该电平。
- 关闭控制信号时，应用需要通过获取到的 ``periph_gpio_handle_t`` 和设备配置主动写入 ``default_level``。
- 升级到支持 ``init_level`` 的版本或修改 YAML 后，需要重新执行 ``idf.py bmgr -b <board>`` 并重新构建。旧生成 C 文件省略该字段，重新编译后会得到 ``0``，不会使用 YAML 的 ``-1`` 默认值。

.. _gpio-ctrl-debug:

调试技巧
------------

.. _gpio-ctrl-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，返回类型为 ``periph_gpio_handle_t``，由底层 GPIO 外设驱动定义。通过 ``gpio_set_level(handle->gpio_num, level)`` 设置运行时电平，``level`` 使用设备配置中的 ``active_level`` 或 ``default_level``。

手写 C 配置时，需显式设置 ``init_level = -1``，使初始化电平使用 ``active_level``。省略 C 结构字段或将结构清零会使 ``init_level`` 为 ``0``，表示初始化输出低电平。

设备配置结构 ``dev_gpio_ctrl_config_t`` 及初始化函数见 ``esp_board_manager/devices/dev_gpio_ctrl/dev_gpio_ctrl.h``。
