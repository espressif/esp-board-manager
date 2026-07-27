Button (button)
===============

:link_to_translation:`zh_CN:[中文]`

Overview
--------

The ``button`` device integrates GPIO buttons or ADC buttons into the ESP Board Manager device management flow. After initialization, applications can obtain a ``dev_button_handles_t`` via ``esp_board_manager_get_device_handle()``, or register button event callbacks enabled in the YAML via ``esp_board_device_callback_register()``.

This device is based on the ``espressif/button`` component. GPIO buttons are suitable for dedicated button pins; ADC buttons are suitable for resistor-divider button networks on the same ADC channel; custom buttons are suitable for matrix keypads, touch keys, I2C or SPI button controllers, and other scenarios that require the application layer to provide a ``button_driver_t``.

Supported Usage Modes
---------------------

``button`` distinguishes usage modes by ``sub_type``:

- `GPIO Button`_
- `ADC Single Button`_
- `ADC Multi-Button`_
- `Custom Button`_

Minimal Configuration
---------------------

GPIO Button
^^^^^^^^^^^

``gpio`` mode creates a single button handle via the ``gpio`` peripheral.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: gpio_button_io_0
        type: gpio
        role: io
        config:
          pin: 0
          mode: "GPIO_MODE_INPUT"

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: gpio_button_0
        type: button
        sub_type: gpio
        config:
          active_level: 0
        peripherals:
          - name: gpio_button_io_0

ADC Single Button
^^^^^^^^^^^^^^^^^

``adc_single`` mode creates a single ADC button via the ``oneshot`` role of the ``adc`` peripheral.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: adc_oneshot
        type: adc
        role: oneshot
        config:
          unit_id: ADC_UNIT_1
          channel_id: 4

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: adc_button_0
        type: button
        sub_type: adc_single
        config:
          button_index: 0
          min_voltage: 0
          max_voltage: 500
        peripherals:
          - name: adc_oneshot

ADC Multi-Button
^^^^^^^^^^^^^^^^

``adc_multi`` mode creates multiple button handles on the same ADC channel. ``button_num`` must not exceed ``CONFIG_ADC_BUTTON_MAX_BUTTON_PER_CHANNEL``. When registering callbacks, the corresponding label from ``button_labels`` is used as ``user_data`` by default; if no labels are configured, callbacks are still registered, but the default ``user_data`` cannot distinguish individual buttons.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: adc_oneshot
        type: adc
        role: oneshot
        config:
          unit_id: ADC_UNIT_1
          channel_id: 4

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: adc_button_group
        type: button
        sub_type: adc_multi
        config:
          button_num: 6
          voltage_range: [380, 820, 1110, 1650, 1980, 2410]
          button_labels: ["VOLUME_UP", "VOLUME_DOWN", "SET", "PLAY", "MUTE", "REC"]
          max_voltage: 3000
        peripherals:
          - name: adc_oneshot

Custom Button
^^^^^^^^^^^^^

``custom`` does not depend on any entry in ``board_peripherals.yaml``; the bus and GPIO are managed by the driver implemented on the application side.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: custom_button_0
        type: button
        sub_type: custom

Application code (for example ``setup_device.c`` or another source file participating in the component build) must use ``DEVICE_EXTRA_FUNC_REGISTER`` to register a ``button_driver_t`` creator function with the same name as the device:

.. code-block:: c

    #include "dev_button.h"
    #include "esp_board_extra_func_entry.h"

    static button_driver_t *custom_button_0(const dev_button_config_t *config)
    {
        button_driver_t *driver = calloc(1, sizeof(button_driver_t));
        // Fill driver->get_key_level / driver->del / driver->enter_power_save according to hardware
        return driver;
    }
    DEVICE_EXTRA_FUNC_REGISTER(custom_button_0, custom_button_0);

The registered function name must exactly match the ``name`` in ``board_devices.yaml``. During BMGR initialization, the function is looked up by device name, and the returned ``button_driver_t`` is passed to ``iot_button_create()``. The ``peripherals`` list for ``custom`` mode must be empty; ``events_cfg`` and timing fields are configured in the same way as other sub-types. See :doc:`/programming-guide/board-directory` for board source placement and build rules.

Full Field Reference
--------------------

GPIO Button Full Fields
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example GPIO Button device configuration
    - name: gpio_button_0            # The name of the device, must be unique
      type: button                   # The type of the device
      sub_type: gpio                 # Button sub type: "gpio", "adc_single", or "adc_multi"
      config:
        # GPIO Configuration
        active_level: 0              # [TO_BE_CONFIRMED] Active level (0-low, 1-high) when button is pressed
        enable_power_save: false     # Enable power save mode (default: false)
        disable_pull: false          # Disable internal pull-up/pull-down (default: false)

        # Button Timing Configuration
        long_press_time: 2000        # Long press time in milliseconds (default: 2000)
        short_press_time: 100        # Short press time in milliseconds (default: 100)

        # Event Configuration (NEW - Easy button features)
        events_cfg:
          press_down: true           # Enable press down event (default: true)
          press_up: true             # Enable press up event (default: true)
          single_click: true         # Enable single click event (default: true)
          double_click: true         # Enable double click event (default: true)
          multi_click: false         # Enable multi click event (default: false)
          click_counts: []           # Multi click counts configuration (default: [])
                                     # eg: Set to [3, 4] means enable multi_click event with 3 and 4 counts
          long_press_start: true     # Enable long press start event (default: true)
          long_press_start_time: []  # Long press start time configuration in ms (default: [])
                                     # eg: Set to [3000, 4000] means enable long_press_start event with 3 and 4 seconds
          long_press_hold: false     # Enable long press hold event (default: false)
          long_press_up: true        # Enable long press up event (default: true)
          long_press_up_time: []     # Long press up time configuration in ms (default: [])
          press_repeat: false        # Enable press repeat event (default: false)
          press_repeat_done: false   # Enable press repeat done event (default: false)
          press_end: false           # Enable press end event (default: false)

      peripherals:
        - name: gpio                 # [TO_BE_CONFIRMED] GPIO peripheral name

ADC Single Button Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example ADC Button device configuration (Single button)
    - name: adc_button_0      # The name of the device, must be unique
      type: button            # The type of the device
      sub_type: adc_single    # Button sub type: "gpio", "adc_single", or "adc_multi"
      config:
        # ADC Configuration
        button_index: 0              # [TO_BE_CONFIRMED] Button index on the channel
        min_voltage: 0               # [TO_BE_CONFIRMED] Minimum voltage in mV for button press
        max_voltage: 500             # [TO_BE_CONFIRMED] Maximum voltage in mV for button press

        # Button Timing Configuration
        long_press_time: 2000        # Long press time in milliseconds (default: 2000)
        short_press_time: 100        # Short press time in milliseconds (default: 100)

        # Event Configuration (NEW - Easy button features)
        events_cfg:
          press_down: true           # Enable press down event (default: true)
          press_up: true             # Enable press up event (default: true)
          single_click: true         # Enable single click event (default: true)
          double_click: true         # Enable double click event (default: true)
          multi_click: false         # Enable multi click event (default: false)
          click_counts: []           # Multi click counts configuration (default: [])
                                     # eg: Set to [3, 4] means enable multi_click event with 3 and 4 counts
          long_press_start: true     # Enable long press start event (default: true)
          long_press_start_time: []  # Long press start time configuration in ms (default: [])
                                     # eg: Set to [3000, 4000] means enable long_press_start event with 3 and 4 seconds
          long_press_hold: false     # Enable long press hold event (default: false)
          long_press_up: true        # Enable long press up event (default: true)
          long_press_up_time: []     # Long press up time configuration in ms (default: [])
          press_repeat: false        # Enable press repeat event (default: false)
          press_repeat_done: false   # Enable press repeat done event (default: false)
          press_end: false           # Enable press end event (default: false)

      peripherals:
        - name: adc_oneshot          # [TO_BE_CONFIRMED] ADC peripheral name

ADC Multi-Button Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example ADC Button device configuration (Multi buttons)
    - name: adc_button_group         # The name of the device, must be unique
      type: button                   # The type of the device
      sub_type: adc_multi            # Button sub type: "gpio", "adc_single", or "adc_multi"
      config:
        # ADC Configuration
        button_num: 6                                      # [TO_BE_CONFIRMED] Number of buttons in the group, must not be greater than CONFIG_ADC_BUTTON_MAX_BUTTON_PER_CHANNEL (default: 8)
        voltage_range: [380, 820, 1110, 1650, 1980, 2410]  # [TO_BE_CONFIRMED] Nominal voltage for each button in mV
        # This voltage_range configuration is only applicable to the Korvo2 V3 development board.
        # Please refer to the schematic of your development board to determine the correct configuration.
        button_labels: ["VOLUME_UP", "VOLUME_DOWN", "SET", "PLAY", "MUTE", "REC"]  # Labels for each button
        # Optional but recommended labels for each button in the group.
        # When esp_board_device_callback_register(name, cb, NULL) is used, these labels are passed as callback user_data by default.
        # If omitted, ADC multi-button callbacks still work, but the default user_data is NULL so the callback cannot distinguish buttons by label.
        max_voltage: 3000                                  # Maximum voltage in mV for this ADC channel (default: 3000)

        # Button Timing Configuration
        long_press_time: 2000        # Long press time in milliseconds (default: 2000)
        short_press_time: 100        # Short press time in milliseconds (default: 100)

        # Event Configuration (NEW - Easy button features)
        events_cfg:
          press_down: true           # Enable press down event (default: true)
          press_up: true             # Enable press up event (default: true)
          single_click: true         # Enable single click event (default: true)
          double_click: true         # Enable double click event (default: true)
          multi_click: false         # Enable multi click event (default: false)
          click_counts: []           # Multi click counts configuration (default: [])
                                     # eg: Set to [3, 4] means enable multi_click event with 3 and 4 counts
          long_press_start: true     # Enable long press start event (default: true)
          long_press_start_time: []  # Long press start time configuration in ms (default: [])
                                     # eg: Set to [3000, 4000] means enable long_press_start event with 3 and 4 seconds
          long_press_hold: false     # Enable long press hold event (default: false)
          long_press_up: true        # Enable long press up event (default: true)
          long_press_up_time: []     # Long press up time configuration in ms (default: [])
          press_repeat: false        # Enable press repeat event (default: false)
          press_repeat_done: false   # Enable press repeat done event (default: false)
          press_end: false           # Enable press end event (default: false)

      peripherals:
        - name: adc_oneshot          # [TO_BE_CONFIRMED] ADC peripheral name

Custom Button Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example Custom Button device configuration
    - name: custom_button_0          # The name of the device, must be unique
      type: button                   # The type of the device
      sub_type: custom               # Button sub type: "gpio", "adc_single", "adc_multi", or "custom"
      config:
        # Custom Driver Configuration
        # The name of the registered custom driver creator function must match the device name.
        # A function with this name must be registered using DEVICE_EXTRA_FUNC_REGISTER.

        # Button Timing Configuration
        long_press_time: 2000        # Long press time in milliseconds (default: 2000)
        short_press_time: 100        # Short press time in milliseconds (default: 100)

        # Event Configuration (NEW - Easy button features)
        events_cfg:
          press_down: true           # Enable press down event (default: true)
          press_up: true             # Enable press up event (default: true)
          single_click: true         # Enable single click event (default: true)
          double_click: true         # Enable double click event (default: true)
          multi_click: false         # Enable multi click event (default: false)
          click_counts: []           # Multi click counts configuration (default: [])
          long_press_start: true     # Enable long press start event (default: true)
          long_press_start_time: []  # Long press start time configuration in ms (default: [])
          long_press_hold: false     # Enable long press hold event (default: false)
          long_press_up: true        # Enable long press up event (default: true)
          long_press_up_time: []     # Long press up time configuration in ms (default: [])
          press_repeat: false        # Enable press repeat event (default: false)
          press_repeat_done: false   # Enable press repeat done event (default: false)
          press_end: false           # Enable press end event (default: false)

Component Dependencies
----------------------

When the ``button`` device is enabled, the component manifest introduces ``espressif/button`` with the version constraint ``^4.1.4`` via ``CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT``. The GPIO and ADC sub-modes use the ``button_gpio`` and ``button_adc`` interfaces provided by this component.

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
     - Provides a single GPIO button input
   * - ``adc``
     - ``oneshot``
     - Required for ``sub_type: adc_single`` and ``sub_type: adc_multi``
     - Provides ADC button voltage sampling channel
   * - N/A
     - N/A
     - ``sub_type: custom`` does not reference any ``board_peripherals.yaml`` entry
     - Bus and GPIO are managed by the application-side ``button_driver_t`` implementation

Code Reference
--------------

- ``esp_board_manager/test_apps/main/test_dev_button.c``
- ``esp_board_manager/devices/dev_button/dev_button.c``
- ``esp_board_manager/devices/dev_button/dev_button_sub_gpio.c``
- ``esp_board_manager/devices/dev_button/dev_button_sub_adc_single.c``
- ``esp_board_manager/devices/dev_button/dev_button_sub_adc_multi.c``
- ``esp_board_manager/devices/dev_button/dev_button_sub_custom.c``

Board-level Reference
---------------------

- ``esp_boards/esp32_s3_korvo_2_3/board_devices.yaml``
- ``esp_boards/esp32_s3_korvo_2_3/board_peripherals.yaml``
- ``esp_boards/esp32_lyrat_mini_1_1/board_devices.yaml``
- ``esp_boards/esp32_s3_lcd_ev_board/board_devices.yaml``
- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_s3_devkitc/board_devices.yaml``

Notes
-----

- For common YAML field rules, see :doc:`/programming-guide/yaml-rules`.
- The ``voltage_range`` and ``button_num`` of ``adc_multi`` must match the board's resistor-divider network; do not copy values from another board directly.
- ``events_cfg`` controls the set of events registered by ``esp_board_device_callback_register()``; the old ``events`` field is no longer used in the current template.
- The driver creator function name registered for the ``custom`` sub-type must match the device ``name``; ``board_devices.yaml`` does not allow adding ``peripherals`` for this sub-type.
- After modifying the YAML, re-run ``idf.py bmgr -b <board>``.

Debugging Tips
--------------

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain the device handle. The handle type is ``dev_button_handles_t``:

.. code-block:: c

   typedef struct {
       uint8_t          num_buttons;                                               /*!< Number of button handles */
       button_handle_t  button_handles[CONFIG_ADC_BUTTON_MAX_BUTTON_PER_CHANNEL];  /*!< Flexible array of button handles */
   } dev_button_handles_t;

``num_buttons`` is the number of buttons actually initialized; ``button_handles`` can be passed to the callback registration interface of the ``espressif/button`` component.

Use :cpp:func:`esp_board_device_callback_register` to register a button callback by device name. This API uses the initialized ``button`` device handle and registers the same ``button_cb_t`` on all button handles under that device, according to the events enabled in ``events_cfg``. The target device must already be initialized before this API is called.

.. code-block:: c

   #include "dev_button.h"
   #include "esp_board_device.h"

   static void button_event_handler(void *button_handle, void *user_data)
   {
       button_handle_t btn = (button_handle_t)button_handle;
       button_event_t event = iot_button_get_event(btn);

       ESP_LOGI(TAG, "button event: %s, user_data: %s",
                iot_button_get_event_str(event),
                (const char *)user_data);
   }

   void app_register_button_callback(void)
   {
       ESP_ERROR_CHECK(esp_board_device_callback_register(
           "gpio_button_0", button_event_handler, NULL));
   }

When ``user_data`` is ``NULL``, BMGR uses a default value: single-button devices use the device ``name``, and ``adc_multi`` devices use the string from ``button_labels`` corresponding to the button index. To pass application-specific context, provide the pointer as the third argument.

Related declarations are in ``esp_board_manager/devices/dev_button/dev_button.h``.
