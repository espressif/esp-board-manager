电源控制 power_ctrl
===================

:link_to_translation:`en:[English]`

简介
------

``power_ctrl`` 设备用于将板级电源使能信号封装为可复用的 device。其他设备可通过 ``power_ctrl_device`` 引用它，在设备初始化与关闭过程中触发上电或下电控制。

``sub_type: gpio`` 通过一个 ``gpio`` 外设设置电源控制引脚电平，``active_level`` 表示上电有效电平。``sub_type: custom`` 通过板级生命周期操作支持 PMIC、IO 扩展器、多路电源时序及其他板级电源实现。

支持的使用模式
---------------------

``power_ctrl`` 按 ``sub_type`` 区分使用模式：

- `GPIO 电源控制`_
- `自定义电源控制`_

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
          - gpio_name: gpio_power_audio
            active_level: 1

      - name: audio_dac
        chip: es8311
        type: audio_codec
        power_ctrl_device: audio_power_ctrl
        config:
          adc_enabled: false
          dac_enabled: true
        peripherals:
          - i2s_name: i2s_audio_out
          - i2c_name: i2c_master
            address: 0x30
            frequency: 400000

``gpio`` 模式初始化时引用配置中的 ``gpio`` 外设并保存外设句柄；收到上电请求时把 GPIO 设置为 ``active_level``，收到下电请求时设置为相反电平。``power_ctrl`` 设备本身只定义电源控制资源，需要被控制的设备通过 ``power_ctrl_device`` 字段引用该设备名，例如 ``audio_codec``、``fs_fat`` 或 ``display_lcd`` 的板级配置。

自定义电源控制
^^^^^^^^^^^^^^^^^

板级需要使用 PMIC、IO 扩展器、多路电源或特定上电时序时使用 ``custom``。在板级代码中以与 ``power_ctrl`` 设备同名的名称注册操作。源文件放置与构建规则见 :doc:`/programming-guide/board-directory`\ 。``init`` 与 ``deinit`` 为可选，``set_power`` 为必选。框架会在调用 ``init`` 前引用 ``peripherals`` 中的外设，并在 ``deinit`` 后释放引用。PMIC 等独立 device 依赖应使用 ``depends_on`` 声明。

``gpio_name``、``i2c_name`` 等语义化选择器仅适用于设备规则明确支持的绑定场景，不适用于 ``custom`` 电源控制器中用于声明生命周期依赖的 ``peripherals`` 列表。``custom`` 电源控制器中的 ``peripherals`` 列表仍使用旧版的 ``name`` 引用写法。

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: board_power_ctrl
        type: power_ctrl
        sub_type: custom
        depends_on: pmic_device
        config:
          startup_delay_ms: 10

``board_power.c``：

.. code-block:: c

    #include "dev_power_ctrl.h"
    #include "gen_board_device_custom.h"
    #include "esp_board_extra_func_entry.h"

    static int board_power_init(const dev_power_ctrl_config_t *config, void **context)
    {
        const dev_custom_board_power_ctrl_custom_config_t *user_cfg =
            config->sub_cfg.custom.user_cfg;
        (void)user_cfg;
        *context = NULL;
        return 0;
    }

    static int board_power_set_power(void *context, const char *consumer, bool power_on)
    {
        (void)context;
        (void)consumer;
        (void)power_on;
        return 0;
    }

    static const dev_power_ctrl_custom_ops_t s_board_power_ctrl_ops = {
        .init = board_power_init,
        .deinit = NULL,
        .set_power = board_power_set_power,
    };

    DEVICE_EXTRA_FUNC_REGISTER(board_power_ctrl, &s_board_power_ctrl_ops);

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
        - gpio_name: gpio             # [TO_BE_CONFIRMED] GPIO peripheral name (must reference a GPIO peripheral)
          active_level: 1             # [TO_BE_CONFIRMED] Active level (0-low, 1-high) when power is on

自定义电源控制完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: board_power_ctrl
      type: power_ctrl
      sub_type: custom
      depends_on: pmic_device          # 可选的 device 依赖
      peripherals:                     # 可选，由框架管理引用的外设
        - name: i2c_pmic
      config:                          # 可选，传入 user_cfg 的强类型板级配置
        startup_delay_ms: 10

    # Example usage in devices, add the power_ctrl_device attribute to the device configuration
    # - name: audio_dac
    #   chip: es8311
    #   type: audio_codec
    #   power_ctrl_device: audio_power_ctrl  # Reference to power control device
    #   config:
    #     adc_enabled: false
    #     dac_enabled: true
    #     sys_cfg:
    #       no_mclk: false
    #   peripherals:
    #     - i2s_name: i2s_audio_out
    #     - i2c_name: i2c_master
    #       address: 0x30
    #       frequency: 400000

组件依赖
------------

``power_ctrl`` 的 ``gpio`` 模式使用 ESP-IDF GPIO driver 和 BMGR ``gpio`` 外设。``custom`` 没有 BMGR 强加的组件依赖，板级所需组件应在使用它们的 device 中声明。

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
   * - 任意已支持的外设
     - 取决于外设类型
     - ``sub_type: custom`` 可选
     - 由框架在自定义控制器生命周期内保持引用

参考代码
------------

- ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl.c``
- ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl_sub_gpio.c``
- ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl_sub_custom.c``
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
- ``custom`` 控制器注册的名称必须与 ``power_ctrl`` 设备名一致，且必须提供 ``set_power``。
- 自定义生命周期代码只拥有自己创建的 ``context``；``peripherals`` 中的引用由框架管理。
- 修改 YAML 后需要重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，句柄类型为 ``dev_power_ctrl_handle_t``：

.. code-block:: c

   typedef struct {
       void *periph_handle;
       const dev_power_ctrl_custom_ops_t *custom_ops;
       void *custom_context;
   } dev_power_ctrl_handle_t;

``periph_handle`` 指向 GPIO 子类型的底层外设句柄。``custom_context`` 是 custom 子类型的板级生命周期状态。一般由 ``esp_board_device_power_ctrl()`` 通过 ``power_ctrl_device`` 引用间接调用，无需直接操作。

相关声明位于 ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl.h``。
