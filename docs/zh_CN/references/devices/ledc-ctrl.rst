LEDC 控制 ledc_ctrl
===================

:link_to_translation:`en:[English]`

简介
------

``ledc_ctrl`` 是基于 ``ledc`` peripheral 的 PWM 控制型 device，用于将一个 LEDC 通道封装为可按名称获取的设备句柄。该设备适合描述 LCD 背光亮度、单路 PWM 输出等需按占空比控制的板级信号。

BMGR 初始化 ``ledc_ctrl`` 时引用已初始化的 ``ledc`` peripheral，根据 ``default_percent`` 与该外设的 ``duty_resolution`` 计算初始 duty，并调用 LEDC 驱动写入通道。应用通过 :cpp:func:`esp_board_manager_get_device_handle` 获取 ``periph_ledc_handle_t``，再调用 LEDC 驱动调整占空比。

支持的使用模式
---------------------

``ledc_ctrl`` 不使用 ``sub_type`` 区分模式，分类轴是被引用的 ``ledc`` peripheral。当前支持一种模式：

- `LEDC PWM 控制`_

最小配置
------------

LEDC PWM 控制
^^^^^^^^^^^^^^^

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: ledc_backlight
        type: ledc
        version: default
        config:
          gpio_num: 26             # [IO]
          speed_mode: LEDC_LOW_SPEED_MODE
          channel: 1
          timer_sel: 1
          duty: 0

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: lcd_brightness
        type: ledc_ctrl
        version: default
        config:
          default_percent: 100
        peripherals:
          - name: ledc_backlight

``ledc_ctrl`` 只绑定一个 ``ledc`` 外设；解析脚本要求 ``peripherals`` 列表中至少包含一个名称以 ``ledc`` 或 ``ledc_`` 开头的外设引用，启用外设校验时该名称必须存在于 ``board_peripherals.yaml``。

``default_percent`` 表示初始化时写入的百分比，驱动会按 ``duty = default_percent * (2^duty_resolution - 1) / 100`` 计算初始 duty，并调用 ``ledc_set_duty`` 与 ``ledc_update_duty`` 生效。后续亮度或 PWM 变化由应用直接使用 LEDC 驱动完成。

完整字段
------------

LEDC PWM 控制完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example board_devices.yaml configuration for LEDC control device
    # This shows how to integrate the LEDC control device into a board configuration

    # Example LEDC control device configuration for LCD brightness
    - name: lcd_brightness          # The name of the device, must be unique
      type: ledc_ctrl               # The type of the device, must be unique
      version: 1.0.0
      config:
        default_percent: 100        # [TO_BE_CONFIRMED] Default brightness percentage (0-100)
      peripherals:
        - name: ledc_backlight      # LEDC peripheral name (must reference an LEDC peripheral)

组件依赖
------------

``ledc_ctrl`` 不需要在 ``dependencies`` 中声明额外组件。它使用 ESP-IDF LEDC 驱动和 BMGR 的 ``ledc`` peripheral。

依赖外设
------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - 必选性
     - 用途
   * - ``ledc``
     - 不使用 ``role`` / ``format``
     - 必选
     - 提供 LEDC 通道、定时器、分辨率和输出 GPIO

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_ledc.c``
- ``esp_board_manager/devices/dev_ledc_ctrl/dev_ledc_ctrl.c``

板级参考
------------

- ``esp_boards/esp32_p4_function_ev_board/board_devices.yaml``：配置 ``lcd_brightness`` 设备。
- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``：配置 ``ledc_backlight`` 外设。
- ``esp_boards/esp32_s3_box_3/board_devices.yaml``：配置 ``ledc_ctrl`` 背光设备。
- ``esp_boards/esp32_s3_box_3/board_peripherals.yaml``：配置被背光设备引用的 ``ledc`` peripheral。

注意事项
------------

- 设备引用的 ``ledc`` 外设名称必须与 ``board_peripherals.yaml`` 中的实例名一致，并且名称需以 ``ledc`` 或 ``ledc_`` 开头。
- ``default_percent`` 只用于初始化时设置 duty；运行时改变亮度需要应用重新调用 LEDC 驱动设置 duty 并更新通道。
- ``ledc_ctrl`` 去初始化时会调用 ``ledc_stop``，停止电平参数为 ``0``。
- 修改 YAML 后需要重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，返回类型为 ``periph_ledc_handle_t``，由底层 LEDC 外设驱动定义。该句柄可传入 ``esp_board_device_power_ctrl()`` 或直接通过 LEDC 外设 API 调用。

设备配置结构 ``dev_ledc_ctrl_config_t`` 及初始化函数见 ``esp_board_manager/devices/dev_ledc_ctrl/dev_ledc_ctrl.h``。
