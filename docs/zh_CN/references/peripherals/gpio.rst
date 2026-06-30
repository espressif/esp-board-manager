gpio
============

:link_to_translation:`en:[English]`

简介
------

``gpio`` 外设描述单个 ESP-IDF GPIO 管脚的方向、上下拉、中断类型与默认输出电平。BMGR 将 ``board_peripherals.yaml`` 中的 ``gpio`` 条目转换为 ``periph_gpio_config_t``，初始化时调用 ``gpio_config``；在输出模式下设置默认电平。

``gpio`` 常用于功放使能、LCD 与 SD 卡电源控制、背光控制、静音控制、按键输入等简单板级信号。需要由设备管理的 GPIO 应写为外设条目，再由 ``gpio_ctrl``、``button``、``audio_codec`` 等设备引用。

支持的工作模式
---------------------

``gpio`` 按 GPIO 方向与输出方式区分配置。当前模板覆盖 ESP-IDF ``gpio_config_t`` 支持的输入、输出、输入输出与开漏模式。

- :ref:`GPIO 输入 <gpio-input>`
- :ref:`GPIO 输出 <gpio-output>`
- :ref:`GPIO 输入输出 / 开漏 <gpio-io-od>`

最小配置
------------

.. _gpio-input:

GPIO 输入
^^^^^^^^^^^

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: gpio_boot_button
        type: gpio
        config:
          pin: 0       # [IO]
          mode: GPIO_MODE_INPUT
          pull_up: true

.. _gpio-output:

GPIO 输出
^^^^^^^^^^^

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: gpio_pa_control
        type: gpio
        config:
          pin: 48      # [IO]
          mode: GPIO_MODE_OUTPUT
          default_level: 0

.. _gpio-io-od:

GPIO 输入输出 / 开漏
^^^^^^^^^^^^^^^^^^^^^^^^^^

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: gpio_control
        type: gpio
        config:
          pin: 10      # [IO]
          mode: GPIO_MODE_INPUT_OUTPUT_OD
          pull_up: true
          default_level: 1

模式说明
------------

输入模式用于按键、检测脚或中断输入。输出模式用于电源、功放、背光、静音等控制脚。输入输出和开漏模式用于需要双向或开漏特性的 GPIO 信号。

``default_level`` 只在输出类模式下由初始化代码设置。设备侧的 ``active_level``、``gain``、按键事件、业务语义不写进 ``gpio`` 外设 ``config``。

完整字段
------------

GPIO 输入 / 输出 / 输入输出完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # GPIO Peripheral Default Configuration
    # This file shows the default values used by the GPIO peripheral parser
    # Based on periph_gpio.py parsing script

    - name: gpio
      type: gpio
      config:
        # GPIO pin number (required, no default - must be >= 0)
        pin: 0                   # [IO] GPIO pin number

        # GPIO mode (default: GPIO_MODE_INPUT)
        mode: "GPIO_MODE_INPUT"  # [TO_BE_CONFIRMED] GPIO mode
        # Valid modes:
        # - GPIO_MODE_INPUT
        # - GPIO_MODE_OUTPUT
        # - GPIO_MODE_INPUT_OUTPUT
        # - GPIO_MODE_OUTPUT_OD
        # - GPIO_MODE_INPUT_OUTPUT_OD

        # Pull-up resistor (default: false)
        pull_up: false

        # Pull-down resistor (default: false)
        pull_down: false

        # Interrupt type (default: GPIO_INTR_DISABLE)
        intr_type: "GPIO_INTR_DISABLE"
        # Valid interrupt types:
        # - GPIO_INTR_DISABLE
        # - GPIO_INTR_POSEDGE
        # - GPIO_INTR_NEGEDGE
        # - GPIO_INTR_ANYEDGE
        # - GPIO_INTR_LOW_LEVEL
        # - GPIO_INTR_HIGH_LEVEL

        # Default output level (default: 0)
        default_level: 0
        # Valid values: 0 or 1

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - ``gpio_ctrl``
     - 设备 ``peripherals`` 引用一个 ``gpio`` 外设
     - ``active_level`` 和 ``default_level`` 的设备语义写在 ``gpio_ctrl`` 配置中
   * - ``button``
     - ``sub_type: gpio`` 的设备引用 ``gpio`` 外设
     - 按键触发电平和事件配置写在 ``button`` 设备中
   * - ``audio_codec``
     - 有功放控制脚时引用 ``gpio`` 外设
     - 设备侧引用条目填写 ``gain`` 和 ``active_level``
   * - ``display_lcd`` / ``lcd_touch``
     - reset、interrupt、backlight 或 power GPIO 可由设备侧字段或独立 GPIO 控制设备表达
     - LCD panel 的 ``reset_gpio_num`` 和 touch 的 ``rst_gpio_num`` / ``int_gpio_num`` 属于设备侧字段

参考代码
------------

- ``esp_board_manager/peripherals/periph_gpio/periph_gpio.c``
- ``esp_board_manager/peripherals/periph_gpio/periph_gpio.h``
- ``esp_board_manager/examples/play_embed_music/main/play_embed_music.c``
- ``esp_board_manager/examples/play_sdcard_music/main/play_sdcard_music.c``

板级参考
------------

- ``esp_boards/esp32_s3_box_3/board_peripherals.yaml``：功放、背光、SD 卡电源和静音 GPIO。
- ``esp_boards/esp32_lyrat_mini_1_1/board_peripherals.yaml``：SD 卡电源、耳机检测、功放、LED 和 SD 卡检测 GPIO。
- ``esp_friends_boards/esp32_c5_spot/board_peripherals.yaml``：功放、电源控制、codec 电源和 IMU interrupt GPIO。
- ``esp_boards/esp32_s3_lcd_ev_board/board_peripherals.yaml``：boot button GPIO。

注意事项
------------

- 公共 YAML 字段规则见 :doc:`/programming-guide/yaml-rules`。
- ``pin`` 必须是非负 GPIO 编号。该编号是否能作为输入、输出、开漏或中断源由目标芯片和板级硬件决定。
- 输出类 GPIO 初始化时会设置 ``default_level``；输入类 GPIO 不会使用该字段改变电平。
- 同一个物理 GPIO 不应在多个外设条目中重复配置为互相冲突的方向或默认电平。
- 修改 GPIO 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 GPIO 外设句柄，句柄类型为 ``periph_gpio_handle_t``：

.. code-block:: c

   typedef struct {
       gpio_num_t  gpio_num;  /*!< GPIO number */
   } periph_gpio_handle_t;

``gpio_num`` 为初始化时配置的 GPIO 编号，可直接传入 ``gpio_set_level``、``gpio_get_level`` 等 ESP-IDF GPIO API。

相关声明位于 ``esp_board_manager/peripherals/periph_gpio/periph_gpio.h``。

底层 ESP-IDF 驱动文档：`GPIO & RTC GPIO <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/gpio.html>`__\ 。
