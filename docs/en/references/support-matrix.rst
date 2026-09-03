Support Matrix
==============

:link_to_translation:`zh_CN:[中文]`

The support matrix provides a quick overview of the device types and peripheral types currently built into BMGR. For detailed configuration instructions, see the Device Reference and Peripheral Reference. For individual board details, see :doc:`Boards Catalog </references/boards/index>`.

Supported Device Types
-----------------------

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - type
     - Description
   * - ``audio_codec``
     - Audio codec, supporting playback, recording, full-duplex, and other modes.
   * - ``button``
     - Button, supporting GPIO single button, ADC single button, ADC multi-button, and custom driver sub-types.
   * - ``camera``
     - Camera, supporting DVP / CSI / SPI.
   * - ``custom``
     - Custom device requiring board-level init/deinit implementation.
   * - ``display_lcd``
     - LCD display, supporting SPI / I80 / DSI / RGB / RGB + 3-wire SPI / ParlIO.
   * - ``fs_fat``
     - FAT file system, supporting SDMMC / SPI.
   * - ``fs_spiffs``
     - SPIFFS partition file system.
   * - ``littlefs``
     - LittleFS file system, supporting flash / SDMMC / SPI.
   * - ``gpio_ctrl``
     - GPIO control device.
   * - ``gpio_expander``
     - IO expander chip.
   * - ``lcd_touch``
     - Touchscreen controller (currently supporting I2C; SPI reserved).
   * - ``led_strip``
     - LED strip, supporting SPI / RMT.
   * - ``ledc_ctrl``
     - LEDC PWM control.
   * - ``power_ctrl``
     - Power control, can be referenced by other devices via ``power_ctrl_device``.

Supported Peripheral Types
---------------------------

``adc``, ``anacmpr``, ``dac``, ``dsi``, ``gpio``, ``i2c``, ``i2s``, ``ldo``, ``ledc``, ``mcpwm``, ``pcnt``, ``rmt``, ``sdm``, ``spi``, ``uart``.
