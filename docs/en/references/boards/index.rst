Boards Catalog
================

:link_to_translation:`zh_CN:[中文]`

**Starting from BMGR 0.6**: Boards are removed from the BMGR component and split into multiple independent board components:

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Component
     - Description
   * - ``espressif/esp_boards``
     - Official Espressif development boards. BMGR 0.6 declares this dependency by default; it is automatically available when BMGR is introduced — no additional project configuration is needed. `View online <https://github.com/espressif/esp-board-manager/tree/main/esp_boards>`__
   * - ``espressif/esp_friends_boards``
     - Partner and community development boards. Requires manually declaring the dependency in the project's main component manifest (``idf_component.yml``). `View online <https://github.com/espressif/esp-board-manager/tree/main/esp_friends_boards>`__
   * - ``espressif/m5stack_boards``
     - M5Stack series development boards. Requires manually declaring the dependency in the project component manifest. `View online <https://github.com/espressif/esp-board-manager/tree/main/m5stack_boards>`__

Official Development Boards (``espressif/esp_boards``)
------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 35 65 65

   * - Name
     - Onboard capabilities
     - Introduction
   * - :doc:`ESP32-C3-LCDKit <esp32_c3_lcdkit>`
     - GC9A01 SPI LCD, PDM speaker, knob, WS2812, SPIFFS
     - ESP32-C3 board for display applications.
   * - :doc:`ESP32-C3-Lyra <esp32_c3_lyra>`
     - Internal ADC, PDM audio output, amplifier control, GPIO boot button
     - ESP32-C3 audio board.
   * - :doc:`ESP32-LyraT V4.3 <esp32_lyrat_4_3>`
     - ES8388, microSD, I2S, amplifier control, GPIO boot button
     - ESP32 audio board.
   * - :doc:`ESP32-LyraT-Mini <esp32_lyrat_mini_1_1>`
     - ES8311, ES7243E, microSD, ADC multi-button, GPIO boot button
     - ESP32 board for audio playback and recording.
   * - :doc:`ESP32-P4-EYE <esp32_p4_eye>`
     - CSI camera, ST7789 LCD, microSD, internal ADC, GPIO buttons, GPIO boot button, knob
     - ESP32-P4 board for vision applications.
   * - :doc:`ESP32-P4-Function-EV-Board <esp32_p4_function_ev_board>`
     - DSI display, touch, audio, CSI camera, microSD, GPIO boot button
     - ESP32-P4 multimedia board.
   * - :doc:`ESP32-S31-Function-CoreBoard-1 <esp32_s31_function_coreboard_1>`
     - ES8311, microphone, speaker interface, WS2812, GPIO boot button
     - Board for connected AIoT prototyping.
   * - :doc:`ESP32-S31-Korvo-1 <esp32_s31_korvo_1>`
     - Dual microphones, ES8389, 4.3-inch LCD, touch, DVP camera, microSD, WS2812, GPIO boot button
     - Multimedia board for smart audio and HMI.
   * - :doc:`ESP32-S3-BOX-3 <esp32_s3_box_3>`
     - ES8311, ES7210, ST77916 LCD, touch, microSD, GPIO boot button
     - ESP32-S3 voice and multimedia board.
   * - :doc:`ESP32-S3-BOX-Lite <esp32_s3_box_lite>`
     - ES8156, ES7243E, ST7789 LCD, GPIO boot button
     - ESP32-S3 audio and display board.
   * - :doc:`ESP32-S3-EYE <esp32_s3_eye>`
     - OV2640 camera, LCD, digital microphone, microSD, accelerometer, ADC buttons, GPIO boot button
     - ESP32-S3 AI board for vision and audio processing.
   * - :doc:`ESP32-S3-Korvo-1 <esp32_s3_korvo_1>`
     - Three-microphone array, ES8311, ES7210, microSD, ADC buttons, WS2812C, GPIO boot button
     - ESP32-S3 board for speech-recognition applications.
   * - :doc:`ESP32-S3-Korvo-2 V3.1 <esp32_s3_korvo_2_3>`
     - ES8311/ES7210, ILI9341, touch, DVP camera, microSD, ADC buttons, GPIO boot button
     - ESP32-S3 board for audio and multimedia applications.
   * - :doc:`ESP32-S3-LCD-EV-Board <esp32_s3_lcd_ev_board>`
     - GC9503 RGB LCD, FT5x06, ES8311/ES7210, GPIO boot button
     - Board for ESP32-S3 display evaluation.
   * - :doc:`ESP-SensairShuttle <esp_sensairshuttle>`
     - BMI270, audio input/output, LCD, touch, WS2812, GPIO boot button
     - ESP32-C5 board for motion sensing and large-model HMI.
   * - :doc:`ESP-VoCat V1.0 <esp_vocat_1_0>`
     - Round touch display, dual microphones, audio codec, microSD, GPIO boot button
     - ESP32-S3 board for voice interaction.
   * - :doc:`ESP-VoCat V1.2 <esp_vocat_1_2>`
     - Round touch display, dual microphones, audio codec, microSD, status LED, GPIO boot button
     - V1.2 hardware revision of ESP-VoCat.
   * - :doc:`ESP32-C3-AWS-ExpressLink-DevKit <esp32_c3_aws_expresslink_devkit>`
     - ESP32-C3-MINI-1-N4-A, UART, event/wake/reset control
     - ESP32-C3 evaluation board for AWS IoT ExpressLink connectivity.
   * - :doc:`ESP32-C3-DevKit-Rust-1 <esp32_c3_devkit_rust_1>`
     - ICM-42670-P IMU, SHTC3, WS2812, button, USB
     - ESP32-C3 sensor board for Rust development.
   * - :doc:`ESP32-C3-DevKit-Rust-2 <esp32_c3_devkit_rust_2>`
     - ICM-42670-P IMU, SHTC3, WS2812, button, USB
     - Updated ESP32-C3 Rust development board.
   * - :doc:`ESP32-C3-DevKitC-02 <esp32_c3_devkitc_02>`
     - ESP32-C3-WROOM-02, USB, GPIO headers
     - General-purpose ESP32-C3 development board.
   * - :doc:`ESP32-C3-DevKitM-1 <esp32_c3_devkitm_1>`
     - ESP32-C3-MINI-1, USB-UART, GPIO headers
     - General-purpose board based on ESP32-C3-MINI-1.
   * - :doc:`ESP32-C5-DevKitC-1 <esp32_c5_devkitc_1>`
     - ESP32-C5-WROOM-1, Wi-Fi 6, 802.15.4, USB
     - Board for ESP32-C5 wireless evaluation.
   * - :doc:`ESP32-C6-DevKitC-1 <esp32_c6_devkitc_1>`
     - ESP32-C6-WROOM-1, Wi-Fi 6, 802.15.4, USB
     - Board for ESP32-C6 wireless and IoT applications.
   * - :doc:`ESP32-C6-DevKitM-1 <esp32_c6_devkitm_1>`
     - ESP32-C6-MINI-1, Wi-Fi 6, 802.15.4, USB
     - Development board based on ESP32-C6-MINI-1.
   * - :doc:`ESP32-DevKitC <esp32_devkitc>`
     - ESP32-WROOM-32, USB-UART, GPIO headers
     - General-purpose ESP32-WROOM-32 development board.
   * - :doc:`ESP32-DevKitM-1 <esp32_devkitm_1>`
     - ESP32-WROOM-32, USB-UART, GPIO headers
     - General-purpose board based on ESP32-WROOM-32.
   * - :doc:`ESP32-Ethernet-Kit <esp32_ethernet_kit>`
     - Ethernet PHY, RJ45, microSD, USB-UART
     - ESP32 board for Ethernet and network application evaluation.
   * - :doc:`ESP32-H2-DevKitM-1 <esp32_h2_devkitm_1>`
     - ESP32-H2-MINI-1, 802.15.4, USB
     - Low-power board for Thread and Zigbee applications.
   * - :doc:`ESP32-LCDKit <esp32_lcdkit>`
     - SPI LCD, touch, knob, audio, infrared, WS2812
     - ESP32 board for compact GUI and HMI applications.
   * - :doc:`ESP32-MeshKit-Sense <esp32_meshkit_sense>`
     - Environmental sensors, LCD, touch, Wi-Fi/BLE, GPIO boot button
     - Development kit for Wi-Fi Mesh and sensing applications.
   * - :doc:`ESP32-P4X-C5-Function-EV-Board <esp32_p4x_c5_function_ev>`
     - ESP32-P4 and ESP32-C5, display, touch, audio, camera, GPIO boot button
     - Board for multi-chip multimedia application evaluation.
   * - :doc:`ESP32-PICO-DevKitM-2 <esp32_pico_devkitm_2>`
     - ESP32-PICO-MINI-02, USB-UART, GPIO headers
     - Compact board based on ESP32-PICO modules.
   * - :doc:`ESP32-PICO-KIT <esp32_pico_kit>`
     - ESP32-PICO-D4, USB-UART, GPIO headers
     - Development board based on ESP32-PICO-D4.
   * - :doc:`ESP32-PICO-KIT-1 <esp32_pico_kit_1>`
     - ESP32-PICO-D4, USB-UART, GPIO headers
     - Hardware revision of ESP32-PICO-KIT.
   * - :doc:`ESP32-S3-DevKitC-1 <esp32_s3_devkitc_1>`
     - ESP32-S3-WROOM-1, USB, GPIO headers
     - Board for ESP32-S3 application development.
   * - :doc:`ESP32-S3-DevKitM-1 <esp32_s3_devkitm_1>`
     - ESP32-S3-MINI-1, USB, GPIO headers
     - General-purpose board based on ESP32-S3-MINI-1.
   * - :doc:`ESP32-S3-USB-OTG <esp32_s3_usb_otg>`
     - USB OTG, USB Type-C, microSD, buttons
     - ESP32-S3 board for USB host and device development.
   * - :doc:`ESP32-Sense-Kit <esp32_sense_kit>`
     - Touch sensing, microphone, speaker, LCD
     - Kit for ESP32 touch and audio evaluation.
   * - :doc:`ESP32-Vaquita-DSPG <esp32_vaquita_dspg>`
     - ESP32-WROVER-E, DBMD5P, ES8311, microphones, speaker, GPIO boot button
     - ESP32 board for Alexa voice and AWS IoT applications.
   * - :doc:`ESP-DualKey <esp_dualkey>`
     - Dual-key input, USB, GPIO expansion
     - ESP32 board for button and HMI validation.
   * - :doc:`ESP-Mosaico V1.0 <esp_mosaico>`
     - 480x480 QSPI touch display, ES8311, BMI270, BMM150, SPI NAND, USB OTG, GPIO boot button
     - Expandable ESP32-S31 smart-interaction development kit.
   * - :doc:`ESP Thread Border Router <esp_thread_border_router>`
     - ESP32-S3 host, ESP32-H2 RCP, Wi-Fi, Ethernet expansion, GPIO boot button
     - Hardware platform for Thread and Zigbee border-router development.

.. toctree::
   :maxdepth: 1
   :hidden:
   :titlesonly:

   esp32_c3_lcdkit
   esp32_c3_lyra
   esp32_lyrat_4_3
   esp32_lyrat_mini_1_1
   esp32_p4_eye
   esp32_p4_function_ev_board
   esp32_s31_function_coreboard_1
   esp32_s31_korvo_1
   esp32_s3_box_3
   esp32_s3_box_lite
   esp32_s3_eye
   esp32_s3_korvo_1
   esp32_s3_korvo_2_3
   esp32_s3_lcd_ev_board
   esp_sensairshuttle
   esp_vocat_1_0
   esp_vocat_1_2
   esp32_c3_aws_expresslink_devkit
   esp32_c3_devkit_rust_1
   esp32_c3_devkit_rust_2
   esp32_c3_devkitc_02
   esp32_c3_devkitm_1
   esp32_c5_devkitc_1
   esp32_c6_devkitc_1
   esp32_c6_devkitm_1
   esp32_devkitc
   esp32_devkitm_1
   esp32_ethernet_kit
   esp32_h2_devkitm_1
   esp32_lcdkit
   esp32_meshkit_sense
   esp32_p4x_c5_function_ev
   esp32_pico_devkitm_2
   esp32_pico_kit
   esp32_pico_kit_1
   esp32_s3_devkitc_1
   esp32_s3_devkitm_1
   esp32_s3_usb_otg
   esp32_sense_kit
   esp32_vaquita_dspg
   esp_dualkey
   esp_mosaico
   esp_thread_border_router

Partner and Community Boards (``espressif/esp_friends_boards``)
---------------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 35 65 65

   * - Name
     - Onboard capabilities
     - Introduction
   * - :doc:`ESP-HI <esp_hi>`
     - Internal ADC, PDM speaker, ILI9341 LCD, GPIO boot button
     - ESP32-C3 AI voice-interaction module.
   * - :doc:`ESP32-C5-Spot <esp32_c5_spot>`
     - Dual ES8311 audio, amplifier and power control
     - ESP32-C5 voice-interaction module.
   * - :doc:`ESP32-S3-BOX-2 <esp32_s3_box_2>`
     - ES8389, ES7210, ST7789 LCD, SPI, GPIO buttons
     - ESP32-S3 voice-interaction board.
   * - :doc:`ESP32-S3-Korvo-2L <esp32_s3_korvo_2l>`
     - ES8311, microSD, ADC buttons
     - ESP32-S3 audio board.
   * - :doc:`ESP-Ditto <esp_Ditto>`
     - BMGR: ILI9341 parallel LCD, CST816S touch, SPI camera, audio input/output; onboard BMI270, BMM150, SI12T, and BQ27220 are not yet adapted
     - ESP32-C5 multi-sensor interaction board.
   * - :doc:`ESP-WROVER-KIT <esp_wrover_kit>`
     - ST7789 LCD, microSD, buttons, blue LED, SPIFFS
     - General-purpose ESP32 board.
   * - :doc:`ESP32-S3 SparkBot <esp_sparkbot>`
     - ST7789 LCD, DVP camera, audio input/output, BMI270, status LED
     - ESP32-S3 robotic interaction board.

.. toctree::
   :maxdepth: 1
   :hidden:
   :titlesonly:

   esp_hi
   esp32_c5_spot
   esp32_s3_box_2
   esp32_s3_korvo_2l
   esp_Ditto
   esp_wrover_kit
   esp_sparkbot

M5Stack Boards (``espressif/m5stack_boards``)
----------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 35 65 65

   * - Name
     - Onboard capabilities
     - Introduction
   * - `M5Stack AtomS3 <https://docs.m5stack.com/en/core/AtomS3>`__
     - GC9A01 LCD, backlight, GPIO button, SPIFFS
     - Compact ESP32-S3 display board.
   * - `M5Stack AtomS3R <https://docs.m5stack.com/en/core/AtomS3R>`__
     - ST7735 LCD, GPIO button, SPIFFS
     - Compact ESP32-S3 display board.
   * - `M5Dial <https://docs.m5stack.com/en/core/M5Dial>`__
     - GC9A01 LCD, FT5x06 touch, knob, button, SPIFFS
     - ESP32-S3 rotary-interaction board.
   * - `ATOM Echo <https://docs.m5stack.com/en/atom/atomecho>`__
     - RGB LED, user button
     - ESP32 smart-speaker module.
   * - `ATOM Lite <https://docs.m5stack.com/en/core/ATOM%20Lite>`__
     - RGB LED, user button, infrared output
     - Compact ESP32 development board.
   * - `ATOM Matrix <https://docs.m5stack.com/en/core/ATOM%20Matrix>`__
     - RGB LED matrix, user button, infrared output
     - Compact ESP32 development board.
   * - `AtomS3 Lite <https://docs.m5stack.com/en/core/AtomS3%20Lite>`__
     - RGB LED, user button, infrared output
     - Compact ESP32-S3 development board.
   * - `AtomS3U <https://docs.m5stack.com/en/core/AtomS3U>`__
     - RGB LED, user button, infrared output
     - ESP32-S3 development board with USB-A.
   * - `ATOM U <https://docs.m5stack.com/en/core/ATOM%20U>`__
     - RGB LED, user button, infrared output
     - Compact ESP32 development board with USB-A.
   * - `M5Capsule <https://docs.m5stack.com/en/core/M5Capsule>`__
     - Power hold, RGB LED, SPI microSD, infrared output, wake/boot buttons
     - ESP32-S3 compact controller.
   * - `Cardputer <https://docs.m5stack.com/en/core/Cardputer>`__
     - ST7789 LCD, SPI microSD, RGB LED, infrared output, boot button
     - ESP32-S3 handheld computer.
   * - `Cardputer Adv <https://docs.m5stack.com/en/core/Cardputer-Adv>`__
     - ST7789 LCD, SPI microSD, audio input/output, RGB LED, infrared output, boot button
     - ESP32-S3 handheld computer.
   * - `M5Stack Basic <https://docs.m5stack.com/en/core/basic_v2.7>`__
     - ILI9341 LCD, speaker control, three buttons, SPI microSD, SPIFFS
     - ESP32 multimedia core.
   * - `M5Stack Core2 <https://docs.m5stack.com/en/core/Core2%20v1.1>`__
     - ILI9341 LCD, FT5x06 touch, audio output, SPI microSD, SPIFFS
     - ESP32 multimedia core with touch and power management.
   * - `CoreInk <https://docs.m5stack.com/en/core/coreink>`__
     - Power hold, status LED, user button, dial buttons
     - ESP32 e-ink controller.
   * - `CoreS3 <https://docs.m5stack.com/en/core/CoreS3>`__
     - Audio input/output, ILI9342C LCD, FT5x06 touch, GC0308 camera
     - ESP32-S3 multimedia core.
   * - `DinMeter <https://docs.m5stack.com/en/core/M5DinMeter>`__
     - ST7789 LCD, backlight, power hold, wake button
     - ESP32-S3 DIN-rail controller.
   * - `Fire v2.7 <https://docs.m5stack.com/en/core/fire_v2.7>`__
     - ILI9342C LCD, SPI microSD, RGB LED strip, three buttons
     - ESP32 multimedia core.
   * - `M5NanoC6 <https://docs.m5stack.com/en/core/M5NanoC6>`__
     - RGB LED, blue LED, infrared output, user button
     - Compact ESP32-C6 development board.
   * - `M5NanoH2 <https://docs.m5stack.com/en/core/NanoH2>`__
     - RGB LED, blue LED, infrared output, user button
     - Compact ESP32-H2 development board.
   * - `M5Paper v1.1 <https://docs.m5stack.com/en/core/m5paper_v1.1>`__
     - GT911 touch, SPI microSD, power controls, wheel buttons
     - ESP32 e-ink development board.
   * - `M5PaperS3 <https://docs.m5stack.com/en/core/PaperS3>`__
     - GT911 touch, SPI microSD, e-ink power control
     - ESP32-S3 e-ink development board.
   * - `M5Stamp C3 <https://docs.m5stack.com/en/core/stamp_c3>`__
     - RGB LED, user button
     - Compact ESP32-C3 module board.
   * - `M5Stamp C3U <https://docs.m5stack.com/en/core/stamp_c3u>`__
     - RGB LED, user button
     - ESP32-C3 module board with native USB.
   * - `M5Stamp Pico <https://docs.m5stack.com/en/core/stamp_pico>`__
     - RGB LED, user button
     - Compact ESP32 module board.
   * - `M5Stamp S3 <https://docs.m5stack.com/en/core/StampS3>`__
     - RGB LED, boot button
     - Compact ESP32-S3 module board.
   * - `M5Station-485 <https://docs.m5stack.com/en/core/station_485>`__
     - ST7789 LCD, RGB LED strip, three buttons
     - ESP32 industrial RS-485 controller.
   * - `M5StickC <https://docs.m5stack.com/en/core/m5stickc>`__
     - Status LED, infrared output, two buttons
     - Compact ESP32 wearable controller.
   * - `M5StickC PLUS <https://docs.m5stack.com/en/core/m5stickc_plus>`__
     - ST7789 LCD, status LED, infrared output, two buttons
     - ESP32 wearable controller.
   * - `M5StickC PLUS2 <https://docs.m5stack.com/en/core/M5StickC%20PLUS2>`__
     - ST7789 LCD, backlight, status LED, power hold, three buttons
     - ESP32 wearable controller.
   * - `M5StickS3 <https://docs.m5stack.com/en/core/StickS3>`__
     - Audio input/output, ST7789 LCD, backlight, infrared output, two buttons
     - ESP32-S3 wearable controller.
   * - `M5Stack Tab5 <https://docs.m5stack.com/en/core/Tab5>`__
     - Audio input/output, MIPI DSI LCD, touch, camera, SDMMC
     - ESP32-P4 tablet-style board.
   * - `M5Stack Tough <https://docs.m5stack.com/en/core/tough>`__
     - ILI9342C touch LCD, SPI microSD
     - Rugged ESP32 controller.
