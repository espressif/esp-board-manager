ledc
========

:link_to_translation:`en:[English]`

简介
------

``ledc`` 外设类型用于描述一个 LEDC PWM 输出通道。BMGR 根据该配置完成 LEDC timer 与 channel 的初始化，并返回包含 channel 与 speed mode 的 ``periph_ledc_handle_t``。

该外设适用于板级背光、简单 PWM 输出等场景。亮度百分比、默认亮度等设备语义属于 ``ledc_ctrl`` 设备配置，不写入 ``ledc`` 外设的 ``config``。

支持的工作模式
---------------------

``ledc`` 当前按单路 PWM 输出配置，不通过 ``role`` 进一步拆分工作模式。

- `PWM 输出`_

最小配置
------------

PWM 输出
^^^^^^^^^^

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: ledc_backlight
        type: ledc
        config:
          gpio_num: 22
          channel: LEDC_CHANNEL_0
          timer_sel: LEDC_TIMER_0
          freq_hz: 5000
          duty: 0
          duty_resolution: LEDC_TIMER_10_BIT
          speed_mode: LEDC_LOW_SPEED_MODE

模式说明
------------

``ledc`` 外设同时配置 LEDC timer 和 channel。多个 LEDC 外设若复用同一个 timer，需要保持 timer 相关参数一致；若频率或分辨率不同，应选择不同 ``timer_sel``。

完整字段
------------

PWM 输出完整字段
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # LEDC Peripheral Default Configuration
    # This file shows the default values used by the LEDC peripheral parser
    # Based on periph_ledc.py parsing script

    - name: ledc_backlight
      type: ledc
      config:
        # GPIO pin number (required, no default - must be >= 0)
        gpio_num: 0                     # [IO] GPIO pin number

        # LEDC channel (default: LEDC_CHANNEL_0)
        channel: "LEDC_CHANNEL_0"
        # Valid values: 0-7 or "LEDC_CHANNEL_0" to "LEDC_CHANNEL_7"

        # LEDC timer (default: LEDC_TIMER_0)
        timer_sel: "LEDC_TIMER_0"
        # Valid values: 0-3 or "LEDC_TIMER_0" to "LEDC_TIMER_3"

        # PWM frequency in Hz (default: 4000)
        freq_hz: 4000                  # [TO_BE_CONFIRMED] PWM frequency in Hz

        # Initial duty cycle (default: 0)
        duty: 0
        # Valid values: 0 to 2^duty_resolution

        # Duty resolution in bits (default: LEDC_TIMER_13_BIT)
        duty_resolution: "LEDC_TIMER_13_BIT"  # [TO_BE_CONFIRMED] Duty resolution in bits
        # Valid values: 1-20 or "LEDC_TIMER_1_BIT" to "LEDC_TIMER_20_BIT"

        # Speed mode (default: LEDC_LOW_SPEED_MODE)
        speed_mode: "LEDC_LOW_SPEED_MODE"
        # Valid values:
        # - LEDC_LOW_SPEED_MODE
        # - LEDC_HIGH_SPEED_MODE

        # Output inversion (default: false)
        output_invert: false

        # Sleep mode (default: LEDC_SLEEP_MODE_NO_ALIVE_NO_PD)
        sleep_mode: "LEDC_SLEEP_MODE_NO_ALIVE_NO_PD"
        # Valid values:
        # - LEDC_SLEEP_MODE_NO_ALIVE_NO_PD
        # - LEDC_SLEEP_MODE_NO_ALIVE_ALLOW_PD
        # - LEDC_SLEEP_MODE_KEEP_ALIVE

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - ``ledc_ctrl``
     - 设备侧 ``peripherals`` 引用 ``ledc`` 外设
     - ``default_percent`` 等亮度控制参数写在设备侧，PWM GPIO、timer、channel、频率和分辨率写在外设侧

参考代码
------------

- ``esp_board_manager/peripherals/periph_ledc/periph_ledc.c``
- ``esp_board_manager/devices/dev_ledc_ctrl/dev_ledc_ctrl.c``
- ``esp_board_manager/test_apps/main/test_dev_ledc.c``

板级参考
------------

- ``m5stack_boards/m5stack_tab5/board_peripherals.yaml``：定义 LCD 背光 ``ledc_backlight``。
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``：``ledc_ctrl`` 设备引用 ``ledc_backlight``。
- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``：定义 LCD 背光 ``ledc_backlight``。
- ``esp_boards/esp32_s3_box_3/board_peripherals.yaml``：定义 LCD 背光 ``ledc_backlight``。
- ``esp_boards/esp_vocat_1_0/board_peripherals.yaml``：定义 LCD 背光 ``ledc_backlight``。

注意事项
------------

- ``gpio_num`` 是 PWM 输出 GPIO，需要按目标芯片和板级连线确认。
- ``duty`` 的有效范围由 ``duty_resolution`` 决定。
- ``ledc_ctrl`` 设备引用 LEDC 外设后，设备侧不要重复配置 LEDC channel、timer、frequency 或 GPIO。
- 修改 LEDC 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 LEDC 外设句柄，句柄类型为 ``periph_ledc_handle_t``：

.. code-block:: c

   typedef struct {
       ledc_channel_t  channel;     /*!< LEDC channel number */
       ledc_mode_t     speed_mode;  /*!< LEDC speed mode */
   } periph_ledc_handle_t;

``channel`` 和 ``speed_mode`` 可传入 ``ledc_set_duty``、``ledc_update_duty`` 等 ESP-IDF LEDC API 进行动态占空比控制。

相关声明位于 ``esp_board_manager/peripherals/periph_ledc/periph_ledc.h``。

底层 ESP-IDF 驱动文档：`LED PWM 控制器 <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/ledc.html>`__\ 。
