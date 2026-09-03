Board Directory Structure and File Responsibilities
===================================================

:link_to_translation:`zh_CN:[中文]`

A minimal board directory requires three files:

.. code-block:: text

   my_board/
     board_info.yaml          # Board metadata: board name, chip, version, description, manufacturer
     board_peripherals.yaml   # Low-level peripheral resource declarations
     board_devices.yaml       # Functional device declarations

BMGR uses the presence of all three files as the recognition criterion when scanning for available boards. The remaining files are optional:

.. code-block:: text

   my_board/
     sdkconfig.defaults.board  # Optional: board-level default sdkconfig entries
     setup_device.c            # Optional: board-level init logic that cannot be expressed in YAML alone
     Kconfig.projbuild         # Optional: board-level Kconfig symbol extensions
     packages/                 # Optional: board-level local components

The board name is determined by the directory name and must match the ``board`` field in ``board_info.yaml``. Board names may only contain letters, digits, and underscores; hyphens and other special characters are not supported. Boards that do not meet these requirements are ignored during scanning.

``board_info.yaml`` Board Metadata
------------------------------------

``board_info.yaml`` describes the static metadata of a board. It is written into ``gen_board_info.c`` during generation and can be printed via :cpp:func:`esp_board_manager_print_board_info`. All fields are informational metadata and do not affect device or peripheral initialization logic.

.. code-block:: yaml

   board: my_board
   chip: esp32s3
   version: "1.0.0"
   description: "My custom board"
   manufacturer: "MyCompany"

.. list-table::
   :header-rows: 1
   :widths: 18 12 12 58

   * - Field
     - Type
     - Required
     - Description
   * - ``board``
     - string
     - Yes
     - Board name; must match the board directory name. Only lowercase letters, digits, and underscores are allowed; hyphens and other special characters are not supported.
   * - ``chip``
     - string
     - Yes
     - The main chip model of the board, for example ``esp32s3`` or ``esp32p4``, corresponding to ``CONFIG_IDF_TARGET``.
   * - ``version``
     - string
     - No
     - Schema identifier for the YAML parsing contract. Currently ``1.0.0``; omitting it, using ``default`` (case-insensitive), or writing ``1.0.0`` are all equivalent. An unrecognized identifier causes a warning during generation. Unrelated to the IDF driver version or the BMGR component version.
   * - ``description``
     - string
     - No
     - Brief description of the board; free-form text. Printed at runtime via ``ESP_LOGI`` by :cpp:func:`esp_board_manager_print_board_info`; not shown by ``idf.py bmgr -l``.
   * - ``manufacturer``
     - string
     - No
     - Board manufacturer name. Printed at runtime by :cpp:func:`esp_board_manager_print_board_info` alongside ``description``; not shown by ``idf.py bmgr -l``.

``board_peripherals.yaml``
--------------------------

``board_peripherals.yaml`` declares the low-level hardware resource instances on the board. Each entry corresponds to one low-level resource, such as an I2C bus, an SPI controller, an I2S interface, or a GPIO.

The basic structure of a peripheral entry:

.. code-block:: yaml

   peripherals:
     - name: <peripheral_name>
       type: <peripheral_type>
       version: <version>
       role: <role>
       format: <format_string>
       config: <configuration>

.. list-table::
   :header-rows: 1
   :widths: 18 14 14 54

   * - Field
     - Type
     - Required
     - Description
   * - ``name``
     - string
     - Yes
     - Peripheral instance name. **Must be prefixed with the type**, for example ``i2c_main``, ``spi_lcd``, or ``gpio_power``. Must be unique within the file; only lowercase letters, digits, and underscores are allowed (cannot be purely numeric). Devices reference peripherals by this name; a name mismatch causes device initialization to fail.
   * - ``type``
     - string
     - Yes
     - Peripheral category; use a standard type name supported by BMGR, for example ``i2c`` / ``spi`` / ``i2s`` / ``gpio``. See :doc:`/references/support-matrix` for the full list.
   * - ``version``
     - string
     - No
     - Schema identifier for this peripheral entry; same meaning as ``version`` in ``board_info.yaml``; omit to use the current schema.
   * - ``role``
     - string
     - Conditional
     - Peripheral operating mode, for example ``master`` / ``slave``, ``tx`` / ``rx``, or ``continuous`` / ``oneshot``. Whether it is required and the set of valid values are determined by ``type``; see the peripheral reference.
   * - ``format``
     - string
     - Conditional
     - Data format declaration; required only for certain peripherals (currently only I2S, for example ``std-out``, ``tdm-in``, ``pdm-out``). See the peripheral reference for valid values.
   * - ``config``
     - mapping
     - Yes
     - Peripheral-specific configuration; fields come from ``peripherals/periph_<type>/periph_<type>.yml``. See the peripheral reference for field details.

``board_devices.yaml``
----------------------

``board_devices.yaml`` declares the functional device instances on the board. Each entry corresponds to one functional device, such as an audio codec, an LCD display, or a button.

The basic structure of a device entry:

.. code-block:: yaml

   devices:
     - name: <device_name>
       type: <device_type>
       chip: <chip_name>
       version: <version>
       sub_type: <sub_type>
       init_skip: false
       depends_on: other_device
       power_ctrl_device: power_ctrl
       config:
         <configurations>
       peripherals:
         - name: <periph_name>
           # Device-side exclusive parameters can be appended here
       dependencies:
         <component_name>:
           require: <scope>
           version: <version_spec>

.. list-table::
   :header-rows: 1
   :widths: 20 16 14 50

   * - Field
     - Type
     - Required
     - Description
   * - ``name``
     - string
     - Yes
     - Device instance name; must be unique within the file. Only lowercase letters, digits, and underscores are allowed (cannot be purely numeric). **Not required to be prefixed with the type**; names should reflect functional semantics, for example ``audio_codec_0``, ``display_main``, or ``button_boot``.
   * - ``type``
     - string
     - Yes
     - Device category; use a standard type name supported by BMGR, for example ``audio_codec`` / ``display_lcd`` / ``button`` / ``power_ctrl`` / ``custom``. See :doc:`/references/support-matrix` for the full list.
   * - ``chip``
     - string
     - Conditional
     - External chip model of the device (different from ``chip`` in ``board_info.yaml``); applies to devices that need to identify a specific part, such as LCD controllers, touch controllers, or I/O expander chips. See the device reference for which types require this field.
   * - ``version``
     - string
     - No
     - Schema identifier for this device entry; same meaning as ``version`` in ``board_info.yaml``; omit to use the current schema.
   * - ``sub_type``
     - string
     - Conditional
     - Sub-implementation path within the same type—for example, ``display_lcd`` is divided into ``spi`` / ``i80`` / ``dsi`` / ``parlio`` / ``rgb`` / ``rgb_3wire_spi``. Whether it is required and the set of valid values are determined by ``type``; see the device reference.
   * - ``init_skip``
     - bool
     - No
     - Defaults to ``false``. When ``true``, :cpp:func:`esp_board_manager_init` does not automatically initialize this device; the application must call :cpp:func:`esp_board_manager_init_device_by_name` at the appropriate time. See :doc:`/programming-guide/runtime-lifecycle`.
   * - ``depends_on``
     - string / list
     - No
     - Declares the names of other device instances this device depends on. BMGR recursively initializes dependencies before initializing this device. See :doc:`/programming-guide/runtime-lifecycle`.
   * - ``power_ctrl_device``
     - string
     - No
     - References a device instance of type ``power_ctrl``. BMGR automatically triggers power-on before this device's ``init`` and power-off after its ``deinit``. See :doc:`/programming-guide/runtime-lifecycle`.
   * - ``config``
     - mapping
     - Yes
     - Device-specific configuration; fields come from ``devices/dev_<type>/dev_<type>.yml``. See the device reference for field details.
   * - ``peripherals``
     - list of mappings
     - Conditional
     - Declares the peripheral instances this device depends on at runtime (for example, a codec depends on ``i2c`` + ``i2s``, and a button depends on ``gpio`` or ``adc``). Whether this is required and which peripherals are needed at minimum depend on the type. The ``name`` of each entry must exactly match the peripheral instance name in ``board_peripherals.yaml``. In addition to ``name``, device-side exclusive parameters (such as the I2C address or PA active level) may be included; they are read by the device parser and **do not modify the peripheral's own config**.
   * - ``dependencies``
     - mapping
     - Conditional
     - Declares additional ESP-IDF components required by the device at runtime. Written into ``components/gen_bmgr_codes/idf_component.yml`` during generation; when the same dependency name appears multiple times, the last one in YAML order is kept. Field semantics match the component manager; see the `IDF Component Manager Manifest documentation <https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/manifest_file.html#dependencies>`_.

Typical syntax for referencing a peripheral in ``peripherals`` with additional device-side parameters (for example, specifying the I2C address on the device side for an audio codec):

.. code-block:: yaml

   devices:
     - name: audio_codec_0
       type: audio_codec
       peripherals:
         - name: i2c_main
           addr: 0x18        # Device-side exclusive parameter

``sdkconfig.defaults.board`` and ``Kconfig.projbuild``
-------------------------------------------------------

Each board can optionally include the following two files in its directory to extend the project configuration system:

- ``sdkconfig.defaults.board``: Declares board-specific sdkconfig defaults (PSRAM, Flash, partition tables, application-level ``CONFIG_*`` entries, etc.); merged by BMGR into ``components/gen_bmgr_codes/board_manager.defaults`` during generation.
- ``Kconfig.projbuild``: Additional Kconfig symbols the board needs to expose in ``menuconfig`` (typically board-specific feature flags, sub-board enumerations, etc.); appended by BMGR to ``components/gen_bmgr_codes/Kconfig.projbuild``.

.. code-block:: ini

   # Example sdkconfig.defaults.board
   CONFIG_SPIRAM_MODE_OCT=y
   CONFIG_SPIRAM_SPEED_80M=y

For how these two files are merged during generation, their override precedence, and their relationship with amend, see :doc:`/programming-guide/generation-flow`.

``board_amend.yaml``
--------------------

``board_amend.yaml`` is the manifest file of an amend (overlay) directory. It is not part of the board directory itself; it lives in a separate overlay directory and is used to append to or override a target board's configuration without modifying the original board.

The manifest explicitly lists the fragments to overlay under ``apply:``, which may include ``board_devices.yaml`` / ``board_peripherals.yaml`` fragments, ``sdkconfig.defaults.board``, ``Kconfig.projbuild``, and ``.c`` source files; only files listed under ``apply:`` participate in the merge.

The overlay directory is selected explicitly with ``-a/--amend`` or discovered automatically by name via auto-amend. For the amend directory structure, ``apply:`` semantics, override rules, and usage examples, see :doc:`/create-board/amend`.

Board-Level C Extension Functions and ``custom`` Devices
--------------------------------------------------------

YAML describes buses, pins, timing parameters, and component dependencies, but it does not replace chip-driver constructor functions. A board must provide the applicable C functions when it needs to call a chip-specific constructor, implement a custom driver, or manage board-level resources.

The following table lists the current entry points that a board must or may implement. The individual device reference pages provide their configuration and examples.

.. list-table::
   :header-rows: 1

   * - Device or sub-type
     - Board-level implementation
     - Entry point
   * - ``display_lcd/dsi``
     - Required
     - ``lcd_dsi_panel_factory_entry_t``
   * - ``display_lcd/spi``, ``display_lcd/i80``, ``display_lcd/parlio``, ``display_lcd/rgb_3wire_spi``
     - Required
     - ``lcd_panel_factory_entry_t``
   * - ``display_lcd/rgb``
     - No panel factory function; a callback is needed only when ``user_fbs_func`` is set
     - ``DEVICE_EXTRA_FUNC_REGISTER``
   * - ``lcd_touch/i2c``
     - Required
     - ``lcd_touch_factory_entry_t``
   * - ``gpio_expander``
     - Required
     - ``io_expander_factory_entry_t``
   * - ``button/custom``
     - Required
     - ``DEVICE_EXTRA_FUNC_REGISTER``
   * - ``power_ctrl/custom``
     - Required
     - ``DEVICE_EXTRA_FUNC_REGISTER``
   * - ``custom``
     - Only when BMGR must automatically initialize and deinitialize the device
     - ``CUSTOM_DEVICE_IMPLEMENT``

Factory functions entered through a fixed function signature must have external linkage and cannot be declared ``static``.

``dependencies`` only adds the chip-driver component to the build; it does not generate these functions. A factory function must call the chip-driver API corresponding to the YAML ``chip`` and ``dependencies``.

``setup_device.c`` is the recommended filename for board-level extension functions. BMGR recursively scans C, C++, and assembly source files in the board directory and adds them to the generated component, so an extension function can also reside in another scanned board source file.

To allow downstream projects to override the board's default behavior via ``-a/--amend``, or to disable a device with ``gen_skip``, board-level source should follow these conventions:

- Declare the factory or hook functions exposed to the outside (such as ``lcd_panel_factory_entry_t``, ``lcd_touch_factory_entry_t``, and ``io_expander_factory_entry_t``) as weak symbols with ``__attribute__((weak))``. A strong symbol implementation with the same name in the amend directory will replace the board's default implementation at link time, without modifying the original board source. See :doc:`/create-board/amend` for the override mechanism.
- Wrap ``#include`` directives for chip driver headers with ``__has_include``, and apply the same guard around the related function body. This allows an amend to not only replace the function itself but also, by removing the corresponding component dependency (or substituting another chip component), cause the board's default implementation to disappear automatically. ``gen_skip`` likewise drops that device's ``dependencies:`` from ``idf_component.yml``, but board-directory source files are still compiled into ``gen_bmgr_codes``; without ``__has_include``, the build fails because the header is missing.

.. code-block:: c

   // setup_device.c: common weak symbol + __has_include combination; see esp32_s3_korvo_2_3/setup_device.c
   #if __has_include(<esp_lcd_ili9341.h>)
   #include "esp_lcd_ili9341.h"

   __attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                                            const esp_lcd_panel_dev_config_t *cfg,
                                                            esp_lcd_panel_handle_t *ret_panel)
   {
       return esp_lcd_new_panel_ili9341(io, cfg, ret_panel);
   }
   #endif  /* __has_include(<esp_lcd_ili9341.h>) */

``custom`` type devices are suitable for hardware not yet built into BMGR, such as specific power management chips or sensors. Set ``type`` to ``custom`` in ``board_devices.yaml``; during generation, BMGR expands the fields under ``config:`` into a dedicated configuration structure and writes it to ``components/gen_bmgr_codes/gen_board_device_custom.h``. To provide custom initialization logic, implement the initialization and deinitialization functions in the board directory and register them with the ``CUSTOM_DEVICE_IMPLEMENT`` macro. If the implementation depends on a third-party chip driver, place the driver header, generated struct, init/deinit, and registration macro in the same ``#if __has_include`` block, for example:

.. code-block:: c

   #if __has_include("axp2101.h")
   #include "axp2101.h"

   CUSTOM_DEVICE_IMPLEMENT(axp2101_power_manager,
                             cores3_power_manager_init,
                             cores3_power_manager_deinit);
   #endif  /* __has_include("axp2101.h") */

For the configuration struct naming convention, type inference table, macro usage, and application-side access methods, see :doc:`/references/devices/custom`; for a complete board-level example, refer to ``boards/m5stack_cores3/power_manager.c``.
