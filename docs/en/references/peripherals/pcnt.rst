pcnt
========

:link_to_translation:`zh_CN:[中文]`

.. _pcnt-intro:

Overview
--------

The ``pcnt`` peripheral type describes an ESP-IDF pulse counter unit, glitch filter, channels, and watch points. BMGR uses this configuration to create a PCNT unit and channels, and returns a ``periph_pcnt_handle_t``.

This peripheral is suited for counting, encoder, or testing scenarios that require direct access to the PCNT handle. Counter value reading, event callbacks, and application logic are handled by the device or application code that uses the handle.

.. _pcnt-working-modes:

Supported Operating Modes
--------------------------

``pcnt`` is currently configured as a single PCNT unit and does not split operating modes via ``role``.

- `PCNT Counter Unit`_

.. _pcnt-min-config:

Minimal Configuration
---------------------

.. _pcnt-unit:

PCNT Counter Unit
^^^^^^^^^^^^^^^^^

See complete fields: :ref:`pcnt-unit-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: pcnt_unit
        type: pcnt
        config:
          unit_config:
            low_limit: -100
            high_limit: 100
            intr_priority: 0
            flags:
              accum_count: false
          filter_config:
            max_glitch_ns: 1000
          channel_count: 1
          channel_list:
            - channel: 0
              channel_config:
                edge_gpio_num: 7
                level_gpio_num: -1
                flags:
                  invert_edge_input: false
                  invert_level_input: false
                  virt_edge_io_level: 0
                  virt_level_io_level: 0
                  io_loop_back: false
              pos_act: PCNT_CHANNEL_EDGE_ACTION_INCREASE
              neg_act: PCNT_CHANNEL_EDGE_ACTION_DECREASE
              high_act: PCNT_CHANNEL_LEVEL_ACTION_KEEP
              low_act: PCNT_CHANNEL_LEVEL_ACTION_KEEP
          watch_point_count: 3
          watch_point_list: [-100, 0, 100]

.. _pcnt-mode-notes:

Mode Description
----------------

The ``pcnt`` peripheral first creates the unit, then configures the glitch filter, then creates channels according to ``channel_count`` and sets edge/level actions. ``watch_point_list`` adds watch points to the unit; event callbacks are not registered during peripheral initialization.

.. _pcnt-full-fields:

Full Field Reference
--------------------

.. _pcnt-unit-full:

PCNT Counter Unit — Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # PCNT Peripheral Default Configuration
    # This file shows the default values used by the PCNT peripheral parser
    # Based on periph_pcnt.py parsing script

    - name: pcnt_unit
      type: pcnt
      config:
        unit_config:
          low_limit: -100  # [TO_BE_CONFIRMED] Low count limit (should be lower than 0 and should not less than 'PCNT_LL_MIN_LIM')
          high_limit: 100  # [TO_BE_CONFIRMED] High count limit (should be higher than 0 and should not exceed 'PCNT_LL_MAX_LIM')
          intr_priority: 0  # PCNT interrupt priority,
                            # if set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)

          flags:
            accum_count: false  # Whether to accumulate the count value when overflows at the high/low limit

            # The following two flags require chip support,
            # which can be determined by checking the macro definition SOC_PCNT_SUPPORT_STEP_NOTIFY
            # or by referring to the IDF Programming Guide
            en_step_notify_up: false  # Enable step notify in the positive direction
            en_step_notify_down: false  # Enable step notify in the negative direction

        filter_config:
          max_glitch_ns: 1000  # Pulse width smaller than this threshold will be treated as glitch and ignored, in the unit of ns

        channel_count: 2  # [TO_BE_CONFIRMED] Number of PCNT channels to configure, should be less than SOC_PCNT_CHANNELS_PER_UNIT
        # Configuration for each PCNT channel
        channel_list:
          # Channel 0 configuration
          - channel: 0
            channel_config:
              edge_gpio_num: -1   # [IO] GPIO number used by the edge signal, input mode with pull up enabled. Set to -1 if unused
              level_gpio_num: -1  # [IO] GPIO number used by the level signal, input mode with pull up enabled. Set to -1 if unused
              flags:
                invert_edge_input: false  # Invert the input edge signal
                invert_level_input: false  # Invert the input level signal
                virt_edge_io_level: 0  # Virtual edge IO level, 0: low, 1: high. Only valid when edge_gpio_num is set to -1
                virt_level_io_level: 0  # Virtual level IO level, 0: low, 1: high. Only valid when level_gpio_num is set to -1
                io_loop_back: false  # For debug/test, the signal output from the GPIO will be fed to the input path as well.
                                     # Note that this flag is deprecated, will be removed in IDF v6.0.
                                     # Instead, you can configure the output mode by calling gpio_config() first, and then do PCNT channel configuration.
                                     # Necessary configurations for the IO to be used as the PCNT input will be appended.

            # Edge actions for pcnt channel
            pos_act: PCNT_CHANNEL_EDGE_ACTION_DECREASE
            neg_act: PCNT_CHANNEL_EDGE_ACTION_INCREASE
            # Valid values:
            # - PCNT_CHANNEL_EDGE_ACTION_HOLD  Hold current count value
            # - PCNT_CHANNEL_EDGE_ACTION_INCREASE  Increase count value
            # - PCNT_CHANNEL_EDGE_ACTION_DECREASE  Decrease count value

            # Level actions for pcnt channel
            high_act: PCNT_CHANNEL_LEVEL_ACTION_KEEP
            low_act: PCNT_CHANNEL_LEVEL_ACTION_INVERSE
            # Valid values:
            # - PCNT_CHANNEL_LEVEL_ACTION_KEEP  Keep the current count value
            # - PCNT_CHANNEL_LEVEL_ACTION_INVERSE  Invert current count mode (increase -> decrease, decrease -> increase)
            # - PCNT_CHANNEL_LEVEL_ACTION_HOLD  Hold current count value

          # Channel 1 configuration
          - channel: 1
            channel_config:
              edge_gpio_num: 38          # [IO] GPIO number used by the edge signal
              level_gpio_num: 37         # [IO] GPIO number used by the level signal
              flags:
                invert_edge_input: false
                invert_level_input: false
                virt_edge_io_level: false
                virt_level_io_level: false
                io_loop_back: false
            pos_act: PCNT_CHANNEL_EDGE_ACTION_INCREASE
            neg_act: PCNT_CHANNEL_EDGE_ACTION_DECREASE
            high_act: PCNT_CHANNEL_LEVEL_ACTION_KEEP
            low_act: PCNT_CHANNEL_LEVEL_ACTION_INVERSE

        # Watch point configuration
        watch_point_count: 5  # Number of watch points to configure,
                              # In addition to setting 0 and +/- limit as watch points,
                              # users can also customize SOC_PCNT_THRES_POINT_PER_UNIT watch points
        watch_point_list: [-100, -50, 0, 50, 100]

.. _pcnt-devices:

Applicable Devices
------------------

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Notes
   * - ``custom``
     - Device side can reference a defined ``pcnt`` peripheral
     - Counter reading, watch point callbacks, and application semantics are handled by the custom device or application code

.. _pcnt-code:

Reference Code
--------------

- ``esp_board_manager/peripherals/periph_pcnt/periph_pcnt.c``
- ``esp_board_manager/test_apps/main/periph/test_periph_pcnt.c``

.. _pcnt-boards:

Board-Level Reference
---------------------

- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_s3_devkitc/board_peripherals.yaml``: defines the ``pcnt_unit`` test peripheral.

.. _pcnt-notes:

Notes
-----

- ``channel_count`` must not exceed the ``SOC_PCNT_CHANNELS_PER_UNIT`` supported by the target chip; BMGR truncates to the upper limit and prints a warning during initialization.
- ``watch_point_count`` must not exceed the number of watch points the target chip supports plus the boundary point capacity; BMGR truncates to the upper limit and prints a warning during initialization.
- When ``edge_gpio_num`` or ``level_gpio_num`` is set to ``-1``, the corresponding virtual IO level field is meaningful.
- After modifying the PCNT peripheral configuration, re-run ``idf.py bmgr -b <board>``.

.. _pcnt-debug:

Debugging Tips
--------------

.. _pcnt-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to retrieve the PCNT peripheral handle. The handle type is ``periph_pcnt_handle_t``:

.. code-block:: c

   typedef struct {
       pcnt_unit_handle_t     pcnt_unit;                             /*!< Handle to the PCNT unit */
       pcnt_channel_handle_t  channels[SOC_PCNT_CHANNELS_PER_UNIT];  /*!< Array of handles for PCNT channels */
   } periph_pcnt_handle_t;

``pcnt_unit`` can be passed to ``pcnt_unit_*`` APIs for counter enable, clear, and value read. The ``channels`` array can be indexed and passed to ``pcnt_channel_*`` APIs to adjust edge and level actions.

Related declarations are in ``esp_board_manager/peripherals/periph_pcnt/periph_pcnt.h``.

Underlying ESP-IDF driver documentation: `Pulse Counter (PCNT) <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/pcnt.html>`__.
