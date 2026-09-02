支持矩阵
============

:link_to_translation:`en:[English]`

支持矩阵用于快速判断 BMGR 当前内置的 device type 与 peripheral type。具体配置方法见"设备参考"与"外设参考"，单块开发板的详情见 :doc:`板卡目录 </references/boards/index>`。

支持的 device type
------------------------

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - type
     - 说明
   * - ``audio_codec``
     - 音频编解码，含播放、录音、全双工等模式。
   * - ``button``
     - 按键，支持 GPIO / ADC 单按键 / ADC 多按键 / 自定义驱动子类型。
   * - ``camera``
     - 摄像头，支持 DVP / CSI / SPI。
   * - ``custom``
     - 自定义设备，需板级实现 init/deinit。
   * - ``display_lcd``
     - LCD 显示，支持 SPI / I80 / DSI / RGB / RGB + 3-wire SPI / ParlIO。
   * - ``fs_fat``
     - FAT 文件系统，支持 SDMMC / SPI。
   * - ``fs_spiffs``
     - SPIFFS 分区文件系统。
   * - ``littlefs``
     - LittleFS 文件系统，支持 flash / SDMMC / SPI。
   * - ``gpio_ctrl``
     - GPIO 控制型设备。
   * - ``gpio_expander``
     - IO 扩展芯片。
   * - ``lcd_touch``
     - 触摸屏控制器（当前支持 I2C，SPI 预留）。
   * - ``led_strip``
     - LED 灯带，支持 SPI / RMT。
   * - ``ledc_ctrl``
     - LEDC PWM 控制。
   * - ``power_ctrl``
     - 电源控制，可被其他设备通过 ``power_ctrl_device`` 引用。

支持的 peripheral type
------------------------

``adc``、``anacmpr``、``dac``、``dsi``、``gpio``、``i2c``、``i2s``、``ldo``、``ledc``、``mcpwm``、``pcnt``、``rmt``、``sdm``、``spi``、``uart``。
