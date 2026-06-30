复制现有开发板
==========================================================================

:link_to_translation:`en:[English]`

新开发板与某块已有开发板硬件高度相似时，复制该开发板目录并在其基础上修改，是最快的适配路径。差异仅限少量引脚或器件时，建议改用 :doc:`amend`，无需复制整份目录。

步骤
--------------

**第一步：找到参考开发板**

在 BMGR 文档的 :doc:`/references/boards/index` 页面中查看已支持的开发板列表，了解各开发板的硬件组合（芯片、外设、设备），从中找到与目标开发板最相似的一块。

**第二步：复制目录并更新名称**

确保工程根目录下已有 ``components/`` 文件夹（标准 IDF 工程通常已存在），然后将参考开发板目录复制过去，目标路径即为新开发板名称：

.. code-block:: bash

   cp -r managed_components/espressif__esp_boards/esp32_s3_korvo_2_3 components/my_board

上例假设参考开发板来自组件管理器下载的 ``espressif/esp_boards``。如果使用源码仓库中的板级组件目录，请将源路径替换为对应的 ``esp_boards/<board_name>``、``esp_friends_boards/<board_name>`` 或 ``m5stack_boards/<board_name>`` 目录。

执行后会生成 ``components/my_board/`` 目录，其中直接包含 ``board_info.yaml`` 等文件。若 ``components/my_board`` 已存在，``cp -r`` 会在其内部再创建一层子目录，导致结构错误；请确认目标路径尚不存在再执行。

复制完成后，将 ``board_info.yaml`` 中的 ``board`` 字段改为与目录名一致的新名称：

.. code-block:: yaml

   board: my_board   # 必须与目录名一致

开发板名称只允许小写字母、数字和下划线，不支持中划线。

**第三步：按原理图修改配置**

对照新开发板的原理图，逐项比对并修改与参考开发板不同的引脚和配置字段。常见修改点：

- ``board_peripherals.yaml`` 中各外设的 GPIO 编号、总线编号等参数。
- ``board_devices.yaml`` 中设备的 GPIO 编号、芯片型号（``chip`` 字段）、依赖的组件（``dependencies``）与 I2C 地址等配置。
- ``board_info.yaml`` 中的 ``chip``、``description``、``manufacturer`` 等元数据字段。
- 如有新增或删除的设备/外设，按 :doc:`/references/devices/index` 和 :doc:`/references/peripherals/index` 中的字段规范修改对应条目。

文件结构与各字段完整说明见 :doc:`/programming-guide/board-directory`。

**第四步：验证**

.. code-block:: bash

   idf.py bmgr -b my_board

生成成功后，``components/gen_bmgr_codes/`` 下应包含：``gen_board_periph_config.c``、``gen_board_device_config.c``、``gen_board_info.c``、``board_manager.defaults``、``Kconfig.projbuild``、``idf_component.yml`` 等文件。

正式使用前逐项确认：

- ``board_info.yaml`` 中的开发板名称与目录名一致。
- 所有引脚号已按原理图核实。
- 设备引用的所有外设名均存在于 ``board_peripherals.yaml`` 中。
- ``dependencies`` 中的组件能正常解析。
- LCD 显示屏、触摸屏、摄像头等需要特殊时序的设备已确认是否需要 ``setup_device.c``。

通过上述检查后，建议在新开发板上运行示例工程或测试应用，进一步验证外设与设备初始化是否正常：

- **示例工程**：``esp_board_manager/examples/`` 目录下提供了音频播放、录音、LVGL 显示等场景示例，可直接切换至新开发板运行，验证实际硬件行为。
- **测试应用**：``esp_board_manager/test_apps/`` 提供了针对板级初始化流程的系统测试，适合全面验证生成代码的正确性。
