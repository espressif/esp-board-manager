Power Control (``power_ctrl``)
==============================

:link_to_translation:`zh_CN:[中文]`

Overview
--------

The ``power_ctrl`` device wraps a board-level power enable signal as a reusable device. Other devices can reference it via the ``power_ctrl_device`` field to trigger power-on or power-off control during device initialization and shutdown.

``sub_type: gpio`` uses a single ``gpio`` peripheral to set the power control pin level; ``active_level`` represents the active-high or active-low sense for power-on. ``sub_type: custom`` registers board-specific lifecycle operations, which supports PMICs, IO expanders, multi-rail sequencing, and any other board-owned power implementation.

Supported Usage Modes
---------------------

``power_ctrl`` distinguishes usage modes with ``sub_type``:

- `GPIO Power Control`_
- `Custom Power Control`_

Minimal Configuration
---------------------

GPIO Power Control
^^^^^^^^^^^^^^^^^^

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: gpio_power_audio
        type: gpio
        role: io
        config:
          pin: 46
          mode: GPIO_MODE_OUTPUT

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: audio_power_ctrl
        type: power_ctrl
        sub_type: gpio
        peripherals:
          - gpio_name: gpio_power_audio
            active_level: 1

      - name: audio_dac
        chip: es8311
        type: audio_codec
        power_ctrl_device: audio_power_ctrl
        config:
          adc_enabled: false
          dac_enabled: true
        peripherals:
          - i2s_name: i2s_audio_out
          - i2c_name: i2c_master
            address: 0x30
            frequency: 400000

In ``gpio`` mode, initialization references the ``gpio`` peripheral from the configuration and saves the peripheral handle. When a power-on request is received, the GPIO is set to ``active_level``; when a power-off request is received, it is set to the opposite level. The ``power_ctrl`` device itself only defines the power control resource; devices that need to be controlled reference this device name via the ``power_ctrl_device`` field, for example in board configurations for ``audio_codec``, ``fs_fat``, or ``display_lcd``.

Custom Power Control
^^^^^^^^^^^^^^^^^^^^

Use ``custom`` when a board needs a PMIC, IO expander, several rails, or an ordered power sequence. Register operations under the same name as the ``power_ctrl`` device. See :doc:`/programming-guide/board-directory` for board source placement and build rules. ``init`` and ``deinit`` are optional; ``set_power`` is required. The framework retains configured ``peripherals`` before ``init`` and releases them after ``deinit``. Use ``depends_on`` for other device dependencies, such as a separately modelled PMIC device.

Role-specific selectors such as ``gpio_name`` and ``i2c_name`` apply only to binding scenarios explicitly supported by the device rules. They do not apply to the ``peripherals`` list of a ``custom`` power controller, which declares lifecycle dependencies. The ``peripherals`` list of a ``custom`` power controller continues to use the legacy ``name`` reference form.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: board_power_ctrl
        type: power_ctrl
        sub_type: custom
        depends_on: pmic_device
        config:
          startup_delay_ms: 10

      - name: display_lcd
        type: display_lcd
        power_ctrl_device: board_power_ctrl

``board_power.c``:

.. code-block:: c

    #include "dev_power_ctrl.h"
    #include "gen_board_device_custom.h"
    #include "esp_board_extra_func_entry.h"

    static int board_power_init(const dev_power_ctrl_config_t *config, void **context)
    {
        const dev_custom_board_power_ctrl_custom_config_t *user_cfg =
            config->sub_cfg.custom.user_cfg;
        (void)user_cfg;
        *context = NULL;
        return 0;
    }

    static int board_power_set_power(void *context, const char *consumer, bool power_on)
    {
        (void)context;
        (void)consumer;
        (void)power_on;
        return 0;
    }

    static const dev_power_ctrl_custom_ops_t s_board_power_ctrl_ops = {
        .init = board_power_init,
        .deinit = NULL,
        .set_power = board_power_set_power,
    };

    DEVICE_EXTRA_FUNC_REGISTER(board_power_ctrl, &s_board_power_ctrl_ops);

All Fields
----------

GPIO Power Control All Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example Power Control device with GPIO sub type configuration
    - name: audio_power_ctrl          # The name of the device, must be unique
      type: power_ctrl                # The type of the device, must be unique
      sub_type: gpio                  # The sub type of the device, must be 'gpio'
      peripherals:
        - gpio_name: gpio             # [TO_BE_CONFIRMED] GPIO peripheral name (must reference a GPIO peripheral)
          active_level: 1             # [TO_BE_CONFIRMED] Active level (0-low, 1-high) when power is on

Custom Power Control All Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: board_power_ctrl
      type: power_ctrl
      sub_type: custom
      depends_on: pmic_device          # Optional device dependencies
      peripherals:                     # Optional framework-managed peripheral references
        - name: i2c_pmic
      config:                          # Optional type-safe board configuration
        startup_delay_ms: 10

    # Example usage in devices, add the power_ctrl_device attribute to the device configuration
    # - name: audio_dac
    #   chip: es8311
    #   type: audio_codec
    #   power_ctrl_device: audio_power_ctrl  # Reference to power control device
    #   config:
    #     adc_enabled: false
    #     dac_enabled: true
    #     sys_cfg:
    #       no_mclk: false
    #   peripherals:
    #     - i2s_name: i2s_audio_out
    #     - i2c_name: i2c_master
    #       address: 0x30
    #       frequency: 400000

Component Dependencies
----------------------

The ``gpio`` mode uses the ESP-IDF GPIO driver and the BMGR ``gpio`` peripheral. A ``custom`` controller has no component dependency imposed by BMGR; declare board-specific component dependencies on the devices that use them.

Required Peripherals
--------------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - Required
     - Purpose
   * - ``gpio``
     - ``io``
     - Required for ``sub_type: gpio``
     - Provides the power enable GPIO
   * - Any supported peripheral
     - Depends on peripheral type
     - Optional for ``sub_type: custom``
     - Referenced by the framework for the custom controller lifecycle

Reference Code
--------------

- ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl.c``
- ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl_sub_gpio.c``
- ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl_sub_custom.c``
- Board customization workflow: :doc:`/create-board/index`

Board Reference
---------------

- ``esp_boards/esp_vocat_1_2/board_devices.yaml``
- ``esp_boards/esp_vocat_1_0/board_devices.yaml``
- ``esp_boards/esp32_lyrat_mini_1_1/board_devices.yaml``
- ``esp_boards/esp32_s3_box_3/board_devices.yaml``
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``
- ``esp_friends_boards/esp32_c5_spot/board_devices.yaml``

Notes
-----

- For common YAML field rules, see :doc:`/programming-guide/yaml-rules`.
- The ``power_ctrl_device`` field of the controlled device must reference a defined ``power_ctrl`` device name.
- ``active_level`` must match the board power switch circuit; the driver outputs the opposite level when powering off.
- The GPIO peripheral referenced by ``power_ctrl`` should be configured as output mode.
- A ``custom`` controller's registered name must equal its ``power_ctrl`` device name, and it must provide ``set_power``.
- Custom lifecycle code owns only its ``context``. The framework owns the references declared by ``peripherals``.
- After modifying YAML, re-run ``idf.py bmgr -b <board>``.

Debugging Tips
--------------

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain the device handle. The handle type is ``dev_power_ctrl_handle_t``:

.. code-block:: c

   typedef struct {
       void *periph_handle;
       const dev_power_ctrl_custom_ops_t *custom_ops;
       void *custom_context;
   } dev_power_ctrl_handle_t;

``periph_handle`` points to the GPIO peripheral handle for the GPIO sub-type. ``custom_context`` is board-owned lifecycle state for the custom sub-type. A power controller is generally invoked indirectly by ``esp_board_device_power_ctrl()`` through the ``power_ctrl_device`` reference and does not need to be operated directly.

The related declarations are located in ``esp_board_manager/devices/dev_power_ctrl/dev_power_ctrl.h``.
