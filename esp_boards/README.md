<p align="center">
  <a href="https://www.espressif.com/"><img src="docs/images/espressif-banner.svg" alt="Espressif" width="560"></a>
</p>

# ESP Boards

[![Component Registry](https://components.espressif.com/components/espressif/esp_boards/badge.svg)](https://components.espressif.com/components/espressif/esp_boards)

[中文](README_CN.md)

Official Espressif board definitions for ESP Board Manager.

For the official Espressif development board list, see [ESP DevKits](https://www.espressif.com/en/products/devkits).

**`esp_board_manager` depends on this component by default**, so as long as the project includes `espressif/esp_board_manager`, it will automatically get all the boards provided by this component.

This component provides board-level configuration files that can be recognized and used by ESP Board Manager, including board metadata, peripheral and device configuration, and board-level default sdkconfig options. After adding this component, use ESP Board Manager commands to list boards or select a board to generate configuration code.

For more information about ESP Board Manager, see the [`esp_board_manager` component documentation](https://github.com/espressif/esp-board-manager/blob/main/esp_board_manager/README.md).

## Known Compatibility Issue

- Versions `0.7.3` and later require `espressif/esp_board_manager` `0.7.6` or later for GPIO control initialization with `init_level`.
- Versions `0.6.0` through `0.7.2` require `espressif/esp_board_manager` `0.7.1` or later.

## Supported Boards

The table highlights audio, SD card, display, touch, camera, buttons, LED strip, knob, and Ethernet capabilities. See each board definition and linked documentation for configuration and hardware options.

### ESP32-S31

| Photo | Board | Chip | Device capabilities |
| :---: | --- | --- | --- |
| <a href="esp32_s31_function_coreboard_1/"><img src="esp32_s31_function_coreboard_1/esp32_s31_function_coreboard_1.png" alt="ESP32-S31-Function-CoreBoard-1" width="120"></a> | <a href="esp32_s31_function_coreboard_1/">ESP32-S31-Function-CoreBoard-1</a><br><code>esp32_s31_function_coreboard_1</code> | ESP32-S31 | <strong>Audio</strong>: ES8311 (DAC + ADC)<br><strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt)<br><strong>Ethernet</strong>: YT8531 (RGMII) |
| <a href="esp32_s31_korvo_1/"><img src="esp32_s31_korvo_1/esp32_s31_korvo_1.png" alt="ESP32-S31-Korvo-1" width="120"></a> | <a href="esp32_s31_korvo_1/">ESP32-S31-Korvo-1</a><br><code>esp32_s31_korvo_1</code> | ESP32-S31 | <strong>Audio</strong>: ES8389 (DAC + ADC)<br><strong>SD Card</strong>: SDMMC<br><strong>LCD</strong>: RGB LCD (rgb)<br><strong>LCD Touch</strong>: GT1151 (i2c)<br><strong>Camera</strong>: OV3660/SC101IOT (dvp)<br><strong>Button</strong>: GPIO button + ADC button<br><strong>LED Strip</strong>: WS2812 (rmt) |
| <a href="esp_mosaico/"><img src="esp_mosaico/esp_mosaico.png" alt="ESP-Mosaico" width="120"></a> | <a href="esp_mosaico/">ESP-Mosaico</a><br><code>esp_mosaico</code> | ESP32-S31 | <strong>Audio</strong>: ES8311 (DAC + ADC)<br><strong>LCD</strong>: CO5300 (spi)<br><strong>LCD Touch</strong>: CST9220 (i2c)<br><strong>Button</strong>: GPIO button |

### ESP32-P4

| Photo | Board | Chip | Device capabilities |
| :---: | --- | --- | --- |
| <a href="https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-eye/user_guide.html"><img src="esp32_p4_eye/esp32_p4_eye.png" alt="ESP32-P4-EYE" width="120"></a> | <a href="https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-eye/user_guide.html">ESP32-P4-EYE</a><br><code>esp32_p4_eye</code> | ESP32-P4 | <strong>Audio</strong>: Built-in ADC<br><strong>SD Card</strong>: SDMMC<br><strong>LCD</strong>: ST7789 (spi)<br><strong>Camera</strong>: Camera (csi)<br><strong>Button</strong>: GPIO button<br><strong>Knob</strong>: Knob (gpio) |
| <a href="https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html"><img src="esp32_p4_function_ev_board/esp32_p4_function_ev_board.png" alt="ESP32-P4-Function-EV-Board" width="120"></a> | <a href="https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html">ESP32-P4-Function-EV-Board</a><br><code>esp32_p4_function_ev_board</code> | ESP32-P4 | <strong>Audio</strong>: ES8311 (DAC + ADC)<br><strong>SD Card</strong>: SDMMC<br><strong>LCD</strong>: EK79007 (dsi)<br><strong>LCD Touch</strong>: GT911 (i2c)<br><strong>Camera</strong>: Camera (csi)<br><strong>Button</strong>: GPIO button<br><strong>Ethernet</strong>: IP101 (RMII) |
| <a href="esp32_p4x_c5_function_ev/"><img src="esp32_p4x_c5_function_ev/esp32_p4x_c5_function_ev.png" alt="ESP32-P4X-C5-Function-EV-Board" width="120"></a> | <a href="esp32_p4x_c5_function_ev/">ESP32-P4X-C5-Function-EV-Board</a><br><code>esp32_p4x_c5_function_ev</code> | ESP32-P4 | <strong>Audio</strong>: ES8311 (DAC + ADC)<br><strong>SD Card</strong>: SDMMC<br><strong>LCD</strong>: EK79007 (dsi)<br><strong>LCD Touch</strong>: GT911 (i2c)<br><strong>Camera</strong>: Camera (csi)<br><strong>Button</strong>: GPIO button<br><strong>Ethernet</strong>: IP101 (RMII) |

### ESP32-S3

| Photo | Board | Chip | Device capabilities |
| :---: | --- | --- | --- |
| <a href="https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_3/hardware_overview_for_box_3.md"><img src="esp32_s3_box_3/esp32_s3_box_3.png" alt="ESP32-S3-BOX-3" width="120"></a> | <a href="https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_3/hardware_overview_for_box_3.md">ESP32-S3-BOX-3</a><br><code>esp32_s3_box_3</code> | ESP32-S3 | <strong>Audio</strong>: ES8311 (DAC) + ES7210 (ADC)<br><strong>SD Card</strong>: SDMMC<br><strong>LCD</strong>: ILI9341 (spi)<br><strong>LCD Touch</strong>: GT911/TT21100 (i2c)<br><strong>Button</strong>: GPIO button |
| <a href="https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_lite/hardware_overview_for_lite.md"><img src="esp32_s3_box_lite/esp32_s3_box_lite.png" alt="ESP32-S3-BOX-Lite" width="120"></a> | <a href="https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_lite/hardware_overview_for_lite.md">ESP32-S3-BOX-Lite</a><br><code>esp32_s3_box_lite</code> | ESP32-S3 | <strong>Audio</strong>: ES8156 (DAC) + ES7243E (ADC)<br><strong>LCD</strong>: ST7789 (spi)<br><strong>Button</strong>: ADC button + GPIO button |
| <a href="esp32_s3_devkitc_1/"><img src="esp32_s3_devkitc_1/esp32_s3_devkitc_1.png" alt="ESP32-S3-DevKitC-1" width="120"></a> | <a href="esp32_s3_devkitc_1/">ESP32-S3-DevKitC-1</a><br><code>esp32_s3_devkitc_1</code> | ESP32-S3 | <strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |
| <a href="esp32_s3_devkitm_1/"><img src="esp32_s3_devkitm_1/esp32_s3_devkitm_1.png" alt="ESP32-S3-DevKitM-1" width="120"></a> | <a href="esp32_s3_devkitm_1/">ESP32-S3-DevKitM-1</a><br><code>esp32_s3_devkitm_1</code> | ESP32-S3 | <strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |
| <a href="esp32_s3_eye/"><img src="esp32_s3_eye/esp32_s3_eye.png" alt="ESP32-S3-EYE" width="80"></a> | <a href="esp32_s3_eye/">ESP32-S3-EYE</a><br><code>esp32_s3_eye</code> | ESP32-S3 | <strong>Audio</strong>: Built-in ADC<br><strong>SD Card</strong>: SDMMC<br><strong>LCD</strong>: ST7789 (spi)<br><strong>Camera</strong>: Camera (dvp)<br><strong>Button</strong>: ADC button + GPIO button |
| <a href="esp32_s3_korvo_1/"><img src="esp32_s3_korvo_1/esp32_s3_korvo_1.png" alt="ESP32-S3-Korvo-1" width="120"></a> | <a href="esp32_s3_korvo_1/">ESP32-S3-Korvo-1</a><br><code>esp32_s3_korvo_1</code> | ESP32-S3 | <strong>Audio</strong>: ES8311 (DAC) + ES7210 (ADC)<br><strong>SD Card</strong>: SDMMC<br><strong>Button</strong>: ADC button + GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |
| <a href="https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/user-guide-esp32-s3-korvo-2.html"><img src="esp32_s3_korvo_2_3/esp32_s3_korvo_2_3.png" alt="ESP32-S3-Korvo-2 V3.1" width="120"></a> | <a href="https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/user-guide-esp32-s3-korvo-2.html">ESP32-S3-Korvo-2 V3.1</a><br><code>esp32_s3_korvo_2_3</code> | ESP32-S3 | <strong>Audio</strong>: ES8311 (DAC) + ES7210 (ADC)<br><strong>SD Card</strong>: SDMMC<br><strong>LCD</strong>: ILI9341 (spi)<br><strong>LCD Touch</strong>: TT21100/GT911 (i2c)<br><strong>Camera</strong>: Camera (dvp)<br><strong>Button</strong>: ADC button + GPIO button |
| <a href="https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-lcd-ev-board/index.html"><img src="esp32_s3_lcd_ev_board/esp32_s3_lcd_ev_board.png" alt="ESP32-S3-LCD-EV-Board" width="120"></a> | <a href="https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-lcd-ev-board/index.html">ESP32-S3-LCD-EV-Board</a><br><code>esp32_s3_lcd_ev_board</code> | ESP32-S3 | <strong>Audio</strong>: ES8311 (DAC) + ES7210 (ADC)<br><strong>LCD</strong>: GC9503 (rgb_3wire_spi)<br><strong>LCD Touch</strong>: FT5X06 (i2c)<br><strong>Button</strong>: GPIO button |
| <a href="esp32_s3_usb_otg/"><img src="esp32_s3_usb_otg/esp32_s3_usb_otg.png" alt="ESP32-S3-USB-OTG" width="120"></a> | <a href="esp32_s3_usb_otg/">ESP32-S3-USB-OTG</a><br><code>esp32_s3_usb_otg</code> | ESP32-S3 | <strong>SD Card</strong>: SDMMC<br><strong>LCD</strong>: ST7789 (spi)<br><strong>Button</strong>: GPIO button |
| <a href="esp_dualkey/"><img src="esp_dualkey/esp_dualkey.jpg" alt="ESP-DualKey" width="120"></a> | <a href="esp_dualkey/">ESP-DualKey</a><br><code>esp_dualkey</code> | ESP32-S3 | <strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (spi) |
| <a href="esp_thread_border_router/"><img src="esp_thread_border_router/esp_thread_border_router.png" alt="ESP Thread Border Router Board" width="119"></a> | <a href="esp_thread_border_router/">ESP Thread Border Router Board</a><br><code>esp_thread_border_router</code> | ESP32-S3 | <strong>Button</strong>: GPIO button |
| <a href="https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp-vocat/user_guide_v1.0.html"><img src="esp_vocat_1_0/esp_vocat_1_0.png" alt="ESP-VoCat V1.0" width="120"></a> | <a href="https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp-vocat/user_guide_v1.0.html">ESP-VoCat V1.0</a><br><code>esp_vocat_1_0</code> | ESP32-S3 | <strong>Audio</strong>: ES8311 (DAC) + ES7210 (ADC)<br><strong>SD Card</strong>: SDMMC<br><strong>LCD</strong>: ST77916 (spi)<br><strong>LCD Touch</strong>: CST816S (i2c)<br><strong>Button</strong>: GPIO button |
| <a href="https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp-vocat/user_guide_v1.2.html"><img src="esp_vocat_1_2/esp_vocat_1_2.png" alt="ESP-VoCat V1.2" width="120"></a> | <a href="https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp-vocat/user_guide_v1.2.html">ESP-VoCat V1.2</a><br><code>esp_vocat_1_2</code> | ESP32-S3 | <strong>Audio</strong>: ES8311 (DAC) + ES7210 (ADC)<br><strong>SD Card</strong>: SDMMC<br><strong>LCD</strong>: ST77916 (spi)<br><strong>LCD Touch</strong>: CST816S (i2c)<br><strong>Button</strong>: GPIO button |

### ESP32

| Photo | Board | Chip | Device capabilities |
| :---: | --- | --- | --- |
| <a href="esp32_devkitc/"><img src="esp32_devkitc/esp32_devkitc.png" alt="ESP32-DevKitC V4" width="120"></a> | <a href="esp32_devkitc/">ESP32-DevKitC V4</a><br><code>esp32_devkitc</code> | ESP32 | <strong>Button</strong>: GPIO button |
| <a href="esp32_devkitm_1/"><img src="esp32_devkitm_1/esp32_devkitm_1.png" alt="ESP32-DevKitM-1" width="120"></a> | <a href="esp32_devkitm_1/">ESP32-DevKitM-1</a><br><code>esp32_devkitm_1</code> | ESP32 | <strong>Button</strong>: GPIO button |
| <a href="esp32_ethernet_kit/"><img src="esp32_ethernet_kit/esp32_ethernet_kit.jpg" alt="ESP32-Ethernet-Kit v1.2" width="120"></a> | <a href="esp32_ethernet_kit/">ESP32-Ethernet-Kit v1.2</a><br><code>esp32_ethernet_kit</code> | ESP32 | <strong>Button</strong>: GPIO button<br><strong>Ethernet</strong>: IP101 (RMII) |
| <a href="esp32_lcdkit/"><img src="esp32_lcdkit/esp32_lcdkit.png" alt="ESP32-LCDKit" width="98"></a> | <a href="esp32_lcdkit/">ESP32-LCDKit</a><br><code>esp32_lcdkit</code> | ESP32 | <strong>SD Card</strong>: SDMMC<br><strong>LCD</strong>: ILI9341 (spi) |
| <a href="https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/get-started-esp32-lyrat.html"><img src="esp32_lyrat_4_3/esp32_lyrat_4_3.jpg" alt="ESP32-LyraT V4.3" width="120"></a> | <a href="https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/get-started-esp32-lyrat.html">ESP32-LyraT V4.3</a><br><code>esp32_lyrat_4_3</code> | ESP32 | <strong>Audio</strong>: ES8388 (DAC + ADC)<br><strong>SD Card</strong>: SDMMC<br><strong>Button</strong>: GPIO button |
| <a href="https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/get-started-esp32-lyrat-mini.html"><img src="esp32_lyrat_mini_1_1/esp32_lyrat_mini_1_1.png" alt="ESP32-LyraT-Mini" width="120"></a><br><sub>Image shows LyraT-Mini V1.2; the board definition is esp32_lyrat_mini_1_1.</sub> | <a href="https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/get-started-esp32-lyrat-mini.html">ESP32-LyraT-Mini</a><br><code>esp32_lyrat_mini_1_1</code> | ESP32 | <strong>Audio</strong>: ES8311 (DAC) + ES7243E (ADC)<br><strong>SD Card</strong>: SDMMC<br><strong>Button</strong>: ADC button + GPIO button |
| <a href="esp32_meshkit_sense/"><img src="esp32_meshkit_sense/esp32_meshkit_sense.png" alt="ESP32-MeshKit-Sense v1.1" width="120"></a> | <a href="esp32_meshkit_sense/">ESP32-MeshKit-Sense v1.1</a><br><code>esp32_meshkit_sense</code> | ESP32 | <strong>Button</strong>: GPIO button |
| <a href="esp32_pico_devkitm_2/"><img src="esp32_pico_devkitm_2/esp32_pico_devkitm_2.png" alt="ESP32-PICO-DevKitM-2" width="120"></a> | <a href="esp32_pico_devkitm_2/">ESP32-PICO-DevKitM-2</a><br><code>esp32_pico_devkitm_2</code> | ESP32 | <strong>Button</strong>: GPIO button |
| <a href="esp32_pico_kit/"><img src="esp32_pico_kit/esp32_pico_kit.jpg" alt="ESP32-PICO-KIT" width="120"></a> | <a href="esp32_pico_kit/">ESP32-PICO-KIT</a><br><code>esp32_pico_kit</code> | ESP32 | <strong>Button</strong>: GPIO button |
| <a href="esp32_pico_kit_1/"><img src="esp32_pico_kit_1/esp32_pico_kit_1.png" alt="ESP32-PICO-KIT-1" width="120"></a> | <a href="esp32_pico_kit_1/">ESP32-PICO-KIT-1</a><br><code>esp32_pico_kit_1</code> | ESP32 | <strong>Button</strong>: GPIO button |
| <a href="esp32_sense_kit/"><img src="esp32_sense_kit/esp32_sense_kit.png" alt="ESP32-Sense-Kit" width="98"></a> | <a href="esp32_sense_kit/">ESP32-Sense-Kit</a><br><code>esp32_sense_kit</code> | ESP32 | — |
| <a href="esp32_vaquita_dspg/"><img src="esp32_vaquita_dspg/esp32_vaquita_dspg.png" alt="ESP32-Vaquita-DSPG v1.0" width="120"></a> | <a href="esp32_vaquita_dspg/">ESP32-Vaquita-DSPG v1.0</a><br><code>esp32_vaquita_dspg</code> | ESP32 | <strong>Audio</strong>: ES8311 (DAC)<br><strong>Button</strong>: ADC button + GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |

### ESP32-C3

| Photo | Board | Chip | Device capabilities |
| :---: | --- | --- | --- |
| <a href="esp32_c3_aws_expresslink_devkit/"><img src="esp32_c3_aws_expresslink_devkit/esp32_c3_aws_expresslink_devkit.png" alt="ESP32-C3-AWS-ExpressLink-DevKit" width="120"></a> | <a href="esp32_c3_aws_expresslink_devkit/">ESP32-C3-AWS-ExpressLink-DevKit</a><br><code>esp32_c3_aws_expresslink_devkit</code> | ESP32-C3 | — |
| <a href="esp32_c3_devkit_rust_1/"><img src="esp32_c3_devkit_rust_1/esp32_c3_devkit_rust_1.jpg" alt="ESP32-C3-DevKit-RUST-1" width="120"></a> | <a href="esp32_c3_devkit_rust_1/">ESP32-C3-DevKit-RUST-1</a><br><code>esp32_c3_devkit_rust_1</code> | ESP32-C3 | <strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |
| <a href="esp32_c3_devkit_rust_2/"><img src="esp32_c3_devkit_rust_2/esp32_c3_devkit_rust_2.png" alt="ESP32-C3-DevKit-RUST-2" width="112"></a> | <a href="esp32_c3_devkit_rust_2/">ESP32-C3-DevKit-RUST-2</a><br><code>esp32_c3_devkit_rust_2</code> | ESP32-C3 | <strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |
| <a href="esp32_c3_devkitc_02/"><img src="esp32_c3_devkitc_02/esp32_c3_devkitc_02.png" alt="ESP32-C3-DevKitC-02" width="120"></a> | <a href="esp32_c3_devkitc_02/">ESP32-C3-DevKitC-02</a><br><code>esp32_c3_devkitc_02</code> | ESP32-C3 | <strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |
| <a href="esp32_c3_devkitm_1/"><img src="esp32_c3_devkitm_1/esp32_c3_devkitm_1.png" alt="ESP32-C3-DevKitM-1" width="120"></a> | <a href="esp32_c3_devkitm_1/">ESP32-C3-DevKitM-1</a><br><code>esp32_c3_devkitm_1</code> | ESP32-C3 | <strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |
| <a href="esp32_c3_lcdkit/"><img src="esp32_c3_lcdkit/esp32_c3_lcdkit.png" alt="ESP32-C3-LCDKit" width="120"></a> | <a href="esp32_c3_lcdkit/">ESP32-C3-LCDKit</a><br><code>esp32_c3_lcdkit</code> | ESP32-C3 | <strong>Audio</strong>: Built-in DAC<br><strong>LCD</strong>: GC9A01 (spi)<br><strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt)<br><strong>Knob</strong>: Knob (gpio) |
| <a href="https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/user-guide-esp32-c3-lyra.html"><img src="esp32_c3_lyra/esp32_c3_lyra.png" alt="ESP32-C3-Lyra" width="120"></a> | <a href="https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/dev-boards/user-guide-esp32-c3-lyra.html">ESP32-C3-Lyra</a><br><code>esp32_c3_lyra</code> | ESP32-C3 | <strong>Audio</strong>: Built-in ADC + DAC<br><strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |

### ESP32-C5

| Photo | Board | Chip | Device capabilities |
| :---: | --- | --- | --- |
| <a href="esp32_c5_devkitc_1/"><img src="esp32_c5_devkitc_1/esp32_c5_devkitc_1.png" alt="ESP32-C5-DevKitC-1" width="120"></a> | <a href="esp32_c5_devkitc_1/">ESP32-C5-DevKitC-1</a><br><code>esp32_c5_devkitc_1</code> | ESP32-C5 | <strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |
| <a href="esp_sensairshuttle/"><img src="esp_sensairshuttle/esp_sensairshuttle.png" alt="ESP-SensairShuttle" width="120"></a> | <a href="esp_sensairshuttle/">ESP-SensairShuttle</a><br><code>esp_sensairshuttle</code> | ESP32-C5 | <strong>Audio</strong>: Built-in DAC + ADC<br><strong>LCD</strong>: ST7789 (spi)<br><strong>LCD Touch</strong>: CST816S (i2c)<br><strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |

### ESP32-C6

| Photo | Board | Chip | Device capabilities |
| :---: | --- | --- | --- |
| <a href="esp32_c6_devkitc_1/"><img src="esp32_c6_devkitc_1/esp32_c6_devkitc_1.png" alt="ESP32-C6-DevKitC-1" width="120"></a> | <a href="esp32_c6_devkitc_1/">ESP32-C6-DevKitC-1</a><br><code>esp32_c6_devkitc_1</code> | ESP32-C6 | <strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |
| <a href="esp32_c6_devkitm_1/"><img src="esp32_c6_devkitm_1/esp32_c6_devkitm_1.png" alt="ESP32-C6-DevKitM-1" width="120"></a> | <a href="esp32_c6_devkitm_1/">ESP32-C6-DevKitM-1</a><br><code>esp32_c6_devkitm_1</code> | ESP32-C6 | <strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |

### ESP32-H2

| Photo | Board | Chip | Device capabilities |
| :---: | --- | --- | --- |
| <a href="esp32_h2_devkitm_1/"><img src="esp32_h2_devkitm_1/esp32_h2_devkitm_1.png" alt="ESP32-H2-DevKitM-1" width="120"></a> | <a href="esp32_h2_devkitm_1/">ESP32-H2-DevKitM-1</a><br><code>esp32_h2_devkitm_1</code> | ESP32-H2 | <strong>Button</strong>: GPIO button<br><strong>LED Strip</strong>: WS2812 (rmt) |
