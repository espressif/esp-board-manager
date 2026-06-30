GPIO 控制 gpio_ctrl
===================

:link_to_translation:`en:[English]`

简介
------

``gpio_ctrl`` 是基于 ``gpio`` peripheral 的 GPIO 控制型 device，用于将一个板级 GPIO 输出封装为可按名称获取的设备句柄。该设备适合描述 LCD 电源、音频电源、模块使能等仅需设置高低电平的板级控制信号。

BMGR 初始化 ``gpio_ctrl`` 时引用已初始化的 ``gpio`` peripheral，读取该 GPIO 的管脚号，并将输出电平设置为 ``active_level``。应用通过 :cpp:func:`esp_board_manager_get_device_handle` 获取 ``periph_gpio_handle_t``，再按设备配置中的 ``active_level`` 与 ``default_level`` 切换状态。

支持的使用模式
---------------------

``gpio_ctrl`` 不使用 ``sub_type`` 区分模式，分类轴是被引用的 ``gpio`` peripheral。当前支持一种模式：

- `GPIO 输出控制`_

最小配置
------------

GPIO 输出控制
^^^^^^^^^^^^^^^^^

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
        peripherals:
          - name: gpio_power

``gpio_ctrl`` 只绑定一个 ``gpio`` 外设；解析脚本要求 ``peripherals`` 列表中至少包含一个名称以 ``gpio`` 或 ``gpio_`` 开头的外设引用，启用外设校验时该名称必须存在于 ``board_peripherals.yaml``。

``active_level`` 是设备初始化时写入的电平，也是应用启用该控制信号时应写入的电平。``default_level`` 不会在去初始化时自动写回，应用需要在关闭电源或禁用信号时根据设备配置主动调用 GPIO 驱动设置该电平。

完整字段
------------

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
        active_level: 1           # [TO_BE_CONFIRMED] Active level (0 or 1) - GPIO level when device is active
        default_level: 0          # Default level (0 or 1) - GPIO level when device is inactive
      peripherals:
        - name: gpio              # [TO_BE_CONFIRMED] GPIO peripheral name (must reference a GPIO peripheral)

组件依赖
------------

``gpio_ctrl`` 不需要在 ``dependencies`` 中声明额外组件。它使用 ESP-IDF GPIO 驱动和 BMGR 的 ``gpio`` peripheral。

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

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_pwr_ctrl.c``
- ``esp_board_manager/devices/dev_gpio_ctrl/dev_gpio_ctrl.c``

板级参考
------------

- ``esp_friends_boards/esp32_c5_spot/board_devices.yaml``：配置 ``gpio_ctrl`` 设备。
- ``esp_friends_boards/esp32_c5_spot/board_peripherals.yaml``：配置被 ``gpio_ctrl`` 引用的 ``gpio`` peripheral。
- ``esp_board_manager/test_apps/test_single_board/board_devices.yaml``：测试板中的 ``gpio_ctrl`` 配置。
- ``esp_board_manager/test_apps/test_single_board/board_peripherals.yaml``：测试板中的 ``gpio`` peripheral 配置。

注意事项
------------

- 设备引用的 ``gpio`` 外设名称必须与 ``board_peripherals.yaml`` 中的实例名一致，并且名称需以 ``gpio`` 或 ``gpio_`` 开头。
- ``gpio_ctrl`` 初始化时会把 GPIO 设置为 ``active_level``；如果板级电源默认应关闭，需要确认初始化顺序和 ``active_level`` 设置符合硬件设计。
- 关闭控制信号时，应用需要通过获取到的 ``periph_gpio_handle_t`` 和设备配置主动写入 ``default_level``。
- 修改 YAML 后需要重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，返回类型为 ``periph_gpio_handle_t``，由底层 GPIO 外设驱动定义。该句柄可传入 ``esp_board_device_power_ctrl()`` 实现运行时电平控制，也可直接通过 GPIO 外设 API 调用。

设备配置结构 ``dev_gpio_ctrl_config_t`` 及初始化函数见 ``esp_board_manager/devices/dev_gpio_ctrl/dev_gpio_ctrl.h``。
