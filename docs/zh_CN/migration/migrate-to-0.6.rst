迁移到 0.6.0
============

:link_to_translation:`en:[English]`

本页汇总升级到 ``esp_board_manager`` 0.6.0 的迁移说明。升级前请先阅读相关小节，再按验证步骤确认迁移结果。

``dev_lcd_touch_i2c`` 迁移到 ``dev_lcd_touch``
----------------------------------------------

``dev_lcd_touch_i2c`` 是旧的 I2C 触摸设备类型。自 ``esp_board_manager`` v0.5.10 起，建议统一使用通用触摸设备 ``dev_lcd_touch``，并通过 ``sub_type: i2c`` 声明 I2C 实现。

自 0.6.0 起，旧的 ``dev_lcd_touch_i2c`` 设备（实现、解析器、``dev_lcd_touch_i2c.h`` 头文件以及 ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_I2C_SUPPORT`` 兼容宏）已被移除。板级配置和 APP 代码必须使用 ``type: lcd_touch`` + ``sub_type: i2c``；旧类型和旧头文件不再提供。

迁移原因
~~~~~~~~~~

- 统一设备模型：触摸设备统一归类为 ``lcd_touch``，总线类型由 ``sub_type`` 表达。
- 支持多地址探测：I2C 地址从旧的单一 ``io_i2c_config.dev_addr`` 迁移到设备级 ``peripherals[].i2c_addr``，最多支持 4 个候选地址。
- 支持运行时查询有效地址：可通过 ``esp_board_device_get_i2c_effective_addr()`` 获取最终探测到的 8-bit 地址。

YAML 迁移
~~~~~~~~~

旧写法：

.. code-block:: yaml

   - name: lcd_touch
     chip: cst816s
     type: lcd_touch_i2c
     dependencies:
       espressif/esp_lcd_touch_cst816s: "*"
     config:
       io_i2c_config:
         dev_addr: 0x15
         lcd_cmd_bits: 8
         flags:
           disable_control_phase: true
         peripherals:
           - name: i2c_master
       touch_config:
         x_max: 284
         y_max: 240
         rst_gpio_num: -1
         int_gpio_num: 3
         flags:
           swap_xy: true
           mirror_x: false
           mirror_y: true

新写法：

.. code-block:: yaml

   - name: lcd_touch
     chip: cst816s
     type: lcd_touch
     sub_type: i2c
     dependencies:
       espressif/esp_lcd_touch_cst816s: "*"
     config:
       io_i2c_config:
         lcd_cmd_bits: 8
         flags:
           disable_control_phase: true
       touch_config:
         x_max: 284
         y_max: 240
         rst_gpio_num: -1
         int_gpio_num: 3
         flags:
           swap_xy: true
           mirror_x: false
           mirror_y: true
     peripherals:
       - name: i2c_master
         i2c_addr: 0x2a

字段对应关系
~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 30 40

   * - 旧字段
     - 新字段
     - 说明
   * - ``type: lcd_touch_i2c``
     - ``type: lcd_touch`` + ``sub_type: i2c``
     - 设备类型统一为 ``lcd_touch``，总线类型放到 ``sub_type``
   * - ``config.io_i2c_config.dev_addr``
     - ``peripherals[].i2c_addr``
     - 新字段使用 8-bit 左移地址
   * - ``config.io_i2c_config.peripherals[].name``
     - ``peripherals[].name``
     - I2C 外设依赖移到设备根级 ``peripherals``
   * - ``config.io_i2c_config.*``
     - ``config.io_i2c_config.*``
     - 除 ``dev_addr`` 和嵌套 ``peripherals`` 外，其余字段保留
   * - ``config.touch_config.*``
     - ``config.touch_config.*``
     - 保持不变
   * - ``dependencies``
     - ``dependencies``
     - 保持不变

I2C 地址规则
~~~~~~~~~~~~

新 ``lcd_touch`` 的 ``peripherals[].i2c_addr`` 使用 8-bit 左移地址。

例如：

- 7-bit 地址 ``0x15`` 应写为 ``0x2a``
- 7-bit 地址 ``0x24`` 应写为 ``0x48``
- 7-bit 地址 ``0x5d`` 应写为 ``0xba``

如果一块开发板可能搭载不同触摸芯片，可以写多个候选地址：

.. code-block:: yaml

   peripherals:
     - name: i2c_master
       i2c_addr: [0xba, 0x48]

Board Manager 会按顺序探测地址，并记录最终命中的 8-bit 地址。

板级 ``setup_device.c`` 迁移
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

触摸工厂函数 ``lcd_touch_factory_entry_t`` 的签名保持不变，迁移时无需修改 ``setup_device.c`` 中工厂函数的签名。

如果开发板需要根据探测到的有效地址选择不同触摸驱动，相关用法见设备参考 :doc:`/references/devices/lcd-touch`。

APP 兼容注意事项
~~~~~~~~~~~~~~~~

迁移后的 APP 必须使用新的通用触摸设备宏和 I2C 子类型宏：

.. code-block:: c

   #if CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT && CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUB_I2C_SUPPORT
   // enable touch-related code
   #endif

旧的 ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_I2C_SUPPORT`` 宏已不再定义。任何仍判断该宏的代码会被静默编译掉，导致触摸功能失效。

旧结构体也已不再定义。请勿使用：

.. code-block:: c

   dev_lcd_touch_i2c_config_t
   dev_lcd_touch_i2c_handles_t

请改用新类型：

.. code-block:: c

   dev_lcd_touch_config_t
   dev_lcd_touch_handles_t

头文件迁移
^^^^^^^^^^

旧头文件 ``dev_lcd_touch_i2c.h`` 已被删除。直接包含它的代码：

.. code-block:: c

   #include "dev_lcd_touch_i2c.h"

将无法编译，必须改为：

.. code-block:: c

   #include "dev_lcd_touch.h"

如果应用统一包含：

.. code-block:: c

   #include "esp_board_manager_includes.h"

include 本身无需修改，但代码中的结构体类型仍需要按下面迁移。

Handle 迁移
^^^^^^^^^^^

旧代码：

.. code-block:: c

   void *touch_handle = NULL;
   ESP_ERROR_CHECK(esp_board_manager_get_device_handle("lcd_touch", &touch_handle));

   dev_lcd_touch_i2c_handles_t *touch = (dev_lcd_touch_i2c_handles_t *)touch_handle;
   esp_lcd_touch_handle_t tp = touch->touch_handle;
   esp_lcd_panel_io_handle_t io = touch->io_handle;

新代码：

.. code-block:: c

   void *touch_handle = NULL;
   ESP_ERROR_CHECK(esp_board_manager_get_device_handle("lcd_touch", &touch_handle));

   dev_lcd_touch_handles_t *touch = (dev_lcd_touch_handles_t *)touch_handle;
   esp_lcd_touch_handle_t tp = touch->touch_handle;
   esp_lcd_panel_io_handle_t io = touch->io_handle;

Config 迁移
^^^^^^^^^^^

旧代码：

.. code-block:: c

   dev_lcd_touch_i2c_config_t *cfg = NULL;
   ESP_ERROR_CHECK(esp_board_manager_get_device_config("lcd_touch", (void **)&cfg));

   const char *i2c_name = cfg->i2c_name;
   uint16_t primary_addr = cfg->i2c_addr[0];
   uint16_t runtime_addr = cfg->io_i2c_config.dev_addr;

新代码：

.. code-block:: c

   dev_lcd_touch_config_t *cfg = NULL;
   ESP_ERROR_CHECK(esp_board_manager_get_device_config("lcd_touch", (void **)&cfg));

   const char *i2c_name = cfg->sub_cfg.i2c.i2c_name;
   size_t addr_count = cfg->sub_cfg.i2c.i2c_addr_count;
   const uint16_t *addr_candidates = cfg->sub_cfg.i2c.i2c_addr;

注意：新配置中的 ``cfg->sub_cfg.i2c.i2c_addr[]`` 是候选地址列表，不一定是最终命中的地址。如果 APP 需要知道当前实际使用的触摸地址，请使用有效地址查询 API ``esp_board_device_get_i2c_effective_addr()``。

有效地址查询迁移
^^^^^^^^^^^^^^^^

旧代码可能从旧 config 中读取 ``io_i2c_config.dev_addr`` 或 ``i2c_addr[0]`` 来判断触摸芯片。迁移后请改为：

.. code-block:: c

   uint16_t touch_addr = 0;
   esp_err_t ret = esp_board_device_get_i2c_effective_addr("lcd_touch", &touch_addr);
   if (ret == ESP_OK) {
       // touch_addr is the selected 8-bit / left-shifted address
   }

编译条件迁移
^^^^^^^^^^^^

现在所有开发板都使用 ``type: lcd_touch``，请用 ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT`` 守护触摸代码，并在需要时结合 ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUB_I2C_SUPPORT`` 判断 I2C 子类型，旧的 ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_I2C_SUPPORT`` 兼容分支已不存在。

验证步骤
~~~~~~~~

#. 运行 ``idf.py bmgr -b <board_name>`` 重新生成配置。
#. 确认没有板级 YAML 仍在使用 ``type: lcd_touch_i2c``。该旧类型在 0.6.0 已被移除，残留的旧 YAML 现在会让 ``idf.py bmgr`` 直接报“未知或不支持的设备类型”错误并中止，而不再是 deprecated warning。
#. 检查 ``components/gen_bmgr_codes/board_manager.defaults`` 中包含：

   .. code-block:: ini

      CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT=y
      CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUB_I2C_SUPPORT=y

#. 确认迁移后的 APP 代码不再使用 ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_I2C_SUPPORT`` 或旧 ``dev_lcd_touch_i2c_*`` 类型（这些已不再定义）。
#. 编译工程，确认没有任何对已删除的 ``dev_lcd_touch_i2c.h`` 头文件的引用（此类引用现在会直接编译失败）。
#. 如果板级 factory 根据触摸芯片地址分支，确认 ``esp_board_device_get_i2c_effective_addr()`` 能返回预期的 8-bit 地址。

辅助迁移工具
~~~~~~~~~~~~

组件内提供了一个可选的 AI Skill，用于辅助完成 ``dev_lcd_touch_i2c`` 到 ``dev_lcd_touch`` 的迁移。该 Skill 会引导 AI 按固定流程检查板级 YAML、``setup_device.c`` 以及 APP 侧旧类型/旧宏的使用，并按照本节的字段映射和验证步骤完成迁移。

建议先阅读本节，明确地址格式、兼容宏和 APP 代码迁移要求；如果希望让 AI 助手协助执行或复查迁移，可使用 :doc:`/tools/ai-skill` 中介绍的 ``lcd-touch-i2c-migration`` AI Skill。
