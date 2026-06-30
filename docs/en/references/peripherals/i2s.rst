i2s
============

:link_to_translation:`zh_CN:[中文]`

The ``i2s`` peripheral describes an audio data channel for the ESP-IDF I2S driver. BMGR distinguishes standard, TDM, and PDM input or output paths by ``format``, and is commonly used with ``audio_codec`` for playback, recording, and PDM audio scenarios.

``i2s`` entries are written in ``board_peripherals.yaml``, and devices reference them by peripheral instance name.

Supported Operating Modes
--------------------------

``i2s`` differentiates operating modes primarily by ``format``: ``role`` indicates the I2S controller role, typically ``master``; ``format`` specifies the data format and direction.

- :ref:`STD Output (std-out) <i2s-std-out>`
- :ref:`STD Input (std-in) <i2s-std-in>`
- :ref:`TDM Output/Input (tdm-out / tdm-in) <i2s-tdm>`
- :ref:`PDM Output (pdm-out) <i2s-pdm-out>`
- :ref:`PDM Input (pdm-in) <i2s-pdm-in>`

Minimal Configuration
---------------------

.. _i2s-std-out:

STD Output (``std-out``)
^^^^^^^^^^^^^^^^^^^^^^^^

STD output is used for audio playback paths, typically referenced by the DAC device of ``audio_codec``.

``board_peripherals.yaml``:

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

STD Input (``std-in``)
^^^^^^^^^^^^^^^^^^^^^^

STD input is used for audio recording paths, typically referenced by the ADC device of ``audio_codec``.

``board_peripherals.yaml``:

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

TDM Output/Input (``tdm-out`` / ``tdm-in``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

TDM is used for multi-channel audio or echo-reference scenarios. Output and input can share the same base clock and pin configuration, then adjust ``format`` and slot configuration for direction.

``board_peripherals.yaml``:

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

PDM Output (``pdm-out``)
^^^^^^^^^^^^^^^^^^^^^^^^

PDM output is used for PDM TX audio paths.

``board_peripherals.yaml``:

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

PDM Input (``pdm-in``)
^^^^^^^^^^^^^^^^^^^^^^

PDM input is used for PDM RX audio paths. Single-line mode uses ``clk`` and ``din``; targets supporting multi-line PDM RX can additionally use ``din0`` through ``din3``.

``board_peripherals.yaml``:

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

Mode Notes
----------

``std-out`` and ``std-in`` use standard I2S, suitable for conventional external codec playback and recording. ``tdm-out`` and ``tdm-in`` use TDM, suitable for multi-channel microphones, echo reference, or multi-slot audio chains. ``pdm-out`` and ``pdm-in`` use PDM, suitable for PDM TX or PDM RX audio paths.

The direction of ``format`` must match the device purpose: playback devices reference output formats, and recording devices reference input formats. ``role`` depends on whether the clock is provided by the ESP chip or an external device; ``master`` is the common choice in board configurations.

Full Field Reference
--------------------

STD Mode Full Fields
^^^^^^^^^^^^^^^^^^^^

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

TDM Mode Full Fields
^^^^^^^^^^^^^^^^^^^^

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

PDM TX Full Fields
^^^^^^^^^^^^^^^^^^

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

PDM RX Full Fields
^^^^^^^^^^^^^^^^^^

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

Field Sources:

- YAML template: ``esp_board_manager/peripherals/periph_i2s/periph_i2s.yml``.
- Header file: ``esp_board_manager/peripherals/periph_i2s/periph_i2s.h``.

Applicable Devices
------------------

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Description
   * - ``audio_codec``
     - Device-side ``peripherals`` references ``i2s_audio_out`` or ``i2s_audio_in``
     - External codec uses I2S as the audio data interface; I2C address, PA gain, and other parameters are written in the device-side reference entry

Reference Code
--------------

- ``esp_board_manager/examples/play_embed_music/main/play_embed_music.c``
- ``esp_board_manager/examples/play_sdcard_music/main/play_sdcard_music.c``
- ``esp_board_manager/examples/record_to_sdcard/main/record_to_sdcard.c``
- ``esp_board_manager/examples/record_and_play/main/record_and_play.c``

Board Examples
--------------

- ``esp_friends_boards/esp32_s3_korvo_2l/board_peripherals.yaml``: STD I2S output and input configuration.
- ``esp_boards/esp32_s3_korvo_2_3/board_peripherals.yaml``: TDM I2S output and input configuration.
- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``: Configuration with I2S audio peripheral alongside other board resources.

Notes
-----

- The direction of ``format`` must match the device purpose: playback devices reference ``std-out``, ``tdm-out``, or ``pdm-out``; recording devices reference ``std-in``, ``tdm-in``, or ``pdm-in``.
- When using 24-bit data, ``mclk_multiple`` must be adjusted according to the codec and ESP-IDF I2S driver requirements; optional multiples are provided in the template.
- PDM RX multi-line mode is only available on targets that support this capability; PDM TX dual-line mode is not supported on ESP32.
- ``ext_clk_freq_hz`` only takes effect when an external clock source is selected; the input clock must satisfy the BCLK requirement.
- After modifying I2S peripheral configuration, re-run ``idf.py bmgr -b <board>``.

Debugging Tips
--------------

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to obtain the I2S peripheral handle. The handle type is the native ESP-IDF ``i2s_chan_handle_t``, which can be passed directly to the ``i2s_channel_*`` family of APIs for read, write, and enable operations.

To query the GPIO matrix signal index for I2S data output (for example, to mirror the output), use ``periph_i2s_get_data_out_signal``:

.. code-block:: c

   esp_err_t periph_i2s_get_data_out_signal(const char *name, int line, uint32_t *sig_idx);

``name`` is the peripheral name defined in ``board_peripherals.yaml``; ``line`` is the TX data line index, where ``0`` indicates the primary data line.

The relevant declarations are in ``esp_board_manager/peripherals/periph_i2s/periph_i2s.h``.

Underlying ESP-IDF driver documentation: `I2S <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/i2s.html>`__\ .
