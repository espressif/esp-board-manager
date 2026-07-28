迁移到 0.7.0
============

:link_to_translation:`en:[English]`

0.7.0 将 ``audio_codec`` 设备迁移到 ``espressif/esp_codec_dev`` 2.0。新增板级 YAML 应使用 codec-dev 2.0 初始化配置结构。

组件依赖
--------

启用音频编解码器支持时，``esp_board_manager`` 依赖 ``espressif/esp_codec_dev`` ``^2.0.0-beta1``。该约束允许兼容的 ``2.x`` 版本，实际解析到的版本记录在 ``dependencies.lock`` 中。

本次迁移支持 ESP-IDF release/v5.4 ``>=5.4.4`` 和 release/v5.5 ``>=5.5.3``。

YAML 迁移
---------

新的 codec 初始化字段位于 ``config`` 下：

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

``adc_cfg.label`` 按 codec-dev 2.0 从 LSB 到 MSB 的通道顺序配置。PA 和复位 GPIO 不属于 ``config`` 配置组，应在 ``peripherals`` 中引用 GPIO 外设，并使用 ``pa_active_level`` 或 ``reset_active_level``。

旧字段兼容性
------------

已有板级 YAML 仍可解析，但会输出警告：

- ``mclk_enabled`` 映射为反向的 ``sys_cfg.no_mclk``。
- ``adc_channel_labels`` 会原样复制，不会调整顺序。替换为 ``adc_cfg.label`` 前需检查旧字段的 MSB 到 LSB 顺序。
- ``adc_channel_mask``、``dac_channel_mask``、``adc_max_channel``、``dac_max_channel``、``adc_init_gain``、``dac_init_gain``、``aec``、``eq`` 和 ``alc`` 不参与 codec 初始化。
- 使用 ``active_level`` 的 GPIO 引用按旧 PA 控制处理，应改为 ``pa_active_level``。

运行时设置
----------

通道掩码和流格式相关设置应通过 ``esp_codec_dev_open()`` 的 sample information 配置。设备打开后通过 ``esp_codec_dev_set_in_gain()`` 和 ``esp_codec_dev_set_out_vol()`` 设置输入增益和输出音量。AEC、EQ 和 ALC 需要芯片专用的运行时处理，不属于 ``audio_codec`` 初始化范围。

验证
----

更新板级 YAML 后执行 ``idf.py bmgr -b <board>`` 重新生成代码，再进行完整构建，并在目标开发板验证播放或录音功能。
