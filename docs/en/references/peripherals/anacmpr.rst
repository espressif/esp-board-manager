anacmpr
===========

:link_to_translation:`zh_CN:[中文]`

.. _anacmpr-intro:

Overview
--------

The ``anacmpr`` peripheral type describes an on-chip analog comparator unit. BMGR generates an ``ana_cmpr_config_t`` from this configuration and calls the ESP-IDF ``driver/ana_cmpr.h`` to create the analog comparator unit.

After BMGR initialization, the application retrieves the ``periph_anacmpr_handle_t`` via :cpp:func:`esp_board_manager_get_periph_handle` and then uses the ESP-IDF analog comparator APIs to configure the internal reference, level-crossing callback, debounce, and start/stop actions.

.. _anacmpr-working-modes:

Supported Operating Modes
--------------------------

``anacmpr`` does not use ``role`` or ``format`` to split modes. The configuration axis is a single analog comparator unit.

- `Analog Comparator Unit`_

.. _anacmpr-min-config:

Minimal Configuration
---------------------

.. _anacmpr-unit:

Analog Comparator Unit
^^^^^^^^^^^^^^^^^^^^^^^^

See complete fields: :ref:`anacmpr-unit-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: anacmpr_unit_0
        type: anacmpr
        config:
          unit: 0
          ref_src: ANA_CMPR_REF_SRC_INTERNAL
          cross_type: ANA_CMPR_CROSS_ANY
          clk_src: ANA_CMPR_CLK_SRC_DEFAULT
          intr_priority: 0

.. _anacmpr-mode-notes:

Mode Description
----------------

``anacmpr`` creates one analog comparator unit. ``ref_src`` selects the internal or external reference input, ``cross_type`` selects the crossing direction that triggers the event, and ``intr_priority`` configures the interrupt priority.

The current BMGR parser only accepts ``unit: 0``. The internal reference voltage, debounce, and event callbacks are not configured in the peripheral YAML; they must be set by the application after obtaining the handle via the ESP-IDF analog comparator APIs.

.. _anacmpr-full-fields:

Full Field Reference
--------------------

.. _anacmpr-unit-full:

Analog Comparator Unit — Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Analog Comparator example with internal reference
    - name: anacmpr_unit_0
      type: anacmpr
      config:
        # Analog Comparator unit number, must be not less than 0 and less than 'SOC_ANA_CMPR_NUM'  (default: 0)
        # Please check the 'SOC_ANA_CMPR_NUM' in 'soc/soc_caps.h' for valid values
        unit: 0  # [TO_BE_CONFIRMED] Analog Comparator unit

        # Reference source type (default: ANA_CMPR_REF_SRC_INTERNAL)
        ref_src: ANA_CMPR_REF_SRC_INTERNAL  # [TO_BE_CONFIRMED] Reference source type
        # Valid values:
        # - ANA_CMPR_REF_SRC_INTERNAL: Use internal reference voltage (divided from VDD)
        # - ANA_CMPR_REF_SRC_EXTERNAL: Use external reference from GPIO pin

        # Cross type for interrupt triggering (default: ANA_CMPR_CROSS_ANY)
        cross_type: ANA_CMPR_CROSS_ANY
        # Valid values:
        # - ANA_CMPR_CROSS_POS: Trigger on positive crossing (source > reference)
        # - ANA_CMPR_CROSS_NEG: Trigger on negative crossing (source < reference)
        # - ANA_CMPR_CROSS_ANY: Trigger on both positive and negative crossing

        # Clock source (default: ANA_CMPR_CLK_SRC_DEFAULT)
        clk_src: ANA_CMPR_CLK_SRC_DEFAULT
        # Please check the 'soc_periph_ana_cmpr_clk_src_t' in 'soc/clk_tree_defs.h' for valid values

        # Interrupt priority (default: 0)
        # If set to 0, the driver will automatically select a relative low priority (1,2,3)
        intr_priority: 0

Field sources:

- YAML template: ``esp_board_manager/peripherals/periph_anacmpr/periph_anacmpr.yml``.
- Header file: ``esp_board_manager/peripherals/periph_anacmpr/periph_anacmpr.h``.

.. _anacmpr-devices:

Applicable Devices
------------------

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Notes
   * - Direct application use
     - Application retrieves the analog comparator peripheral handle via ``esp_board_manager_get_periph_handle``
     - No device type in the current repository references the ``anacmpr`` peripheral; reference voltage, debounce, callbacks, and start/stop are handled by application or test code

.. _anacmpr-code:

Reference Code
--------------

- ``esp_board_manager/test_apps/main/periph/test_periph_anacmpr.c``

.. _anacmpr-boards:

Board-Level Reference
---------------------

- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_p4_core/board_peripherals.yaml``: ``anacmpr_unit_0`` configuration, along with ``gpio_monitor`` for observing crossing events during testing.

.. _anacmpr-notes:

Notes
-----

- The current parser only accepts ``unit: 0``.
- ``ref_src`` only accepts ``ANA_CMPR_REF_SRC_INTERNAL`` or ``ANA_CMPR_REF_SRC_EXTERNAL``.
- ``cross_type`` only accepts ``ANA_CMPR_CROSS_POS``, ``ANA_CMPR_CROSS_NEG``, or ``ANA_CMPR_CROSS_ANY``.
- ``clk_src`` currently only accepts ``ANA_CMPR_CLK_SRC_DEFAULT``.
- The internal reference voltage, debounce, and event callbacks must be configured in application code and must not be written into the ``anacmpr`` peripheral ``config``.
- After modifying the analog comparator peripheral configuration, re-run ``idf.py bmgr -b <board>``.

.. _anacmpr-debug:

Debugging Tips
--------------

.. _anacmpr-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to retrieve the analog comparator peripheral handle. The handle type is ``periph_anacmpr_handle_t``:

.. code-block:: c

   typedef struct {
       ana_cmpr_handle_t  cmpr_handle;  /*!< Analog comparator handle */
       ana_cmpr_unit_t    unit;         /*!< Analog comparator unit */
   } periph_anacmpr_handle_t;

``cmpr_handle`` can be passed to the ESP-IDF ``ana_cmpr_*`` APIs. ``unit`` identifies the current comparator controller number.

Related declarations are in ``esp_board_manager/peripherals/periph_anacmpr/periph_anacmpr.h``.

Underlying ESP-IDF driver documentation: `Analog Comparator <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32p4/api-reference/peripherals/ana_cmpr.html>`__.
