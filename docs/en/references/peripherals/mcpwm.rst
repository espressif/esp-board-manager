mcpwm
=========

:link_to_translation:`zh_CN:[中文]`

.. _mcpwm-intro:

Overview
--------

The ``mcpwm`` peripheral type is used to describe a group of MCPWM timer, operator, comparator, and generator resources. BMGR creates MCPWM resources based on this configuration and returns a ``periph_mcpwm_handle_t``.

This peripheral only creates the basic MCPWM resources. Generator event actions and comparator duty cycles are not set during peripheral initialization; they must be further configured by the device or application code using the handle.

.. _mcpwm-working-modes:

Supported Operating Modes
--------------------------

``mcpwm`` is currently configured as a single MCPWM resource group, without further splitting of operating modes through ``role``.

- `MCPWM Resource Group`_

.. _mcpwm-min-config:

Minimal Configuration
---------------------

.. _mcpwm-group:

MCPWM Resource Group
^^^^^^^^^^^^^^^^^^^^

See complete fields: :ref:`mcpwm-group-idf5`.
See complete fields: :ref:`mcpwm-group-idf6`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: mcpwm_group_0
        type: mcpwm
        config:
          timer_config:
            group_id: 0
            clk_src: MCPWM_TIMER_CLK_SRC_DEFAULT
            resolution_hz: 1000000
            count_mode: MCPWM_TIMER_COUNT_MODE_UP
            period_ticks: 20000
            intr_priority: 0
            flags:
              update_period_on_empty: false
              update_period_on_sync: false
              allow_pd: false
          operator_config:
            group_id: 0
            intr_priority: 0
            flags:
              update_gen_action_on_tez: false
              update_gen_action_on_tep: false
              update_gen_action_on_sync: false
              update_dead_time_on_tez: false
              update_dead_time_on_tep: false
              update_dead_time_on_sync: false
          comparator_configs:
            - comparator: 0
              intr_priority: 0
              flags:
                update_cmp_on_tez: true
                update_cmp_on_tep: false
                update_cmp_on_sync: false
          generator_config:
            gpio_num: 45
            flags:
              invert_pwm: false
              io_od_mode: false
              pull_up: false
              pull_down: false

.. _mcpwm-mode-notes:

Mode Notes
----------

The ``group_id`` in ``timer_config`` and ``operator_config`` should point to the same MCPWM group. ``comparator_configs`` is used to create one or more comparators; alternatively, use the ``comparator_config`` syntax in the template to configure a single comparator. ``generator_config`` only binds the PWM output GPIO and GPIO-related flags; it does not set waveform actions.

The MCPWM templates for IDF 5 and IDF 6 are mostly identical, with a difference in ``generator_config.flags``: the IDF 5 template includes ``io_loop_back``, while the IDF 6 template does not include this field.

.. _mcpwm-full-fields:

Full Field Reference
--------------------

.. _mcpwm-group-idf5:

IDF 5 MCPWM Resource Group Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # MCPWM Peripheral Default Configuration
    # This file shows the default values used by the MCPWM peripheral parser
    # Based on periph_mcpwm.py parsing script
    # Version: 1.1.0 - Added support for multiple comparators

    - name: mcpwm_group_0
      type: mcpwm
      config:
        # Timer configuration
        timer_config:
          # Group ID (default: 0), must be less than 'SOC_MCPWM_GROUPS'
          # Please check the 'SOC_MCPWM_GROUPS' in 'soc/soc_caps.h' for valid values
          group_id: 0

          # Clock source (default: MCPWM_TIMER_CLK_SRC_DEFAULT)
          # Valid values depend on the selected chip,
          # please refer to the enum 'soc_periph_mcpwm_timer_clk_src_t' in 'soc/clk_tree_defs.h'
          clk_src: MCPWM_TIMER_CLK_SRC_DEFAULT

          # Timer resolution in Hz (default: 1000000, 1MHz = 1us per tick)
          # The step size of each count tick equals to (1 / resolution_hz) seconds
          resolution_hz: 1000000          # [TO_BE_CONFIRMED] Timer resolution in Hz

          # Count mode (default: MCPWM_TIMER_COUNT_MODE_UP)
          count_mode: MCPWM_TIMER_COUNT_MODE_UP
          # Valid values:
          # - MCPWM_TIMER_COUNT_MODE_PAUSE
          # - MCPWM_TIMER_COUNT_MODE_UP
          # - MCPWM_TIMER_COUNT_MODE_DOWN
          # - MCPWM_TIMER_COUNT_MODE_UP_DOWN

          # Number of count ticks within a period (default: 20000, 20ms for 50Hz PWM)
          period_ticks: 20000             # [TO_BE_CONFIRMED] Number of count ticks within a period

          # MCPWM timer interrupt priority
          # If set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)
          intr_priority: 0

          # Timer flags
          flags:
            # Whether to update period when timer counts to zero (default: false)
            update_period_on_empty: false

            # Whether to update period on sync event (default: false)
            update_period_on_sync: false

            # Allow power down (default: false)
            # When this flag set, the driver will backup/restore the MCPWM registers before/after entering/exit sleep mode.
            # By this approach, the system can power off MCPWM's power domain.
            # This can save power, but at the expense of more RAM being consumed.
            allow_pd: false

        # Operator configuration
        operator_config:
          # Group ID, must be less than 'SOC_MCPWM_GROUPS' (default: 0)
          # Please check the 'SOC_MCPWM_GROUPS' in 'soc/soc_caps.h' for valid values
          # Operator and timer should reside in the same group
          group_id: 0

          # Interrupt priority (default: 0)
          intr_priority: 0

          # Operator flags
          flags:
            # Whether to update generator action when timer counts to zero (default: false)
            update_gen_action_on_tez: false

            # Whether to update generator action when timer counts to peak (default: false)
            update_gen_action_on_tep: false

            # Whether to update generator action on sync event (default: false)
            update_gen_action_on_sync: false

            # Whether to update dead time when timer counts to zero (default: false)
            update_dead_time_on_tez: false

            # Whether to update dead time when timer counts to peak (default: false)
            update_dead_time_on_tep: false

            # Whether to update dead time on sync event (default: false)
            update_dead_time_on_sync: false

        # Comparator configurations, support for multiple comparators,
        # the supported number of comparators for each operator can be determined by checking the 'SOC_MCPWM_COMPARATORS_PER_OPERATOR' in soc/soc_caps.h
        # Use either 'comparator_config' for single comparator or 'comparator_configs' for multiple comparators
        comparator_configs:

          # First comparator configuration
          - comparator: 0

            # Interrupt priority (default: 0)
            intr_priority: 0
            flags:
              # Whether to update compare value when timer counts to zero (default: true)
              update_cmp_on_tez: true

              # Whether to update compare value when timer counts to peak (default: false)
              update_cmp_on_tep: false

              # Whether to update compare value on sync event (default: false)
              update_cmp_on_sync: false

          # Second comparator configuration (optional)
          # - comparator: 1
          #   intr_priority: 0
          #   flags:
          #     update_cmp_on_tez: true
          #     update_cmp_on_tep: false
          #     update_cmp_on_sync: false

        # Alternative: single comparator configuration
        # comparator_config:
        #   intr_priority: 0
        #   flags:
        #     update_cmp_on_tez: true
        #     update_cmp_on_tep: false
        #     update_cmp_on_sync: false

        # Generator configuration
        generator_config:
          # GPIO pin number for PWM output (required, no default - must be >= 0)
          gpio_num: 0                     # [IO] GPIO pin number for PWM output

          # Generator flags
          flags:
            # Whether to invert PWM output signal (default: false)
            invert_pwm: false

            # IO loop back for debug/test (default: false)
            io_loop_back: false

            # Configure GPIO as open-drain mode (default: false)
            io_od_mode: false

            # Whether to pull up internally (default: false)
            pull_up: false

            # Whether to pull down internally (default: false)
            pull_down: false

.. _mcpwm-group-idf6:

IDF 6 MCPWM Resource Group Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # MCPWM Peripheral Default Configuration
    # This file shows the default values used by the MCPWM peripheral parser
    # Based on periph_mcpwm.py parsing script
    # Version: 1.1.0 - Added support for multiple comparators

    - name: mcpwm_group_0
      type: mcpwm
      config:
        # Timer configuration
        timer_config:
          # Group ID (default: 0), must be less than 'SOC_MCPWM_GROUPS'
          # Please check the 'SOC_MCPWM_GROUPS' in 'soc/soc_caps.h' for valid values
          group_id: 0

          # Clock source (default: MCPWM_TIMER_CLK_SRC_DEFAULT)
          # Valid values depend on the selected chip,
          # please refer to the enum 'soc_periph_mcpwm_timer_clk_src_t' in 'soc/clk_tree_defs.h'
          clk_src: MCPWM_TIMER_CLK_SRC_DEFAULT

          # Timer resolution in Hz (default: 1000000, 1MHz = 1us per tick)
          # The step size of each count tick equals to (1 / resolution_hz) seconds
          resolution_hz: 1000000          # [TO_BE_CONFIRMED] Timer resolution in Hz

          # Count mode (default: MCPWM_TIMER_COUNT_MODE_UP)
          count_mode: MCPWM_TIMER_COUNT_MODE_UP
          # Valid values:
          # - MCPWM_TIMER_COUNT_MODE_PAUSE
          # - MCPWM_TIMER_COUNT_MODE_UP
          # - MCPWM_TIMER_COUNT_MODE_DOWN
          # - MCPWM_TIMER_COUNT_MODE_UP_DOWN

          # Number of count ticks within a period (default: 20000, 20ms for 50Hz PWM)
          period_ticks: 20000             # [TO_BE_CONFIRMED] Number of count ticks within a period

          # MCPWM timer interrupt priority
          # If set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)
          intr_priority: 0

          # Timer flags
          flags:
            # Whether to update period when timer counts to zero (default: false)
            update_period_on_empty: false

            # Whether to update period on sync event (default: false)
            update_period_on_sync: false

            # Allow power down (default: false)
            # When this flag set, the driver will backup/restore the MCPWM registers before/after entering/exit sleep mode.
            # By this approach, the system can power off MCPWM's power domain.
            # This can save power, but at the expense of more RAM being consumed.
            allow_pd: false

        # Operator configuration
        operator_config:
          # Group ID, must be less than 'SOC_MCPWM_GROUPS' (default: 0)
          # Please check the 'SOC_MCPWM_GROUPS' in 'soc/soc_caps.h' for valid values
          # Operator and timer should reside in the same group
          group_id: 0

          # Interrupt priority (default: 0)
          intr_priority: 0

          # Operator flags
          flags:
            # Whether to update generator action when timer counts to zero (default: false)
            update_gen_action_on_tez: false

            # Whether to update generator action when timer counts to peak (default: false)
            update_gen_action_on_tep: false

            # Whether to update generator action on sync event (default: false)
            update_gen_action_on_sync: false

            # Whether to update dead time when timer counts to zero (default: false)
            update_dead_time_on_tez: false

            # Whether to update dead time when timer counts to peak (default: false)
            update_dead_time_on_tep: false

            # Whether to update dead time on sync event (default: false)
            update_dead_time_on_sync: false

        # Comparator configurations, support for multiple comparators,
        # the supported number of comparators for each operator can be determined by checking the 'SOC_MCPWM_COMPARATORS_PER_OPERATOR' in soc/soc_caps.h
        # Use either 'comparator_config' for single comparator or 'comparator_configs' for multiple comparators
        comparator_configs:

          # First comparator configuration
          - comparator: 0

            # Interrupt priority (default: 0)
            intr_priority: 0
            flags:
              # Whether to update compare value when timer counts to zero (default: true)
              update_cmp_on_tez: true

              # Whether to update compare value when timer counts to peak (default: false)
              update_cmp_on_tep: false

              # Whether to update compare value on sync event (default: false)
              update_cmp_on_sync: false

          # Second comparator configuration (optional)
          # - comparator: 1
          #   intr_priority: 0
          #   flags:
          #     update_cmp_on_tez: true
          #     update_cmp_on_tep: false
          #     update_cmp_on_sync: false

        # Alternative: single comparator configuration
        # comparator_config:
        #   intr_priority: 0
        #   flags:
        #     update_cmp_on_tez: true
        #     update_cmp_on_tep: false
        #     update_cmp_on_sync: false

        # Generator configuration
        generator_config:
          # GPIO pin number for PWM output (required, no default - must be >= 0)
          gpio_num: 0                     # [IO] GPIO pin number for PWM output

          # Generator flags
          flags:
            # Whether to invert PWM output signal (default: false)
            invert_pwm: false

            # Configure GPIO as open-drain mode (default: false)
            io_od_mode: false

            # Whether to pull up internally (default: false)
            pull_up: false

            # Whether to pull down internally (default: false)
            pull_down: false

.. _mcpwm-devices:

Applicable Devices
------------------

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Description
   * - ``custom``
     - The device side can reference a defined ``mcpwm`` peripheral
     - Waveform actions, duty cycle updates, and motor control logic are handled by the custom device or application code

.. _mcpwm-code:

Reference Code
--------------

- ``esp_board_manager/peripherals/periph_mcpwm/idf5/periph_mcpwm.c``
- ``esp_board_manager/peripherals/periph_mcpwm/idf6/periph_mcpwm.c``
- ``esp_board_manager/test_apps/main/periph/test_periph_mcpwm.c``

.. _mcpwm-boards:

Board Examples
--------------

- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_s3_devkitc/board_peripherals.yaml``: Defines the ``mcpwm_group_0`` test peripheral.

.. _mcpwm-notes:

Notes
-----

- ``operator_config.group_id`` must be consistent with ``timer_config.group_id``.
- The number of comparators is limited by ``SOC_MCPWM_COMPARATORS_PER_OPERATOR``; BMGR validates the comparator count during initialization.
- ``periph_mcpwm_init`` only creates the timer, operator, comparator, and generator; it does not set generator event actions or comparator duty cycles.
- After modifying MCPWM peripheral configuration, re-run ``idf.py bmgr -b <board>``.

.. _mcpwm-debug:

Debugging Tips
--------------

.. _mcpwm-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to obtain the MCPWM peripheral handle. The handle type is ``periph_mcpwm_handle_t``:

.. code-block:: c

   typedef struct {
       mcpwm_timer_handle_t  timer;                                            /*!< MCPWM timer handle */
       mcpwm_oper_handle_t   operator;                                         /*!< MCPWM operator handle */
       mcpwm_cmpr_handle_t   comparators[SOC_MCPWM_COMPARATORS_PER_OPERATOR];  /*!< MCPWM comparator handles */
       mcpwm_gen_handle_t    generator;                                        /*!< MCPWM generator handle */
   } periph_mcpwm_handle_t;

Each member can be passed directly to the corresponding ``mcpwm_timer_*``, ``mcpwm_operator_*``, ``mcpwm_comparator_*``, ``mcpwm_generator_*`` ESP-IDF APIs for dynamic duty cycle, period, and fault protection control.

The relevant declarations are in ``esp_board_manager/peripherals/periph_mcpwm/periph_mcpwm.h``.

Underlying ESP-IDF driver documentation: `Motor Control Pulse Width Modulator (MCPWM) <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/mcpwm.html>`__\ .
