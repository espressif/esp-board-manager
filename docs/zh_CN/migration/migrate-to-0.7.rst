迁移到 0.7.1
============

:link_to_translation:`en:[English]`

0.7.1 包含 0.7.0 中 ``audio_codec`` 迁移到 ``espressif/esp_codec_dev`` 2.0 的变更，并引入语义化外设绑定和 ``display_lcd`` 帧格式。更新板级 YAML 后应重新生成代码并完成目标开发板验证。

组件依赖
--------

启用音频编解码器支持时，``esp_board_manager`` 依赖 ``espressif/esp_codec_dev`` ``^2.0.0-beta1``。该约束允许兼容的 ``2.x`` 版本，实际解析到的版本记录在 ``dependencies.lock`` 中。

``dev_audio_codec`` YAML 迁移
----------------------------------

新的 ``audio_codec`` 配置模板如下：

.. code-block:: yaml

   config:
     adc_enabled: true
     dac_enabled: false
     sys_cfg:
       is_master: false
       no_mclk: true
     adc_cfg:
       digital_mic: false
       label: [FL, FR, RE, NA]
     dac_cfg:
       ref_enable: false
       ref_dac_ch: 0
       real_adc_data_ch: 0
   peripherals:
     - pa_name: gpio_power_amp
       gain: 0.0
       active_level: 1
     - reset_name: gpio_codec_reset
       active_level: 0
     - i2s_name: i2s_audio_out
       clk_src: 0
       tx_aux_out_io: -1
       tx_aux_out_line: 0
       tx_aux_out_invert: false
     - i2c_name: i2c_master
       address: 0x30
       frequency: 100000

``adc_cfg.label`` 按 codec-dev 2.0 从 LSB 到 MSB 的通道顺序配置。PA 和复位 GPIO 在 ``peripherals`` 中通过 ``pa_name`` 或 ``reset_name`` 引用 GPIO 外设，并在同一条目中设置 ``active_level``。

旧字段兼容性
------------

已有板级 YAML 仍可解析，但会输出警告：

- ``mclk_enabled`` 映射为反向的 ``sys_cfg.no_mclk``。
- ``adc_channel_labels`` 会原样复制，不会调整顺序。替换为 ``adc_cfg.label`` 前需检查旧字段的 MSB 到 LSB 顺序。
- ``adc_channel_mask``、``dac_channel_mask``、``adc_max_channel``、``dac_max_channel``、``adc_init_gain``、``dac_init_gain``、``aec``、``eq`` 和 ``alc`` 不参与 codec 初始化。

语义化外设绑定
--------------

设备应使用与其功能角色对应的 ``*_name`` 字段引用外设，不再依赖外设名称前缀或列表顺序。例如，SPI LCD 的外设绑定应从只含 ``name`` 的映射改为 ``spi_name``：

.. code-block:: yaml

   # 旧写法
   devices:
     - name: display_lcd
       type: display_lcd
       sub_type: spi
       peripherals:
         - name: spi_lcd

   # 0.7.1 写法
   devices:
     - name: display_lcd
       type: display_lcd
       sub_type: spi
       peripherals:
         - spi_name: spi_lcd

选择器由设备类型和子类型决定。常用选择器包括 ``i2c_name``、``spi_name``、``gpio_name``、``pa_name``、``reset_name``、``dsi_name`` 和 ``ldo_name``。每个外设引用条目只能包含一个角色选择器，且引用的外设类型必须与该角色匹配。

受本次迁移影响的设备应将原来的字符串或 ``- name: <外设名>`` 改为下表中的选择器。

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - 设备类型
     - 迁移前：``peripherals`` 条目
     - 迁移后：``peripherals`` 条目
   * - ``audio_codec``
     - ``name: gpio_power_amp`` + ``pa_active_level: 1``；``name: gpio_codec_reset``；``name: i2s_audio_out``；``name: i2c_master``；``name: adc_audio_in``
     - ``pa_name: gpio_power_amp`` + ``active_level: 1``；``reset_name: gpio_codec_reset``；``i2s_name: i2s_audio_out``；``i2c_name: i2c_master``；``adc_name: adc_audio_in``
   * - ``button``
     - ``sub_type: gpio``：``name: gpio_button``；``sub_type: adc_single`` / ``adc_multi``：``name: adc_button``
     - ``sub_type: gpio``：``gpio_name: gpio_button``；``sub_type: adc_single`` / ``adc_multi``：``adc_name: adc_button``
   * - ``camera``
     - ``sub_type: dvp`` / ``spi``：``name: i2c_master``；``sub_type: csi``：``name: i2c_master`` + ``name: ldo_mipi``
     - ``sub_type: dvp`` / ``spi``：``i2c_name: i2c_master``；``sub_type: csi``：``i2c_name: i2c_master`` + ``ldo_name: ldo_mipi``
   * - ``display_lcd``
     - ``sub_type: spi``：``name: spi_display``；``sub_type: dsi``：``name: dsi_display`` + ``name: ldo_mipi``
     - ``sub_type: spi``：``spi_name: spi_display``；``sub_type: dsi``：``dsi_name: dsi_display`` + ``ldo_name: ldo_mipi``
   * - ``lcd_touch``
     - ``name: i2c_master``
     - ``i2c_name: i2c_master``
   * - ``gpio_expander``
     - ``name: i2c_master``
     - ``i2c_name: i2c_master``
   * - ``gpio_ctrl``
     - ``name: gpio``
     - ``gpio_name: gpio``
   * - ``power_ctrl``
     - ``sub_type: gpio``：``name: gpio_power``；``sub_type: custom``：``name: i2c_pmic``
     - ``sub_type: gpio``：``gpio_name: gpio_power``；``sub_type: custom``：保持 ``name: i2c_pmic``
   * - ``ledc_ctrl``
     - ``name: ledc_backlight``
     - ``ledc_name: ledc_backlight``
   * - ``fs_fat``
     - ``sub_type: spi``：``name: spi_master``
     - ``sub_type: spi``：``spi_name: spi_master``
   * - ``littlefs``
     - ``sub_type: spi``：``name: spi_master``
     - ``sub_type: spi``：``spi_name: spi_master``

``power_ctrl: custom`` 的 ``peripherals`` 是框架在自定义初始化前引用的通用外设，不使用角色选择器，保留 ``name`` 写法。

在 0.7.0 中，PA 引用使用 ``name`` 指定外设，并通过 ``pa_active_level`` 配置有效电平。0.7.1 将 PA 绑定改为 ``pa_name``，并将有效电平字段统一为 ``active_level``；复位 GPIO 同样使用 ``active_level``。

字符串引用和只含 ``name`` 的映射仍可解析，但 BMGR 会根据设备上下文推断角色并输出兼容性告警。未知选择器、同一角色的重复引用、同时使用 ``name`` 与 ``*_name``，以及外设类型不匹配都会在解析阶段报错。完整规则见 :doc:`/programming-guide/yaml-rules`。

LCD 帧格式
----------

``display_lcd`` 设备配置现在会生成 ``frame_format``，应用应使用该字段选择像素缓冲区、LVGL 字节交换或图像转换输出，不应继续依赖具体 panel 驱动的字节序假设。

自动推导成功时，``frame_format`` 为 ``RGB565_LE``、``RGB565_BE``、``BGR888`` 或 ``RGB888``；无法确定或不支持的组合会生成 ``UNKNOWN``。SPI 和 I80 模式会依据 ``data_endian`` 推导，DSI 和 RGB 模式会依据像素或颜色格式推导。对于无法自动确定的 PARLIO 配置，建议显式设置 ``config.frame_format``：

.. code-block:: yaml

   config:
     frame_format: RGB565_LE

显式值只能使用 ``RGB565_LE``、``RGB565_BE``、``BGR888`` 或 ``RGB888``。当显式值与自动推导结果不一致时，BMGR 会输出告警并使用显式值。详细推导规则见 :doc:`/references/devices/display-lcd`。

运行时设置
----------

通道掩码和流格式相关设置应通过 ``esp_codec_dev_open()`` 的 sample information 配置。设备打开后通过 ``esp_codec_dev_set_in_gain()`` 和 ``esp_codec_dev_set_out_vol()`` 设置输入增益和输出音量。AEC、EQ 和 ALC 需要芯片专用的运行时处理，不属于 ``audio_codec`` 初始化范围。

验证
----

更新板级 YAML 后执行 ``idf.py bmgr -b <board>`` 重新生成代码，再进行完整构建，并在目标开发板验证播放、录音或显示功能。
