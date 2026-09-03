i2s
============

:link_to_translation:`en:[English]`

.. _i2s-intro:

简介
------

``i2s`` 外设用于描述 ESP-IDF I2S 驱动的音频数据通道。BMGR 按 ``format`` 区分 standard、TDM 与 PDM 的输入或输出路径，常用于 ``audio_codec`` 的播放、录音以及 PDM 音频场景。

``i2s`` 条目写在 ``board_peripherals.yaml`` 中，设备通过外设实例名引用它。

.. _i2s-working-modes:

支持的工作模式
---------------------

``i2s`` 主要按 ``format`` 区分工作模式：``role`` 表示 I2S 控制器角色，常用 ``master``；``format`` 表示数据格式与方向。

- :ref:`STD 输出（std-out） <i2s-std-out>`
- :ref:`STD 输入（std-in） <i2s-std-in>`
- :ref:`TDM 输出/输入（tdm-out / tdm-in） <i2s-tdm>`
- :ref:`PDM 输出（pdm-out） <i2s-pdm-out>`
- :ref:`PDM 输入（pdm-in） <i2s-pdm-in>`

.. _i2s-min-config:

最小配置
------------

.. _i2s-std-out:

STD 输出（``std-out``）
^^^^^^^^^^^^^^^^^^^^^^^^^^^

完整字段见 :ref:`i2s-std-full`。

STD 输出用于音频播放路径，常被 ``audio_codec`` 的 DAC 设备引用。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: i2s_audio_out
        type: i2s
        role: master
        format: std-out
        config:
          port: 0                    # [TO_BE_CONFIRMED]
          sample_rate_hz: 48000      # [TO_BE_CONFIRMED]
          data_bit_width: 16         # [TO_BE_CONFIRMED]
          slot_mode: I2S_SLOT_MODE_STEREO
          pins:
            bclk: 9                  # [IO]
            ws: 45                   # [IO]
            dout: 8                  # [IO]

.. _i2s-std-in:

STD 输入（``std-in``）
^^^^^^^^^^^^^^^^^^^^^^^^^^

完整字段见 :ref:`i2s-std-full`。

STD 输入用于音频录音路径，常被 ``audio_codec`` 的 ADC 设备引用。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: i2s_audio_in
        type: i2s
        role: master
        format: std-in
        config:
          port: 0                    # [TO_BE_CONFIRMED]
          sample_rate_hz: 48000      # [TO_BE_CONFIRMED]
          data_bit_width: 16         # [TO_BE_CONFIRMED]
          slot_mode: I2S_SLOT_MODE_STEREO
          pins:
            bclk: 9                  # [IO]
            ws: 45                   # [IO]
            din: 10                  # [IO]

.. _i2s-tdm:

TDM 输出/输入（``tdm-out`` / ``tdm-in``）
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

完整字段见 :ref:`i2s-tdm-full`。

TDM 用于多通道音频或回采场景。输出与输入可使用同一组基础时钟与管脚配置，再按输入或输出方向调整 ``format`` 与 slot 配置。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: i2s_audio_out
        type: i2s
        role: master
        format: tdm-out
        config: &i2s_config
          port: 0                    # [TO_BE_CONFIRMED]
          sample_rate_hz: 48000      # [TO_BE_CONFIRMED]
          data_bit_width: 16         # [TO_BE_CONFIRMED]
          slot_mode: I2S_SLOT_MODE_MONO
          total_slot: 2
          pins:
            mclk: 16                 # [IO]
            bclk: 9                  # [IO]
            ws: 45                   # [IO]
            dout: 8                  # [IO]
            din: 10                  # [IO]

      - name: i2s_audio_in
        type: i2s
        role: master
        format: tdm-in
        config:
          <<: *i2s_config
          slot_mode: I2S_SLOT_MODE_STEREO

.. _i2s-pdm-out:

PDM 输出（``pdm-out``）
^^^^^^^^^^^^^^^^^^^^^^^^^^^

完整字段见 :ref:`i2s-pdm-out-full`。

PDM 输出用于 PDM TX 音频路径。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: i2s_audio_out
        type: i2s
        role: master
        format: pdm-out
        config:
          port: 0                    # [TO_BE_CONFIRMED]
          sample_rate_hz: 48000      # [TO_BE_CONFIRMED]
          data_bit_width: 16         # [TO_BE_CONFIRMED]
          slot_mode: I2S_SLOT_MODE_STEREO
          pins:
            clk: 1                   # [IO]
            dout: 2                  # [IO]

.. _i2s-pdm-in:

PDM 输入（``pdm-in``）
^^^^^^^^^^^^^^^^^^^^^^^^^^

完整字段见 :ref:`i2s-pdm-in-full`。

PDM 输入用于 PDM RX 音频路径。单线模式使用 ``clk`` 和 ``din``；支持多线 PDM RX 的目标还可以使用 ``din0`` 到 ``din3``。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: i2s_audio_in
        type: i2s
        role: master
        format: pdm-in
        config:
          port: 0                    # [TO_BE_CONFIRMED]
          sample_rate_hz: 48000      # [TO_BE_CONFIRMED]
          data_bit_width: 16         # [TO_BE_CONFIRMED]
          slot_mode: I2S_SLOT_MODE_STEREO
          pins:
            clk: 1                   # [IO]
            din: 2                   # [IO]

.. _i2s-mode-notes:

模式说明
------------

``std-out`` 与 ``std-in`` 使用 standard I2S，适用于常规外接 codec 播放与录音。``tdm-out`` 与 ``tdm-in`` 使用 TDM，适用于多通道麦克风、回采或多个 slot 的音频链路。``pdm-out`` 与 ``pdm-in`` 使用 PDM，适用于 PDM TX 或 PDM RX 音频路径。

``format`` 的方向需与设备用途一致：播放设备引用输出格式，录音设备引用输入格式。``role`` 取决于时钟由 ESP 芯片提供还是由外部器件提供，板级配置中常用 ``master``。

.. _i2s-full-fields:

完整字段
------------

.. _i2s-std-full:

STD 模式完整字段
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: i2s_audio_out
      type: i2s
      # Valid role values: master, slave
      role: master
      # Valid format values: std-out, std-in, default direction is output
      format: std-out
      config:
        port: 0  # [TO_BE_CONFIRMED] I2S port
        # ===== CLOCK CONFIGURATION =====
        # Sample rate in Hz (default: 48000)
        sample_rate_hz: 48000           # [TO_BE_CONFIRMED] Sample rate in Hz

        # Clock source (default: I2S_CLK_SRC_DEFAULT)
        clk_src: "I2S_CLK_SRC_DEFAULT"
        # Valid values:
        # - I2S_CLK_SRC_DEFAULT
        # - I2S_CLK_SRC_EXTERNAL
        # - I2S_CLK_SRC_APB
        # - I2S_CLK_SRC_PLL_F160M
        # - I2S_CLK_SRC_PLL_F240M

        # External clock source frequency in Hz. Only takes effect when clk_src is
        # I2S_CLK_SRC_EXTERNAL; otherwise the ESP-IDF driver ignores it.
        # STD mode emits this field only for SOC_I2S_HW_VERSION_2 layouts because
        # SOC_I2S_HW_VERSION_1 std clock config does not provide ext_clk_freq_hz.
        # Ensure the input clock is >= BCLK: sample_rate_hz * slot_bits * 2.
        ext_clk_freq_hz: 0

        # MCLK multiple (default: 256)
        mclk_multiple: 256
        # Valid values: 128, 192, 256, 384, 512, 576, 768, 1024, 1152

        # BCLK divider (default: 8). STD mode emits this field only on ESP-IDF >= 5.5.
        bclk_div: 8

        # ===== SLOT CONFIGURATION =====
        # Data bit width (default: 16)
        data_bit_width: 16             # [TO_BE_CONFIRMED] Data bit width
        # Valid values: 8, 16, 24, 32

        # Slot bit width (default: I2S_SLOT_BIT_WIDTH_AUTO)
        slot_bit_width: "I2S_SLOT_BIT_WIDTH_AUTO"
        # Valid values:
        # - I2S_SLOT_BIT_WIDTH_AUTO
        # - I2S_SLOT_BIT_WIDTH_8BIT
        # - I2S_SLOT_BIT_WIDTH_16BIT
        # - I2S_SLOT_BIT_WIDTH_24BIT
        # - I2S_SLOT_BIT_WIDTH_32BIT

        # Slot mode (default: I2S_SLOT_MODE_STEREO)
        slot_mode: "I2S_SLOT_MODE_STEREO"  # [TO_BE_CONFIRMED] Slot mode
        # Valid values:
        # - I2S_SLOT_MODE_MONO
        # - I2S_SLOT_MODE_STEREO

        # Standard mode slot mask (default: I2S_STD_SLOT_BOTH)
        slot_mask: "I2S_STD_SLOT_BOTH"
        # Valid values:
        # - I2S_STD_SLOT_LEFT
        # - I2S_STD_SLOT_RIGHT
        # - I2S_STD_SLOT_BOTH

        # WS width (default: 16)
        ws_width: 16

        # WS polarity (default: false)
        ws_pol: false

        # Bit shift (default: true)
        bit_shift: true

        # SOC_I2S_HW_VERSION_1 slot field (ESP32/ESP32-S2; default: true)
        # SOC_I2S_HW_VERSION_2 layouts do not provide msb_right.
        msb_right: true

        # SOC_I2S_HW_VERSION_2 slot fields (default: true/false/false)
        # SOC_I2S_HW_VERSION_1 layouts do not provide these fields.
        left_align: true
        big_endian: false
        bit_order_lsb: false

        # ===== GPIO CONFIGURATION =====
        pins:
          # Standard mode pins
          mclk: -1                     # [IO] MCLK pin
          bclk: -1                     # [IO] BCLK pin
          ws: -1                       # [IO] WS pin
          dout: -1                     # [IO] Data output pin
          din: -1                      # [IO] Data input pin

        # Invert flags
        invert_flags:
          mclk_inv: false
          bclk_inv: false
          ws_inv: false

.. _i2s-tdm-full:

TDM 模式完整字段
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: i2s_audio_out
      type: i2s
      # Valid role values: master, slave
      role: master
      # Valid format values: tdm-out, tdm-in, default direction is output
      format: tdm-out
      config:
        port: 0  # [TO_BE_CONFIRMED] I2S port
        # ===== CLOCK CONFIGURATION =====
        # Sample rate in Hz (default: 48000)
        sample_rate_hz: 48000           # [TO_BE_CONFIRMED] Sample rate in Hz

        # Clock source (default: I2S_CLK_SRC_DEFAULT)
        clk_src: "I2S_CLK_SRC_DEFAULT"

        # External clock source frequency in Hz. Only takes effect when clk_src is
        # I2S_CLK_SRC_EXTERNAL; otherwise the ESP-IDF driver ignores it.
        # TDM clock config includes this field on supported ESP-IDF I2S layouts.
        # Ensure the input clock is >= BCLK: sample_rate_hz * slot_bits * 2.
        ext_clk_freq_hz: 0

        # MCLK multiple (default: 256)
        mclk_multiple: 256

        # BCLK divider (default: 8)
        bclk_div: 8

        # ===== SLOT CONFIGURATION =====
        # Data bit width (default: 16)
        data_bit_width: 16             # [TO_BE_CONFIRMED] Data bit width

        # Slot bit width (default: I2S_SLOT_BIT_WIDTH_AUTO)
        slot_bit_width: "I2S_SLOT_BIT_WIDTH_AUTO"

        # Slot mode (default: I2S_SLOT_MODE_STEREO)
        slot_mode: "I2S_SLOT_MODE_STEREO"  # [TO_BE_CONFIRMED] Slot mode

        # TDM slot mask (default is based on slot_mode)
        # For STEREO: I2S_TDM_SLOT0 | I2S_TDM_SLOT1
        # For MONO: I2S_TDM_SLOT0
        # You may also use a C expression such as:
        # I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2
        slot_mask: "I2S_TDM_SLOT0 | I2S_TDM_SLOT1"

        # WS width (default: 0, IDF I2S_TDM_AUTO_WS_WIDTH)
        ws_width: 0

        # WS polarity (default: false)
        ws_pol: false

        # Bit shift (default: true)
        bit_shift: true

        # Left align (default: false for TDM)
        left_align: false

        # Big endian (default: false)
        big_endian: false

        # Bit order LSB (default: false)
        bit_order_lsb: false

        # Skip mask (default: false)
        skip_mask: false

        # Total slots (default: 0, IDF I2S_TDM_AUTO_SLOT_NUM)
        total_slot: 0

        # ===== GPIO CONFIGURATION =====
        pins:
          # TDM mode pins (same as Standard mode)
          mclk: -1                     # [IO] MCLK pin
          bclk: -1                     # [IO] BCLK pin
          ws: -1                       # [IO] WS pin
          dout: -1                     # [IO] Data output pin
          din: -1                      # [IO] Data input pin

        # Invert flags
        invert_flags:
          mclk_inv: false
          bclk_inv: false
          ws_inv: false

.. _i2s-pdm-out-full:

PDM 输出完整字段
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: i2s_audio_out
      type: i2s
      role: master
      format: pdm-out
      config:
        port: 0  # [TO_BE_CONFIRMED] I2S port
        # ===== CLOCK CONFIGURATION =====
        # Sample rate in Hz (default: 48000)
        sample_rate_hz: 48000           # [TO_BE_CONFIRMED] Sample rate in Hz

        # Clock source (default: I2S_CLK_SRC_DEFAULT)
        clk_src: "I2S_CLK_SRC_DEFAULT"

        # MCLK multiple (default: 256)
        mclk_multiple: 256

        # Up-sampling parameters (default: 960, 480)
        up_sample_fp: 960
        up_sample_fs: 480

        # BCLK divider (default: 8)
        bclk_div: 8

        # ===== SLOT CONFIGURATION =====
        # Data bit width (default: 16, fixed for PDM)
        data_bit_width: 16              # [TO_BE_CONFIRMED] Data bit width

        # Slot bit width (default: I2S_SLOT_BIT_WIDTH_AUTO)
        slot_bit_width: "I2S_SLOT_BIT_WIDTH_AUTO"

        # Slot mode (default: I2S_SLOT_MODE_STEREO)
        slot_mode: "I2S_SLOT_MODE_STEREO"  # [TO_BE_CONFIRMED] Slot mode

        # PDM TX slot mask (default: I2S_PDM_SLOT_BOTH).
        # Emitted only for SOC_I2S_HW_VERSION_1 layouts.
        slot_mask: "I2S_PDM_SLOT_BOTH"
        # Valid values:
        # - I2S_PDM_SLOT_LEFT
        # - I2S_PDM_SLOT_RIGHT
        # - I2S_PDM_SLOT_BOTH

        # Data format (default: I2S_PDM_DATA_FMT_PCM)
        # This field is emitted only on ESP-IDF >= 5.5. On older IDF versions,
        # explicit I2S_PDM_DATA_FMT_RAW is rejected; PCM is treated as the default.
        data_fmt: "I2S_PDM_DATA_FMT_PCM"
        # Valid values:
        # - I2S_PDM_DATA_FMT_PCM
        # - I2S_PDM_DATA_FMT_RAW

        # Sigma-delta filter settings
        sd_prescale: 0
        sd_scale: "I2S_PDM_SIG_SCALING_MUL_1"
        # Valid values:
        # - I2S_PDM_SIG_SCALING_DIV_2
        # - I2S_PDM_SIG_SCALING_MUL_1
        # - I2S_PDM_SIG_SCALING_MUL_2
        # - I2S_PDM_SIG_SCALING_MUL_4

        # Filter scaling values
        hp_scale: "I2S_PDM_SIG_SCALING_MUL_1"
        lp_scale: "I2S_PDM_SIG_SCALING_MUL_1"
        sinc_scale: "I2S_PDM_SIG_SCALING_MUL_1"

        # Line mode (default: I2S_PDM_TX_ONE_LINE_CODEC).
        # Emitted only for SOC_I2S_HW_VERSION_2 layouts.
        line_mode: "I2S_PDM_TX_ONE_LINE_CODEC"
        # Valid values:
        # - I2S_PDM_TX_ONE_LINE_CODEC
        # - I2S_PDM_TX_ONE_LINE_DAC
        # - I2S_PDM_TX_TWO_LINE_DAC

        # High pass filter settings.
        # Emitted only for SOC_I2S_HW_VERSION_2 layouts.
        hp_en: true
        hp_cut_off_freq_hz: 35.5

        # Sigma-delta dither settings.
        # Emitted only for SOC_I2S_HW_VERSION_2 layouts.
        sd_dither: 0
        sd_dither2: 1

        # ===== GPIO CONFIGURATION =====
        pins:
          # PDM TX mode pins
          clk: -1                     # [IO] Clock pin
          dout: -1                    # [IO] Data output pin
          # Emitted only when the target exposes a second PDM TX data line.
          # ESP32 defines SOC_I2S_PDM_MAX_TX_LINES (1), so no dout2 field is generated.
          dout2: -1                   # [IO] Second data pin for dual-line DAC mode

        # Invert flags
        invert_flags:
          clk_inv: false

.. _i2s-pdm-in-full:

PDM 输入完整字段
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: i2s_audio_in
      type: i2s
      role: master
      format: pdm-in
      config:
        port: 0  # [TO_BE_CONFIRMED] I2S port
        # ===== CLOCK CONFIGURATION =====
        # Sample rate in Hz (default: 48000)
        sample_rate_hz: 48000           # [TO_BE_CONFIRMED] Sample rate in Hz

        # Clock source (default: I2S_CLK_SRC_DEFAULT)
        clk_src: "I2S_CLK_SRC_DEFAULT"

        # MCLK multiple (default: 256)
        mclk_multiple: 256

        # Down-sampling mode (default: I2S_PDM_DSR_8S)
        dn_sample_mode: "I2S_PDM_DSR_8S"
        # Valid values:
        # - I2S_PDM_DSR_8S
        # - I2S_PDM_DSR_16S

        # BCLK divider (default: 8)
        bclk_div: 8

        # ===== SLOT CONFIGURATION =====
        # Data bit width (default: 16, fixed for PDM)
        data_bit_width: 16              # [TO_BE_CONFIRMED] Data bit width

        # Slot bit width (default: I2S_SLOT_BIT_WIDTH_AUTO)
        slot_bit_width: "I2S_SLOT_BIT_WIDTH_AUTO"

        # Slot mode (default: I2S_SLOT_MODE_STEREO)
        slot_mode: "I2S_SLOT_MODE_STEREO"  # [TO_BE_CONFIRMED] Slot mode

        # PDM slot mask (default: I2S_PDM_SLOT_BOTH)
        slot_mask: "I2S_PDM_SLOT_BOTH"
        # Valid values for single-line mode:
        # - I2S_PDM_SLOT_LEFT
        # - I2S_PDM_SLOT_RIGHT
        # - I2S_PDM_SLOT_BOTH
        # Valid values for multi-line mode (ESP32S3/ESP32P4):
        # - I2S_PDM_RX_LINE0_SLOT_LEFT
        # - I2S_PDM_RX_LINE0_SLOT_RIGHT
        # - I2S_PDM_RX_LINE1_SLOT_LEFT
        # - I2S_PDM_RX_LINE1_SLOT_RIGHT
        # - I2S_PDM_RX_LINE2_SLOT_LEFT
        # - I2S_PDM_RX_LINE2_SLOT_RIGHT
        # - I2S_PDM_RX_LINE3_SLOT_LEFT
        # - I2S_PDM_RX_LINE3_SLOT_RIGHT
        # - I2S_PDM_LINE_SLOT_ALL

        # Data format (default: I2S_PDM_DATA_FMT_PCM)
        # This field is emitted only on ESP-IDF >= 5.5. On older IDF versions,
        # explicit I2S_PDM_DATA_FMT_RAW is rejected; PCM is treated as the default.
        data_fmt: "I2S_PDM_DATA_FMT_PCM"
        # Valid values:
        # - I2S_PDM_DATA_FMT_PCM
        # - I2S_PDM_DATA_FMT_RAW

        # High pass filter settings.
        # Emitted only when SOC_I2S_SUPPORTS_PDM_RX_HP_FILTER is enabled.
        hp_en: true
        hp_cut_off_freq_hz: 35.5

        # Amplification number (default: 1).
        # Emitted only when SOC_I2S_SUPPORTS_PDM_RX_HP_FILTER is enabled.
        amplify_num: 1

        # ===== GPIO CONFIGURATION =====
        pins:
          # PDM RX mode pins - Single line mode
          clk: -1                     # [IO] Clock pin
          din: -1                     # [IO] Data input pin

          # PDM RX mode pins - Multi-line mode (ESP32S3/ESP32P4)
          # Uncomment and configure for multi-line mode:
          # din0: -1  # [IO] Line 0 data pin
          # din1: -1  # [IO] Line 1 data pin
          # din2: -1  # [IO] Line 2 data pin
          # din3: -1  # [IO] Line 3 data pin

        # Invert flags
        invert_flags:
          clk_inv: false

字段来源：

- YAML 模板：``esp_board_manager/peripherals/periph_i2s/periph_i2s.yml``。
- 头文件：``esp_board_manager/peripherals/periph_i2s/periph_i2s.h``。

.. _i2s-devices:

适用设备
------------

.. list-table::
   :header-rows: 1

   * - device type
     - 使用方式
     - 说明
   * - ``audio_codec``
     - 设备侧 ``peripherals`` 引用 ``i2s_audio_out`` 或 ``i2s_audio_in``
     - 外接 codec 使用 I2S 作为音频数据接口；I2C 地址、PA 增益等参数写在设备侧引用条目中

.. _i2s-code:

参考代码
------------

- ``esp_board_manager/examples/play_embed_music/main/play_embed_music.c``
- ``esp_board_manager/examples/play_sdcard_music/main/play_sdcard_music.c``
- ``esp_board_manager/examples/record_to_sdcard/main/record_to_sdcard.c``
- ``esp_board_manager/examples/record_and_play/main/record_and_play.c``

.. _i2s-boards:

板级参考
------------

- ``esp_friends_boards/esp32_s3_korvo_2l/board_peripherals.yaml``：STD I2S 输出和输入配置。
- ``esp_boards/esp32_s3_korvo_2_3/board_peripherals.yaml``：TDM I2S 输出和输入配置。
- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``：I2S 音频外设与其他板级资源并存的配置。

.. _i2s-notes:

注意事项
------------

- ``format`` 的方向需与设备用途一致：播放设备引用 ``std-out``、``tdm-out`` 或 ``pdm-out``，录音设备引用 ``std-in``、``tdm-in`` 或 ``pdm-in``。
- 使用 24-bit 数据时，``mclk_multiple`` 需按 codec 与 ESP-IDF I2S 驱动的要求调整，模板中给出了可选倍数。
- PDM RX 多线模式仅在支持该能力的目标上可用；PDM TX 双线模式不适用于 ESP32。
- ``ext_clk_freq_hz`` 仅在选择外部时钟源时生效，输入时钟需满足 BCLK 需求。
- 修改 I2S 外设配置后，需重新执行 ``idf.py bmgr -b <board>``。

.. _i2s-debug:

调试技巧
------------

.. _i2s-api:

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_periph_handle` 获取 I2S 外设句柄，句柄类型为 ESP-IDF 原生的 ``i2s_chan_handle_t``，可直接传入 ``i2s_channel_*`` 系列 API 进行读写和使能。

若需查询 I2S 数据输出的 GPIO matrix 信号索引（例如用于镜像输出），可使用 ``periph_i2s_get_data_out_signal``：

.. code-block:: c

   esp_err_t periph_i2s_get_data_out_signal(const char *name, int line, uint32_t *sig_idx);

``name`` 为 ``board_peripherals.yaml`` 中定义的外设名称；``line`` 为 TX 数据线索引，``0`` 表示主数据线。

相关声明位于 ``esp_board_manager/peripherals/periph_i2s/periph_i2s.h``。

底层 ESP-IDF 驱动文档：`I2S <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/i2s.html>`__\ 。
