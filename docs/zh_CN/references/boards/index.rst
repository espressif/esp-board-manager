板卡目录
============

:link_to_translation:`en:[English]`

**自 BMGR 0.6 起**\ ：开发板从 BMGR 组件内移除，拆分为多个独立板卡组件，具体如下：

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - 组件
     - 说明
   * - ``espressif/esp_boards``
     - 乐鑫官方开发板。BMGR 0.6 默认声明此依赖，引入 BMGR 即自动可用，无需工程额外配置。`在线查阅 <https://github.com/espressif/esp-board-manager/tree/main/esp_boards>`__
   * - ``espressif/esp_friends_boards``
     - 合作伙伴与社区开发板。需在工程主组件清单（``idf_component.yml``）中手动声明依赖。`在线查阅 <https://github.com/espressif/esp-board-manager/tree/main/esp_friends_boards>`__
   * - ``espressif/m5stack_boards``
     - M5Stack 系列开发板。需在工程主组件清单中手动声明依赖。`在线查阅 <https://github.com/espressif/esp-board-manager/tree/main/m5stack_boards>`__

官方开发板（``espressif/esp_boards``）
------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 35 65 65

   * - 名称
     - 板载能力
     - 简介
   * - :doc:`ESP32-C3-LCDKit <esp32_c3_lcdkit>`
     - GC9A01 SPI LCD、PDM 扬声器、旋钮、WS2812、SPIFFS
     - 基于 ESP32-C3 的屏幕应用开发板。
   * - :doc:`ESP32-C3-Lyra <esp32_c3_lyra>`
     - 内部 ADC、PDM 音频输出、功放控制、GPIO Boot 按键
     - 基于 ESP32-C3 的音频开发板。
   * - :doc:`ESP32-LyraT V4.3 <esp32_lyrat_4_3>`
     - ES8388、microSD、I2S、功放控制、GPIO Boot 按键
     - 基于 ESP32 的音频开发板。
   * - :doc:`ESP32-LyraT-Mini <esp32_lyrat_mini_1_1>`
     - ES8311、ES7243E、microSD、ADC 多按键、GPIO Boot 按键
     - 面向音频播放和录音的 ESP32 开发板。
   * - :doc:`ESP32-P4-EYE <esp32_p4_eye>`
     - CSI 摄像头、ST7789 LCD、microSD、内部 ADC、GPIO 按键、GPIO Boot 按键、旋钮
     - 面向视觉应用的 ESP32-P4 开发板。
   * - :doc:`ESP32-P4-Function-EV-Board <esp32_p4_function_ev_board>`
     - DSI 显示、触摸、音频、CSI 摄像头、microSD、GPIO Boot 按键
     - 基于 ESP32-P4 的多媒体开发板。
   * - :doc:`ESP32-S31-Function-CoreBoard-1 <esp32_s31_function_coreboard_1>`
     - ES8311、麦克风、扬声器接口、WS2812、GPIO Boot 按键
     - 面向联网 AIoT 原型验证的开发板。
   * - :doc:`ESP32-S31-Korvo-1 <esp32_s31_korvo_1>`
     - 双麦克风、ES8389、4.3 英寸 LCD、触摸、DVP 摄像头、microSD、WS2812、GPIO Boot 按键
     - 面向智能音频和人机交互的多媒体开发板。
   * - :doc:`ESP32-S3-BOX-3 <esp32_s3_box_3>`
     - ES8311、ES7210、ST77916 LCD、触摸、microSD、GPIO Boot 按键
     - ESP32-S3 语音和多媒体开发板。
   * - :doc:`ESP32-S3-BOX-Lite <esp32_s3_box_lite>`
     - ES8156、ES7243E、ST7789 LCD、GPIO Boot 按键
     - ESP32-S3 音频和显示开发板。
   * - :doc:`ESP32-S3-EYE <esp32_s3_eye>`
     - OV2640 摄像头、LCD、数字麦克风、microSD、加速度传感器、ADC 按键、GPIO Boot 按键
     - 面向视觉和音频处理的 ESP32-S3 AI 开发板。
   * - :doc:`ESP32-S3-Korvo-1 <esp32_s3_korvo_1>`
     - 三麦克风阵列、ES8311、ES7210、microSD、ADC 按键、WS2812C、GPIO Boot 按键
     - 面向语音识别应用的 ESP32-S3 开发板。
   * - :doc:`ESP32-S3-Korvo-2 V3.1 <esp32_s3_korvo_2_3>`
     - ES8311/ES7210、ILI9341、触摸、DVP 摄像头、microSD、ADC 按键、GPIO Boot 按键
     - 面向音频和多媒体应用的 ESP32-S3 开发板。
   * - :doc:`ESP32-S3-LCD-EV-Board <esp32_s3_lcd_ev_board>`
     - GC9503 RGB LCD、FT5x06、ES8311/ES7210、GPIO Boot 按键
     - 用于评估 ESP32-S3 屏幕应用的开发板。
   * - :doc:`ESP-SensairShuttle <esp_sensairshuttle>`
     - BMI270、音频输入输出、LCD、触摸、WS2812、GPIO Boot 按键
     - 面向动作感知和大模型人机交互的 ESP32-C5 开发板。
   * - :doc:`ESP-VoCat V1.0 <esp_vocat_1_0>`
     - 圆形触摸屏、双麦克风、音频编解码、microSD、GPIO Boot 按键
     - 面向语音交互的 ESP32-S3 开发板。
   * - :doc:`ESP-VoCat V1.2 <esp_vocat_1_2>`
     - 圆形触摸屏、双麦克风、音频编解码、microSD、状态 LED、GPIO Boot 按键
     - ESP-VoCat 的 V1.2 硬件版本。
   * - :doc:`ESP32-C3-AWS-ExpressLink-DevKit <esp32_c3_aws_expresslink_devkit>`
     - ESP32-C3-MINI-1-N4-A、UART、事件/唤醒/复位控制
     - 面向 AWS IoT ExpressLink 云连接的 ESP32-C3 评估开发板。
   * - :doc:`ESP32-C3-DevKit-Rust-1 <esp32_c3_devkit_rust_1>`
     - ICM-42670-P IMU、SHTC3 温湿度、WS2812、按键、USB
     - 面向 Rust 开发的 ESP32-C3 传感器开发板。
   * - :doc:`ESP32-C3-DevKit-Rust-2 <esp32_c3_devkit_rust_2>`
     - ICM-42670-P IMU、SHTC3 温湿度、WS2812、按键、USB
     - ESP32-C3 Rust 开发板的改进版本。
   * - :doc:`ESP32-C3-DevKitC-02 <esp32_c3_devkitc_02>`
     - ESP32-C3-WROOM-02、USB、GPIO 引脚
     - 基于 ESP32-C3 的通用入门开发板。
   * - :doc:`ESP32-C3-DevKitM-1 <esp32_c3_devkitm_1>`
     - ESP32-C3-MINI-1、USB-UART、GPIO 引脚
     - 基于 ESP32-C3-MINI-1 模组的通用开发板。
   * - :doc:`ESP32-C5-DevKitC-1 <esp32_c5_devkitc_1>`
     - ESP32-C5-WROOM-1、Wi-Fi 6、802.15.4、USB
     - 面向 ESP32-C5 无线功能评估的开发板。
   * - :doc:`ESP32-C6-DevKitC-1 <esp32_c6_devkitc_1>`
     - ESP32-C6-WROOM-1、Wi-Fi 6、802.15.4、USB
     - 面向 ESP32-C6 无线和物联网应用的开发板。
   * - :doc:`ESP32-C6-DevKitM-1 <esp32_c6_devkitm_1>`
     - ESP32-C6-MINI-1、Wi-Fi 6、802.15.4、USB
     - 基于 ESP32-C6-MINI-1 模组的开发板。
   * - :doc:`ESP32-DevKitC <esp32_devkitc>`
     - ESP32-WROOM-32、USB-UART、GPIO 引脚
     - 基于 ESP32-WROOM-32 的通用开发板。
   * - :doc:`ESP32-DevKitM-1 <esp32_devkitm_1>`
     - ESP32-WROOM-32、USB-UART、GPIO 引脚
     - 基于 ESP32-WROOM-32 模组的通用开发板。
   * - :doc:`ESP32-Ethernet-Kit <esp32_ethernet_kit>`
     - Ethernet PHY、RJ45、microSD、USB-UART
     - 用于以太网连接和网络应用评估的 ESP32 开发板。
   * - :doc:`ESP32-H2-DevKitM-1 <esp32_h2_devkitm_1>`
     - ESP32-H2-MINI-1、802.15.4、USB
     - 面向 Thread 和 Zigbee 应用的低功耗开发板。
   * - :doc:`ESP32-LCDKit <esp32_lcdkit>`
     - SPI LCD、触摸、旋钮、音频、红外、WS2812
     - 面向小尺寸图形界面和人机交互的 ESP32 开发板。
   * - :doc:`ESP32-MeshKit-Sense <esp32_meshkit_sense>`
     - 环境传感器、LCD、触摸、Wi-Fi/BLE、GPIO Boot 按键
     - 面向 Wi-Fi Mesh 和环境感知应用的开发套件。
   * - :doc:`ESP32-P4X-C5-Function-EV-Board <esp32_p4x_c5_function_ev>`
     - ESP32-P4 与 ESP32-C5、显示、触摸、音频、摄像头、GPIO Boot 按键
     - 用于多芯片协同多媒体应用评估的开发板。
   * - :doc:`ESP32-PICO-DevKitM-2 <esp32_pico_devkitm_2>`
     - ESP32-PICO-MINI-02、USB-UART、GPIO 引脚
     - 基于 ESP32-PICO 系列模组的紧凑型开发板。
   * - :doc:`ESP32-PICO-KIT <esp32_pico_kit>`
     - ESP32-PICO-D4、USB-UART、GPIO 引脚
     - 基于 ESP32-PICO-D4 的开发板。
   * - :doc:`ESP32-PICO-KIT-1 <esp32_pico_kit_1>`
     - ESP32-PICO-D4、USB-UART、GPIO 引脚
     - ESP32-PICO-KIT 的硬件版本。
   * - :doc:`ESP32-S3-DevKitC-1 <esp32_s3_devkitc_1>`
     - ESP32-S3-WROOM-1、USB、GPIO 引脚
     - 面向 ESP32-S3 应用开发和功能评估的开发板。
   * - :doc:`ESP32-S3-DevKitM-1 <esp32_s3_devkitm_1>`
     - ESP32-S3-MINI-1、USB、GPIO 引脚
     - 基于 ESP32-S3-MINI-1 模组的通用开发板。
   * - :doc:`ESP32-S3-USB-OTG <esp32_s3_usb_otg>`
     - USB OTG、USB Type-C、microSD、按键
     - 面向 USB 主机和设备应用开发的 ESP32-S3 开发板。
   * - :doc:`ESP32-Sense-Kit <esp32_sense_kit>`
     - 触摸传感器、麦克风、扬声器、LCD
     - 用于 ESP32 触摸和音频功能评估的开发套件。
   * - :doc:`ESP32-Vaquita-DSPG <esp32_vaquita_dspg>`
     - ESP32-WROVER-E、DBMD5P、ES8311、麦克风、扬声器、GPIO Boot 按键
     - 面向 Alexa 语音和 AWS IoT 应用的 ESP32 开发板。
   * - :doc:`ESP-DualKey <esp_dualkey>`
     - 双按键输入、USB、GPIO 扩展
     - 用于按键和人机交互功能验证的 ESP32 开发板。
   * - :doc:`ESP-Mosaico V1.0 <esp_mosaico>`
     - 480×480 QSPI 触摸屏、ES8311、BMI270、BMM150、SPI NAND、USB OTG、GPIO Boot 按键
     - 基于 ESP32-S31 的可扩展智能交互开发套件。
   * - :doc:`ESP Thread Border Router <esp_thread_border_router>`
     - ESP32-S3 主控、ESP32-H2 RCP、Wi-Fi、以太网扩展、GPIO Boot 按键
     - 用于 Thread/Zigbee 边界路由器开发的硬件平台。

.. toctree::
   :maxdepth: 1
   :hidden:
   :titlesonly:

   esp32_c3_lcdkit
   esp32_c3_lyra
   esp32_lyrat_4_3
   esp32_lyrat_mini_1_1
   esp32_p4_eye
   esp32_p4_function_ev_board
   esp32_s31_function_coreboard_1
   esp32_s31_korvo_1
   esp32_s3_box_3
   esp32_s3_box_lite
   esp32_s3_eye
   esp32_s3_korvo_1
   esp32_s3_korvo_2_3
   esp32_s3_lcd_ev_board
   esp_sensairshuttle
   esp_vocat_1_0
   esp_vocat_1_2
   esp32_c3_aws_expresslink_devkit
   esp32_c3_devkit_rust_1
   esp32_c3_devkit_rust_2
   esp32_c3_devkitc_02
   esp32_c3_devkitm_1
   esp32_c5_devkitc_1
   esp32_c6_devkitc_1
   esp32_c6_devkitm_1
   esp32_devkitc
   esp32_devkitm_1
   esp32_ethernet_kit
   esp32_h2_devkitm_1
   esp32_lcdkit
   esp32_meshkit_sense
   esp32_p4x_c5_function_ev
   esp32_pico_devkitm_2
   esp32_pico_kit
   esp32_pico_kit_1
   esp32_s3_devkitc_1
   esp32_s3_devkitm_1
   esp32_s3_usb_otg
   esp32_sense_kit
   esp32_vaquita_dspg
   esp_dualkey
   esp_mosaico
   esp_thread_border_router

社区与合作伙伴开发板（``espressif/esp_friends_boards``）
------------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 35 65 65

   * - 名称
     - 板载能力
     - 简介
   * - :doc:`ESP-HI <esp_hi>`
     - 内部 ADC、PDM 扬声器、ILI9341 LCD、GPIO Boot 按键
     - ESP32-C3 AI 语音交互模块。
   * - :doc:`ESP32-C5-Spot <esp32_c5_spot>`
     - ES8311 双路音频、功放和电源控制
     - ESP32-C5 语音交互模块。
   * - :doc:`ESP32-S3-BOX-2 <esp32_s3_box_2>`
     - ES8389、ES7210、ST7789 LCD、SPI、GPIO 按键
     - ESP32-S3 语音交互开发板。
   * - :doc:`ESP32-S3-Korvo-2L <esp32_s3_korvo_2l>`
     - ES8311、microSD、ADC 按键
     - ESP32-S3 音频开发板。
   * - :doc:`ESP-Ditto <esp_Ditto>`
     - BMGR：ILI9341 并口 LCD、CST816S 触摸、SPI 摄像头、音频输入输出；板载 BMI270、BMM150、SI12T 和 BQ27220 尚未适配
     - 基于 ESP32-C5 的多传感器交互开发板。
   * - :doc:`ESP-WROVER-KIT <esp_wrover_kit>`
     - ST7789 LCD、microSD、按键、蓝色 LED、SPIFFS
     - 基于 ESP32 的通用开发板。
   * - :doc:`ESP32-S3 SparkBot <esp_sparkbot>`
     - ST7789 LCD、DVP 摄像头、音频输入输出、BMI270、状态 LED
     - 基于 ESP32-S3 的机器人交互开发板。

.. toctree::
   :maxdepth: 1
   :hidden:
   :titlesonly:

   esp_hi
   esp32_c5_spot
   esp32_s3_box_2
   esp32_s3_korvo_2l
   esp_Ditto
   esp_wrover_kit
   esp_sparkbot

M5Stack 开发板（``espressif/m5stack_boards``）
--------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 35 65 65

   * - 名称
     - 板载能力
     - 简介
   * - `M5Stack AtomS3 <https://docs.m5stack.com/zh_CN/core/AtomS3>`__
     - GC9A01 LCD、背光、GPIO 按键、SPIFFS
     - ESP32-S3 小型显示开发板。
   * - `M5Stack AtomS3R <https://docs.m5stack.com/zh_CN/core/AtomS3R>`__
     - ST7735 LCD、GPIO 按键、SPIFFS
     - ESP32-S3 小型显示开发板。
   * - `M5Dial <https://docs.m5stack.com/zh_CN/core/M5Dial>`__
     - GC9A01 LCD、FT5x06 触摸、旋钮、按键、SPIFFS
     - ESP32-S3 旋钮交互开发板。
   * - `ATOM Echo <https://docs.m5stack.com/zh_CN/atom/atomecho>`__
     - RGB LED、用户按键
     - ESP32 智能音箱模块。
   * - `ATOM Lite <https://docs.m5stack.com/zh_CN/core/ATOM%20Lite>`__
     - RGB LED、用户按键、红外输出
     - 紧凑型 ESP32 开发板。
   * - `ATOM Matrix <https://docs.m5stack.com/zh_CN/core/ATOM%20Matrix>`__
     - RGB LED 矩阵、用户按键、红外输出
     - 紧凑型 ESP32 开发板。
   * - `AtomS3 Lite <https://docs.m5stack.com/zh_CN/core/AtomS3%20Lite>`__
     - RGB LED、用户按键、红外输出
     - 紧凑型 ESP32-S3 开发板。
   * - `AtomS3U <https://docs.m5stack.com/zh_CN/core/AtomS3U>`__
     - RGB LED、用户按键、红外输出
     - 带 USB-A 接口的 ESP32-S3 开发板。
   * - `ATOM U <https://docs.m5stack.com/zh_CN/core/ATOM%20U>`__
     - RGB LED、用户按键、红外输出
     - 带 USB-A 接口的紧凑型 ESP32 开发板。
   * - `M5Capsule <https://docs.m5stack.com/zh_CN/core/M5Capsule>`__
     - 电源保持、RGB LED、SPI microSD、红外输出、唤醒/启动按键
     - ESP32-S3 紧凑型控制器。
   * - `Cardputer <https://docs.m5stack.com/zh_CN/core/Cardputer>`__
     - ST7789 LCD、SPI microSD、RGB LED、红外输出、启动按键
     - ESP32-S3 掌上电脑。
   * - `Cardputer Adv <https://docs.m5stack.com/zh_CN/core/Cardputer-Adv>`__
     - ST7789 LCD、SPI microSD、音频输入输出、RGB LED、红外输出、启动按键
     - ESP32-S3 掌上电脑。
   * - `M5Stack Basic <https://docs.m5stack.com/zh_CN/core/basic_v2.7>`__
     - ILI9341 LCD、扬声器控制、三个按键、SPI microSD、SPIFFS
     - ESP32 多媒体核心板。
   * - `M5Stack Core2 <https://docs.m5stack.com/zh_CN/core/Core2%20v1.1>`__
     - ILI9341 LCD、FT5x06 触摸、音频输出、SPI microSD、SPIFFS
     - 带触摸和电源管理的 ESP32 多媒体核心板。
   * - `CoreInk <https://docs.m5stack.com/zh_CN/core/coreink>`__
     - 电源保持、状态 LED、用户按键、拨轮按键
     - ESP32 电子墨水控制器。
   * - `CoreS3 <https://docs.m5stack.com/zh_CN/core/CoreS3>`__
     - 音频输入输出、ILI9342C LCD、FT5x06 触摸、GC0308 摄像头
     - ESP32-S3 多媒体核心板。
   * - `DinMeter <https://docs.m5stack.com/zh_CN/core/M5DinMeter>`__
     - ST7789 LCD、背光、电源保持、唤醒按键
     - ESP32-S3 导轨式控制器。
   * - `Fire v2.7 <https://docs.m5stack.com/zh_CN/core/fire_v2.7>`__
     - ILI9342C LCD、SPI microSD、RGB LED 灯带、三个按键
     - ESP32 多媒体核心板。
   * - `M5NanoC6 <https://docs.m5stack.com/zh_CN/core/M5NanoC6>`__
     - RGB LED、蓝色 LED、红外输出、用户按键
     - 紧凑型 ESP32-C6 开发板。
   * - `M5NanoH2 <https://docs.m5stack.com/zh_CN/core/NanoH2>`__
     - RGB LED、蓝色 LED、红外输出、用户按键
     - 紧凑型 ESP32-H2 开发板。
   * - `M5Paper v1.1 <https://docs.m5stack.com/zh_CN/core/m5paper_v1.1>`__
     - GT911 触摸、SPI microSD、电源控制、拨轮按键
     - ESP32 电子墨水开发板。
   * - `M5PaperS3 <https://docs.m5stack.com/zh_CN/core/PaperS3>`__
     - GT911 触摸、SPI microSD、电子墨水电源控制
     - ESP32-S3 电子墨水开发板。
   * - `M5Stamp C3 <https://docs.m5stack.com/zh_CN/core/stamp_c3>`__
     - RGB LED、用户按键
     - 紧凑型 ESP32-C3 模组开发板。
   * - `M5Stamp C3U <https://docs.m5stack.com/zh_CN/core/stamp_c3u>`__
     - RGB LED、用户按键
     - 带原生 USB 的 ESP32-C3 模组开发板。
   * - `M5Stamp Pico <https://docs.m5stack.com/zh_CN/core/stamp_pico>`__
     - RGB LED、用户按键
     - 紧凑型 ESP32 模组开发板。
   * - `M5Stamp S3 <https://docs.m5stack.com/zh_CN/core/StampS3>`__
     - RGB LED、启动按键
     - 紧凑型 ESP32-S3 模组开发板。
   * - `M5Station-485 <https://docs.m5stack.com/zh_CN/core/station_485>`__
     - ST7789 LCD、RGB LED 灯带、三个按键
     - ESP32 工业 RS-485 控制器。
   * - `M5StickC <https://docs.m5stack.com/zh_CN/core/m5stickc>`__
     - 状态 LED、红外输出、两个按键
     - 紧凑型 ESP32 可穿戴控制器。
   * - `M5StickC PLUS <https://docs.m5stack.com/zh_CN/core/m5stickc_plus>`__
     - ST7789 LCD、状态 LED、红外输出、两个按键
     - ESP32 可穿戴控制器。
   * - `M5StickC PLUS2 <https://docs.m5stack.com/zh_CN/core/M5StickC%20PLUS2>`__
     - ST7789 LCD、背光、状态 LED、电源保持、三个按键
     - ESP32 可穿戴控制器。
   * - `M5StickS3 <https://docs.m5stack.com/zh_CN/core/StickS3>`__
     - 音频输入输出、ST7789 LCD、背光、红外输出、两个按键
     - ESP32-S3 可穿戴控制器。
   * - `M5Stack Tab5 <https://docs.m5stack.com/zh_CN/core/Tab5>`__
     - 音频输入输出、MIPI DSI LCD、触摸、摄像头、SDMMC
     - ESP32-P4 平板型开发板。
   * - `M5Stack Tough <https://docs.m5stack.com/zh_CN/core/tough>`__
     - ILI9342C 触摸 LCD、SPI microSD
     - 坚固型 ESP32 控制器。
