# ESP Boards

[![Component Registry](https://components.espressif.com/components/espressif/esp_boards/badge.svg)](https://components.espressif.com/components/espressif/esp_boards)

[中文](README_CN.md)

Official Espressif board definitions for ESP Board Manager.

For the official Espressif development board list, see [ESP DevKits](https://www.espressif.com/en/products/devkits).

**`esp_board_manager` depends on this component by default**, so as long as the project includes `espressif/esp_board_manager`, it will automatically get all the boards provided by this component.

This component provides board-level configuration files that can be recognized and used by ESP Board Manager, including board metadata, peripheral and device configuration, and board-level default sdkconfig options. After adding this component, use ESP Board Manager commands to list boards or select a board to generate configuration code.

For more information about ESP Board Manager, see the [`esp_board_manager` component documentation](https://github.com/espressif/esp-board-manager/blob/main/esp_board_manager/README.md).

## Supported Boards

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip |
|---|---|---|---|---|---|---|---|---|
| [`ESP-VoCat V1.0`](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp-vocat/user_guide_v1.0.html) | ESP32-S3 | ES8311 + ES7210 | SDMMC | ST77916 | CTS816S | - | - | - |
| [`ESP-VoCat V1.2`](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp-vocat/user_guide_v1.2.html) | ESP32-S3 | ES8311 + ES7210 | SDMMC | ST77916 | CTS816S | - | - | - |
| [`ESP32-S3-BOX-3`](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_3/hardware_overview_for_box_3.md) | ESP32-S3 | ES8311 + ES7210 | SDMMC | ST77916 | GT911/TT21100 auto detect | - | - | - |
| [`ESP32-C3-Lyra`](https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/user-guide-esp32-c3-lyra.html) | ESP32-C3 | Built-in ADC + PDM speaker | - | - | - | - | - | - |
| [`ESP32-S3-Korvo-2 V3.1`](https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/user-guide-esp32-s3-korvo-2.html) | ESP32-S3 | ES8311 + ES7210 | SDMMC | ILI9341 | TT21100/GT911 auto detect | DVP Camera | ADC button | - |
| [`ESP32-S3-LCD-EV-Board`](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-lcd-ev-board/index.html) | ESP32-S3 | ES8311 + ES7210 | - | GC9503 RGB LCD | FT5x06 | - | GPIO button | - |
| ESP32-S31-Function-Coreboard-1 | ESP32-S31 | ES8311 | - | - | - | - | - | WS2812 |
| ESP32-S31-Korvo-1 | ESP32-S31 | ES8389 | - | RGB LCD | GT1151 | - | - | WS2812 |
| [`ESP32-LyraT V4.3`](https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/get-started-esp32-lyrat.html) | ESP32 | ES8388 | SDMMC | - | - | - | - | - |
| [`ESP32-LyraT-Mini`](https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/get-started-esp32-lyrat-mini.html) | ESP32 | ES8388 | SDMMC | - | - | - | ADC button | - |
| [`ESP32-P4-Function-EV-Board`](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html) | ESP32-P4 | ES8311 | SDMMC | EK79007 | GT911 | CSI Camera | - | - |
| [`ESP32-P4-EYE`](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-eye/user_guide.html) | ESP32-P4 | Built-in ADC | SDMMC | ST7789 | - | CSI Camera | GPIO button | - |
| [`ESP32-S3-BOX-Lite`](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_lite/hardware_overview_for_lite.md) | ESP32-S3 | ES8156 + ES7243E | - | ST7789 | - | - | - | - |

Note: `-` means the board does not provide that hardware capability.
