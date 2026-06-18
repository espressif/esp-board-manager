快速开始
============

:link_to_translation:`en:[English]`

安装辅助工具 ``esp-bmgr-assist``
----------------------------------

推荐使用 ``esp-bmgr-assist`` 作为默认入口。该工具接入 ``idf.py`` 启动流程，自动发现当前工程中的 ESP Board Manager 组件，并将其加入 ``IDF_EXTRA_ACTIONS_PATH``。在已激活的 ESP-IDF Python 环境（在 IDF 目录下执行 ``./install.sh`` 与 ``. ./export.sh``）中安装一次后，后续同环境下的工程均可直接使用 ``idf.py bmgr`` 命令。

.. code-block:: bash

   pip install esp-bmgr-assist

.. note::

   ``esp-bmgr-assist`` 仅用于免去手动配置 ``IDF_EXTRA_ACTIONS_PATH``，并不替代 ``esp_board_manager`` 组件本身。仍需要按下面的步骤为工程添加 ``esp_board_manager`` 依赖。详见 :doc:`/tools/esp-bmgr-assist`。

添加 BMGR 依赖
----------------------------------

通过 ``idf.py add-dependency`` 命令快速添加（推荐）：

.. code-block:: bash

   idf.py add-dependency "espressif/esp_board_manager"

命令执行后会自动更新工程 ``main/idf_component.yml``。

也可以手动在 ``main/idf_component.yml`` 中添加：

.. code-block:: yaml

   espressif/esp_board_manager:
     version: "*"
     require: public

如果使用本地仓库路径：

.. code-block:: yaml

   espressif/esp_board_manager:
     override_path: /PATH/TO/esp_board_manager
     version: "*"
     require: public

``idf.py bmgr`` 基本用法
----------------------------------

入门阶段只需要使用以下两个命令：

.. code-block:: bash

   # 列出当前可见的开发板
   idf.py bmgr -l

   # 选择目标开发板并生成板级代码
   idf.py bmgr -b <board>

推荐执行顺序：

1. 先执行 ``idf.py bmgr -l``，确认 BMGR 路径配置正确，且目标开发板能被扫描到。
2. 再执行 ``idf.py bmgr -b <board>``，生成 ``components/gen_bmgr_codes`` 板级代码。生成过程中，日志会输出以下关键信息，可据此确认选中的开发板是否符合预期：

   - ``Resolved board: <board>``：实际命中的开发板名称。
   - ``Board path: <path>``：开发板配置文件的来源目录。
   - ``Board configuration generation completed successfully for board: <board>``：确认生成完成。

3. 然后执行正常的构建、烧录或运行。

切换开发板时，BMGR 会同时处理以下与开发板绑定的状态：

- 重新生成 ``components/gen_bmgr_codes`` 下的 C 源码和构建文件。
- 更新 ``board_manager.defaults``。
- 处理当前开发板对应的 ``Kconfig.projbuild``。
- 备份并清理旧的 ``sdkconfig``，避免旧开发板的 ``CONFIG_IDF_TARGET`` 或设备配置残留。

基本 API 用法
----------------------------------

应用层的典型使用路径为：初始化 BMGR、按名称获取设备或外设句柄、按需查询配置、最后反初始化。

.. code-block:: c

   #include "esp_board_manager_includes.h"

   void app_main(void)
   {
       ESP_ERROR_CHECK(esp_board_manager_init());

       void *lcd_handle = NULL;
       ESP_ERROR_CHECK(esp_board_manager_get_device_handle("display_lcd", &lcd_handle));

       void *lcd_config = NULL;
       ESP_ERROR_CHECK(esp_board_manager_get_device_config("display_lcd", &lcd_config));

       ESP_ERROR_CHECK(esp_board_manager_deinit());
   }

:cpp:func:`esp_board_manager_init` 按 ``board_peripherals.yaml`` 与 ``board_devices.yaml`` 中的顺序先初始化外设，再初始化设备。

常用 API 入口：

- 初始化与反初始化：:cpp:func:`esp_board_manager_init`、:cpp:func:`esp_board_manager_deinit`。
- 获取运行时句柄：:cpp:func:`esp_board_manager_get_periph_handle`、:cpp:func:`esp_board_manager_get_device_handle`。
- 查询生成后的配置：:cpp:func:`esp_board_manager_get_periph_config`、:cpp:func:`esp_board_manager_get_device_config`。

常见入门问题
----------------------------------

本节列出首次接入 BMGR 时常见的几类问题，便于快速定位。完整排查思路见 :doc:`/faq`。

``idf.py bmgr`` 命令未找到
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- 在 ESP-IDF v5.x 下，首先检查 ``IDF_EXTRA_ACTIONS_PATH`` 是否已正确指向 BMGR 目录。
- ``idf.py bmgr`` 是 BMGR v0.5.8 之后的简化命令；版本较旧时请使用旧命令 ``idf.py gen-bmgr-config``，或升级 BMGR。
- 请确保执行命令的路径是有效的 IDF 工程路径。

编译时提示缺少 ``dev_xxx.h`` 或 ``periph_xxx.h``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- 多数情况下是尚未执行 ``idf.py bmgr -b <board>``，或上一次生成结果过期或不完整。
- 正常情况下，``components/gen_bmgr_codes`` 除若干 ``.c`` 文件外，还需包含 ``CMakeLists.txt``、``idf_component.yml``、``board_manager.defaults``、``Kconfig.projbuild``。

修改 YAML 后未生效
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

BMGR 不会因为板级 YAML 修改而自动刷新旧的生成物。修改后需要重新执行 ``idf.py bmgr -b <board>``，然后继续构建。

示例
----------------------------------

``esp_board_manager/examples/`` 提供若干可独立构建的示例，演示完整的使用路径：选择开发板、生成板级代码、初始化、按设备名获取句柄。各例程目录下提供 ``README_CN.md``，其中包含硬件要求与构建步骤。

.. list-table::
   :header-rows: 1
   :widths: 22 34 32 12

   * - 例程
     - 典型场景
     - 主要涉及的 device
     - Registry
   * - ``play_embed_music``
     - 从固件内嵌 WAV 播放提示音
     - ``audio_codec``\ （DAC）
     - `链接 <https://components.espressif.com/components/espressif/esp_board_manager/examples/play_embed_music>`__
   * - ``play_sdcard_music``
     - 从 microSD 卡播放 WAV
     - ``audio_codec``、``fs_fat``
     - `链接 <https://components.espressif.com/components/espressif/esp_board_manager/examples/play_sdcard_music>`__
   * - ``record_to_sdcard``
     - 麦克风录音并写入 SD 卡 WAV
     - ``audio_codec``\ （ADC）、``fs_fat``
     - `链接 <https://components.espressif.com/components/espressif/esp_board_manager/examples/record_to_sdcard>`__
   * - ``record_and_play``
     - 麦克风采集并实时从扬声器播放（回环）
     - ``audio_codec``\ （ADC + DAC）
     - `链接 <https://components.espressif.com/components/espressif/esp_board_manager/examples/record_and_play>`__
   * - ``display_lvgl``
     - 初始化 LCD 与触摸屏，运行 LVGL 测试 UI
     - ``display_lcd``、``lcd_touch``
     - `链接 <https://components.espressif.com/components/espressif/esp_board_manager/examples/display_lvgl>`__

``examples/common/`` 提供 WAV 头解析等共用代码，供 SD 卡相关例程引用。

除了上述示例， ``esp_board_manager/test_apps/`` 提供面向 CI 和本地验证的测试应用，覆盖 board manager 内已经支持的各个设备和外设。在完成新板卡适配后，可用该测试应用验证配置的正确性。

SDK 资源
----------------------------------

下列开源项目使用 BMGR 完成板级初始化，开发板适配通过独立组件提供，可作为完整工程的参考。

- `ESP-ADF <https://github.com/espressif/esp-adf>`_：乐鑫官方高级应用层开发框架，面向音频、视频及 IoT 产品开发。v3.0 以 ESP-GMF 重构核心媒体管线，提供音视频播放、电池管理、OTA、Wi-Fi 服务等模块化产品服务，并支持 MCP 协议调用；Board Manager 将作为官方板级初始化方案集成。
- `ESP-GMF <https://github.com/espressif/esp-gmf>`_：乐鑫面向 IoT 多媒体应用的轻量、模块化软件框架，支持音频、图像、视频处理及任意数据流产品，内存占用最低 7 KB。框架按 GMF-Core、Elements、Packages 四层组织，Board Manager 将作为官方板级初始化方案集成。
- `ESP-Claw <https://github.com/espressif/esp-claw>`_：基于 ESP32 系列芯片的 Chat Coding AI Agent 框架。通过对话定义设备行为，在端侧完成感知、决策、执行的闭环，依托 BMGR 完成开发板初始化，并通过 MCP 协议与外部工具交互。
- `ESP-Brookesia <https://github.com/espressif/esp-brookesia>`_：面向 AIoT 设备的人机交互开发框架。基于 ESP-IDF，按 HAL、服务、AI Agent 三层组件化提供音频、显示、存储、Wi-Fi、SNTP、视频以及 OpenAI、Coze、小智等 LLM 适配，板级配置通过 ``brookesia_hal_boards`` 调用 BMGR 完成。
