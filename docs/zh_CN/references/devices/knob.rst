旋钮 knob
===========

:link_to_translation:`en:[English]`

.. _knob-intro:

简介
------

``knob`` 设备通过 ``espressif/knob`` 组件初始化低速正交旋转编码器。应用通过 :cpp:func:`esp_board_manager_get_device_handle` 获取 ``dev_knob_handles_t``，并使用其中的 ``knob_handle`` 调用组件 API 注册旋转回调或读取计数值。

设备只提供 ``sub_type: gpio`` 输入接口。``use_rtc`` 用于选择上游组件的 GPIO HAL，不会形成第二种应用接口。

.. _knob-usage-modes:

支持的使用模式
-----------------

- :ref:`GPIO 正交旋钮 <knob-gpio>`

.. _knob-min-config:

最小配置
------------

.. _knob-gpio:

GPIO 正交旋钮
^^^^^^^^^^^^^^^^

完整字段见 :ref:`knob-gpio-full`。

``knob`` 直接管理编码器 A/B 引脚，不应为这两个引脚额外声明 BMGR ``gpio`` peripheral。

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: knob
        type: knob
        sub_type: gpio
        config:
          gpio_encoder_a: 41       # [IO]
          gpio_encoder_b: 40       # [IO]
          default_direction: 0
          enable_power_save: false
          use_rtc: false

仅当两个编码器引脚都支持 RTC GPIO 时设置 ``use_rtc: true``。BMGR 会调用 ``iot_knob_create_rtc()``；默认路径调用 ``iot_knob_create()``。

.. _knob-full-fields:

完整字段
------------

.. _knob-gpio-full:

GPIO 正交旋钮完整字段
^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Knob device configuration example
    - name: knob
      type: knob
      sub_type: gpio
      config:
        gpio_encoder_a: 41       # [TO_BE_CONFIRMED] Encoder phase A GPIO
        gpio_encoder_b: 40       # [TO_BE_CONFIRMED] Encoder phase B GPIO
        default_direction: 0     # 0: positive count for the default direction; 1: negative
        enable_power_save: false # Enable the upstream knob driver's power-save path
        use_rtc: false           # Use RTC GPIO HAL; both encoder GPIOs must be RTC-capable

.. _knob-deps:

组件依赖
------------

启用 ``CONFIG_ESP_BOARD_DEV_KNOB_SUPPORT`` 时，BMGR 会引入版本为 ``^1.1.0`` 的公开依赖 ``espressif/knob``。板级 YAML 无需重复声明该依赖。

.. _knob-peripherals:

依赖外设
------------

不需要 BMGR peripheral。``espressif/knob`` 组件负责初始化和释放编码器的两个引脚。

.. _knob-code:

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_knob.c``
- ``esp_board_manager/devices/dev_knob/dev_knob.c``

.. _knob-notes:

注意事项
------------

- ``gpio_encoder_a`` 与 ``gpio_encoder_b`` 必须是不同的非负 GPIO 编号。
- GPIO 编号和 ``default_direction`` 必须使用 YAML 整数；浮点数、布尔值和数字字符串不会自动转换。
- ``default_direction`` 仅可取 ``0`` 或 ``1``。
- ``use_rtc`` 要求编码器引脚支持 RTC GPIO。无需 RTC GPIO 低功耗路径时使用普通 GPIO 后端。
- ``knob`` 不接受非空 ``peripherals`` 绑定；编码器引脚由该设备直接管理。
- 该设备使用上游软件正交解码器。需要硬件脉冲计数时使用独立的 ``pcnt`` peripheral。
- 修改 YAML 后，构建前需执行 ``idf.py bmgr -b <board>``。

.. _knob-debug:

调试技巧
------------

在定义了旋钮的开发板上执行测试应用命令 ``case run knob.read``。测试会清零计数值、注册左右旋转回调，并在十秒后输出计数值。

.. _knob-api:

API 参考
------------

通过 :cpp:func:`esp_board_manager_get_device_handle` 获取 ``dev_knob_handles_t``：

.. code-block:: c

    typedef struct {
        knob_handle_t  knob_handle;
    } dev_knob_handles_t;

将 ``knob_handle`` 传入 ``iot_knob_register_cb()``、``iot_knob_get_count_value()`` 与 ``iot_knob_clear_count_value()``。声明位于 ``esp_board_manager/devices/dev_knob/dev_knob.h``。
