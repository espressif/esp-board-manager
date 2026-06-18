# ESP Boards

[![组件注册表](https://components.espressif.com/components/espressif/esp_boards/badge.svg)](https://components.espressif.com/components/espressif/esp_boards)

[English](README.md)

ESP Board Manager 的乐鑫官方板卡定义组件。

乐鑫官方开发板列表请参考：[开发板](https://www.espressif.com/zh-hans/products/devkits)。

**`esp_board_manager` 默认依赖本组件**，因此工程只要添加 `espressif/esp_board_manager`，就会自动获得本组件的所有板子。

本组件提供可被 ESP Board Manager 识别和使用的板级配置文件，包括板子信息、外设及设备配置、板级默认 sdkconfig 等。添加本组件后，可通过 ESP Board Manager 的命令查看板子，或是选中板子生成配置代码。

关于 ESP Board Manager 的更多信息，请参考 [`esp_board_manager` 组件文档](https://github.com/espressif/esp-board-manager/blob/main/esp_board_manager/README_CN.md)。

## 支持的板级

| 板子名称 | 芯片 | 音频 | SD卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 |
|---|---|---|---|---|---|---|---|---|
| [`ESP-VoCat V1.0`](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32s3/esp-vocat/user_guide_v1.0.html) | ESP32-S3 | ES8311 + ES7210 | SDMMC | ST77916 | CTS816S | - | - | - |
| [`ESP-VoCat V1.2`](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32s3/esp-vocat/user_guide_v1.2.html) | ESP32-S3 | ES8311 + ES7210 | SDMMC | ST77916 | CTS816S | - | - | - |
| [`ESP32-S3-BOX-3`](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_3/hardware_overview_for_box_3_cn.md) | ESP32-S3 | ES8311 + ES7210 | SDMMC | ST77916 | GT911/TT21100 自动探测 | - | - | - |
| [`ESP32-C3-Lyra`](https://docs.espressif.com/projects/esp-adf/zh_CN/latest/design-guide/dev-boards/user-guide-esp32-c3-lyra.html) | ESP32-C3 | 内置 ADC + PDM 扬声器 | - | - | - | - | - | - |
| [`ESP32-S3-Korvo-2 V3.1`](https://docs.espressif.com/projects/esp-adf/zh_CN/latest/design-guide/dev-boards/user-guide-esp32-s3-korvo-2.html) | ESP32-S3 | ES8311 + ES7210 | SDMMC | ILI9341 | TT21100/GT911 自动探测 | DVP Camera | ADC button | - |
| [`ESP32-S3-LCD-EV-Board`](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32s3/esp32-s3-lcd-ev-board/index.html) | ESP32-S3 | ES8311 + ES7210 | - | GC9503 RGB LCD | FT5x06 | - | GPIO 按键 | - |
| ESP32-S31-Function-Coreboard-1 | ESP32-S31 | ES8311 | - | - | - | - | - | WS2812 |
| ESP32-S31-Korvo-1 | ESP32-S31 | ES8389 | - | RGB LCD | GT1151 | - | - | WS2812 |
| [`ESP32-LyraT V4.3`](https://docs.espressif.com/projects/esp-adf/zh_CN/latest/design-guide/dev-boards/get-started-esp32-lyrat.html) | ESP32 | ES8388 | SDMMC | - | - | - | - | - |
| [`ESP32-LyraT-Mini`](https://docs.espressif.com/projects/esp-adf/zh_CN/latest/design-guide/dev-boards/get-started-esp32-lyrat-mini.html) | ESP32 | ES8388 | SDMMC | - | - | - | ADC button | - |
| [`ESP32-P4-Function-EV-Board`](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html) | ESP32-P4 | ES8311 | SDMMC | EK79007 | GT911 | CSI Camera | - | - |
| [`ESP32-P4-EYE`](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32p4/esp32-p4-eye/user_guide.html) | ESP32-P4 | 内置 ADC | SDMMC | ST7789 | - | CSI Camera | GPIO 按键 | - |
| [`ESP32-S3-BOX-Lite`](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_lite/hardware_overview_for_lite.md) | ESP32-S3 | ES8156 + ES7243E | - | ST7789 | - | - | - | - |

注：`-` 表示硬件不具备相应能力。
