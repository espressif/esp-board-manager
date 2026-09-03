# Changelog

## 0.6.1

### Features

- Added board definitions for ESP32-C3-LCDKit, ESP32-LCDKit, ESP32-MeshKit-Sense, ESP32-P4X-C5-Function-EV-Board, ESP-DualKey, ESP-Mosaico, ESP-SensairShuttle, ESP Thread Border Router, and the ESP32-C3/C5/C6/H2/S3/ESP32 DevKit families.
- Added board definitions for ESP32-C3-AWS-ExpressLink-DevKit, ESP32-C3 DevKit Rust 1/2, ESP32-Ethernet-Kit, ESP32-PICO DevKitM-2, ESP32-PICO-KIT/KIT-1, ESP32-S3-EYE, ESP32-S3-Korvo-1, ESP32-S3-USB-OTG, ESP32-Sense-Kit, and ESP32-Vaquita-DSPG.

### Modifications

- Added configured boot-button resources and normalized device/peripheral YAML formatting across supported official boards.
- Updated optional LCD, touch, and IO-expander factory implementations for amend and `gen_skip` compatibility.

## 0.6.0

### Modifications

- Migrated official board device peripheral references to role-specific selectors.
- Updated LCD device configurations to use the generated frame-format model where application pixel byte order must be identified.
- Fixed I2S slot configurations on some boards to ensure that the total slot count and slot mask match the actual ADC/DAC channel configuration of the codec.

## 0.5.4~1

### Bug Fixes

- Aligned the **ESP32-P4-Function-EV-Board** DSI LCD `bits_per_pixel` setting with its RGB565 color format.

## 0.5.4

### Modifications

- Migrated official audio codec board configurations to the `esp_codec_dev` 2.0 initialization layout (`sys_cfg`, `adc_cfg`, `dac_cfg`, and explicit PA GPIO fields).
- Added explicit camera XCLK and sensor bring-up sequencing for the **ESP32-S31-Korvo-1** DVP camera.

### Bug Fixes

- Corrected the default PSRAM frequency to 200 MHz for **ESP32-S31-Function-Coreboard-1** and **ESP32-S31-Korvo-1**.

## 0.5.3~1

### Bug Fixes

- Corrected the **ESP32-S31-Korvo-1** button labels.

## 0.5.3

### Bug Fixes

- Corrected the **ESP32-LyraT-Mini** flash size default to match the board hardware.

## 0.5.2

### Bug Fixes

- Re-enabled the `adc_oneshot` peripheral on **ESP32-S31-Korvo-1** that was mistakenly commented out, restoring ADC button support, and adjusted its attenuation and bit width to the SoC defaults.
- Fixed the **ESP32-S31-Korvo-1** SD-card `power_ctrl` device to declare `peripherals` at the device top level instead of nesting it under `config`, matching the current device schema.
- Guarded the GT1151 touch factory entry on the **ESP32-S3-LCD-EV-Board** 800x480 sub-board behind an internal macro so the setup source compiles whether or not the touch driver header is present.

### Modifications

- Corrected IO expander initialization ordering: added `depends_on: gpio_expander` to the **ESP32-S3-LCD-EV-Board** display panel and removed the redundant dependency from the **ESP32-S3-Korvo-2 V3.1** LCD touch device.
- Removed the redundant `CONFIG_ESP_VIDEO_ENABLE_SWAP_BYTE` and `CONFIG_ESP_VIDEO_ENABLE_SWAP_BYTE_RISCV` defaults from **ESP32-S31-Korvo-1**.

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
