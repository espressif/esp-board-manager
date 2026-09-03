# M5Stack Boards

[![组件注册表](https://components.espressif.com/components/espressif/m5stack_boards/badge.svg)](https://components.espressif.com/components/espressif/m5stack_boards)

[English](README.md)

ESP Board Manager 的 M5Stack 板卡定义组件。

本组件不会作为 `esp_board_manager` 的默认依赖。如果工程需要这些板子，请显式添加：

```yaml
dependencies:
  espressif/m5stack_boards:
    version: "^0.6.1"
```

本组件提供可被 ESP Board Manager 识别和使用的板级配置文件，包括板子信息、外设及设备配置、板级默认 sdkconfig 等。添加本组件后，可通过 ESP Board Manager 的命令查看板子，或是选中板子生成配置代码。

关于 ESP Board Manager 的更多信息，请参考 [`esp_board_manager` 组件文档](https://github.com/espressif/esp-board-manager/blob/main/esp_board_manager/README_CN.md)。

## 已知兼容性问题

本组件 `0.6.0` 及更新版本要求配合 `0.7.1` 或更高版本的 `espressif/esp_board_manager` 使用。

## 支持的板级

### ESP32-P4

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| [`M5Stack Tab5`](https://docs.m5stack.com/zh_CN/core/Tab5) | ESP32-P4 | ES8388 (DAC) + ES7210 (ADC) | SDMMC | Tab5 LCD (dsi) | Tab5 Touch (i2c) | 摄像头 (csi) | - | - | - |

### ESP32-S3

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| M5Stack AtomS3 | ESP32-S3 | - | - | GC9A01 (spi) | - | - | GPIO 按键 | - | - |
| M5Stack AtomS3R | ESP32-S3 | - | - | ST7735 (spi) | - | - | GPIO 按键 | - | - |
| M5Dial | ESP32-S3 | - | - | GC9A01 (spi) | FT5X06 (i2c) | - | GPIO 按键 | - | 旋钮 (gpio) |
| AtomS3-Lite | ESP32-S3 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| AtomS3U | ESP32-S3 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| M5Capsule | ESP32-S3 | - | SPI | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| M5Stack Cardputer | ESP32-S3 | - | SPI | ST7789 (spi) | - | - | GPIO 按键 | WS2812 (rmt) | - |
| M5Stack Cardputer Adv | ESP32-S3 | ES8311 (DAC + ADC) | SPI | ST7789 (spi) | - | - | GPIO 按键 | WS2812 (rmt) | - |
| [`M5STACK CORES3`](https://docs.m5stack.com/zh_CN/core/CoreS3) | ESP32-S3 | AW88298 (DAC) + ES7210 (ADC) | - | ILI9341 (spi) | FT5X06 (i2c) | 摄像头 (dvp) | - | - | - |
| M5Stack Din Meter | ESP32-S3 | - | - | ST7789 (spi) | - | - | GPIO 按键 | - | - |
| M5PaperS3 | ESP32-S3 | - | SPI | - | GT911 (i2c) | - | - | - | - |
| M5Stamp S3 | ESP32-S3 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| M5StickS3 | ESP32-S3 | ES8311 (DAC + ADC) | - | ST7789 (spi) | - | - | GPIO 按键 | - | - |

### ESP32

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| M5Stack ATOM Echo | ESP32 | - | - | - | - | - | GPIO 按键 | SK6812 (rmt) | - |
| M5Stack ATOM Lite | ESP32 | - | - | - | - | - | GPIO 按键 | SK6812 (rmt) | - |
| M5Stack ATOM Matrix | ESP32 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |
| M5Stack ATOM U | ESP32 | - | - | - | - | - | GPIO 按键 | SK6812 (rmt) | - |
| M5Stack Core | ESP32 | - | SPI | ILI9341 (spi) | - | - | GPIO 按键 | - | - |
| M5Stack Core2 | ESP32 | 内置 DAC | SPI | ILI9341 (spi) | FT5X06 (i2c) | - | - | - | - |
| M5Stack CoreInk | ESP32 | - | - | - | - | - | GPIO 按键 | - | - |
| M5Stack Fire | ESP32 | - | SPI | ILI9341 (spi) | - | - | GPIO 按键 | SK6812 (rmt) | - |
| M5Paper | ESP32 | - | SPI | - | GT911 (i2c) | - | GPIO 按键 | - | - |
| M5Stamp Pico | ESP32 | - | - | - | - | - | GPIO 按键 | SK6812 (rmt) | - |
| M5Station-485 | ESP32 | - | - | ST7789 (spi) | - | - | GPIO 按键 | SK6812 (rmt) | - |
| M5StickC | ESP32 | - | - | - | - | - | GPIO 按键 | - | - |
| M5StickC PLUS | ESP32 | - | - | ST7789 (spi) | - | - | GPIO 按键 | - | - |
| M5StickC PLUS2 | ESP32 | - | - | ST7789 (spi) | - | - | GPIO 按键 | - | - |
| M5Stack Tough | ESP32 | - | SPI | ILI9341 (spi) | - | - | - | - | - |

### ESP32-C3

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| M5Stamp C3 | ESP32-C3 | - | - | - | - | - | GPIO 按键 | SK6812 (rmt) | - |
| M5Stamp C3U | ESP32-C3 | - | - | - | - | - | GPIO 按键 | SK6812 (rmt) | - |

### ESP32-C6

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| M5NanoC6 | ESP32-C6 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |

### ESP32-H2

| 板子名称 | 芯片 | 音频 | SD 卡 | LCD | LCD 触摸 | 摄像头 | 按键 | LED 灯带 | 旋钮 |
|---|---|---|---|---|---|---|---|---|---|
| M5NanoH2 | ESP32-H2 | - | - | - | - | - | GPIO 按键 | WS2812 (rmt) | - |

注：`-` 表示开发板不具备相应的 BMGR 标准设备能力。
