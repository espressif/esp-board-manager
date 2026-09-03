# ESP Boards

[![Component Registry](https://components.espressif.com/components/espressif/esp_boards/badge.svg)](https://components.espressif.com/components/espressif/esp_boards)

[中文](README_CN.md)

Official Espressif board definitions for ESP Board Manager.

For the official Espressif development board list, see [ESP DevKits](https://www.espressif.com/en/products/devkits).

**`esp_board_manager` depends on this component by default**, so as long as the project includes `espressif/esp_board_manager`, it will automatically get all the boards provided by this component.

This component provides board-level configuration files that can be recognized and used by ESP Board Manager, including board metadata, peripheral and device configuration, and board-level default sdkconfig options. After adding this component, use ESP Board Manager commands to list boards or select a board to generate configuration code.

For more information about ESP Board Manager, see the [`esp_board_manager` component documentation](https://github.com/espressif/esp-board-manager/blob/main/esp_board_manager/README.md).

## Known Compatibility Issue

Versions `0.6.0` and later of this component require `espressif/esp_board_manager` `0.7.1` or later.

## Supported Boards

### ESP32-S31

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-S31-Function-CoreBoard-1 | ESP32-S31 | ES8311 (DAC + ADC) | - | - | - | - | GPIO button | WS2812 (rmt) | - |
| ESP32-S31-Korvo-1 | ESP32-S31 | ES8389 (DAC + ADC) | SDMMC | RGB LCD (rgb) | GT1151 (i2c) | Camera (dvp) | GPIO button + ADC button | WS2812 (rmt) | - |
| ESP-Mosaico | ESP32-S31 | ES8311 (DAC + ADC) | - | CO5300 (spi) | CST9217 (i2c) | - | GPIO button | - | - |

### ESP32-P4

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| [`ESP32-P4-EYE`](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-eye/user_guide.html) | ESP32-P4 | Built-in ADC | SDMMC | ST7789 (spi) | - | Camera (csi) | GPIO button | - | Knob (gpio) |
| [`ESP32-P4-Function-EV-Board`](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html) | ESP32-P4 | ES8311 (DAC + ADC) | SDMMC | EK79007 (dsi) | GT911 (i2c) | Camera (csi) | GPIO button | - | - |
| ESP32-P4X-C5-Function-EV-Board | ESP32-P4 | ES8311 (DAC + ADC) | SDMMC | EK79007 (dsi) | GT911 (i2c) | Camera (csi) | GPIO button | - | - |

### ESP32-S3

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| [`ESP32-S3-BOX-3`](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_3/hardware_overview_for_box_3.md) | ESP32-S3 | ES8311 (DAC) + ES7210 (ADC) | SDMMC | ILI9341 (spi) | GT911/TT21100 (i2c) | - | GPIO button | - | - |
| [`ESP32-S3-BOX-Lite`](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_lite/hardware_overview_for_lite.md) | ESP32-S3 | ES8156 (DAC) + ES7243E (ADC) | - | ST7789 (spi) | - | - | GPIO button | - | - |
| ESP32-S3-DevKitC-1 | ESP32-S3 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |
| ESP32-S3-DevKitM-1 | ESP32-S3 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |
| ESP32-S3-EYE | ESP32-S3 | Built-in ADC | SDMMC | ST7789 (spi) | - | Camera (dvp) | ADC button + GPIO button | - | - |
| ESP32-S3-Korvo-1 | ESP32-S3 | ES8311 (DAC) + ES7210 (ADC) | SDMMC | - | - | - | ADC button + GPIO button | WS2812 (rmt) | - |
| [`ESP32-S3-Korvo-2 V3.1`](https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/user-guide-esp32-s3-korvo-2.html) | ESP32-S3 | ES8311 (DAC) + ES7210 (ADC) | SDMMC | ILI9341 (spi) | TT21100/GT911 (i2c) | Camera (dvp) | ADC button + GPIO button | - | - |
| [`ESP32-S3-LCD-EV-Board`](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-lcd-ev-board/index.html) | ESP32-S3 | ES8311 (DAC) + ES7210 (ADC) | - | GC9503 (rgb_3wire_spi) | FT5X06 (i2c) | - | GPIO button | - | - |
| ESP32-S3-USB-OTG | ESP32-S3 | - | SDMMC | ST7789 (spi) | - | - | GPIO button | - | - |
| ESP-DualKey | ESP32-S3 | - | - | - | - | - | GPIO button | WS2812 (spi) | - |
| ESP Thread Border Router Board | ESP32-S3 | - | - | - | - | - | GPIO button | - | - |
| [`ESP-VoCat V1.0`](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp-vocat/user_guide_v1.0.html) | ESP32-S3 | ES8311 (DAC) + ES7210 (ADC) | SDMMC | ST77916 (spi) | CST816S (i2c) | - | GPIO button | - | - |
| [`ESP-VoCat V1.2`](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp-vocat/user_guide_v1.2.html) | ESP32-S3 | ES8311 (DAC) + ES7210 (ADC) | SDMMC | ST77916 (spi) | CST816S (i2c) | - | GPIO button | - | - |

### ESP32

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-DevKitC V4 | ESP32 | - | - | - | - | - | GPIO button | - | - |
| ESP32-DevKitM-1 | ESP32 | - | - | - | - | - | GPIO button | - | - |
| ESP32-Ethernet-Kit v1.2 | ESP32 | - | - | - | - | - | GPIO button | - | - |
| ESP32-LCDKit | ESP32 | - | SDMMC | ILI9341 (spi) | - | - | - | - | - |
| [`ESP32-LyraT V4.3`](https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/get-started-esp32-lyrat.html) | ESP32 | ES8388 (DAC + ADC) | SDMMC | - | - | - | GPIO button | - | - |
| [`ESP32-LyraT-Mini`](https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/get-started-esp32-lyrat-mini.html) | ESP32 | ES8311 (DAC) + ES7243E (ADC) | SDMMC | - | - | - | ADC button + GPIO button | - | - |
| ESP32-MeshKit-Sense v1.1 | ESP32 | - | - | - | - | - | GPIO button | - | - |
| ESP32-PICO-DevKitM-2 | ESP32 | - | - | - | - | - | GPIO button | - | - |
| ESP32-PICO-KIT | ESP32 | - | - | - | - | - | GPIO button | - | - |
| ESP32-PICO-KIT-1 | ESP32 | - | - | - | - | - | GPIO button | - | - |
| ESP32-Sense-Kit | ESP32 | - | - | - | - | - | - | - | - |
| ESP32-Vaquita-DSPG v1.0 | ESP32 | ES8311 (DAC) | - | - | - | - | ADC button + GPIO button | WS2812 (rmt) | - |

### ESP32-C3

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-C3-AWS-ExpressLink-DevKit | ESP32-C3 | - | - | - | - | - | - | - | - |
| ESP32-C3-DevKit-RUST-1 | ESP32-C3 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |
| ESP32-C3-DevKit-RUST-2 | ESP32-C3 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |
| ESP32-C3-DevKitC-02 | ESP32-C3 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |
| ESP32-C3-DevKitM-1 | ESP32-C3 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |
| ESP32-C3-LCDKit | ESP32-C3 | Built-in DAC | - | GC9A01 (spi) | - | - | GPIO button | WS2812 (rmt) | Knob (gpio) |
| [`ESP32-C3-Lyra`](https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/user-guide-esp32-c3-lyra.html) | ESP32-C3 | Built-in ADC + DAC | - | - | - | - | GPIO button | - | - |

### ESP32-C5

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-C5-DevKitC-1 | ESP32-C5 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |
| ESP-SensairShuttle | ESP32-C5 | Built-in DAC + ADC | - | ST7789 (spi) | CST816S (i2c) | - | GPIO button | WS2812 (rmt) | - |

### ESP32-C6

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-C6-DevKitC-1 | ESP32-C6 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |
| ESP32-C6-DevKitM-1 | ESP32-C6 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |

### ESP32-H2

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip | Knob |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-H2-DevKitM-1 | ESP32-H2 | - | - | - | - | - | GPIO button | WS2812 (rmt) | - |

Note: `-` means the board does not provide the corresponding BMGR standard device capability.
