# ESP Friends Boards

[![Component Registry](https://components.espressif.com/components/espressif/esp_friends_boards/badge.svg)](https://components.espressif.com/components/espressif/esp_friends_boards)

[中文](README_CN.md)

Community, partner, and non-official board definitions for ESP Board Manager.

This component is not a default dependency of `esp_board_manager`. Add it to a project explicitly when these boards are needed:

```yaml
dependencies:
  espressif/esp_friends_boards:
    version: "^0.6.1"
```

This component provides board-level configuration files that can be recognized and used by ESP Board Manager, including board metadata, peripheral and device configuration, and board-level default sdkconfig options. After adding this component, use ESP Board Manager commands to list boards or select a board to generate configuration code.

For more information about ESP Board Manager, see the [`esp_board_manager` component documentation](https://github.com/espressif/esp-board-manager/blob/main/esp_board_manager/README.md).

## Known Compatibility Issue

Versions `0.6.0` and later of this component require `espressif/esp_board_manager` `0.7.1` or later.

## Supported Boards

### ESP32-S3

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-S3-BOX-2 | ESP32-S3 | ES8389 (DAC + ADC) | SPI | ST7789 (i80) | - | - | Custom button | - | - |
| ESP32-S3-Korvo-2L | ESP32-S3 | ES8311 (DAC + ADC) | SDMMC | - | - | - | ADC button | - | - |
| ESP32-S3-SparkBot | ESP32-S3 | Built-in ADC | - | ST7789 (spi) | - | Camera (dvp) | - | - | - |

### ESP32

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| ESP-WROVER-KIT | ESP32 | - | SPI | ST7789 (spi) | - | - | GPIO button | - | - |

### ESP32-C3

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| ESP-HI | ESP32-C3 | Built-in ADC + DAC | - | ILI9341 (spi) | - | - | GPIO button | - | - |

### ESP32-C5

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| [`ESP32-C5-Spot`](https://oshwhub.com/esp-college/esp-spot) | ESP32-C5 | ES8311 (ADC + DAC) | - | - | - | - | - | - | - |
| ESP-Ditto | ESP32-C5 | Built-in (DAC + ADC) | - | ILI9341 (parlio) | CST816S (i2c) | Camera (spi) | - | - | - |
