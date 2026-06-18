# M5Stack Boards

[![Component Registry](https://components.espressif.com/components/espressif/m5stack_boards/badge.svg)](https://components.espressif.com/components/espressif/m5stack_boards)

[中文](README_CN.md)

M5Stack board definitions for ESP Board Manager.

This component is not a default dependency of `esp_board_manager`. Add it to a project explicitly when these boards are needed:

```yaml
dependencies:
  espressif/m5stack_boards:
    version: "^0.5.0"
```

This component provides board-level configuration files that can be recognized and used by ESP Board Manager, including board metadata, peripheral and device configuration, and board-level default sdkconfig options. After adding this component, use ESP Board Manager commands to list boards or select a board to generate configuration code.

For more information about ESP Board Manager, see the [`esp_board_manager` component documentation](https://github.com/espressif/esp-board-manager/blob/main/esp_board_manager/README.md).

## Supported Boards

| Board | Chip | Audio | SD Card | LCD | LCD Touch | Camera | Button | LED Strip |
|---|---|---|---|---|---|---|---|---|
| [`M5STACK CORES3`](https://docs.m5stack.com/en/core/CoreS3) | ESP32-S3 | AW88298 + ES7210 | SDSPI | ILI9342C | FT5x06 | - | - | - |
| [`M5STACK TAB5`](https://docs.m5stack.com/en/core/Tab5) | ESP32-P4 | ES8388 + ES7210 | SDMMC | ILI9881C / ST7121 / ST7123 auto detect | GT911 / ST712x auto detect | SC202CS | - | - |
