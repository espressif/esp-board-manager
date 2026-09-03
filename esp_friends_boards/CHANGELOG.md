# Changelog

## 0.6.1

### Features

- Added board definitions for ESP-Ditto, ESP32-S3 SparkBot, and ESP-WROVER-KIT.

### Modifications

- Added configured boot-button resources and normalized board YAML formatting for supported friend boards.
- Updated optional LCD, touch, and audio setup code for amend and `gen_skip` compatibility.

## 0.6.0

### Modifications

- Migrated friend-board device peripheral references to role-specific selectors.

## 0.5.3

### Modifications

- Migrated friend-board audio codec configurations to the `esp_codec_dev` 2.0 initialization layout (`sys_cfg`, `adc_cfg`, and explicit PA GPIO fields).

## 0.5.2

### Bug Fixes

- Renamed the `init_level` field to `default_level` for the **ESP32-S3-BOX-2** LCD read-strobe and backlight GPIO peripherals to match the current peripheral schema.
- Added the missing `<stdlib.h>` and `<string.h>` includes to the **ESP32-S3-BOX-2** setup source so it compiles cleanly.

## 0.5.1

### Modifications

- Renamed **ESP32-S3-Korvo-2L** board directory and board ID from `esp32_s3_korvo2l` to `esp32_s3_korvo_2l` to align with Board Manager naming conventions.
- Updated README board names for ESP32-C5-Spot, ESP32-S3-BOX-2, and ESP32-S3-Korvo-2L.

## 0.5.0 (Initial Release)

### Features

- Initial release with non-official/friend board definitions:
  - ESP-HI (`esp_hi`)
  - ESP32-C5-Spot (`esp32_c5_spot`)
  - ESP32-S3-BOX-2 (`esp32_s3_box_2`)
  - ESP32-S3-Korvo-2L (`esp32_s3_korvo2l`)
