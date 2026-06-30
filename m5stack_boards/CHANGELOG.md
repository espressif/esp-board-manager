# Changelog

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
