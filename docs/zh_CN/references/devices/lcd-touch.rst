触摸屏控制器 lcd_touch
======================

:link_to_translation:`en:[English]`

简介
------

``lcd_touch`` 是通用触摸屏控制器设备，用于将触摸控制器芯片、触摸总线与 ``esp_lcd_touch`` 驱动实例封装为一个 BMGR 设备。该设备适用于已迁移到通用触摸模型的板级配置：通过 ``chip`` 与 ``dependencies`` 选择具体驱动组件，通过 ``sub_type`` 选择总线实现。

当前源码中 ``lcd_touch`` 仅实现 ``sub_type: i2c``。``sub_type: spi`` 在头文件与 Kconfig 中保留，但解析脚本会拒绝该配置，且 SPI 子类型源文件未加入组件构建。

支持的使用模式
---------------------

``lcd_touch`` 按 ``sub_type`` 区分触摸总线模式：

- `I2C 触摸`_
- `SPI 触摸预留`_

最小配置
------------

I2C 触摸
^^^^^^^^^^

``lcd_touch`` I2C 模式使用 ``i2c`` 外设提供总线句柄，初始化时创建 LCD panel IO I2C 句柄，再调用工程中链接的 ``lcd_touch_factory_entry_t`` 创建具体触摸驱动；触摸芯片依赖组件需在 ``dependencies`` 中声明。设备侧 ``peripherals`` 引用条目中的 ``i2c_addr`` 采用 8-bit 左移格式，运行时探测成功后再右移为 ESP-IDF I2C API 使用的地址；最多可写 4 个候选地址，解析脚本会拒绝奇数地址、超过 ``0xfe`` 的地址以及空列表。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: i2c_master
        type: i2c
        role: master
        config:
          port: 0
          pins:
            sda: 7               # [IO]
            scl: 8               # [IO]

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: lcd_touch
        chip: gt911              # [TO_BE_CONFIRMED]
        type: lcd_touch
        sub_type: i2c
        version: default
        dependencies:
          espressif/esp_lcd_touch_gt911: "*"
        config:
          io_i2c_config:
            lcd_cmd_bits: 16
          touch_config:
            x_max: 1024          # [TO_BE_CONFIRMED]
            y_max: 600           # [TO_BE_CONFIRMED]
        peripherals:
          - i2c_name: i2c_master
            i2c_addr: [0xBA, 0x28]

SPI 触摸预留
^^^^^^^^^^^^^^^^

``sub_type: spi`` 当前不能作为可用配置写入 ``board_devices.yaml``。解析脚本会报错提示该子类型已预留但尚未实现。

完整字段
------------

I2C 触摸完整字段
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example board_devices.yaml configuration for LCD Touch device
    # This shows how to integrate the generic LCD touch device into a board configuration.
    # The generic type uses sub_type to select the bus implementation. Only sub_type: i2c
    # is implemented now; sub_type: spi is reserved for a future implementation.
    # I2C addresses are 8-bit / left-shifted values, matching audio_codec and gpio_expander.
    # ESP-IDF I2C runtime APIs receive the selected address after shifting right by one.

    - name: lcd_touch          # The name of the device, must be unique
      chip: generic_touch      # [TO_BE_CONFIRMED] Touch chip type (e.g., cst816s, ft5x06, gt911, tt21100)
      type: lcd_touch          # Generic LCD touch device type
      sub_type: i2c            # Touch bus sub-type; only i2c is supported in this phase
      version: 1.0.0
      dependencies:
        espressif/esp_lcd_touch_generic: "*"  # [TO_BE_CONFIRMED] Component dependency for the touch chip
      config:
        # esp_lcd_panel_io_i2c_config_t fields for I2C communication
        io_i2c_config:
          control_phase_bytes: 1            # Control phase bytes (default: 1)
          dc_bit_offset: 0                  # DC bit offset in control phase (default: 0)
          lcd_cmd_bits: 8                   # [TO_BE_CONFIRMED] Bit-width of LCD command (default: 8)
          lcd_param_bits: 0                 # Bit-width of LCD parameter (default: 0)
          scl_speed_hz: 100000              # I2C SCL frequency (default: 100kHz)
          transaction_timeout_ms: 0         # IDF v6.1+: transfer timeout in ms; 0/-1 waits forever (default: 0)
          flags:
            dc_low_on_data: false           # DC level for data transfer (default: false)
            disable_control_phase: true     # Disable control phase for touch (default: true)

        # esp_lcd_touch_config_t fields for touch configuration
        touch_config:
          x_max: 320                        # [TO_BE_CONFIRMED] Maximum X coordinate (default: 320)
          y_max: 240                        # [TO_BE_CONFIRMED] Maximum Y coordinate (default: 240)
          rst_gpio_num: -1                  # [IO] Reset GPIO (default: -1, GPIO_NUM_NC)
          int_gpio_num: -1                  # [IO] Interrupt GPIO (default: -1, GPIO_NUM_NC)
          levels:
            reset: 0                        # Reset pin active level (default: 0)
            interrupt: 0                    # Interrupt pin active level (default: 0)
          flags:
            swap_xy: false                  # Swap X and Y coordinates (default: false)
            mirror_x: false                 # Mirror X coordinates (default: false)
            mirror_y: false                 # Mirror Y coordinates (default: false)
      peripherals:
        - i2c_name: i2c_master           # I2C peripheral for touch communication
          i2c_addr: [0xba]              # [TO_BE_CONFIRMED] I2C address candidates, 8-bit / left-shifted values, up to 4 entries

SPI 触摸完整字段
^^^^^^^^^^^^^^^^^^^^^^

``sub_type: spi`` 当前没有可用 YAML 模板。源码中仅保留 ``dev_lcd_touch_spi_sub_config_t`` 和未参与构建的 ``dev_lcd_touch_sub_spi.c`` 预留实现。

组件依赖
------------

``lcd_touch`` 通过 ``esp_board_manager/idf_component.yml`` 在启用 ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT`` 时引入公共组件 ``espressif/esp_lcd_touch``，版本为 ``"*"``。

具体触摸芯片驱动需在设备条目的 ``dependencies`` 中声明。已有板级配置中，GT911 触摸使用 ``espressif/esp_lcd_touch_gt911: "*"``。YAML 模板中的 ``espressif/esp_lcd_touch_generic: "*"`` 是待确认的芯片组件占位，板级维护者需替换为实际触摸芯片对应的组件。

依赖外设
------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - 必选性
     - 用途
   * - ``i2c``
     - ``master``
     - ``sub_type: i2c`` 必选
     - 提供触摸控制器通信总线；设备侧引用条目填写 ``i2c_addr``

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_lcd_init.c``
- ``esp_board_manager/devices/dev_lcd_touch/dev_lcd_touch.c``
- ``esp_board_manager/devices/dev_lcd_touch/dev_lcd_touch_sub_i2c.c``

板级参考
------------

- ``esp_boards/esp32_p4_function_ev_board/board_devices.yaml``：GT911 ``lcd_touch`` I2C 配置。
- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``：触摸设备引用的 ``i2c_master`` 配置。
- ``esp_boards/esp32_s3_box_3/board_devices.yaml``：I2C 触摸配置。
- ``m5stack_boards/m5stack_cores3/board_devices.yaml``：I2C 触摸配置。

注意事项
------------

- 新配置使用 ``type: lcd_touch`` 与 ``sub_type: i2c``。
- ``sub_type: spi`` 当前不可用；不要在板级 YAML 中配置该子类型。
- ``i2c_addr`` 使用 8-bit 左移地址，最多 4 个候选值。源码会逐个探测并记录探测成功的有效地址。
- 工程必须提供 ``lcd_touch_factory_entry_t``，用于根据触摸芯片组件创建 ``esp_lcd_touch_handle_t``。
- 修改 YAML 后需重新执行 ``idf.py bmgr -b <board>``。

工厂函数与多地址选择
--------------------

工程必须在板级源文件中提供 ``lcd_touch_factory_entry_t``，由 BMGR 在创建触摸设备时回调，用于根据触摸芯片组件创建 ``esp_lcd_touch_handle_t``。函数签名为：

.. code-block:: c

   esp_err_t lcd_touch_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                       const esp_lcd_touch_config_t *touch_dev_config,
                                       esp_lcd_touch_handle_t *ret_touch)

源文件放置与弱符号覆盖规则见 :doc:`/programming-guide/board-directory`\ 。

如果同一块开发板可能搭载不同触摸芯片（即 ``i2c_addr`` 写了多个候选地址），可以在工厂函数中通过 ``esp_board_device_get_i2c_effective_addr()`` 获取实际探测命中的地址，再选择对应驱动：

.. code-block:: c

   uint16_t touch_addr = 0;
   esp_err_t ret = esp_board_device_get_i2c_effective_addr("lcd_touch", &touch_addr);
   if (ret != ESP_OK) {
       return ret;
   }

   if (touch_addr == 0xba) {
       return esp_lcd_touch_new_i2c_gt911(io, touch_dev_config, ret_touch);
   }

   if (touch_addr == 0x48) {
       return esp_lcd_touch_new_i2c_tt21100(io, touch_dev_config, ret_touch);
   }

``esp_board_device_get_i2c_effective_addr()`` 返回的是 8-bit 左移地址，与 YAML 中的 ``i2c_addr`` 语义一致。

调试技巧
------------

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，句柄类型为 ``dev_lcd_touch_handles_t``：

.. code-block:: c

   typedef struct {
       esp_lcd_touch_handle_t     touch_handle;  /*!< LCD touch driver handle */
       esp_lcd_panel_io_handle_t  io_handle;     /*!< LCD panel IO handle */
   } dev_lcd_touch_handles_t;

相关声明位于 ``esp_board_manager/devices/dev_lcd_touch/dev_lcd_touch.h``。
