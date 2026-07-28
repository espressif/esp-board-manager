# Changelog

## 0.5.3

### Changed

- **M5STACK CORES3**: migrated AXP2101 power sequencing from a `type: custom` device (`axp2101_power_manager`) to the board manager's new `power_ctrl` `custom` sub-type (requires `esp_board_manager >= 0.6.1`). Consumer devices (`audio_dac`, `audio_adc`, `display_lcd`, `lcd_touch`, `fs_sdcard`) now reference it via `power_ctrl_device` instead of `depends_on`, so each device's power rail is enabled on demand when that device initializes, instead of all rails being enabled unconditionally during power-manager init.
- **M5STACK CORES3**: replaced the board-local AW9523B IO expander driver with the published `espressif/esp_io_expander_aw9523` component (`1.0.0`).

## 0.5.2

### Modifications

- Added explicit `depends_on` ordering on **M5STACK CORES3** so the AXP2101 power manager (and the IO expander it relies on) initializes before the audio codecs, display panel, and touch controller.

## 0.5.1

### Bug Fixes

- Guarded M5STACK CORES3 LCD panel and touch factory entries behind detected component headers, and made the factory entries weak so applications can override them.
- Updated M5STACK TAB5 setup code to use the Board Manager aggregate include header and fixed the missing `ret` declaration in the DSI panel factory.

### Modifications

- Updated M5STACK TAB5 defaults with the ESP32-P4 chip revision options (`CONFIG_ESP32P4_SELECTS_REV_LESS_V3`, `CONFIG_ESP32P4_REV_MIN_100`) and ST7121 LCD panel/touch variant.

## 0.5.0 (Initial Release)

### Features

- Initial release with M5Stack board definitions:
  - M5STACK CORES3 (`m5stack_cores3`)
  - M5STACK TAB5 (`m5stack_tab5`)
