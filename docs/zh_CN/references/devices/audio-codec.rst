音频编解码器 audio_codec
========================

:link_to_translation:`en:[English]`

``audio_codec`` 设备用于描述音频播放、录音和内部 ADC 音频输入路径。当外接编解码器（codec）芯片用于播放或采集时，外设侧至少需要一路 ``i2c`` 控制接口与一路 ``i2s`` 数据接口；功放 GPIO 为可选。

同一逻辑 ``audio_codec`` 设备不可同时启用 ``adc_enabled`` 与 ``dac_enabled``。当一颗物理编解码器芯片需要全双工时，在 ``board_devices.yaml`` 中拆为两个单向设备，例如 ``audio_dac`` 与 ``audio_adc``。

支持的使用模式
---------------------

``dev_audio_codec`` 不通过 ``sub_type`` 区分不同使用模式，而是通过所用外设与配置项适配不同使用场景：

- :ref:`播放（DAC，dac_enabled: true） <audio-codec-dac>`
- :ref:`录音（ADC，adc_enabled: true） <audio-codec-adc>`
- :ref:`全双工（同一颗编解码器芯片） <audio-codec-full-duplex>`
- :ref:`无外部 Codec 的录音和播放 <audio-codec-internal>`
- :ref:`PDM 数字麦克风 <audio-codec-pdm>`
- :ref:`PDM 扬声器 <audio-codec-pdm-dac>`

最小配置
------------

.. _audio-codec-dac:

播放（DAC，``dac_enabled: true``）
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

播放模式用于外接 codec 的 DAC 输出。``board_peripherals.yaml`` 至少配置 ``i2c`` 与 ``i2s`` 输出外设；当开发板带有功放控制脚时，额外配置 ``gpio`` 外设。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: i2c_master
        type: i2c
        role: master
        config:
          port: 0
          pins:
            sda: 17                     # [IO] SDA pin
            scl: 18                     # [IO] SCL pin

      - name: i2s_audio_out
        type: i2s
        role: master
        format: std-out
        config:
          port: 0
          sample_rate_hz: 16000         # [TO_BE_CONFIRMED] Sample rate in Hz
          data_bit_width: 16            # [TO_BE_CONFIRMED] Data bit width
          slot_mode: I2S_SLOT_MODE_STEREO
          pins:
            bclk: 9                     # [IO] BCLK pin
            ws: 45                      # [IO] WS pin
            dout: 8                     # [IO] Data output pin

      - name: gpio_pa_control
        type: gpio
        role: io
        config:
          pin: 46                       # [IO] GPIO pin number
          mode: GPIO_MODE_OUTPUT

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: audio_dac
        chip: es8311                    # [TO_BE_CONFIRMED] Codec chip type
        type: audio_codec
        config:
          adc_enabled: false
          dac_enabled: true
        peripherals:
          - name: gpio_pa_control
            pa_active_level: 1
          - name: i2s_audio_out
          - name: i2c_master
            address: 0x30               # [TO_BE_CONFIRMED] I2C device address
            frequency: 400000

若开发板无 PA 控制引脚，则不配置 ``gpio_pa_control``，设备引用列表中也不保留该外设。

.. _audio-codec-adc:

录音（ADC，``adc_enabled: true``）
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

录音模式用于外接 codec 的 ADC 输入。``board_peripherals.yaml`` 可与播放共用同一个 ``i2c_master``；I2S 录音建议使用独立的 ``std-in`` 外设名。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: i2c_master
        type: i2c
        role: master
        config:
          port: 0
          pins:
            sda: 17                     # [IO] SDA pin
            scl: 18                     # [IO] SCL pin

      - name: i2s_audio_in
        type: i2s
        role: master
        format: std-in
        config:
          port: 0
          sample_rate_hz: 48000         # [TO_BE_CONFIRMED] Sample rate in Hz
          data_bit_width: 16            # [TO_BE_CONFIRMED] Data bit width
          slot_mode: I2S_SLOT_MODE_STEREO
          pins:
            bclk: 9                     # [IO] BCLK pin
            ws: 45                      # [IO] WS pin
            din: 10                     # [IO] Data input pin

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: audio_adc
        chip: es8311                    # [TO_BE_CONFIRMED] Codec chip type
        type: audio_codec
        config:
          adc_enabled: true
          dac_enabled: false
        peripherals:
          - name: i2s_audio_in
          - name: i2c_master
            address: 0x30               # [TO_BE_CONFIRMED] I2C device address
            frequency: 400000

.. _audio-codec-full-duplex:

全双工（同一颗编解码器芯片）
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

全双工模式用于一颗物理 codec 同时承担播放与录音。BMGR 中仍配置两个 ``audio_codec`` 逻辑设备，分别只启用 DAC 或 ADC；I2C 控制接口可指向同一个 ``i2c_master``。

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: audio_dac
        chip: es8311
        type: audio_codec
        config:
          adc_enabled: false
          dac_enabled: true
        peripherals:
          - name: i2s_audio_out
          - &es8311_i2c_master
            name: i2c_master
            address: 0x30
            frequency: 400000

      - name: audio_adc
        chip: es8311
        type: audio_codec
        config:
          adc_enabled: true
          dac_enabled: false
        peripherals:
          - name: i2s_audio_in
          - *es8311_i2c_master

.. _audio-codec-internal:

无外部 Codec 的录音和播放
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

将 ``chip`` 设为 ``internal`` 时，``audio_codec`` 不通过 ``esp_codec_dev`` 驱动初始化外部编解码器芯片，适用于以下场景：

- 无外部编解码器芯片，音频信号直接送入 SoC 内部 ADC 或经 I2S 接口传输。
- 存在编解码器芯片，但无需经 I2C 软件配置即可工作，音频数据直接经 I2S 接口读写。

关键在于将 ``chip`` 设为 ``internal``，再根据实际硬件接口配置对应的 I2S 或 ADC 外设。

内部 ADC 路径：当板级已定义 ``adc`` 外设时，可在设备 ``peripherals`` 中引用该外设；不需要复用外设时，可通过 ``config.adc_local_cfg`` 让 ``audio_codec`` 创建设备本地的 ADC continuous 配置。

复用 ``adc`` 外设：

.. code-block:: yaml

    devices:
      - name: audio_adc_mic_reuse
        chip: internal
        type: audio_codec
        config:
          adc_enabled: true
          dac_enabled: false
        peripherals:
          - name: adc_audio_in

通过 ``audio_codec`` 创建 ADC continuous 句柄：

.. code-block:: yaml

    devices:
      - name: audio_adc_mic
        chip: internal
        type: audio_codec
        config:
          adc_enabled: true
          dac_enabled: false
          adc_local_cfg:
            sample_rate_hz: 16000
            max_store_buf_size: 1024
            conv_frame_size: 256
            format: ADC_DIGI_OUTPUT_FORMAT_TYPE2
            conv_mode: ADC_CONV_SINGLE_UNIT_1
            unit_id: ADC_UNIT_1        # [TO_BE_CONFIRMED] ADC unit
            channel_list: [4]          # [TO_BE_CONFIRMED] ADC channels
            atten: ADC_ATTEN_DB_0
            bit_width: SOC_ADC_DIGI_MAX_BITWIDTH

.. _audio-codec-pdm:

PDM 数字麦克风
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

PDM 数字麦克风是\ ``chip: internal``\ 路径中使用 I2S 的典型形态。外设侧将 ``format`` 设为 ``pdm-in``，仅需 ``clk`` 和 ``din`` 两个引脚，无 ``bclk`` 和 ``ws``。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: i2s_audio_in
        type: i2s
        role: master
        format: pdm-in
        config:
          port: 0
          sample_rate_hz: 16000             # [TO_BE_CONFIRMED] 采样率（Hz）
          data_bit_width: 16                # [TO_BE_CONFIRMED] 数据位宽
          slot_mode: I2S_SLOT_MODE_MONO     # [TO_BE_CONFIRMED] 单声道/立体声
          slot_mask: I2S_PDM_SLOT_LEFT      # [TO_BE_CONFIRMED] PDM slot 选择（LEFT / RIGHT / BOTH）
          dn_sample_mode: I2S_PDM_DSR_8S    # [TO_BE_CONFIRMED] 下采样模式
          hp_en: true                       # [TO_BE_CONFIRMED] 高通滤波使能
          hp_cut_off_freq_hz: 35.5          # [TO_BE_CONFIRMED] 高通截止频率（Hz）
          amplify_num: 1                    # [TO_BE_CONFIRMED] 增益倍数
          pins:
            clk: 22                         # [IO] PDM CLK pin
            din: 21                         # [IO] PDM DIN pin

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: audio_adc
        chip: internal
        type: audio_codec
        config:
          adc_enabled: true
          dac_enabled: false
        peripherals:
          - name: i2s_audio_in

若麦克风有独立供电控制，可在设备中添加 ``power_ctrl_device: mic_power_ctrl``，并在 ``board_devices.yaml`` 中同时定义对应的 ``power_ctrl`` 设备。板级参考：``boards/esp32_p4_eye/board_devices.yaml``。

.. _audio-codec-pdm-dac:

PDM 扬声器
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

I2S PDM 输出可直接驱动 PDM 扬声器或 PDM 功放，无需外部编解码器芯片。外设侧将 ``format`` 设为 ``pdm-out``，设备侧使用 ``chip: internal``。PDM TX 模式只需 ``dout`` 引脚，``clk`` 引脚视硬件而定（不使用时填 ``-1``）。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: i2s_audio_out
        type: i2s
        role: master
        format: pdm-out
        config:
          sample_rate_hz: 16000             # [TO_BE_CONFIRMED] 采样率（Hz）
          data_bit_width: 16                # [TO_BE_CONFIRMED] 数据位宽
          slot_mode: I2S_SLOT_MODE_MONO     # [TO_BE_CONFIRMED] 单声道/立体声
          slot_mask: I2S_PDM_SLOT_LEFT      # [TO_BE_CONFIRMED] PDM slot 选择（LEFT / RIGHT / BOTH）
          line_mode: I2S_PDM_TX_ONE_LINE_CODEC  # [TO_BE_CONFIRMED] 输出线模式
          up_sample_fp: 960                 # [TO_BE_CONFIRMED] 上采样参数 fp
          up_sample_fs: 441                 # [TO_BE_CONFIRMED] 上采样参数 fs
          sd_prescale: 0                    # [TO_BE_CONFIRMED]
          sd_scale: I2S_PDM_SIG_SCALING_MUL_4   # [TO_BE_CONFIRMED] Sigma-delta 缩放
          hp_scale: I2S_PDM_SIG_SCALING_MUL_4   # [TO_BE_CONFIRMED]
          lp_scale: I2S_PDM_SIG_SCALING_MUL_4   # [TO_BE_CONFIRMED]
          sinc_scale: I2S_PDM_SIG_SCALING_MUL_4 # [TO_BE_CONFIRMED]
          hp_en: false                      # [TO_BE_CONFIRMED] 高通滤波使能
          hp_cut_off_freq_hz: 35.5          # [TO_BE_CONFIRMED] 高通截止频率（Hz）
          sd_dither: 0                      # [TO_BE_CONFIRMED]
          sd_dither2: 1                     # [TO_BE_CONFIRMED]
          pins:
            clk: -1                         # [IO] PDM CLK pin（-1 表示不使用）
            dout: 3                         # [IO] PDM data output pin

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: audio_dac
        chip: internal
        type: audio_codec
        config:
          dac_enabled: true
          adc_enabled: false
        peripherals:
          - name: i2s_audio_out
          - name: gpio_pa_control           # 可选：PA 控制引脚
            gain: 6                         # [TO_BE_CONFIRMED] PA 增益（dB）
            pa_active_level: 1              # PA 控制引脚有效电平

若扬声器有 PA 控制引脚，需在 ``board_peripherals.yaml`` 中额外定义 ``gpio_pa_control`` 外设（``type: gpio``）。板级参考：``boards/esp32_c3_lyra/board_peripherals.yaml``、``boards/esp32_c3_lyra/board_devices.yaml``。

模式说明
------------

播放与录音是两个互斥的逻辑设备方向。播放设备仅启用 ``dac_enabled``，录音设备仅启用 ``adc_enabled``。同一物理 codec 需要全双工时，配置两个逻辑设备，并分别引用输出和输入 I2S 外设。

外接编解码器路径使用 I2S 传输音频数据、使用 I2C 配置编解码器芯片。将 ``chip`` 设为 ``internal`` 时不依赖 ``esp_codec_dev`` 驱动，适用于无外部编解码器芯片或编解码器无需软件配置的场景；音频数据直接经由内部 ADC 或 I2S 外设读写，I2S ``format`` 按实际接口选择。

完整字段
------------

外接编解码器芯片
^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Codec_dev 2.0 initialization configuration (single-direction logical device)
    - name: audio_codec          # The name of the device, must be unique
      chip: generic_codec        # [TO_BE_CONFIRMED] Codec chip type (es8311, es7210, etc.)
      type: audio_codec
      config:
        # A single logical dev_audio_codec instance must not enable both ADC and DAC at the same time.
        # Model a full-duplex physical codec as two devices, for example audio_adc and audio_dac.
        adc_enabled: false       # [TO_BE_CONFIRMED] Enable input device creation
        dac_enabled: false       # [TO_BE_CONFIRMED] Enable output device creation

        # Direct mapping to audio_codec_cfg_t.sys_cfg. Do not add a `codec:` level.
        sys_cfg:
          is_master: false       # Codec I2S role: false is slave, true is master
          no_mclk: true          # true disables external MCLK; false requires external MCLK

        # Direct mapping to audio_codec_cfg_t.adc_cfg.
        adc_cfg:
          digital_mic: false
          # ADC channel logical labels (supports the following only), comma separated string, MSB to LSB:
          # - FC: Front Center
          # - RE: Reference
          # - FL/FR: Front Left/Right
          # - SL/SR: Side Left/Right
          # - BL/BR: Back Left/Right
          # - NA: Not Available/Not Enable
          # Labels are in codec_dev 2.0 channel order, from LSB to MSB.
          label: []

        # Direct mapping to audio_codec_cfg_t.dac_cfg. Configure only when the codec supports it.
        dac_cfg:
          ref_enable: false
          ref_dac_ch: 0
          real_adc_data_ch: 0

        # adc_channel_mask/dac_channel_mask are esp_codec_dev_open() sample-info settings.
        # *_init_gain requires post-open set_in_gain()/set_out_vol(); AEC, EQ and ALC need a separate
        # runtime processing lifecycle. They are intentionally outside dev_audio_codec initialization.

        # Data interface will be auto-selected by parser:
        # 1) if peripherals includes adc_* -> ADC path (reuse handle)
        # 2) else if config.adc_local_cfg exists -> ADC path (local create)
        # 3) else -> I2S path (digital input/output without dedicated ADC peripheral)

      # Peripheral configuration
      peripherals:
        # PA GPIO dependency. It must reference a type: gpio peripheral from board_peripherals.yaml.
        - name: gpio_power_amp
          gain: 0.0
          pa_active_level: 1

        # Optional codec reset GPIO. reset_active_level is the asserted reset level.
        - name: gpio_codec_reset
          reset_active_level: 0

        # I2S interface configuration
        - name: i2s_audio_out                # [TO_BE_CONFIRMED] I2S peripheral for audio data interface
          clk_src: 0                         # I2S clock source, need converted from `i2s_clock_src_t`. If set to 0 will use default clock source (default: 0)
          tx_aux_out_io: -1                  # Optional mirrored/inverted I2S TX auxiliary output IO, -1 disables this feature
          tx_aux_out_line: 0                 # Optional TX data line index (0: main line, >0: extra TX data line when supported)
          tx_aux_out_invert: false           # Optional invert flag for the auxiliary output signal

        # I2C interface configuration
        - name: i2c_master                   # [TO_BE_CONFIRMED] I2C peripheral for codec control
          address: 0x30                      # [TO_BE_CONFIRMED] I2C device address, include the read/write bit (hex format) (default: 0x30)
          frequency: 100000                  # I2C clock frequency in Hz (default: 100000)

使用芯片内部 ADC 读取数据：复用 ADC 外设
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # ADC Mic Usage Mode 1: Reuse ADC peripheral handle from board_peripherals.yaml
    # Priority path: peripherals contains adc_* -> reuse periph_adc handle
    # This mode is intended for boards that already define an ADC peripheral with:
    # - type: adc
    # - role: continuous
    # The audio codec device will reuse the existing periph_adc handle directly,
    # instead of creating a local ADC continuous handle inside dev_audio_codec.
    - name: audio_adc_mic_reuse
      chip: internal                         # Internal ADC path, no external codec chip is used
      type: audio_codec                      # Audio codec device type, must remain audio_codec
      config:
        adc_enabled: true                    # Enable ADC functionality; required for ADC reuse path
        dac_enabled: false                   # Reuse path shown here is input-only; DAC should remain disabled
      peripherals:
        # ADC peripheral reference. The peripheral must be declared in board_peripherals.yaml
        # and configured as continuous mode (`role: continuous`).
        # Example:
        # - name: adc_audio_in
        #   type: adc
        #   role: continuous
        - name: adc_audio_in                 # ADC peripheral name to be reused by dev_audio_codec

使用芯片内部 ADC 读取数据：本地采样模式列表（patterns）配置
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # ADC Mic Usage Mode 2: Local ADC handle creation in dev_audio_codec
    # Priority path: no adc_* peripheral -> parse config.adc_local_cfg
    # Local cfg supports auto-detected modes:
    # - single_unit: channel_list + unit_id/atten/bit_width
    # - pattern: patterns[]
    # This mode is intended for simpler boards that do not want to declare a
    # reusable ADC peripheral in board_peripherals.yaml. dev_audio_codec will
    # convert the YAML below into an audio_codec_adc_continuous_cfg_t-equivalent
    # configuration and create the ADC data interface locally.
    - name: audio_adc_mic
      chip: internal                         # Internal ADC path, no external codec chip is used
      type: audio_codec                      # Audio codec device type, must remain audio_codec
      config:
        adc_enabled: true                    # Enable ADC functionality; required for local ADC path
        dac_enabled: false                   # Local ADC example is input-only; DAC should remain disabled
        adc_local_cfg:
          # The expected ADC sampling frequency in Hz. Please check soc_caps / ADC driver
          # limitations on your target before using high sample rates.
          sample_rate_hz: 16000

          # Max length of conversion results that the ADC driver can store, in bytes.
          max_store_buf_size: 1024

          # Conversion frame size in bytes. It should be aligned with the ADC DMA data format
          # used by your target; 256 is a commonly used safe default.
          conv_frame_size: 256

          # ADC DMA output format. (default: ADC_DIGI_OUTPUT_FORMAT_TYPE2)
          # Valid values:
          # - ADC_DIGI_OUTPUT_FORMAT_TYPE1
          # - ADC_DIGI_OUTPUT_FORMAT_TYPE2
          format: ADC_DIGI_OUTPUT_FORMAT_TYPE2

          # Conversion mode for DMA operation.
          # If omitted, parser auto-derives:
          # - single_unit + ADC_UNIT_1 => ADC_CONV_SINGLE_UNIT_1
          # - single_unit + ADC_UNIT_2 => ADC_CONV_SINGLE_UNIT_2
          # - pattern mixed unit       => ADC_CONV_BOTH_UNIT
          # Valid values:
          # - ADC_CONV_SINGLE_UNIT_1
          # - ADC_CONV_SINGLE_UNIT_2
          # - ADC_CONV_BOTH_UNIT
          # - ADC_CONV_ALTER_UNIT
          conv_mode: ADC_CONV_SINGLE_UNIT_1

          # ADC pattern configuration
          patterns:
            - unit: ADC_UNIT_1  # [TO_BE_CONFIRMED] ADC unit, optional values are ADC_UNIT_1 and ADC_UNIT_2
              channel: 4        # [TO_BE_CONFIRMED] ADC channel used by this audio input path
                                # Use adc_continuous_io_to_channel() / adc_continuous_channel_to_io() to map IO to channel.
              # ADC attenuation. Different parameters determine the input range. (default: ADC_ATTEN_DB_0)
              # Valid values:
              # - ADC_ATTEN_DB_0
              # - ADC_ATTEN_DB_2_5
              # - ADC_ATTEN_DB_6
              # - ADC_ATTEN_DB_12
              # - ADC_ATTEN_DB_11 (deprecated on some targets)
              atten: ADC_ATTEN_DB_0
              # ADC raw output bit-width.
              # Valid values:
              # - ADC_BITWIDTH_9
              # - ADC_BITWIDTH_10
              # - ADC_BITWIDTH_11
              # - ADC_BITWIDTH_12
              # - ADC_BITWIDTH_13
              # - ADC_BITWIDTH_DEFAULT
              # - SOC_ADC_DIGI_MAX_BITWIDTH
              bit_width: SOC_ADC_DIGI_MAX_BITWIDTH

使用芯片内部 ADC 读取数据：本地单 Unit 配置
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Single Unit configuration mode, simpler mode for single unit ADC path
    - name: audio_adc_mic
      chip: internal
      type: audio_codec
      config:
        adc_enabled: true
        dac_enabled: false
        adc_local_cfg:
          sample_rate_hz: 16000
          max_store_buf_size: 1024
          conv_frame_size: 256
          format: ADC_DIGI_OUTPUT_FORMAT_TYPE2
          conv_mode: ADC_CONV_SINGLE_UNIT_1

          # Below are the single unit configuration parameters, alternative to patterns[]
          unit_id: ADC_UNIT_1  # [TO_BE_CONFIRMED] ADC unit, optional values are ADC_UNIT_1 and ADC_UNIT_2
          channel_list: [4]    # [TO_BE_CONFIRMED] List of ADC channels used by this audio input path.
          atten: ADC_ATTEN_DB_0
          bit_width: SOC_ADC_DIGI_MAX_BITWIDTH

字段来源：

- YAML 模板：``esp_board_manager/devices/dev_audio_codec/dev_audio_codec.yaml``。
- 头文件：``esp_board_manager/devices/dev_audio_codec/dev_audio_codec.h``。

组件依赖
------------

``audio_codec`` 使用 ``esp_codec_dev`` 提供 codec 设备抽象。板级 YAML 中只需在 ``chip`` 字段指定芯片型号即可。

依赖外设
------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - 必选性
     - 用途
   * - ``i2c``
     - ``master``
     - 外接编解码器必选
     - 编解码器控制接口，设备侧引用条目填写 ``address`` 和 ``frequency``
   * - ``i2s``
     - ``master`` / ``std-out``、``std-in``、``tdm-out``、``tdm-in``
     - 外接编解码器必选
     - 音频数据接口
   * - ``gpio``
     - ``io``
     - 有 PA 控制或静音控制时使用
     - 设备侧引用条目填写 ``gain`` 和 ``pa_active_level``
   * - ``adc``
     - ``continuous``
     - 使用内部 ADC 音频输入并复用外设时使用
     - 提供 ADC continuous 句柄

参考代码
------------

- ``esp_board_manager/examples/play_embed_music/main/play_embed_music.c``
- ``esp_board_manager/examples/play_sdcard_music/main/play_sdcard_music.c``
- ``esp_board_manager/examples/record_to_sdcard/main/record_to_sdcard.c``
- ``esp_board_manager/examples/record_and_play/main/record_and_play.c``

板级参考
------------

- ``esp_friends_boards/esp32_s3_korvo_2l/board_devices.yaml``：同一 ES8311 芯片拆成 ``audio_dac`` 和 ``audio_adc`` 两个逻辑设备。
- ``esp_friends_boards/esp32_s3_korvo_2l/board_peripherals.yaml``：STD I2S 输入/输出配置。
- ``esp_boards/esp32_s3_korvo_2_3/board_devices.yaml``：ES8311 播放和 ES7210 录音配置。
- ``esp_boards/esp32_s3_korvo_2_3/board_peripherals.yaml``：TDM I2S 输入/输出配置。

注意事项
------------

- 设备引用的 ``i2s`` 外设方向需与 ``adc_enabled`` / ``dac_enabled`` 对齐：播放使用输出格式，录音使用输入格式。
- 外接编解码器的 I2C 地址写在设备侧 ``peripherals`` 引用条目中，不写入 ``i2c`` 外设的 ``config``。
- 开发板无 PA 控制引脚时，无需配置 ``gpio_pa_control``，且不应在设备引用列表中保留该外设。
- 修改音频设备或 I2S 外设配置后，需重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
------------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，句柄类型为 ``dev_audio_codec_handles_t``：

.. code-block:: c

   typedef struct {
       esp_codec_dev_handle_t       codec_dev;      /*!< Codec device handle */
       const audio_codec_data_if_t *data_if;        /*!< Data interface handle */
       const audio_codec_ctrl_if_t *ctrl_if;        /*!< Control interface handle */
       const audio_codec_gpio_if_t *gpio_if;        /*!< GPIO interface handle */
       const audio_codec_if_t      *codec_if;       /*!< Codec interface handle */
       int16_t                      tx_aux_out_io;  /*!< Optional mirrored/inverted I2S TX auxiliary output IO */
   } dev_audio_codec_handles_t;

相关声明位于 ``esp_board_manager/devices/dev_audio_codec/dev_audio_codec.h``。
