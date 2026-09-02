自定义设备 custom
=================

:link_to_translation:`en:[English]`

.. _custom-intro:

简介
------

``custom`` 设备用于将板级专有硬件或软件对象纳入 BMGR 统一的设备管理流程。与其他设备类型不同，``custom`` 没有内置驱动逻辑——BMGR 根据 YAML 中 ``config:`` 下的字段\ **自动生成**\ 专属配置结构体，并将其纳入统一生命周期管理。

初始化时，BMGR 按设备 ``name`` 查找已注册的初始化入口：

- \ **注册了初始化入口**\ ：调用板级 init 函数，函数返回的用户句柄通过 :cpp:func:`esp_board_manager_get_device_handle` 取回。
- \ **未注册初始化入口**\ ：初始化不报错，但句柄为 ``NULL``；配置结构体仍可通过 :cpp:func:`esp_board_manager_get_device_config` 直接访问。

典型用途：

- 自定义驱动的外围芯片（电源管理 IC、传感器、执行器等），板级代码提供初始化函数。
- 仅需将板级配置参数（I2C 地址、GPIO 编号、默认值等）暴露给应用层，无需额外初始化逻辑。

.. _custom-min:

最小配置
------------

完整字段见 :ref:`custom-full`。

``custom`` 不需要额外的 ``board_peripherals.yaml`` 条目；外设按需引用。

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: axp2101_power_manager
        chip: axp2101
        type: custom
        config:
          frequency: 400000
          i2c_addr: 0x34
        peripherals:
          - name: i2c_master

.. _custom-codegen:

代码生成结果
--------------

执行\ ``idf.py bmgr -b <board>`` 后，BMGR 为每个\ ``type: custom`` 条目在构建产物 ``components/gen_bmgr_codes/gen_board_device_custom.h`` 中生成一个专属配置结构体及其初始化值。

命名规则
^^^^^^^^^^^^^^^

结构体命名规则为\ ``dev_custom_{sanitized_name}_config_t``，其中\ ``{sanitized_name}`` 是将设备 ``name`` 中所有非法 C 标识符字符（非字母、数字、下划线）替换为 ``_`` 后的结果；以数字开头时额外补一个 ``_`` 前缀。

示例：

- ``name: axp2101_power_manager`` → ``dev_custom_axp2101_power_manager_config_t``
- ``name: my-sensor`` → ``dev_custom_my_sensor_config_t``
- ``name: 2channel`` → ``dev_custom__2channel_config_t``

固定字段
^^^^^^^^^^^^^^^

每个生成结构体的开头固定包含三个描述字段，值来自 YAML 顶层属性：

.. code-block:: c

    const char *name;  /*!< Device name */
    const char *type;  /*!< "custom" */
    const char *chip;  /*!< Chip name, or "unknown" if omitted */

用户配置字段
^^^^^^^^^^^^^^^

``config:`` 下的每个键值对生成一个结构体字段，字段名由键名按相同规则规范化得到，字段类型由值自动推断：

.. list-table::
   :header-rows: 1
   :widths: 20 20 60

   * - YAML 值类型
     - C 类型
     - 说明
   * - ``bool``
     - ``bool``
     - ``true`` / ``false``
   * - 整数
     - ``int8_t`` → ``uint8_t`` → ``int16_t`` → ``uint16_t`` → ``int32_t`` → ``uint32_t``
     - 按值域从小到大选最合适的类型，有符号优先
   * - 浮点数
     - ``float``
     - 任意小数
   * - 字符串
     - ``const char *``
     - 字面量存入 rodata
   * - 字典（嵌套对象）
     - 内联子结构体
     - 递归生成，子结构体名为\ ``{parent}_{key}_t``
   * - 列表（基本类型）
     - 对应类型的 C 数组
     - 如\ ``[1, 2, 3]`` → ``int8_t field[3]``
   * - 列表（字典）
     - 子结构体数组
     - 所有字典的键合并为一个结构体后生成数组
   * - 其他
     - ``void *``
     - 复杂类型退化为指针，值为 ``NULL``

.. note::

   ``config:`` 中的 ``peripherals`` 键保留作外设列表使用，不会生成为普通字段。

完整生成示例
^^^^^^^^^^^^^^^

以下 YAML 取自 ``test_apps/components/test_board_e/board_devices.yaml`` 的 ``motor_driver`` 条目，展示了嵌套字典、字典列表和基本类型列表如何映射到 C struct：

.. code-block:: yaml

    - name: motor_driver
      chip: mx16161
      type: custom
      config:
        motors:
          - motor_id: 1
            gpio_motor_ina: 26
            gpio_motor_inb: 27
          - motor_id: 2
            gpio_motor_ina: 28
            gpio_motor_inb: 29
          - motor_id: 3
            gpio_motor_ina: 23
            gpio_motor_inb: 22
        test_bool: true
        test_int: 123
        test_float: 3.14
        test_string: "hello"
        test_int_list: [1, 2, 3, 4, 5]
        test_dict:
          sub_val_1: 10
          sub_val_2: "nested"

执行 ``idf.py bmgr -b <board>`` 后，``gen_board_device_custom.h`` 中生成的 typedef（实际输出，节选）：

.. code-block:: c

    typedef struct {
        int8_t       motor_id;
        int8_t       gpio_motor_ina;
        int8_t       gpio_motor_inb;
    } dev_custom_motor_driver_motors_t;

    typedef struct {
        int8_t       sub_val_1;
        const char  *sub_val_2;
    } dev_custom_motor_driver_test_dict_t;

    typedef struct {
        const char *name;           /*!< Custom device name */
        const char *type;           /*!< Device type: "custom" */
        const char *chip;           /*!< Chip name */
        dev_custom_motor_driver_motors_t  motors[3];
        bool         test_bool;
        int8_t       test_int;
        float        test_float;
        const char  *test_string;
        int8_t       test_int_list[5];
        dev_custom_motor_driver_test_dict_t test_dict;
    } dev_custom_motor_driver_config_t;

可以验证以下规则：整型均推断为 ``int8_t``\ （值域在有符号范围内优先选有符号类型）；\ ``motors`` 字典列表展开为 ``dev_custom_motor_driver_motors_t motors[3]``；\ ``test_dict`` 嵌套字典生成同名子 struct；该设备未声明 ``peripherals:``，因此顶层 struct 中无外设字段。

外设字段
^^^^^^^^^^^^^^^

在 YAML 顶层 ``peripherals:`` 下声明外设时，结构体末尾追加外设相关字段：

- \ **单个外设**\ ：追加 ``uint8_t peripheral_count`` 和 ``const char *peripheral_name``。
- \ **多个外设**\ ：追加 ``uint8_t peripheral_count`` 和 ``const char *peripheral_names[DEV_CUSTOM_MAX_PERIPHERALS]``。

解析器内部常量 ``DEV_CUSTOM_MAX_PERIPHERALS`` 固定为 4。``peripherals:`` 列表超过四项时，解析器会拒绝该配置。

.. _custom-factory:

工厂函数
------------

若需要在 BMGR 初始化时自动调用板级驱动代码，需在板级源文件中实现初始化/反初始化函数，并使用 ``CUSTOM_DEVICE_IMPLEMENT`` 宏注册。

实现若依赖第三方芯片驱动组件，须将驱动头文件、生成的配置结构体、init/deinit 与 ``CUSTOM_DEVICE_IMPLEMENT`` 放在同一 ``#if __has_include`` 中。下游用 ``gen_skip`` 去掉该设备后，对应组件不再写入 ``idf_component.yml``，但板级源码仍会编译。没有独立组件头文件时，``__has_include`` 无法防护生成结构体缺失。完整约定见 :doc:`/programming-guide/board-directory`。

.. code-block:: c

    #if __has_include("my_sensor.h")
    #include "my_sensor.h"
    #include "dev_custom.h"
    #include "gen_board_device_custom.h"  /* 生成的配置结构体头文件 */

    static int my_sensor_init(void *cfg, int cfg_size, void **device_handle)
    {
        dev_custom_my_sensor_config_t *config = (dev_custom_my_sensor_config_t *)cfg;

        /* 使用 config 中的字段初始化硬件 */
        my_sensor_handle_t *handle = malloc(sizeof(my_sensor_handle_t));
        if (!handle) {
            return -1;
        }
        /* ... 初始化驱动 ... */
        *device_handle = handle;
        return 0;
    }

    static int my_sensor_deinit(void *device_handle)
    {
        free(device_handle);
        return 0;
    }

    /* 第一个参数必须与 board_devices.yaml 中该设备的 name 完全一致 */
    CUSTOM_DEVICE_IMPLEMENT(my_sensor, my_sensor_init, my_sensor_deinit);
    #endif  /* __has_include("my_sensor.h") */

``CUSTOM_DEVICE_IMPLEMENT`` 通过 GCC 属性将描述符放入特殊链接器段（``.esp_board_entries_desc``），运行时 BMGR 按设备名线性扫描该段查找初始化/反初始化函数。

CMakeLists.txt 要求
^^^^^^^^^^^^^^^^^^^

``CUSTOM_DEVICE_IMPLEMENT`` 依赖链接器保留其所在链接器段的符号。具体要求因文件位置而异：

- \ **放在板目录下**\ （与 ``board_devices.yaml`` 同级）：BMGR 生成的 ``gen_bmgr_codes`` 组件 CMakeLists.txt 已自动设置 ``WHOLE_ARCHIVE``，无需手动处理。``m5stack_cores3/power_manager.c`` 即采用此方式。

- \ **放在独立应用组件里**\ ：该组件必须在 ``idf_component_register`` 中设置 ``WHOLE_ARCHIVE``，否则链接器会将未被直接引用的 linker section 内容优化丢弃：

  .. code-block:: cmake

      idf_component_register(
          SRCS "my_device.c"
          INCLUDE_DIRS "."
          WHOLE_ARCHIVE
      )

.. _custom-app-access:

应用侧访问
--------------

BMGR 初始化完成后，可通过以下方式访问\ ``custom`` 设备。

\ **获取配置结构体**\ （有无初始化入口均可用）：

.. code-block:: c

    #include "gen_board_device_custom.h"

    dev_custom_axp2101_power_manager_config_t *cfg = NULL;
    esp_err_t ret = esp_board_manager_get_device_config(
            "axp2101_power_manager", (void **)&cfg);
    if (ret == ESP_OK) {
        uint32_t freq = cfg->frequency;
        uint8_t  addr = cfg->i2c_addr;
    }

\ **获取用户句柄**\ （仅在注册了初始化入口且初始化函数执行成功时有效）：

.. code-block:: c

    my_sensor_handle_t *handle = NULL;
    esp_err_t ret = esp_board_manager_get_device_handle(
            "my_sensor", (void **)&handle);

.. warning::

    未注册同名 entry 时，``esp_board_manager_get_device_handle`` 返回错误而非 ``NULL``，因为内部 handle 本身为 ``NULL``。\ **仅配置（config-only）用法**\ 应仅使用 :cpp:func:`esp_board_manager_get_device_config`，不应调用 :cpp:func:`esp_board_manager_get_device_handle`。

.. _custom-full:

完整字段
------------

.. code-block:: yaml

    devices:
      - name: my_custom_device          # 必填；唯一标识符，驱动注册宏的第一个参数须与此一致
        type: custom                    # 固定值
        chip: esp32s3                   # 可选，芯片名，省略时结构体中为 "unknown"
        version: 1.0.0                  # 可选，YAML schema 代号；省略等同于 default
        init_skip: false                # 可选，true 时 esp_board_manager_init() 跳过此设备
        config:
          sensor_id: 0x01               # int → int8_t（0 ≤ value ≤ 127）
          sample_rate: 100              # int → int8_t
          enable_filter: true           # bool → bool
          filter_cutoff: 50.5           # float → float
          timeout_ms: 1000              # int → int16_t
          unit: "celsius"               # str → const char *
        peripherals:                    # 可选，引用已声明的外设
          - name: i2c_master
        depends_on: []                  # 可选，初始化依赖（设备名列表）
        dependencies:                   # 可选，额外组件依赖
          esp_driver_i2c:
            require: public
            version: "^2.0.0"

.. _custom-deps:

组件依赖
------------

``custom`` 设备本身无固定依赖。需要驱动组件时在该设备的 ``dependencies`` 字段中声明，例如 ``esp_driver_i2c``、``esp_driver_gpio``、``esp_driver_ledc`` 或 ``esp_driver_adc``。

.. _custom-peripherals:

依赖外设
------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - 必选性
     - 用途
   * - 任意已支持外设
     - 由外设自身决定
     - 按板级实现决定
     - 初始化函数中引用的板级硬件资源
   * - 无
     - 无
     - 可省略
     - 仅暴露配置参数，不需要外设句柄

.. _custom-code:

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_custom.c``
- ``esp_board_manager/test_apps/components/test_board_e/board_devices.yaml``
- ``esp_board_manager/devices/dev_custom/dev_custom.c``
- ``esp_board_manager/devices/dev_custom/dev_custom.h``

.. _custom-boards:

板级参考
------------

- ``m5stack_boards/m5stack_cores3/board_devices.yaml`` + ``power_manager.c``：``axp2101_power_manager``，注册了完整初始化/反初始化入口，通过 ``config->i2c_addr``、``config->peripheral_name`` 等字段驱动 AXP2101 电源管理芯片。
- ``esp_board_manager/test_apps/components/test_board_e/board_devices.yaml``：含嵌套结构体、列表、字典列表的完整测试用例。

.. _custom-notes:

注意事项
------------

- 公共 YAML 字段规则见 :doc:`/programming-guide/yaml-rules`。
- ``CUSTOM_DEVICE_IMPLEMENT`` 的第一个参数必须与设备 ``name`` 完全一致，区分大小写。
- 包含 ``CUSTOM_DEVICE_IMPLEMENT`` 的组件须在 CMakeLists.txt 中设置 ``WHOLE_ARCHIVE``，否则链接器会优化丢弃描述符。
- 未注册同名初始化入口时，初始化不会失败，但\ :cpp:func:`esp_board_manager_get_device_handle` 返回错误；:cpp:func:`esp_board_manager_get_device_config` 仍返回有效配置结构体。
- ``peripherals:`` 最多支持四项（``DEV_CUSTOM_MAX_PERIPHERALS = 4``），更大的列表会被解析器拒绝。
- 修改 YAML 后需重新执行\ ``idf.py bmgr -b <board>``。

.. _custom-debug:

调试技巧
------------

.. _custom-api:

API 参考
----------

``custom`` 设备的配置结构体由 BMGR 在代码生成阶段按\ ``config:`` 字段自动生成，定义位于构建产物 ``components/gen_bmgr_codes/gen_board_device_custom.h``。结构体名称规则为\ ``dev_custom_{sanitized_name}_config_t``，使用前需包含此头文件。

使用 :cpp:func:`esp_board_manager_get_device_config` 获取配置结构体；若注册了初始化入口，使用 :cpp:func:`esp_board_manager_get_device_handle` 获取用户句柄。

注册宏 ``CUSTOM_DEVICE_IMPLEMENT`` 及基础配置结构 ``dev_custom_base_config_t`` 定义见 ``esp_board_manager/devices/dev_custom/dev_custom.h``。
