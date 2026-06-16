# Changelog

## 0.5.1

### Features

- Added full board support for **ESP32-P4-EYE**, including PDM microphone, SDMMC SD card, SPI LCD, CSI camera, GPIO buttons, flashlight LED, and power controls.
- Added board support for **ESP32-LyraT V4.3**, including ES8388 audio codec and SD card.

### Bug Fixes

- Updated ESP32-P4-Function-EV-Board, ESP32-S3-Korvo-2 V3.1, and ESP32-S3-LCD-EV-Board setup code to use the Board Manager aggregate include header.
- Guarded optional LCD, touch, and IO expander factory entries behind detected component headers so board setup sources can compile when optional driver dependencies are not enabled.

## 0.5.0 (Initial Release)

### Features

- Initial release with official Espressif board definitions:
  - ESP32-C3-Lyra (`esp32_c3_lyra`)
  - ESP32-LyraT-Mini (`esp32_lyrat_mini_1_1`)
  - ESP32-P4-Function-EV-Board (`esp32_p4_function_ev_board`)
  - ESP32-S31-Function-Coreboard-1 (`esp32_s31_function_coreboard_1`)
  - ESP32-S31-Korvo-1 (`esp32_s31_korvo_1`)
  - ESP32-S3-BOX-3 (`esp32_s3_box_3`)
  - ESP32-S3-BOX-Lite (`esp32_s3_box_lite`)
  - ESP32-S3-Korvo-2 V3.1 (`esp32_s3_korvo_2_3`)
  - ESP32-S3-LCD-EV-Board (`esp32_s3_lcd_ev_board`)
  - ESP-VoCat V1.0 (`esp_vocat_1_0`)
  - ESP-VoCat V1.2 (`esp_vocat_1_2`)
