sdm
=======

:link_to_translation:`zh_CN:[中文]`

.. _sdm-intro:

Overview
--------

The ``sdm`` peripheral type describes an on-chip Sigma Delta Modulation output channel. BMGR generates an ``sdm_config_t`` from this configuration and calls the ESP-IDF ``driver/sdm.h`` to create the SDM channel.

BMGR is only responsible for creating and storing the SDM channel handle. After obtaining the ``periph_sdm_handle_t``, the application can call ESP-IDF SDM APIs to enable the channel, set the pulse density, and close the channel when finished.

.. _sdm-working-modes:

Supported Operating Modes
--------------------------

``sdm`` does not use ``role`` or ``format`` to split modes. The configuration axis is a single SDM output channel.

- `SDM Output Channel`_

.. _sdm-min-config:

Minimal Configuration
---------------------

.. _sdm-channel:

SDM Output Channel
^^^^^^^^^^^^^^^^^^

See complete fields: :ref:`sdm-channel-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: sdm
        type: sdm
        config:
          gpio_num: 4
          clk_src: "SDM_CLK_SRC_DEFAULT"
          sample_rate_hz: 1000000
          invert_out: false
          io_loop_back: false

.. _sdm-mode-notes:

Mode Description
----------------

``sdm`` creates one output channel. The output GPIO is specified by ``gpio_num``. ``sample_rate_hz`` and ``clk_src`` determine the channel sampling clock configuration, and ``invert_out`` controls output polarity.

``io_loop_back`` and ``allow_pd`` are flags that vary with the ESP-IDF version: IDF v5.x generates ``io_loop_back``, while IDF v6.x and later generate ``allow_pd``. When the corresponding field is configured on an unsupported ESP-IDF version, the parser ignores that field and prints a warning.

.. _sdm-full-fields:

Full Field Reference
--------------------

.. _sdm-channel-full:

SDM Output Channel — Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: sdm
      type: sdm
      config:
        # GPIO pin number for SDM output (required, no default - must be >= 0)
        gpio_num: 0                     # [IO] GPIO pin number for SDM output

        # Clock source (default: SDM_CLK_SRC_DEFAULT)
        # Valid values depend on the selected chip,
        # please refer to the enum 'soc_periph_sdm_clk_src_t' in 'soc/clk_tree_defs.h'
        clk_src: "SDM_CLK_SRC_DEFAULT"

        # Sample rate in Hz (default: 1000000)
        sample_rate_hz: 1000000         # [TO_BE_CONFIRMED] Sample rate in Hz

        # Invert output signal (default: false)
        invert_out: false

        # IO loop back for debug/test (IDF < 6.0 only; ignored with a warning on IDF >= 6.0)
        io_loop_back: false

        # Allow SDM power domain to power off during sleep (IDF >= 6.0 only; ignored with a warning on IDF < 6.0)
        allow_pd: false

Field sources:

- YAML template: ``esp_board_manager/peripherals/periph_sdm/periph_sdm.yml``.
- Header file: ``esp_board_manager/peripherals/periph_sdm/periph_sdm.h``.

.. _sdm-devices:

Applicable Devices
------------------

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Notes
   * - Direct application use
     - Application retrieves the SDM peripheral handle via ``esp_board_manager_get_periph_handle``
     - No device type in the current repository references the ``sdm`` peripheral; pulse density setting is handled by application or test code

.. _sdm-code:

Reference Code
--------------

- ``esp_board_manager/test_apps/main/periph/test_periph_sdm.c``

.. _sdm-boards:

Board-Level Reference
---------------------

- ``esp_boards/esp32_c3_lyra/board_peripherals.yaml``: ``sdm`` output channel configuration.

.. _sdm-notes:

Notes
-----

- ``gpio_num`` must be a non-negative GPIO number; the parser rejects values less than 0.
- ``sample_rate_hz`` must be greater than 0.
- ``invert_out``, ``io_loop_back``, and ``allow_pd`` must be boolean values.
- Under IDF v5.x, ``allow_pd`` is not written into the generated configuration. Under IDF v6.x and later, ``io_loop_back`` is not written into the generated configuration.
- After modifying the SDM peripheral configuration, re-run ``idf.py bmgr -b <board>``.

.. _sdm-debug:

Debugging Tips
--------------

.. _sdm-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to retrieve the SDM peripheral handle. The handle type is ``periph_sdm_handle_t``:

.. code-block:: c

   typedef struct {
       sdm_channel_handle_t  sdm_chan;  /*!< SDM channel handle */
   } periph_sdm_handle_t;

``sdm_chan`` can be passed to ``sdm_channel_set_pulse_density`` and other ESP-IDF SDM APIs to dynamically adjust the output pulse density.

Related declarations are in ``esp_board_manager/peripherals/periph_sdm/periph_sdm.h``.

Underlying ESP-IDF driver documentation: `Sigma-Delta Modulator (SDM) <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/sdm.html>`__.
