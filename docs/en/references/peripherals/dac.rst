dac
=======

:link_to_translation:`zh_CN:[中文]`

.. _dac-intro:

Overview
--------

The ``dac`` peripheral type is used to describe an on-chip DAC channel. BMGR generates a ``periph_dac_config_t`` based on this configuration and creates the ESP-IDF DAC driver handle during initialization. This type covers the three DAC driver entry points in ``driver/dac_oneshot.h``, ``driver/dac_continuous.h``, and ``driver/dac_cosine.h``.

When configuring ``dac`` in ``board_peripherals.yaml``, select the operating mode via ``role``. After BMGR initialization, the application obtains the ``periph_dac_handle_t`` via :cpp:func:`esp_board_manager_get_periph_handle`, then calls the corresponding ESP-IDF DAC API to output voltage, voltage sequences, or cosine waves.

.. _dac-working-modes:

Supported Operating Modes
--------------------------

The classification axis for ``dac`` is ``role``. Each peripheral instance can only select one ``role``, and the runtime handle corresponds only to that mode.

- `oneshot`_
- `continuous`_
- `cosine`_

.. _dac-min-config:

Minimal Configuration
---------------------

.. _dac-oneshot:

oneshot
^^^^^^^

See complete fields: :ref:`dac-oneshot-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: dac_channel_0
        type: dac
        role: oneshot
        config:
          channel: 0

.. _dac-continuous:

continuous
^^^^^^^^^^

See complete fields: :ref:`dac-continuous-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: dac_continuous
        type: dac
        role: continuous
        config:
          chan_mask: DAC_CHANNEL_MASK_CH0
          desc_num: 8
          buf_size: 2048
          freq_hz: 1000000
          offset: 0
          clk_src: DAC_DIGI_CLK_SRC_DEFAULT
          chan_mode: DAC_CHANNEL_MODE_SIMUL

.. _dac-cosine:

cosine
^^^^^^

See complete fields: :ref:`dac-cosine-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: dac_cosine
        type: dac
        role: cosine
        config:
          channel: 0
          freq_hz: 1000
          clk_src: DAC_COSINE_CLK_SRC_DEFAULT
          atten: DAC_COSINE_ATTEN_DEFAULT
          phase: DAC_COSINE_PHASE_0
          offset: 0
          force_set_freq: false

.. _dac-mode-notes:

Mode Notes
----------

``oneshot`` creates a single DAC channel for the application to actively call ``dac_oneshot_output_voltage`` and output discrete voltage values. ``continuous`` creates a continuous-mode channel, suitable for writing cyclic or streaming data after the channel is enabled. ``cosine`` creates a cosine wave generator channel, suitable for outputting a cosine wave at a specified frequency driven by the DAC.

BMGR fills the union members of ``periph_dac_config_t`` according to ``role``. After obtaining the handle, the application must access the corresponding ``oneshot``, ``continuous``, or ``cosine`` member in ``periph_dac_handle_t`` based on the ``role`` in the configuration.

.. _dac-full-fields:

Full Field Reference
--------------------

.. _dac-oneshot-full:

oneshot Full Fields
^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # DAC oneshot mode example
    - name: dac_oneshot
      type: dac
      role: oneshot
      config:
        # DAC channel number (default: 0)
        channel: 0                       # [TO_BE_CONFIRMED] DAC channel number
        # Valid values: 0, 1
        # Channel 0: GPIO25 on ESP32, GPIO17 on ESP32S2
        # Channel 1: GPIO26 on ESP32, GPIO18 on ESP32S2

.. _dac-continuous-full:

continuous Full Fields
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # DAC continuous mode example
    - name: dac_continuous
      type: dac
      role: continuous
      config:
        # DAC channels' mask (default: DAC_CHANNEL_MASK_CH0)
        chan_mask: DAC_CHANNEL_MASK_CH0  # [TO_BE_CONFIRMED] DAC channels' mask
        # Valid values:
        # - DAC_CHANNEL_MASK_CH0
        # - DAC_CHANNEL_MASK_CH1
        # - DAC_CHANNEL_MASK_ALL

        # Number of DMA descriptors (default: 8)
        desc_num: 8
        # At least 2 descriptors are required, suggest >5

        # DMA buffer size (default: 2048)
        buf_size: 2048
        # Should be within 32~4092 bytes, typically multiple of 4

        # DAC conversion frequency (default: 1000000 Hz)
        freq_hz: 1000000                 # [TO_BE_CONFIRMED] DAC conversion frequency
        # Range depends on target and clock source

        # Data offset (default: 0)
        offset: 0
        # Range: -128~127

        # Clock source (default: DAC_DIGI_CLK_SRC_DEFAULT)
        # Valid values depend on the selected chip,
        # please refer to the enum 'soc_periph_dac_digi_clk_src_t' in 'soc/clk_tree_defs.h'
        clk_src: DAC_DIGI_CLK_SRC_DEFAULT

        # Assume the data in buffer is 'A B C D E F'
        # DAC_CHANNEL_MODE_SIMUL:
        #     channel 0: A B C D E F
        #     channel 1: A B C D E F
        # DAC_CHANNEL_MODE_ALTER:
        #     channel 0: A C E
        #     channel 1: B D F
        # Channel mode (default: DAC_CHANNEL_MODE_SIMUL)
        chan_mode: DAC_CHANNEL_MODE_SIMUL
        # Valid values:
        # - DAC_CHANNEL_MODE_SIMUL
        # - DAC_CHANNEL_MODE_ALTER

.. _dac-cosine-full:

cosine Full Fields
^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # DAC cosine mode example
    - name: dac_cosine
      type: dac
      role: cosine
      config:
        # DAC channel number (default: 0)
        channel: 0                       # [TO_BE_CONFIRMED] DAC channel number
        # Valid values: 0, 1
        # On ESP32: Channel 0: GPIO25, Channel 1: GPIO26
        # On ESP32S2: Channel 0: GPIO17, Channel 1: GPIO18

        # Cosine wave frequency (default: 1000 Hz)
        freq_hz: 1000                    # [TO_BE_CONFIRMED] Cosine wave frequency

        # Clock source (default: DAC_COSINE_CLK_SRC_DEFAULT)
        # Valid values depend on the selected chip,
        # please refer to the enum 'soc_periph_dac_cosine_clk_src_t' in 'soc/clk_tree_defs.h'
        clk_src: DAC_COSINE_CLK_SRC_DEFAULT

        # Attenuation (default: DAC_COSINE_ATTEN_DEFAULT)
        atten: DAC_COSINE_ATTEN_DEFAULT
        # Valid values:
        # - DAC_COSINE_ATTEN_DEFAULT
        # - DAC_COSINE_ATTEN_DB_0
        # - DAC_COSINE_ATTEN_DB_6
        # - DAC_COSINE_ATTEN_DB_12
        # - DAC_COSINE_ATTEN_DB_18

        # Phase (default: DAC_COSINE_PHASE_0)
        phase: DAC_COSINE_PHASE_0
        # Valid values:
        # - DAC_COSINE_PHASE_0
        # - DAC_COSINE_PHASE_180

        # DC offset (default: 0)
        offset: 0
        # Range: -128~127

        # Force set frequency (default: false)
        # Set true to force update frequency when multiple channels are used
        force_set_freq: false

Field Sources:

- YAML template: ``esp_board_manager/peripherals/periph_dac/periph_dac.yml``.
- Header file: ``esp_board_manager/peripherals/periph_dac/periph_dac.h``.

.. _dac-devices:

Applicable Devices
------------------

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Description
   * - Used directly by the application
     - Application obtains the DAC peripheral handle via ``esp_board_manager_get_periph_handle``
     - The current repository provides no device type that references a ``dac`` peripheral; data output and start/stop actions are handled by the application or test code

.. _dac-code:

Reference Code
--------------

- ``esp_board_manager/test_apps/main/periph/test_periph_dac.c``

.. _dac-boards:

Board Examples
--------------

- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_devkitc/board_peripherals.yaml``: ``oneshot`` example and commented ``continuous`` / ``cosine`` examples.

.. _dac-notes:

Notes
-----

- ``role`` must be ``oneshot``, ``continuous``, or ``cosine``; an absent or invalid value causes YAML parsing to fail.
- ``channel`` only accepts ``0`` or ``1``; the BMGR parser maps these to ``DAC_CHAN_0`` or ``DAC_CHAN_1``.
- The ``chan_mask`` for ``continuous`` only accepts ``DAC_CHANNEL_MASK_CH0``, ``DAC_CHANNEL_MASK_CH1``, or ``DAC_CHANNEL_MASK_ALL``.
- The ``atten`` and ``phase`` for ``cosine`` must use the enumeration values listed in the template.
- After modifying DAC peripheral configuration, re-run ``idf.py bmgr -b <board>``.

.. _dac-debug:

Debugging Tips
--------------

.. _dac-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to obtain the DAC peripheral handle. The handle type is ``periph_dac_handle_t``:

.. code-block:: c

   typedef union {
       dac_oneshot_handle_t     oneshot;     /*!< Oneshot mode handle */
       dac_continuous_handle_t  continuous;  /*!< Continuous mode handle */
       dac_cosine_handle_t      cosine;      /*!< Cosine wave mode handle */
   } periph_dac_handle_t;

The three members are mutually exclusive; which one is valid depends on the ``role`` field configured in ``board_peripherals.yaml`` (``oneshot``, ``continuous``, or ``cosine``).

The relevant declarations are in ``esp_board_manager/peripherals/periph_dac/periph_dac.h``.

Underlying ESP-IDF driver documentation: `Digital-to-Analog Converter (DAC) <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32/api-reference/peripherals/dac.html>`__\ .
