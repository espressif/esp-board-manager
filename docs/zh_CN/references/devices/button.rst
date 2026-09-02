按键 button
===========

:link_to_translation:`en:[English]`

.. _button-intro:

简介
------

``button`` 设备用于将 GPIO 按键或 ADC 按键接入 ESP Board Manager 的 device 管理流程。初始化后，应用可通过 ``esp_board_manager_get_device_handle()`` 获取 ``dev_button_handles_t``，也可通过 ``esp_board_device_callback_register()`` 注册 YAML 中已启用的按键事件回调。

该设备基于 ``espressif/button`` 组件。GPIO 按键适用于独立按键引脚；ADC 按键适用于同一 ADC 通道上的电阻分压按键网络；自定义按键适用于矩阵键盘、触摸键、I2C 或 SPI 按键控制器等需要应用层提供 ``button_driver_t`` 的场景。

.. _button-usage-modes:

支持的使用模式
---------------------

``button`` 按 ``sub_type`` 区分使用模式：

- `GPIO 按键`_
- `ADC 单按键`_
- `ADC 多按键`_
- `自定义按键`_

.. _button-min-config:

最小配置
------------

.. _button-gpio:

GPIO 按键
^^^^^^^^^^^

完整字段见 :ref:`button-gpio-full`。

``gpio`` 模式通过 ``gpio`` 外设创建单个按键句柄。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: gpio_button_io_0
        type: gpio
        role: io
        config:
          pin: 0
          mode: "GPIO_MODE_INPUT"

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: gpio_button_0
        type: button
        sub_type: gpio
        config:
          active_level: 0
        peripherals:
          - gpio_name: gpio_button_io_0

.. _button-adc-single:

ADC 单按键
^^^^^^^^^^^^^

完整字段见 :ref:`button-adc-single-full`。

``adc_single`` 模式通过 ``adc`` 外设的 ``oneshot`` 角色创建单个 ADC 按键。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: adc_oneshot
        type: adc
        role: oneshot
        config:
          unit_id: ADC_UNIT_1
          channel_id: 4

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: adc_button_0
        type: button
        sub_type: adc_single
        config:
          button_index: 0
          min_voltage: 0
          max_voltage: 500
        peripherals:
          - adc_name: adc_oneshot

.. _button-adc-multi:

ADC 多按键
^^^^^^^^^^^^^

完整字段见 :ref:`button-adc-multi-full`。

``adc_multi`` 模式在同一 ADC 通道上创建多个按键句柄，``button_num`` 不能超过 ``CONFIG_ADC_BUTTON_MAX_BUTTON_PER_CHANNEL``。回调注册时默认使用 ``button_labels`` 中对应标签作为 ``user_data``；未配置标签时回调仍会注册，但无法通过默认 ``user_data`` 区分具体按键。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: adc_oneshot
        type: adc
        role: oneshot
        config:
          unit_id: ADC_UNIT_1
          channel_id: 4

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: adc_button_group
        type: button
        sub_type: adc_multi
        config:
          button_num: 6
          voltage_range: [380, 820, 1110, 1650, 1980, 2410]
          button_labels: ["VOLUME_UP", "VOLUME_DOWN", "SET", "PLAY", "MUTE", "REC"]
          max_voltage: 3000
        peripherals:
          - adc_name: adc_oneshot

.. _button-custom:

自定义按键
^^^^^^^^^^^^

完整字段见 :ref:`button-custom-full`。

``custom`` 不依赖 ``board_peripherals.yaml`` 中的条目；总线和 GPIO 由应用侧实现的驱动自行管理。

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: custom_button_0
        type: button
        sub_type: custom

应用代码（例如 ``setup_device.c`` 或其他参与组件构建的源文件）需要使用 ``DEVICE_EXTRA_FUNC_REGISTER`` 注册一个与设备名同名的 ``button_driver_t`` 创建函数：

.. code-block:: c

    #include "dev_button.h"
    #include "esp_board_extra_func_entry.h"

    static button_driver_t *custom_button_0(const dev_button_config_t *config)
    {
        button_driver_t *driver = calloc(1, sizeof(button_driver_t));
        // Fill driver->get_key_level / driver->del / driver->enter_power_save according to hardware
        return driver;
    }
    DEVICE_EXTRA_FUNC_REGISTER(custom_button_0, custom_button_0);

注册函数名必须与 ``board_devices.yaml`` 中的 ``name`` 完全一致。BMGR 初始化时通过设备名查找该函数，并将返回的 ``button_driver_t`` 交给 ``iot_button_create()``。``custom`` 模式的 ``peripherals`` 列表必须为空；``events_cfg`` 与计时字段的配置方式与其他子类型一致。

.. _button-full-fields:

完整字段
------------

.. _button-gpio-full:

GPIO 按键完整字段
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example GPIO Button device configuration
    - name: gpio_button_0            # The name of the device, must be unique
      type: button                   # The type of the device
      sub_type: gpio                 # Button sub type: "gpio", "adc_single", or "adc_multi"
      config:
        # GPIO Configuration
        active_level: 0              # [TO_BE_CONFIRMED] Active level (0-low, 1-high) when button is pressed
        enable_power_save: false     # Enable power save mode (default: false)
        disable_pull: false          # Disable internal pull-up/pull-down (default: false)

        # Button Timing Configuration
        long_press_time: 2000        # Long press time in milliseconds (default: 2000)
        short_press_time: 100        # Short press time in milliseconds (default: 100)

        # Event Configuration (NEW - Easy button features)
        events_cfg:
          press_down: true           # Enable press down event (default: true)
          press_up: true             # Enable press up event (default: true)
          single_click: true         # Enable single click event (default: true)
          double_click: true         # Enable double click event (default: true)
          multi_click: false         # Enable multi click event (default: false)
          click_counts: []           # Multi click counts configuration (default: [])
                                     # eg: Set to [3, 4] means enable multi_click event with 3 and 4 counts
          long_press_start: true     # Enable long press start event (default: true)
          long_press_start_time: []  # Long press start time configuration in ms (default: [])
                                     # eg: Set to [3000, 4000] means enable long_press_start event with 3 and 4 seconds
          long_press_hold: false     # Enable long press hold event (default: false)
          long_press_up: true        # Enable long press up event (default: true)
          long_press_up_time: []     # Long press up time configuration in ms (default: [])
          press_repeat: false        # Enable press repeat event (default: false)
          press_repeat_done: false   # Enable press repeat done event (default: false)
          press_end: false           # Enable press end event (default: false)

      peripherals:
        - gpio_name: gpio            # [TO_BE_CONFIRMED] GPIO peripheral name

.. _button-adc-single-full:

ADC 单按键完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example ADC Button device configuration (Single button)
    - name: adc_button_0      # The name of the device, must be unique
      type: button            # The type of the device
      sub_type: adc_single    # Button sub type: "gpio", "adc_single", or "adc_multi"
      config:
        # ADC Configuration
        button_index: 0              # [TO_BE_CONFIRMED] Button index on the channel
        min_voltage: 0               # [TO_BE_CONFIRMED] Minimum voltage in mV for button press
        max_voltage: 500             # [TO_BE_CONFIRMED] Maximum voltage in mV for button press

        # Button Timing Configuration
        long_press_time: 2000        # Long press time in milliseconds (default: 2000)
        short_press_time: 100        # Short press time in milliseconds (default: 100)

        # Event Configuration (NEW - Easy button features)
        events_cfg:
          press_down: true           # Enable press down event (default: true)
          press_up: true             # Enable press up event (default: true)
          single_click: true         # Enable single click event (default: true)
          double_click: true         # Enable double click event (default: true)
          multi_click: false         # Enable multi click event (default: false)
          click_counts: []           # Multi click counts configuration (default: [])
                                     # eg: Set to [3, 4] means enable multi_click event with 3 and 4 counts
          long_press_start: true     # Enable long press start event (default: true)
          long_press_start_time: []  # Long press start time configuration in ms (default: [])
                                     # eg: Set to [3000, 4000] means enable long_press_start event with 3 and 4 seconds
          long_press_hold: false     # Enable long press hold event (default: false)
          long_press_up: true        # Enable long press up event (default: true)
          long_press_up_time: []     # Long press up time configuration in ms (default: [])
          press_repeat: false        # Enable press repeat event (default: false)
          press_repeat_done: false   # Enable press repeat done event (default: false)
          press_end: false           # Enable press end event (default: false)

      peripherals:
        - adc_name: adc_oneshot       # [TO_BE_CONFIRMED] ADC peripheral name

.. _button-adc-multi-full:

ADC 多按键完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example ADC Button device configuration (Multi buttons)
    - name: adc_button_group         # The name of the device, must be unique
      type: button                   # The type of the device
      sub_type: adc_multi            # Button sub type: "gpio", "adc_single", or "adc_multi"
      config:
        # ADC Configuration
        button_num: 6                                      # [TO_BE_CONFIRMED] Number of buttons in the group, must not be greater than CONFIG_ADC_BUTTON_MAX_BUTTON_PER_CHANNEL (default: 8)
        voltage_range: [380, 820, 1110, 1650, 1980, 2410]  # [TO_BE_CONFIRMED] Nominal voltage for each button in mV
        # This voltage_range configuration is only applicable to the Korvo2 V3 development board.
        # Please refer to the schematic of your development board to determine the correct configuration.
        button_labels: ["VOLUME_UP", "VOLUME_DOWN", "SET", "PLAY", "MUTE", "REC"]  # Labels for each button
        # Optional but recommended labels for each button in the group.
        # When esp_board_device_callback_register(name, cb, NULL) is used, these labels are passed as callback user_data by default.
        # If omitted, ADC multi-button callbacks still work, but the default user_data is NULL so the callback cannot distinguish buttons by label.
        max_voltage: 3000                                  # Maximum voltage in mV for this ADC channel (default: 3000)

        # Button Timing Configuration
        long_press_time: 2000        # Long press time in milliseconds (default: 2000)
        short_press_time: 100        # Short press time in milliseconds (default: 100)

        # Event Configuration (NEW - Easy button features)
        events_cfg:
          press_down: true           # Enable press down event (default: true)
          press_up: true             # Enable press up event (default: true)
          single_click: true         # Enable single click event (default: true)
          double_click: true         # Enable double click event (default: true)
          multi_click: false         # Enable multi click event (default: false)
          click_counts: []           # Multi click counts configuration (default: [])
                                     # eg: Set to [3, 4] means enable multi_click event with 3 and 4 counts
          long_press_start: true     # Enable long press start event (default: true)
          long_press_start_time: []  # Long press start time configuration in ms (default: [])
                                     # eg: Set to [3000, 4000] means enable long_press_start event with 3 and 4 seconds
          long_press_hold: false     # Enable long press hold event (default: false)
          long_press_up: true        # Enable long press up event (default: true)
          long_press_up_time: []     # Long press up time configuration in ms (default: [])
          press_repeat: false        # Enable press repeat event (default: false)
          press_repeat_done: false   # Enable press repeat done event (default: false)
          press_end: false           # Enable press end event (default: false)

      peripherals:
        - adc_name: adc_oneshot       # [TO_BE_CONFIRMED] ADC peripheral name

.. _button-custom-full:

自定义按键完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example Custom Button device configuration
    - name: custom_button_0          # The name of the device, must be unique
      type: button                   # The type of the device
      sub_type: custom               # Button sub type: "gpio", "adc_single", "adc_multi", or "custom"
      config:
        # Custom Driver Configuration
        # The name of the registered custom driver creator function must match the device name.
        # A function with this name must be registered using DEVICE_EXTRA_FUNC_REGISTER.

        # Button Timing Configuration
        long_press_time: 2000        # Long press time in milliseconds (default: 2000)
        short_press_time: 100        # Short press time in milliseconds (default: 100)

        # Event Configuration (NEW - Easy button features)
        events_cfg:
          press_down: true           # Enable press down event (default: true)
          press_up: true             # Enable press up event (default: true)
          single_click: true         # Enable single click event (default: true)
          double_click: true         # Enable double click event (default: true)
          multi_click: false         # Enable multi click event (default: false)
          click_counts: []           # Multi click counts configuration (default: [])
          long_press_start: true     # Enable long press start event (default: true)
          long_press_start_time: []  # Long press start time configuration in ms (default: [])
          long_press_hold: false     # Enable long press hold event (default: false)
          long_press_up: true        # Enable long press up event (default: true)
          long_press_up_time: []     # Long press up time configuration in ms (default: [])
          press_repeat: false        # Enable press repeat event (default: false)
          press_repeat_done: false   # Enable press repeat done event (default: false)
          press_end: false           # Enable press end event (default: false)

.. _button-deps:

组件依赖
------------

启用 ``button`` 设备时，组件清单会按 ``CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT`` 引入 ``espressif/button``，版本约束为 ``^4.1.4``。GPIO 与 ADC 子模式使用该组件提供的 ``button_gpio`` 和 ``button_adc`` 接口。

.. _button-peripherals:

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
     - 提供单个 GPIO 按键输入
   * - ``adc``
     - ``oneshot``
     - ``sub_type: adc_single`` 和 ``sub_type: adc_multi`` 必选
     - 提供 ADC 按键电压采样通道
   * - 无
     - 无
     - ``sub_type: custom`` 不引用任何 ``board_peripherals.yaml`` 条目
     - 总线和 GPIO 由应用侧 ``button_driver_t`` 实现自行管理

.. _button-code:

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_button.c``
- ``esp_board_manager/devices/dev_button/dev_button.c``
- ``esp_board_manager/devices/dev_button/dev_button_sub_gpio.c``
- ``esp_board_manager/devices/dev_button/dev_button_sub_adc_single.c``
- ``esp_board_manager/devices/dev_button/dev_button_sub_adc_multi.c``
- ``esp_board_manager/devices/dev_button/dev_button_sub_custom.c``

.. _button-boards:

板级参考
------------

- ``esp_boards/esp32_s3_korvo_2_3/board_devices.yaml``
- ``esp_boards/esp32_s3_korvo_2_3/board_peripherals.yaml``
- ``esp_boards/esp32_lyrat_mini_1_1/board_devices.yaml``
- ``esp_boards/esp32_s3_lcd_ev_board/board_devices.yaml``
- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_s3_devkitc/board_devices.yaml``

.. _button-notes:

注意事项
------------

- 公共 YAML 字段规则见 :doc:`/programming-guide/yaml-rules`。
- ``adc_multi`` 的 ``voltage_range`` 与 ``button_num`` 必须与板级电阻分压网络一致，不应直接照搬其他开发板的数值。
- ``events_cfg`` 控制 ``esp_board_device_callback_register()`` 注册的事件集合；旧的 ``events`` 字段在当前模板中不再使用。
- ``custom`` 子类型注册的 driver 创建函数名必须与设备 ``name`` 一致；``board_devices.yaml`` 不允许为该子类型添加 ``peripherals``。
- 修改 YAML 后需重新执行 ``idf.py bmgr -b <board>``。

.. _button-factory:

工厂函数
------------

``sub_type: custom`` 需要用 ``DEVICE_EXTRA_FUNC_REGISTER`` 注册与设备 ``name`` 同名的 ``button_driver_t`` 创建函数。注册函数名必须等于 ``board_devices.yaml`` 中的设备 ``name``。说明与示例见 :ref:`button-custom`。

.. _button-debug:

调试技巧
------------

.. _button-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，句柄类型为 ``dev_button_handles_t``：

.. code-block:: c

   typedef struct {
       uint8_t          num_buttons;                                               /*!< Number of button handles */
       button_handle_t  button_handles[CONFIG_ADC_BUTTON_MAX_BUTTON_PER_CHANNEL];  /*!< Flexible array of button handles */
   } dev_button_handles_t;

``num_buttons`` 为实际初始化的按键数量；``button_handles`` 可传入 ``espressif/button`` 组件的回调注册接口。

使用 :cpp:func:`esp_board_device_callback_register` 可以按 device 名称注册按键回调。该 API 会使用已初始化的 ``button`` 设备句柄，并按照 ``events_cfg`` 中启用的事件，将同一个 ``button_cb_t`` 注册到该设备下的所有按键句柄。调用该 API 前，目标 device 必须已经初始化。

.. code-block:: c

   #include "dev_button.h"
   #include "esp_board_device.h"

   static void button_event_handler(void *button_handle, void *user_data)
   {
       button_handle_t btn = (button_handle_t)button_handle;
       button_event_t event = iot_button_get_event(btn);

       ESP_LOGI(TAG, "button event: %s, user_data: %s",
                iot_button_get_event_str(event),
                (const char *)user_data);
   }

   void app_register_button_callback(void)
   {
       ESP_ERROR_CHECK(esp_board_device_callback_register(
           "gpio_button_0", button_event_handler, NULL));
   }

``user_data`` 可以用于传入应用自定义上下文，传入 ``NULL`` 时，BMGR 会使用默认值：单按键设备使用设备 ``name``，``adc_multi`` 设备使用 ``button_labels`` 中与按键索引对应的字符串。

相关声明位于 ``esp_board_manager/devices/dev_button/dev_button.h``。
