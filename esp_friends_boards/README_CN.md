# ESP Friends Boards

[![组件注册表](https://components.espressif.com/components/espressif/esp_friends_boards/badge.svg)](https://components.espressif.com/components/espressif/esp_friends_boards)

[English](README.md)

ESP Board Manager 的社区、合作伙伴以及非官方售卖板卡定义组件。

本组件不会作为 `esp_board_manager` 的默认依赖。如果工程需要这些板子，请显式添加：

```yaml
dependencies:
  espressif/esp_friends_boards:
    version: "^0.6.1"
```

本组件提供可被 ESP Board Manager 识别和使用的板级配置文件，包括板子信息、外设及设备配置、板级默认 sdkconfig 等。添加本组件后，可通过 ESP Board Manager 的命令查看板子，或是选中板子生成配置代码。

关于 ESP Board Manager 的更多信息，请参考 [`esp_board_manager` 组件文档](https://github.com/espressif/esp-board-manager/blob/main/esp_board_manager/README_CN.md)。

## 已知兼容性问题

本组件 `0.6.0` 及更新版本要求配合 `0.7.1` 或更高版本的 `espressif/esp_board_manager` 使用。

## 支持的板级

### ESP32-S3

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-S3-BOX-2 | ESP32-S3 | ES8389 (DAC + ADC) | SPI | ST7789 (i80) | - | - | 自定义按键 | - | - |
| ESP32-S3-Korvo-2L | ESP32-S3 | ES8311 (DAC + ADC) | SDMMC | - | - | - | ADC 按键 | - | - |
| ESP32-S3-SparkBot | ESP32-S3 | 内置 ADC | - | ST7789 (spi) | - | 摄像头 (dvp) | - | - | - |

### ESP32

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| ESP-WROVER-KIT | ESP32 | - | SPI | ST7789 (spi) | - | - | GPIO 按键 | - | - |

### ESP32-C3

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| ESP-HI | ESP32-C3 | 内置 ADC + DAC | - | ILI9341 (spi) | - | - | GPIO 按键 | - | - |

### ESP32-C5

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| [`ESP32-C5-Spot`](https://oshwhub.com/esp-college/esp-spot) | ESP32-C5 | ES8311 (ADC + DAC) | - | - | - | - | - | - | - |
| ESP-Ditto | ESP32-C5 | 内置 (DAC + ADC) | - | ILI9341 (parlio) | CST816S (i2c) | 摄像头 (spi) | - | - | - |
