# M5Stack Boards

[![Component Registry](https://components.espressif.com/components/espressif/m5stack_boards/badge.svg)](https://components.espressif.com/components/espressif/m5stack_boards)

[中文](README_CN.md)

M5Stack board definitions for ESP Board Manager.

This component is not a default dependency of `esp_board_manager`. Add it to a project explicitly when these boards are needed:

```yaml
dependencies:
  espressif/m5stack_boards:
    version: "^0.6.1"
```

This component provides board-level configuration files that can be recognized and used by ESP Board Manager, including board metadata, peripheral and device configuration, and board-level default sdkconfig options. After adding this component, use ESP Board Manager commands to list boards or select a board to generate configuration code.

For more information about ESP Board Manager, see the [`esp_board_manager` component documentation](https://github.com/espressif/esp-board-manager/blob/main/esp_board_manager/README.md).

## Known Compatibility Issue

Versions `0.6.0` and later of this component require `espressif/esp_board_manager` `0.7.1` or later.

## Supported Boards

### ESP32-P4

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| [`M5Stack Tab5`](https://docs.m5stack.com/en/core/Tab5) | ESP32-P4 | ES8388 (DAC) + ES7210 (ADC) | SDMMC | Tab5 LCD (dsi) | Tab5 Touch (i2c) | Camera (csi) | - | - | - |

### ESP32-S3

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| M5Stack AtomS3 | ESP32-S3 | - | - | GC9A01 (spi) | - | - | GPIO button | - | - |
| M5Stack AtomS3R | ESP32-S3 | - | - | ST7735 (spi) | - | - | GPIO button | - | - |
| M5Dial | ESP32-S3 | - | - | GC9A01 (spi) | FT5X06 (i2c) | - | GPIO button | - | Knob (gpio) |
| AtomS3-Lite | ESP32-S3 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |
| AtomS3U | ESP32-S3 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |
| M5Capsule | ESP32-S3 | - | SPI | - | - | - | GPIO button | WS2812 (rmt) | - |
| M5Stack Cardputer | ESP32-S3 | - | SPI | ST7789 (spi) | - | - | GPIO button | WS2812 (rmt) | - |
| M5Stack Cardputer Adv | ESP32-S3 | ES8311 (DAC + ADC) | SPI | ST7789 (spi) | - | - | GPIO button | WS2812 (rmt) | - |
| [`M5STACK CORES3`](https://docs.m5stack.com/en/core/CoreS3) | ESP32-S3 | AW88298 (DAC) + ES7210 (ADC) | - | ILI9341 (spi) | FT5X06 (i2c) | Camera (dvp) | - | - | - |
| M5Stack Din Meter | ESP32-S3 | - | - | ST7789 (spi) | - | - | GPIO button | - | - |
| M5PaperS3 | ESP32-S3 | - | SPI | - | GT911 (i2c) | - | - | - | - |
| M5Stamp S3 | ESP32-S3 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |
| M5StickS3 | ESP32-S3 | ES8311 (DAC + ADC) | - | ST7789 (spi) | - | - | GPIO button | - | - |

### ESP32

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| M5Stack ATOM Echo | ESP32 | - | - | - | - | - | GPIO button | SK6812 (rmt) | - |
| M5Stack ATOM Lite | ESP32 | - | - | - | - | - | GPIO button | SK6812 (rmt) | - |
| M5Stack ATOM Matrix | ESP32 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |
| M5Stack ATOM U | ESP32 | - | - | - | - | - | GPIO button | SK6812 (rmt) | - |
| M5Stack Core | ESP32 | - | SPI | ILI9341 (spi) | - | - | GPIO button | - | - |
| M5Stack Core2 | ESP32 | Built-in DAC | SPI | ILI9341 (spi) | FT5X06 (i2c) | - | - | - | - |
| M5Stack CoreInk | ESP32 | - | - | - | - | - | GPIO button | - | - |
| M5Stack Fire | ESP32 | - | SPI | ILI9341 (spi) | - | - | GPIO button | SK6812 (rmt) | - |
| M5Paper | ESP32 | - | SPI | - | GT911 (i2c) | - | GPIO button | - | - |
| M5Stamp Pico | ESP32 | - | - | - | - | - | GPIO button | SK6812 (rmt) | - |
| M5Station-485 | ESP32 | - | - | ST7789 (spi) | - | - | GPIO button | SK6812 (rmt) | - |
| M5StickC | ESP32 | - | - | - | - | - | GPIO button | - | - |
| M5StickC PLUS | ESP32 | - | - | ST7789 (spi) | - | - | GPIO button | - | - |
| M5StickC PLUS2 | ESP32 | - | - | ST7789 (spi) | - | - | GPIO button | - | - |
| M5Stack Tough | ESP32 | - | SPI | ILI9341 (spi) | - | - | - | - | - |

### ESP32-C3

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| M5Stamp C3 | ESP32-C3 | - | - | - | - | - | GPIO button | SK6812 (rmt) | - |
| M5Stamp C3U | ESP32-C3 | - | - | - | - | - | GPIO button | SK6812 (rmt) | - |

### ESP32-C6

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| M5NanoC6 | ESP32-C6 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |

### ESP32-H2

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| M5NanoH2 | ESP32-H2 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |

Note: `-` means the board does not provide the corresponding BMGR standard device capability.
