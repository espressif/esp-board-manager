GPIO 扩展器 gpio_expander
=========================

:link_to_translation:`en:[English]`

.. _gpio-expander-intro:

简介
------

``gpio_expander`` 设备用于描述通过 I2C 连接的 IO 扩展芯片，初始化后返回 ``esp_io_expander_handle_t`` 句柄。板级代码或其他设备可通过 :cpp:func:`esp_board_manager_get_device_handle` 获取该句柄，再设置扩展 IO 的方向、电平或上下拉能力。

该类型适用于板上 GPIO 数量不足，或 LCD 初始化线、按键、电源控制脚接在 IO 扩展芯片上的场景。

.. _gpio-expander-usage-modes:

支持的使用模式
---------------------

``gpio_expander`` 当前只有 I2C IO 扩展一种使用模式：

- :ref:`gpio-expander-i2c`

.. _gpio-expander-min-config:

最小配置
------------

.. _gpio-expander-i2c:

I2C IO 扩展
^^^^^^^^^^^^^

完整字段见 :ref:`gpio-expander-i2c-full`。

``board_peripherals.yaml`` 至少需要一个 ``i2c`` master 外设。

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: gpio_expander
        chip: tca9554                     # [TO_BE_CONFIRMED] IO expander chip
        type: gpio_expander
        dependencies:
          espressif/esp_io_expander_tca9554: "*"  # [TO_BE_CONFIRMED]
        config:
          max_pins: 8
          output_io_mask: [1, 2, 3]
          output_io_level_mask: [1, 1, 0]
          input_io_mask: NULL
        peripherals:
          - i2c_name: i2c_master
            i2c_addr: [0x70, 0x7A]        # [TO_BE_CONFIRMED]

``gpio_expander`` 初始化时引用 ``i2c`` 外设句柄，并按 ``i2c_addr`` 列表探测可响应的地址。驱动创建成功后，设备会按配置设置输出、输入、默认输出电平、可选输出模式以及可选上下拉。初始化完成后，BMGR 会记录探测到的有效 I2C 地址，供同一 I2C 总线上的其他逻辑使用。

GPIO API 映射
--------------------------------------------------

将 ``config.gpio_mapping.enabled`` 设置为 ``true``，并启用 ``CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER``，即可通过上游封装的 ESP-IDF GPIO API 访问扩展引脚。可选字段 ``start_io_num`` 指定第一个虚拟 GPIO 编号，必须大于或等于目标芯片的 ``GPIO_NUM_MAX``，省略时使用 ``GPIO_NUM_MAX``。扩展器引脚 N 对应 ``start_io_num + N``。映射范围由驱动的 ``io_count`` 决定，不随输入、输出引脚列表缩减，多个映射范围不得重叠。

BMGR 在扩展器配置完成后注册映射，在删除句柄前移除映射。使用虚拟 GPIO 前必须初始化扩展器；使用这些引脚的设备可通过 ``depends_on`` 声明对扩展器的依赖。``esp_board_manager_init()`` 先初始化外设、再初始化设备，因此普通 GPIO 外设不能在扩展器映射建立前配置这些引脚。

``gpio_config()`` 使用 64 位引脚掩码，只能表达小于 64 的引脚编号。编号大于或等于 64 时，使用 ``gpio_set_direction()`` 和 ``gpio_set_level()`` 等单引脚 API。可用操作限于上游 GPIO wrapper 支持的 API，不包括中断和 GPIO matrix 路由。

映射引脚可以配置为普通 ``gpio`` 外设，例如绑定到 audio codec 的 ``pa_name``。该外设需设置 ``init_skip: true``，使用它的设备需在 ``depends_on`` 中声明扩展器，使外设获取发生在映射注册之后。对于编号大于或等于 ``GPIO_NUM_MAX`` 的引脚，GPIO 外设初始化仅创建句柄，跳过 ``mode``、上下拉、中断和 ``default_level`` 配置。引脚配置由扩展器的输入、输出掩码及电平配置或板级 setup 代码完成。编号大于或等于 64 时，解析器输出警告，并保存独立引脚编号，不生成 GPIO 配置位掩码。使用该引脚的驱动也需支持虚拟编号；codec PA 绑定要求 codec GPIO 接口对虚拟引脚跳过掩码配置。生成阶段依据已启用映射的 ``start_io_num`` 和 ``max_pins`` 校验这类 GPIO 外设；I2C、I2S 等外设的信号引脚仍要求主控原生 GPIO。启用映射时，BMGR 要求 ``max_pins`` 与驱动的 ``io_count`` 相等，确保生成校验与运行时映射范围一致。

Audio codec 的 ``pa_name`` 和 ``reset_name`` 绑定使用 ``int16_t`` 保存引脚编号，接受 0 到 32767。生成阶段拒绝这些绑定中超出范围的虚拟 GPIO 编号。

.. _gpio-expander-full-fields:

完整字段
------------

.. _gpio-expander-i2c-full:

I2C IO 扩展完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example board_devices.yaml configuration for IO expander device
    # This shows how to integrate the IO expander device into a board configuration

    # Example IO expander device configuration
    - name: gpio_expander                 # The name of the device, must be unique
      chip: tca9554                       # [TO_BE_CONFIRMED] The chip of the IO expander
      type: gpio_expander                 # The type of the device, must be unique
      dependencies:
        espressif/esp_io_expander_generic: "*"  # [TO_BE_CONFIRMED] Component dependency for the IO expander
      config:
        max_pins: 8                       # Maximum number of IO pins supported
        gpio_mapping:
          enabled: false                 # Optional; requires IO_EXPANDER_ENABLE_GPIO_API_WRAPPER=y
          start_io_num: 64                # Optional; omit to use GPIO_NUM_MAX; ranges must not overlap
        output_io_mask: [1, 2, 3]         # List of pins configured as output, maximum number is 32
        output_io_level_mask : [1, 1, 0]  # List of output levels for output pins (eg. set io 1,2 to high level, 3 to low level)
        output_io_mode_mask: [0, 1, 1]    # Optional configuration, need to determine if the IO expansion chip is supported
                                          # List of output modes for output pins (0: push-pull, 1: open-drain),
        io_pullup_list: [1, 2]            # Optional configuration, need to determine if the IO expansion chip is supported
                                          # List of pins configured with pull-up resistors
        io_pulldown_list: [3, 4]          # Optional configuration, need to determine if the IO expansion chip is supported
                                          # List of pins configured with pull-down resistors
        input_io_mask: NULL               # List of pins configured as input (NULL if unused), maximum number is 32
      peripherals:
        - i2c_name: i2c_master             # I2C peripheral used by the IO expander
          i2c_addr: [0x70, 0x7A]          # [TO_BE_CONFIRMED] I2C address of the IO expander

.. _gpio-expander-deps:

组件依赖
------------

``gpio_expander`` 需要在 ``dependencies`` 中声明对应 IO 扩展芯片驱动组件。模板使用 ``espressif/esp_io_expander_generic`` 作为占位，板级配置应替换为与 ``chip`` 和 ``io_expander_factory_entry_t`` 匹配的组件。

.. _gpio-expander-peripherals:

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
     - 必选
     - IO 扩展芯片通信

.. _gpio-expander-code:

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_gpio_expander.c``：获取 ``gpio_expander`` 配置和句柄，并打印 IO 扩展状态。
- ``esp_board_manager/devices/dev_gpio_expander/dev_gpio_expander.c``：I2C 地址探测、IO 方向和电平初始化实现。

.. _gpio-expander-boards:

板级参考
------------

- ``esp_boards/esp32_s3_korvo_2_3/board_devices.yaml``：``gpio_expander`` 配置。
- ``esp_boards/esp32_s3_korvo_2_3/board_peripherals.yaml``：I2C 外设配置。
- ``esp_boards/esp32_s3_korvo_2_3/setup_device.c``：IO 扩展工厂函数。
- ``esp_boards/esp32_s3_lcd_ev_board/board_devices.yaml``：``gpio_expander`` 与 ``rgb_3wire_spi`` LCD 同板配置。
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``：两个 ``gpio_expander`` 设备配置。
- ``m5stack_boards/m5stack_cores3/board_devices.yaml``：``gpio_expander`` 配置。

.. _gpio-expander-notes:

注意事项
------------

- 模板中的 ``i2c_addr`` 使用地址列表，初始化会按列表探测并选择可响应的地址。
- ``output_io_mask``、``output_io_level_mask``、``output_io_mode_mask``、``io_pullup_list`` 和 ``io_pulldown_list`` 需要符合具体芯片能力；不支持的能力不应写入板级配置。
- 被其他设备引用的扩展 IO 需要在使用方初始化前可用。
- 修改 IO 扩展设备或 I2C 外设配置后，需要重新执行 ``idf.py bmgr -b <board>``。

.. _gpio-expander-factory:

工厂函数
------------

工程需要提供 ``io_expander_factory_entry_t``，由 BMGR 在创建 IO 扩展设备时回调，用于根据芯片组件创建 ``esp_io_expander_handle_t``。``dependencies`` 中的组件须与 ``chip`` 及该工厂函数匹配。函数签名为：

.. code-block:: c

   esp_err_t io_expander_factory_entry_t(i2c_master_bus_handle_t i2c_handle,
                                         const uint16_t dev_addr,
                                         esp_io_expander_handle_t *handle_ret)

板级实现须将该函数声明为弱符号 ``__attribute__((weak))``，并用 ``__has_include`` 包裹芯片驱动头文件与函数体。下游通过 ``gen_skip`` 去掉该设备，或通过 amend 替换工厂函数后，板级源码仍可编译。完整约定见 :doc:`/programming-guide/board-directory`。

最小实现示例：

.. code-block:: c

   #if __has_include(<esp_io_expander_tca9554.h>)
   #include "esp_io_expander_tca9554.h"

   __attribute__((weak)) esp_err_t io_expander_factory_entry_t(i2c_master_bus_handle_t i2c_handle,
                                                              const uint16_t dev_addr,
                                                              esp_io_expander_handle_t *handle_ret)
   {
       return esp_io_expander_new_i2c_tca9554(i2c_handle, dev_addr, handle_ret);
   }
   #endif  /* __has_include(<esp_io_expander_tca9554.h>) */

.. _gpio-expander-debug:

调试技巧
------------

.. _gpio-expander-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，返回类型为 ``esp_io_expander_handle_t``，由 ``espressif/esp_io_expander`` 组件定义。该句柄可直接传入 IO 扩展器组件的 API（读写 IO 电平、配置方向等）。

设备配置结构 ``dev_io_expander_config_t`` 及初始化函数见 ``esp_board_manager/devices/dev_gpio_expander/dev_gpio_expander.h``。
