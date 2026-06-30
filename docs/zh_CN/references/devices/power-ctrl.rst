电源控制 power_ctrl
===================

:link_to_translation:`en:[English]`

简介
------

``power_ctrl`` 设备用于将板级电源使能信号封装为可复用的 device。其他设备可通过 ``power_ctrl_device`` 引用它，在设备初始化与关闭过程中触发上电或下电控制。

当前模板与实现支持 ``sub_type: gpio``。该模式通过一个 ``gpio`` 外设设置电源控制引脚电平，``active_level`` 表示上电有效电平。

支持的使用模式
---------------------

``power_ctrl`` 按 ``sub_type`` 区分使用模式：

- `GPIO 电源控制`_

最小配置
------------

GPIO 电源控制
^^^^^^^^^^^^^^^^^

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: gpio_power_audio
        type: gpio
        role: io
        config:
          pin: 46
          mode: GPIO_MODE_OUTPUT

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: audio_power_ctrl
        type: power_ctrl
        sub_type: gpio
        peripherals:
          - name: gpio_power_audio
            active_level: 1

      - name: audio_dac
        chip: es8311
        type: audio_codec
        power_ctrl_device: audio_power_ctrl
        config:
          adc_enabled: false
          dac_enabled: true
        peripherals:
          - name: i2s_audio_out
          - name: i2c_master
            address: 0x30
            frequency: 400000

``gpio`` 模式初始化时引用配置中的 ``gpio`` 外设并保存外设句柄；收到上电请求时把 GPIO 设置为 ``active_level``，收到下电请求时设置为相反电平。``power_ctrl`` 设备本身只定义电源控制资源，需要被控制的设备通过 ``power_ctrl_device`` 字段引用该设备名，例如 ``audio_codec``、``fs_fat`` 或 ``display_lcd`` 的板级配置。

完整字段
------------

GPIO 电源控制完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example Power Control device with GPIO sub type configuration
    - name: audio_power_ctrl          # The name of the device, must be unique
      type: power_ctrl                # The type of the device, must be unique
      sub_type: gpio                  # The sub type of the device, must be 'gpio'
      peripherals:
        - name: gpio                  # [TO_BE_CONFIRMED] GPIO peripheral name (must reference a GPIO peripheral)
          active_level: 1             # [TO_BE_CONFIRMED] Active level (0-low, 1-high) when power is on

    # Example usage in devices, add the power_ctrl_device attribute to the device configuration
    # - name: audio_dac
    #   chip: es8311
    #   type: audio_codec
    #   version: default
    #   power_ctrl_device: audio_power_ctrl  # Reference to power control device
    #   config:
    #     adc_enabled: false
    #     dac_enabled: true
    #     dac_max_channel: 1
    #     dac_channel_mask: "1"
    #     mclk_enabled: true
    #   peripherals:
    #     - name: i2s_audio_out
    #     - name: i2c_master
    #       address: 0x30
    #       frequency: 400000

组件依赖
------------

``power_ctrl`` 的 ``gpio`` 模式使用 ESP-IDF GPIO driver 和 BMGR ``gpio`` 外设。当前设备模板未要求在 ``board_devices.yaml`` 中为该设备额外声明 ``dependencies``。

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
     - ``sub_type: gpio`` 必选
     - 提供电源使能 GPIO

参考代码
------------

- ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl.c``
- ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl_sub_gpio.c``
- 板级自定义流程：:doc:`/create-board/index`

板级参考
------------

- ``esp_boards/esp_vocat_1_2/board_devices.yaml``
- ``esp_boards/esp_vocat_1_0/board_devices.yaml``
- ``esp_boards/esp32_lyrat_mini_1_1/board_devices.yaml``
- ``esp_boards/esp32_s3_box_3/board_devices.yaml``
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``
- ``esp_friends_boards/esp32_c5_spot/board_devices.yaml``

注意事项
------------

- 公共 YAML 字段规则见 :doc:`/programming-guide/yaml-rules`。
- 被控制设备的 ``power_ctrl_device`` 必须引用已定义的 ``power_ctrl`` 设备名。
- ``active_level`` 必须与板级电源开关电路一致；下电时驱动会输出相反电平。
- ``power_ctrl`` 引用的 GPIO 外设应配置为输出模式。
- 修改 YAML 后需要重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，句柄类型为 ``dev_power_ctrl_handle_t``：

.. code-block:: c

   typedef struct {
       void *periph_handle;  /*!< Peripheral handle */
   } dev_power_ctrl_handle_t;

``periph_handle`` 指向底层外设驱动的句柄（GPIO 子类型下为对应 GPIO 外设句柄），一般由 ``esp_board_device_power_ctrl()`` 通过 ``power_ctrl_device`` 引用间接调用，无需直接操作。

相关声明位于 ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl.h``。
