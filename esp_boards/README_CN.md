# ESP Boards

[![组件注册表](https://components.espressif.com/components/espressif/esp_boards/badge.svg)](https://components.espressif.com/components/espressif/esp_boards)

[English](README.md)

ESP Board Manager 的乐鑫官方板卡定义组件。

乐鑫官方开发板列表请参考：[开发板](https://www.espressif.com/zh-hans/products/devkits)。

**`esp_board_manager` 默认依赖本组件**，因此工程只要添加 `espressif/esp_board_manager`，就会自动获得本组件的所有板子。

本组件提供可被 ESP Board Manager 识别和使用的板级配置文件，包括板子信息、外设及设备配置、板级默认 sdkconfig 等。添加本组件后，可通过 ESP Board Manager 的命令查看板子，或是选中板子生成配置代码。

关于 ESP Board Manager 的更多信息，请参考 [`esp_board_manager` 组件文档](https://github.com/espressif/esp-board-manager/blob/main/esp_board_manager/README_CN.md)。

## 已知兼容性问题

本组件 `0.6.0` 及更新版本要求配合 `0.7.1` 或更高版本的 `espressif/esp_board_manager` 使用。

## 支持的板级

### ESP32-S31

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-S31-Function-CoreBoard-1 | ESP32-S31 | ES8311 (DAC + ADC) | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| ESP32-S31-Korvo-1 | ESP32-S31 | ES8389 (DAC + ADC) | SDMMC | RGB LCD (rgb) | GT1151 (i2c) | 摄像头 (dvp) | GPIO 按键 + ADC 按键 | WS2812 (rmt) | - |
| ESP-Mosaico | ESP32-S31 | ES8311 (DAC + ADC) | - | CO5300 (spi) | CST9217 (i2c) | - | GPIO 按键 | - | - |

### ESP32-P4

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| [`ESP32-P4-EYE`](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32p4/esp32-p4-eye/user_guide.html) | ESP32-P4 | 内置 ADC | SDMMC | ST7789 (spi) | - | 摄像头 (csi) | GPIO 按键 | - | 旋钮 (gpio) |
| [`ESP32-P4-Function-EV-Board`](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html) | ESP32-P4 | ES8311 (DAC + ADC) | SDMMC | EK79007 (dsi) | GT911 (i2c) | 摄像头 (csi) | GPIO 按键 | - | - |
| ESP32-P4X-C5-Function-EV-Board | ESP32-P4 | ES8311 (DAC + ADC) | SDMMC | EK79007 (dsi) | GT911 (i2c) | 摄像头 (csi) | GPIO 按键 | - | - |

### ESP32-S3

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| [`ESP32-S3-BOX-3`](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_3/hardware_overview_for_box_3_cn.md) | ESP32-S3 | ES8311 (DAC) + ES7210 (ADC) | SDMMC | ILI9341 (spi) | GT911/TT21100 (i2c) | - | GPIO 按键 | - | - |
| [`ESP32-S3-BOX-Lite`](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_lite/hardware_overview_for_lite.md) | ESP32-S3 | ES8156 (DAC) + ES7243E (ADC) | - | ST7789 (spi) | - | - | GPIO 按键 | - | - |
| ESP32-S3-DevKitC-1 | ESP32-S3 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| ESP32-S3-DevKitM-1 | ESP32-S3 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| ESP32-S3-EYE | ESP32-S3 | 内置 ADC | SDMMC | ST7789 (spi) | - | 摄像头 (dvp) | ADC 按键 + GPIO 按键 | - | - |
| ESP32-S3-Korvo-1 | ESP32-S3 | ES8311 (DAC) + ES7210 (ADC) | SDMMC | - | - | - | ADC 按键 + GPIO 按键 | WS2812 (rmt) | - |
| [`ESP32-S3-Korvo-2 V3.1`](https://docs.espressif.com/projects/esp-adf/zh_CN/latest/design-guide/dev-boards/user-guide-esp32-s3-korvo-2.html) | ESP32-S3 | ES8311 (DAC) + ES7210 (ADC) | SDMMC | ILI9341 (spi) | TT21100/GT911 (i2c) | 摄像头 (dvp) | ADC 按键 + GPIO 按键 | - | - |
| [`ESP32-S3-LCD-EV-Board`](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32s3/esp32-s3-lcd-ev-board/index.html) | ESP32-S3 | ES8311 (DAC) + ES7210 (ADC) | - | GC9503 (rgb_3wire_spi) | FT5x06 (i2c) | - | GPIO 按键 | - | - |
| ESP32-S3-USB-OTG | ESP32-S3 | - | SDMMC | ST7789 (spi) | - | - | GPIO 按键 | - | - |
| ESP-DualKey | ESP32-S3 | - | - | - | - | - | GPIO 按键 | WS2812 (spi) | - |
| ESP Thread Border Router Board | ESP32-S3 | - | - | - | - | - | GPIO 按键 | - | - |
| [`ESP-VoCat V1.0`](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32s3/esp-vocat/user_guide_v1.0.html) | ESP32-S3 | ES8311 (DAC) + ES7210 (ADC) | SDMMC | ST77916 (spi) | CST816S (i2c) | - | GPIO 按键 | - | - |
| [`ESP-VoCat V1.2`](https://docs.espressif.com/projects/esp-dev-kits/zh_CN/latest/esp32s3/esp-vocat/user_guide_v1.2.html) | ESP32-S3 | ES8311 (DAC) + ES7210 (ADC) | SDMMC | ST77916 (spi) | CST816S (i2c) | - | GPIO 按键 | - | - |

### ESP32

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-DevKitC V4 | ESP32 | - | - | - | - | - | GPIO 按键 | - | - |
| ESP32-DevKitM-1 | ESP32 | - | - | - | - | - | GPIO 按键 | - | - |
| ESP32-Ethernet-Kit v1.2 | ESP32 | - | - | - | - | - | GPIO 按键 | - | - |
| ESP32-LCDKit | ESP32 | - | SDMMC | ILI9341 (spi) | - | - | - | - | - |
| [`ESP32-LyraT V4.3`](https://docs.espressif.com/projects/esp-adf/zh_CN/latest/design-guide/dev-boards/get-started-esp32-lyrat.html) | ESP32 | ES8388 (DAC + ADC) | SDMMC | - | - | - | GPIO 按键 | - | - |
| [`ESP32-LyraT-Mini`](https://docs.espressif.com/projects/esp-adf/zh_CN/latest/design-guide/dev-boards/get-started-esp32-lyrat-mini.html) | ESP32 | ES8311 (DAC) + ES7243E (ADC) | SDMMC | - | - | - | ADC 按键 + GPIO 按键 | - | - |
| ESP32-MeshKit-Sense v1.1 | ESP32 | - | - | - | - | - | GPIO 按键 | - | - |
| ESP32-PICO-DevKitM-2 | ESP32 | - | - | - | - | - | GPIO 按键 | - | - |
| ESP32-PICO-KIT | ESP32 | - | - | - | - | - | GPIO 按键 | - | - |
| ESP32-PICO-KIT-1 | ESP32 | - | - | - | - | - | GPIO 按键 | - | - |
| ESP32-Sense-Kit | ESP32 | - | - | - | - | - | - | - | - |
| ESP32-Vaquita-DSPG v1.0 | ESP32 | ES8311 (DAC) | - | - | - | - | ADC 按键 + GPIO 按键 | WS2812 (rmt) | - |

### ESP32-C3

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-C3-AWS-ExpressLink-DevKit | ESP32-C3 | - | - | - | - | - | - | - | - |
| ESP32-C3-DevKit-RUST-1 | ESP32-C3 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| ESP32-C3-DevKit-RUST-2 | ESP32-C3 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| ESP32-C3-DevKitC-02 | ESP32-C3 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| ESP32-C3-DevKitM-1 | ESP32-C3 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| ESP32-C3-LCDKit | ESP32-C3 | 内置 DAC | - | GC9A01 (spi) | - | - | GPIO 按键 | WS2812 (rmt) | 旋钮 (gpio) |
| [`ESP32-C3-Lyra`](https://docs.espressif.com/projects/esp-adf/zh_CN/latest/design-guide/dev-boards/user-guide-esp32-c3-lyra.html) | ESP32-C3 | 内置 ADC + DAC | - | - | - | - | GPIO 按键 | - | - |

### ESP32-C5

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-C5-DevKitC-1 | ESP32-C5 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| ESP-SensairShuttle | ESP32-C5 | 内置 DAC + ADC | - | ST7789 (spi) | CST816S (i2c) | - | GPIO 按键 | WS2812 (rmt) | - |

### ESP32-C6

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-C6-DevKitC-1 | ESP32-C6 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| ESP32-C6-DevKitM-1 | ESP32-C6 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |

### ESP32-H2

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-H2-DevKitM-1 | ESP32-H2 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |

注：`-` 表示开发板不具备相应的 BMGR 标准设备能力。
