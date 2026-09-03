Custom Device (``custom``)
==========================

:link_to_translation:`zh_CN:[中文]`

.. _custom-intro:

Overview
--------

The ``custom`` device integrates board-specific hardware or software objects into the BMGR unified device management pipeline. Unlike other device types, ``custom`` has no built-in driver logic — BMGR **automatically generates** a dedicated configuration struct from the fields under ``config:`` in the YAML and includes it in the unified lifecycle management.

During initialization, BMGR looks up the registered init entry by device ``name``:

- **Init entry registered**: calls the board-level init function; the user handle returned by the function is retrieved via :cpp:func:`esp_board_manager_get_device_handle`.
- **No init entry registered**: initialization does not report an error, but the handle is ``NULL``; the configuration struct is still accessible directly via :cpp:func:`esp_board_manager_get_device_config`.

Typical use cases:

- Peripheral chips with custom drivers (PMICs, sensors, actuators, etc.) where board code provides the init function.
- Exposing board-level configuration parameters (I2C address, GPIO number, default values, etc.) to the application layer without any additional initialization logic.

.. _custom-min:

Minimal Configuration
---------------------

See complete fields: :ref:`custom-full`.

``custom`` does not require additional ``board_peripherals.yaml`` entries; peripherals are referenced on demand.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: axp2101_power_manager
        chip: axp2101
        type: custom
        config:
          frequency: 400000
          i2c_addr: 0x34
        peripherals:
          - name: i2c_master

.. _custom-codegen:

Code Generation Output
----------------------

After running ``idf.py bmgr -b <board>``, BMGR generates a dedicated configuration struct and its initialization values for each ``type: custom`` entry in the build artifact ``components/gen_bmgr_codes/gen_board_device_custom.h``.

Naming Convention
^^^^^^^^^^^^^^^^^

The struct is named ``dev_custom_{sanitized_name}_config_t``, where ``{sanitized_name}`` is obtained by replacing all illegal C identifier characters (non-letters, non-digits, non-underscores) in the device ``name`` with ``_``; an extra ``_`` prefix is added when the name starts with a digit.

Examples:

- ``name: axp2101_power_manager`` → ``dev_custom_axp2101_power_manager_config_t``
- ``name: my-sensor`` → ``dev_custom_my_sensor_config_t``
- ``name: 2channel`` → ``dev_custom__2channel_config_t``

Fixed Fields
^^^^^^^^^^^^

Each generated struct always starts with three descriptor fields whose values come from the top-level YAML attributes:

.. code-block:: c

    const char *name;  /*!< Device name */
    const char *type;  /*!< "custom" */
    const char *chip;  /*!< Chip name, or "unknown" if omitted */

User Configuration Fields
^^^^^^^^^^^^^^^^^^^^^^^^^

Each key-value pair under ``config:`` generates one struct field. The field name is derived from the key using the same sanitization rule; the field type is automatically inferred from the value:

.. list-table::
   :header-rows: 1
   :widths: 20 20 60

   * - YAML value type
     - C type
     - Description
   * - ``bool``
     - ``bool``
     - ``true`` / ``false``
   * - Integer
     - ``int8_t`` → ``uint8_t`` → ``int16_t`` → ``uint16_t`` → ``int32_t`` → ``uint32_t``
     - Smallest fitting type selected in order; signed types are preferred
   * - Floating-point
     - ``float``
     - Any decimal number
   * - String
     - ``const char *``
     - Literal stored in rodata
   * - Dict (nested object)
     - Inline sub-struct
     - Generated recursively; sub-struct name is ``{parent}_{key}_t``
   * - List (scalar types)
     - C array of the corresponding type
     - e.g. ``[1, 2, 3]`` → ``int8_t field[3]``
   * - List (dicts)
     - Array of sub-structs
     - All dict keys are merged into one struct before generating the array
   * - Other
     - ``void *``
     - Complex types degrade to pointer; value is ``NULL``

.. note::

   The ``peripherals`` key inside ``config:`` is reserved for the peripheral list and will not be generated as a regular field.

Full Generation Example
^^^^^^^^^^^^^^^^^^^^^^^

The YAML below is taken from the ``motor_driver`` entry in ``test_apps/components/test_board_e/board_devices.yaml``. It shows how nested dicts, lists of dicts, and lists of scalars map to C structs:

.. code-block:: yaml

    - name: motor_driver
      chip: mx16161
      type: custom
      config:
        motors:
          - motor_id: 1
            gpio_motor_ina: 26
            gpio_motor_inb: 27
          - motor_id: 2
            gpio_motor_ina: 28
            gpio_motor_inb: 29
          - motor_id: 3
            gpio_motor_ina: 23
            gpio_motor_inb: 22
        test_bool: true
        test_int: 123
        test_float: 3.14
        test_string: "hello"
        test_int_list: [1, 2, 3, 4, 5]
        test_dict:
          sub_val_1: 10
          sub_val_2: "nested"

After running ``idf.py bmgr -b <board>``, the typedefs generated in ``gen_board_device_custom.h`` (actual output, excerpted):

.. code-block:: c

    typedef struct {
        int8_t       motor_id;
        int8_t       gpio_motor_ina;
        int8_t       gpio_motor_inb;
    } dev_custom_motor_driver_motors_t;

    typedef struct {
        int8_t       sub_val_1;
        const char  *sub_val_2;
    } dev_custom_motor_driver_test_dict_t;

    typedef struct {
        const char *name;           /*!< Custom device name */
        const char *type;           /*!< Device type: "custom" */
        const char *chip;           /*!< Chip name */
        dev_custom_motor_driver_motors_t  motors[3];
        bool         test_bool;
        int8_t       test_int;
        float        test_float;
        const char  *test_string;
        int8_t       test_int_list[5];
        dev_custom_motor_driver_test_dict_t test_dict;
    } dev_custom_motor_driver_config_t;

The following rules can be verified: all integers are inferred as ``int8_t`` (values within the signed range select signed types first); the ``motors`` list of dicts expands to ``dev_custom_motor_driver_motors_t motors[3]``; the ``test_dict`` nested dict generates a sub-struct of the same name; since this device declares no ``peripherals:``, no peripheral fields appear in the top-level struct.

Peripheral Fields
^^^^^^^^^^^^^^^^^

When peripherals are declared under the top-level ``peripherals:``, peripheral-related fields are appended to the end of the struct:

- **Single peripheral**: appends ``uint8_t peripheral_count`` and ``const char *peripheral_name``.
- **Multiple peripherals**: appends ``uint8_t peripheral_count`` and ``const char *peripheral_names[DEV_CUSTOM_MAX_PERIPHERALS]``.

The parser-internal constant ``DEV_CUSTOM_MAX_PERIPHERALS`` is fixed at 4. The parser rejects a ``peripherals:`` list with more than four entries.

.. _custom-factory:

Factory Functions
-----------------

To have BMGR automatically call board-level driver code during initialization, implement init/deinit functions in the board source file and register them with the ``CUSTOM_DEVICE_IMPLEMENT`` macro.

If the implementation depends on a third-party chip-driver component, place the driver header, the generated config struct, init/deinit, and ``CUSTOM_DEVICE_IMPLEMENT`` inside the same ``#if __has_include`` block. After a downstream ``gen_skip`` of this device, that component is no longer written to ``idf_component.yml``, but the board source is still compiled. ``__has_include`` cannot guard a missing generated struct when there is no separate component header. See :doc:`/programming-guide/board-directory` for the full convention.

.. code-block:: c

    #if __has_include("my_sensor.h")
    #include "my_sensor.h"
    #include "dev_custom.h"
    #include "gen_board_device_custom.h"  /* Generated configuration struct header */

    static int my_sensor_init(void *cfg, int cfg_size, void **device_handle)
    {
        dev_custom_my_sensor_config_t *config = (dev_custom_my_sensor_config_t *)cfg;

        /* Initialize hardware using the fields in config */
        my_sensor_handle_t *handle = malloc(sizeof(my_sensor_handle_t));
        if (!handle) {
            return -1;
        }
        /* ... initialize the driver ... */
        *device_handle = handle;
        return 0;
    }

    static int my_sensor_deinit(void *device_handle)
    {
        free(device_handle);
        return 0;
    }

    /* The first argument must exactly match the device name in board_devices.yaml */
    CUSTOM_DEVICE_IMPLEMENT(my_sensor, my_sensor_init, my_sensor_deinit);
    #endif  /* __has_include("my_sensor.h") */

``CUSTOM_DEVICE_IMPLEMENT`` uses GCC attributes to place the descriptor into a special linker section (``.esp_board_entries_desc``). At runtime, BMGR performs a linear scan of this section to find the init/deinit functions by device name.

CMakeLists.txt Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^

``CUSTOM_DEVICE_IMPLEMENT`` depends on the linker retaining the symbols in its linker section. The requirement varies by file location:

- **Placed in the board directory** (alongside ``board_devices.yaml``): the ``gen_bmgr_codes`` component CMakeLists.txt generated by BMGR already sets ``WHOLE_ARCHIVE``, so no manual action is needed. ``m5stack_cores3/power_manager.c`` uses this approach.

- **Placed in a standalone application component**: that component must set ``WHOLE_ARCHIVE`` in ``idf_component_register``, otherwise the linker will discard the unreferenced linker section contents:

  .. code-block:: cmake

      idf_component_register(
          SRCS "my_device.c"
          INCLUDE_DIRS "."
          WHOLE_ARCHIVE
      )

.. _custom-app-access:

Application-Side Access
------------------------

After BMGR initialization, access ``custom`` devices as follows.

**Get configuration struct** (available whether or not an init entry is registered):

.. code-block:: c

    #include "gen_board_device_custom.h"

    dev_custom_axp2101_power_manager_config_t *cfg = NULL;
    esp_err_t ret = esp_board_manager_get_device_config(
            "axp2101_power_manager", (void **)&cfg);
    if (ret == ESP_OK) {
        uint32_t freq = cfg->frequency;
        uint8_t  addr = cfg->i2c_addr;
    }

**Get user handle** (valid only when an init entry is registered and the init function succeeds):

.. code-block:: c

    my_sensor_handle_t *handle = NULL;
    esp_err_t ret = esp_board_manager_get_device_handle(
            "my_sensor", (void **)&handle);

.. warning::

    When no entry with a matching name is registered, ``esp_board_manager_get_device_handle`` returns an error rather than ``NULL``, because the internal handle itself is ``NULL``. **Config-only** usage should use only :cpp:func:`esp_board_manager_get_device_config` and must not call :cpp:func:`esp_board_manager_get_device_handle`.

.. _custom-full:

All Fields
----------

.. code-block:: yaml

    devices:
      - name: my_custom_device          # Required; unique identifier. The first argument of the driver registration macro must match this
        type: custom                    # Fixed value
        chip: esp32s3                   # Optional; chip name. Defaults to "unknown" in the struct when omitted
        version: 1.0.0                  # Optional; YAML schema identifier. Omitting it is equivalent to default
        init_skip: false                # Optional; when true, esp_board_manager_init() skips this device
        config:
          sensor_id: 0x01               # int → int8_t (0 ≤ value ≤ 127)
          sample_rate: 100              # int → int8_t
          enable_filter: true           # bool → bool
          filter_cutoff: 50.5           # float → float
          timeout_ms: 1000              # int → int16_t
          unit: "celsius"               # str → const char *
        peripherals:                    # Optional; references declared peripherals
          - name: i2c_master
        depends_on: []                  # Optional; initialization dependencies (list of device names)
        dependencies:                   # Optional; additional component dependencies
          esp_driver_i2c:
            require: public
            version: "^2.0.0"

.. _custom-deps:

Component Dependencies
----------------------

``custom`` has no fixed dependencies. Declare driver components in the device's ``dependencies`` field as needed, such as ``esp_driver_i2c``, ``esp_driver_gpio``, ``esp_driver_ledc``, or ``esp_driver_adc``.

.. _custom-peripherals:

Required Peripherals
--------------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - Required
     - Purpose
   * - Any supported peripheral
     - Determined by the peripheral itself
     - Depends on board implementation
     - Board hardware resources referenced in the init function
   * - None
     - None
     - Optional
     - Config-only usage; no peripheral handle needed

.. _custom-code:

Reference Code
--------------

- ``esp_board_manager/test_apps/main/test_dev_custom.c``
- ``esp_board_manager/test_apps/components/test_board_e/board_devices.yaml``
- ``esp_board_manager/devices/dev_custom/dev_custom.c``
- ``esp_board_manager/devices/dev_custom/dev_custom.h``

.. _custom-boards:

Board Reference
---------------

- ``m5stack_boards/m5stack_cores3/board_devices.yaml`` + ``power_manager.c``: ``axp2101_power_manager``, with full init/deinit entry registered, driving the AXP2101 PMIC via fields such as ``config->i2c_addr`` and ``config->peripheral_name``.
- ``esp_board_manager/test_apps/components/test_board_e/board_devices.yaml``: Complete test case with nested structs, lists, and lists of dicts.

.. _custom-notes:

Notes
-----

- For common YAML field rules, see :doc:`/programming-guide/yaml-rules`.
- The first argument of ``CUSTOM_DEVICE_IMPLEMENT`` must exactly match the device ``name``, case-sensitively.
- Components containing ``CUSTOM_DEVICE_IMPLEMENT`` must set ``WHOLE_ARCHIVE`` in CMakeLists.txt, otherwise the linker will discard the descriptor.
- When no matching init entry is registered, initialization will not fail, but :cpp:func:`esp_board_manager_get_device_handle` returns an error; :cpp:func:`esp_board_manager_get_device_config` still returns a valid configuration struct.
- ``peripherals:`` supports at most four entries (``DEV_CUSTOM_MAX_PERIPHERALS = 4``); the parser rejects larger lists.
- After modifying YAML, re-run ``idf.py bmgr -b <board>``.

.. _custom-debug:

Debugging Tips
--------------

.. _custom-api:

API Reference
-------------

The configuration struct for a ``custom`` device is auto-generated by BMGR during code generation based on the ``config:`` fields, and is defined in the build artifact ``components/gen_bmgr_codes/gen_board_device_custom.h``. The struct naming rule is ``dev_custom_{sanitized_name}_config_t``; this header must be included before use.

Use :cpp:func:`esp_board_manager_get_device_config` to retrieve the configuration struct; if an init entry is registered, use :cpp:func:`esp_board_manager_get_device_handle` to retrieve the user handle.

The registration macro ``CUSTOM_DEVICE_IMPLEMENT`` and the base configuration struct ``dev_custom_base_config_t`` are defined in ``esp_board_manager/devices/dev_custom/dev_custom.h``.
