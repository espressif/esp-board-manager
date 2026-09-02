Audio Codec (audio_codec)
=========================

:link_to_translation:`zh_CN:[中文]`

.. _audio-codec-intro:

Introduction
------------

The ``audio_codec`` device describes audio playback, recording, and internal ADC audio input paths. When an external codec chip is used for playback or capture, the peripheral side requires at least one ``i2c`` control interface and one ``i2s`` data interface; a power amplifier GPIO is optional.

A single logical ``audio_codec`` device must not enable both ``adc_enabled`` and ``dac_enabled`` at the same time. When one physical codec chip is required to operate in full-duplex, split it into two unidirectional devices in ``board_devices.yaml``, for example ``audio_dac`` and ``audio_adc``.

.. _audio-codec-usage-modes:

Supported Usage Modes
---------------------

``dev_audio_codec`` does not use ``sub_type`` to distinguish usage modes; instead it adapts to different use cases through the peripherals and configuration fields used:

- :ref:`Playback (DAC, dac_enabled: true) <audio-codec-dac>`
- :ref:`Recording (ADC, adc_enabled: true) <audio-codec-adc>`
- :ref:`Full-duplex (same physical codec chip) <audio-codec-full-duplex>`
- :ref:`Audio recording with the SoC internal ADC <audio-codec-internal>`
- :ref:`PDM digital microphone <audio-codec-pdm>`
- :ref:`PDM speaker <audio-codec-pdm-dac>`

.. _audio-codec-min-config:

Minimal Configuration
---------------------

.. _audio-codec-dac:

Playback (DAC, ``dac_enabled: true``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See complete fields: :ref:`audio-codec-full-external`.

Playback mode is for DAC output through an external codec. ``board_peripherals.yaml`` requires at least ``i2c`` and ``i2s`` output peripherals; when the board has a power amplifier control pin, an additional ``gpio`` peripheral is configured.

``board_peripherals.yaml``:

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

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: audio_dac
        chip: es8311                    # [TO_BE_CONFIRMED] Codec chip type
        type: audio_codec
        config:
          adc_enabled: false
          dac_enabled: true
        peripherals:
          - pa_name: gpio_pa_control
            active_level: 1
          - i2s_name: i2s_audio_out
          - i2c_name: i2c_master
            address: 0x30               # [TO_BE_CONFIRMED] I2C device address
            frequency: 400000

If the board has no PA control pin, do not configure ``gpio_pa_control`` and do not include that peripheral in the device reference list.

.. _audio-codec-adc:

Recording (ADC, ``adc_enabled: true``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See complete fields: :ref:`audio-codec-full-external`.

Recording mode is for ADC input through an external codec. ``board_peripherals.yaml`` may share the same ``i2c_master`` with playback; it is recommended to use a dedicated ``std-in`` peripheral name for I2S recording.

``board_peripherals.yaml``:

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

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: audio_adc
        chip: es8311                    # [TO_BE_CONFIRMED] Codec chip type
        type: audio_codec
        config:
          adc_enabled: true
          dac_enabled: false
        peripherals:
          - i2s_name: i2s_audio_in
          - i2c_name: i2c_master
            address: 0x30               # [TO_BE_CONFIRMED] I2C device address
            frequency: 400000

.. _audio-codec-full-duplex:

Full-duplex (Same Physical Codec Chip)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See complete fields: :ref:`audio-codec-full-external`.

Full-duplex mode is for a single physical codec chip that handles both playback and recording. In BMGR, two ``audio_codec`` logical devices are still configured, each enabling only DAC or ADC respectively; the I2C control interface may point to the same ``i2c_master``.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: audio_dac
        chip: es8311
        type: audio_codec
        config:
          adc_enabled: false
          dac_enabled: true
        peripherals:
          - i2s_name: i2s_audio_out
          - &es8311_i2c_master
            i2c_name: i2c_master
            address: 0x30
            frequency: 400000

      - name: audio_adc
        chip: es8311
        type: audio_codec
        config:
          adc_enabled: true
          dac_enabled: false
        peripherals:
          - i2s_name: i2s_audio_in
          - *es8311_i2c_master

.. _audio-codec-internal:

Audio Recording with the SoC Internal ADC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See complete fields: :ref:`audio-codec-full-adc-reuse`.
See complete fields: :ref:`audio-codec-full-adc-patterns`.
See complete fields: :ref:`audio-codec-full-adc-single-unit`.

When ``chip`` is set to ``internal`` and ``adc_enabled`` is enabled, ``audio_codec`` does not initialize an external codec through ``esp_codec_dev``; it can use the SoC's internal ADC to capture audio. When the board already defines an ``adc`` peripheral, it can be referenced in the device's ``peripherals``; otherwise, ``config.adc_local_cfg`` can create a device-local ADC continuous configuration.

Reusing an ``adc`` peripheral:

.. code-block:: yaml

    devices:
      - name: audio_adc_mic_reuse
        chip: internal
        type: audio_codec
        config:
          adc_enabled: true
          dac_enabled: false
        peripherals:
          - adc_name: adc_audio_in

Creating an ADC continuous handle via ``audio_codec``:

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

PDM Digital Microphone
^^^^^^^^^^^^^^^^^^^^^^

A PDM digital microphone is the typical I2S form in the ``chip: internal`` path. On the peripheral side, set ``format`` to ``pdm-in``; only the ``clk`` and ``din`` pins are required — no ``bclk`` or ``ws``.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: i2s_audio_in
        type: i2s
        role: master
        format: pdm-in
        config:
          port: 0
          sample_rate_hz: 16000             # [TO_BE_CONFIRMED] Sample rate in Hz
          data_bit_width: 16                # [TO_BE_CONFIRMED] Data bit width
          slot_mode: I2S_SLOT_MODE_MONO     # [TO_BE_CONFIRMED] Mono / stereo
          slot_mask: I2S_PDM_SLOT_LEFT      # [TO_BE_CONFIRMED] PDM slot selection (LEFT / RIGHT / BOTH)
          dn_sample_mode: I2S_PDM_DSR_8S    # [TO_BE_CONFIRMED] Down-sampling mode
          hp_en: true                       # [TO_BE_CONFIRMED] High-pass filter enable
          hp_cut_off_freq_hz: 35.5          # [TO_BE_CONFIRMED] High-pass cutoff frequency in Hz
          amplify_num: 1                    # [TO_BE_CONFIRMED] Amplification factor
          pins:
            clk: 22                         # [IO] PDM CLK pin
            din: 21                         # [IO] PDM DIN pin

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: audio_adc
        chip: internal
        type: audio_codec
        config:
          adc_enabled: true
          dac_enabled: false
        peripherals:
          - i2s_name: i2s_audio_in

If the microphone has a dedicated power control, add ``power_ctrl_device: mic_power_ctrl`` to the device and define the corresponding ``power_ctrl`` device in ``board_devices.yaml``. Board reference: ``boards/esp32_p4_eye/board_devices.yaml``.

.. _audio-codec-pdm-dac:

PDM Speaker
^^^^^^^^^^^

I2S PDM output can directly drive a PDM speaker or PDM amplifier without an external codec chip. On the peripheral side, set ``format`` to ``pdm-out``; on the device side, use ``chip: internal``. PDM TX mode only requires the ``dout`` pin; the ``clk`` pin depends on the hardware (fill ``-1`` when not used).

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: i2s_audio_out
        type: i2s
        role: master
        format: pdm-out
        config:
          sample_rate_hz: 16000             # [TO_BE_CONFIRMED] Sample rate in Hz
          data_bit_width: 16                # [TO_BE_CONFIRMED] Data bit width
          slot_mode: I2S_SLOT_MODE_MONO     # [TO_BE_CONFIRMED] Mono / stereo
          slot_mask: I2S_PDM_SLOT_LEFT      # [TO_BE_CONFIRMED] PDM slot selection (LEFT / RIGHT / BOTH)
          line_mode: I2S_PDM_TX_ONE_LINE_CODEC  # [TO_BE_CONFIRMED] Output line mode
          up_sample_fp: 960                 # [TO_BE_CONFIRMED] Up-sampling parameter fp
          up_sample_fs: 441                 # [TO_BE_CONFIRMED] Up-sampling parameter fs
          sd_prescale: 0                    # [TO_BE_CONFIRMED]
          sd_scale: I2S_PDM_SIG_SCALING_MUL_4   # [TO_BE_CONFIRMED] Sigma-delta scaling
          hp_scale: I2S_PDM_SIG_SCALING_MUL_4   # [TO_BE_CONFIRMED]
          lp_scale: I2S_PDM_SIG_SCALING_MUL_4   # [TO_BE_CONFIRMED]
          sinc_scale: I2S_PDM_SIG_SCALING_MUL_4 # [TO_BE_CONFIRMED]
          hp_en: false                      # [TO_BE_CONFIRMED] High-pass filter enable
          hp_cut_off_freq_hz: 35.5          # [TO_BE_CONFIRMED] High-pass cutoff frequency in Hz
          sd_dither: 0                      # [TO_BE_CONFIRMED]
          sd_dither2: 1                     # [TO_BE_CONFIRMED]
          pins:
            clk: -1                         # [IO] PDM CLK pin (-1 to disable)
            dout: 3                         # [IO] PDM data output pin

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: audio_dac
        chip: internal
        type: audio_codec
        config:
          dac_enabled: true
          adc_enabled: false
        peripherals:
          - i2s_name: i2s_audio_out
          - pa_name: gpio_pa_control           # Optional: PA control pin
            gain: 6                         # [TO_BE_CONFIRMED] PA gain in dB
            active_level: 1                 # Active level of the PA control pin

If the speaker has a PA control pin, an additional ``gpio_pa_control`` peripheral (``type: gpio``) must be defined in ``board_peripherals.yaml``. Board references: ``boards/esp32_c3_lyra/board_peripherals.yaml``, ``boards/esp32_c3_lyra/board_devices.yaml``.

.. _audio-codec-mode-notes:

Mode Description
----------------

Playback and recording are two mutually exclusive logical device directions. A playback device enables only ``dac_enabled``; a recording device enables only ``adc_enabled``. When a single physical codec requires full-duplex, configure two logical devices and reference the output and input I2S peripherals respectively.

The external codec path uses I2S for audio data transfer and I2C for codec chip configuration. Setting ``chip`` to ``internal`` skips external codec control-interface initialization, but still creates the data interface and ``esp_codec_dev`` handle. This is suitable for boards with no external codec chip or where the codec requires no software configuration; audio data uses the internal ADC or I2S peripheral, with the I2S ``format`` chosen according to the actual interface. Internal DAC paths use a dummy codec; when no PA GPIO is configured, ``esp_codec_dev`` ignores PA control operations.

.. _audio-codec-full-fields:

Full Field Reference
--------------------

.. _audio-codec-full-external:

External Codec Chip
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
        - pa_name: gpio_power_amp
          gain: 0.0
          active_level: 1

        # Optional codec reset GPIO. active_level is the asserted reset level.
        - reset_name: gpio_codec_reset
          active_level: 0

        # I2S interface configuration
        - i2s_name: i2s_audio_out            # [TO_BE_CONFIRMED] I2S peripheral for audio data interface
          clk_src: 0                         # I2S clock source, need converted from `i2s_clock_src_t`. If set to 0 will use default clock source (default: 0)
          tx_aux_out_io: -1                  # Optional mirrored/inverted I2S TX auxiliary output IO, -1 disables this feature
          tx_aux_out_line: 0                 # Optional TX data line index (0: main line, >0: extra TX data line when supported)
          tx_aux_out_invert: false           # Optional invert flag for the auxiliary output signal

        # I2C interface configuration
        - i2c_name: i2c_master                # [TO_BE_CONFIRMED] I2C peripheral for codec control
          address: 0x30                      # [TO_BE_CONFIRMED] I2C device address, include the read/write bit (hex format) (default: 0x30)
          frequency: 100000                  # I2C clock frequency in Hz (default: 100000)

.. _audio-codec-full-adc-reuse:

Reading Data with the SoC Internal ADC: Reusing ADC Peripheral
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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
        # - adc_name: adc_audio_in
        #   type: adc
        #   role: continuous
        - adc_name: adc_audio_in              # ADC peripheral name to be reused by dev_audio_codec

.. _audio-codec-full-adc-patterns:

Reading Data with the SoC Internal ADC: Local Pattern List (patterns) Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

.. _audio-codec-full-adc-single-unit:

Reading Data with the SoC Internal ADC: Local Single-Unit Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

Field sources:

- YAML template: ``esp_board_manager/devices/dev_audio_codec/dev_audio_codec.yaml``.
- Header file: ``esp_board_manager/devices/dev_audio_codec/dev_audio_codec.h``.

.. _audio-codec-deps:

Component Dependencies
----------------------

``audio_codec`` uses ``esp_codec_dev`` to provide the codec device abstraction. Only the chip model needs to be specified in the ``chip`` field of the board-level YAML.

.. _audio-codec-peripherals:

Required Peripherals
--------------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - Required
     - Purpose
   * - ``i2c``
     - ``master``
     - Required for external codec
     - Codec control interface; the device-side reference entry fills in ``address`` and ``frequency``
   * - ``i2s``
     - ``master`` / ``std-out``, ``std-in``, ``tdm-out``, ``tdm-in``
     - Required for external codec
     - Audio data interface
   * - ``gpio``
     - ``io``
     - Used when PA control or mute control is present
     - Device-side reference entry fills in ``gain`` and ``active_level``
   * - ``adc``
     - ``continuous``
     - Used when internal ADC audio input is used and the peripheral is reused
     - Provides ADC continuous handle

.. _audio-codec-code:

Code Reference
--------------

- ``esp_board_manager/examples/play_embed_music/main/play_embed_music.c``
- ``esp_board_manager/examples/play_sdcard_music/main/play_sdcard_music.c``
- ``esp_board_manager/examples/record_to_sdcard/main/record_to_sdcard.c``
- ``esp_board_manager/examples/record_and_play/main/record_and_play.c``

.. _audio-codec-boards:

Board-level Reference
---------------------

- ``esp_friends_boards/esp32_s3_korvo_2l/board_devices.yaml``: The same ES8311 chip split into two logical devices, ``audio_dac`` and ``audio_adc``.
- ``esp_friends_boards/esp32_s3_korvo_2l/board_peripherals.yaml``: STD I2S input/output configuration.
- ``esp_boards/esp32_s3_korvo_2_3/board_devices.yaml``: ES8311 playback and ES7210 recording configuration.
- ``esp_boards/esp32_s3_korvo_2_3/board_peripherals.yaml``: TDM I2S input/output configuration.

.. _audio-codec-notes:

Notes
-----

- The ``i2s`` peripheral direction referenced by the device must align with ``adc_enabled`` / ``dac_enabled``: playback uses an output format, recording uses an input format.
- The I2C address of an external codec is written in the device-side ``peripherals`` reference entry, not in the ``i2c`` peripheral's ``config``.
- When the board has no PA control pin, do not configure ``gpio_pa_control`` and do not include that peripheral in the device reference list.
- After modifying the audio device or I2S peripheral configuration, re-run ``idf.py bmgr -b <board>``.

.. _audio-codec-debug:

Debugging Tips
--------------

.. _audio-codec-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain the device handle. The handle type is ``dev_audio_codec_handles_t``:

.. code-block:: c

   typedef struct {
       esp_codec_dev_handle_t       codec_dev;      /*!< Codec device handle */
       const audio_codec_data_if_t *data_if;        /*!< Data interface handle */
       const audio_codec_ctrl_if_t *ctrl_if;        /*!< Control interface handle */
       const audio_codec_gpio_if_t *gpio_if;        /*!< GPIO interface handle */
       const audio_codec_if_t      *codec_if;       /*!< Codec interface handle */
       int16_t                      tx_aux_out_io;  /*!< Optional mirrored/inverted I2S TX auxiliary output IO */
   } dev_audio_codec_handles_t;

Related declarations are in ``esp_board_manager/devices/dev_audio_codec/dev_audio_codec.h``.
