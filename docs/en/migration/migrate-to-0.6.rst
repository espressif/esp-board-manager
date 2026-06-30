Migrate to 0.6.0
================

:link_to_translation:`zh_CN:[中文]`

This page collects the migration notes for upgrading to ``esp_board_manager`` 0.6.0. Read the relevant section before upgrading, then follow the validation steps to confirm the migration.

Migrating ``dev_lcd_touch_i2c`` to ``dev_lcd_touch``
----------------------------------------------------

``dev_lcd_touch_i2c`` is the legacy I2C touch device type. Starting from ``esp_board_manager`` v0.5.10, use the generic ``dev_lcd_touch`` device and select the I2C implementation with ``sub_type: i2c``.

Starting from 0.6.0 the legacy ``dev_lcd_touch_i2c`` device (its implementation, parser, header, and the ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_I2C_SUPPORT`` compatibility symbol) has been removed. Board definitions and application code must use ``type: lcd_touch`` with ``sub_type: i2c``; the legacy type and header are no longer available.

Why migrate
~~~~~~~~~~~

- Unified device model: touch devices use the common ``lcd_touch`` type, and the bus type is represented by ``sub_type``.
- Multiple address probing: I2C addresses move from the old single ``io_i2c_config.dev_addr`` field to root-level ``peripherals[].i2c_addr``, with up to 4 candidate addresses.
- Runtime effective-address query: applications and board factory code can call ``esp_board_device_get_i2c_effective_addr()`` to read the selected 8-bit address.

YAML migration
~~~~~~~~~~~~~~~

Old style:

.. code-block:: yaml

   - name: lcd_touch
     chip: cst816s
     type: lcd_touch_i2c
     dependencies:
       espressif/esp_lcd_touch_cst816s: "*"
     config:
       io_i2c_config:
         dev_addr: 0x15
         lcd_cmd_bits: 8
         flags:
           disable_control_phase: true
         peripherals:
           - name: i2c_master
       touch_config:
         x_max: 284
         y_max: 240
         rst_gpio_num: -1
         int_gpio_num: 3
         flags:
           swap_xy: true
           mirror_x: false
           mirror_y: true

New style:

.. code-block:: yaml

   - name: lcd_touch
     chip: cst816s
     type: lcd_touch
     sub_type: i2c
     dependencies:
       espressif/esp_lcd_touch_cst816s: "*"
     config:
       io_i2c_config:
         lcd_cmd_bits: 8
         flags:
           disable_control_phase: true
       touch_config:
         x_max: 284
         y_max: 240
         rst_gpio_num: -1
         int_gpio_num: 3
         flags:
           swap_xy: true
           mirror_x: false
           mirror_y: true
     peripherals:
       - name: i2c_master
         i2c_addr: 0x2a

Field mapping
~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 30 40

   * - Old field
     - New field
     - Notes
   * - ``type: lcd_touch_i2c``
     - ``type: lcd_touch`` + ``sub_type: i2c``
     - Device type is unified as ``lcd_touch``; bus type moves to ``sub_type``
   * - ``config.io_i2c_config.dev_addr``
     - ``peripherals[].i2c_addr``
     - New field uses an 8-bit left-shifted address
   * - ``config.io_i2c_config.peripherals[].name``
     - ``peripherals[].name``
     - I2C peripheral dependency moves to root-level ``peripherals``
   * - ``config.io_i2c_config.*``
     - ``config.io_i2c_config.*``
     - Keep all fields except ``dev_addr`` and nested ``peripherals``
   * - ``config.touch_config.*``
     - ``config.touch_config.*``
     - Unchanged
   * - ``dependencies``
     - ``dependencies``
     - Unchanged

I2C address rules
~~~~~~~~~~~~~~~~~

The new ``lcd_touch`` ``peripherals[].i2c_addr`` field uses 8-bit left-shifted addresses.

Examples:

- 7-bit address ``0x15`` should be written as ``0x2a``
- 7-bit address ``0x24`` should be written as ``0x48``
- 7-bit address ``0x5d`` should be written as ``0xba``

If a board may use different touch chips, list multiple candidate addresses:

.. code-block:: yaml

   peripherals:
     - name: i2c_master
       i2c_addr: [0xba, 0x48]

Board Manager probes the addresses in order and records the matched 8-bit address.

Migrating board ``setup_device.c``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``lcd_touch_factory_entry_t`` factory signature does not change, so you do not need to modify the factory-function signature in ``setup_device.c`` when migrating.

If a board needs to select different touch drivers based on the probed effective address, see the device reference :doc:`/references/devices/lcd-touch`.

Application compatibility notes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Migrated applications must use the new generic touch and I2C sub-type switches:

.. code-block:: c

   #if CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT && CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUB_I2C_SUPPORT
   // enable touch-related code
   #endif

The legacy ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_I2C_SUPPORT`` switch is no longer defined. Any code that still checks it will silently compile out the touch path.

The legacy structs are no longer defined either. Do not use:

.. code-block:: c

   dev_lcd_touch_i2c_config_t
   dev_lcd_touch_i2c_handles_t

Use the new types instead:

.. code-block:: c

   dev_lcd_touch_config_t
   dev_lcd_touch_handles_t

Header migration
^^^^^^^^^^^^^^^^

The legacy header ``dev_lcd_touch_i2c.h`` has been removed. Code that directly includes it:

.. code-block:: c

   #include "dev_lcd_touch_i2c.h"

will fail to compile and must be changed to:

.. code-block:: c

   #include "dev_lcd_touch.h"

If the application includes:

.. code-block:: c

   #include "esp_board_manager_includes.h"

the include itself does not need to change, but struct types still need to be migrated as shown below.

Handle migration
^^^^^^^^^^^^^^^^

Old code:

.. code-block:: c

   void *touch_handle = NULL;
   ESP_ERROR_CHECK(esp_board_manager_get_device_handle("lcd_touch", &touch_handle));

   dev_lcd_touch_i2c_handles_t *touch = (dev_lcd_touch_i2c_handles_t *)touch_handle;
   esp_lcd_touch_handle_t tp = touch->touch_handle;
   esp_lcd_panel_io_handle_t io = touch->io_handle;

New code:

.. code-block:: c

   void *touch_handle = NULL;
   ESP_ERROR_CHECK(esp_board_manager_get_device_handle("lcd_touch", &touch_handle));

   dev_lcd_touch_handles_t *touch = (dev_lcd_touch_handles_t *)touch_handle;
   esp_lcd_touch_handle_t tp = touch->touch_handle;
   esp_lcd_panel_io_handle_t io = touch->io_handle;

Config migration
^^^^^^^^^^^^^^^^

Old code:

.. code-block:: c

   dev_lcd_touch_i2c_config_t *cfg = NULL;
   ESP_ERROR_CHECK(esp_board_manager_get_device_config("lcd_touch", (void **)&cfg));

   const char *i2c_name = cfg->i2c_name;
   uint16_t primary_addr = cfg->i2c_addr[0];
   uint16_t runtime_addr = cfg->io_i2c_config.dev_addr;

New code:

.. code-block:: c

   dev_lcd_touch_config_t *cfg = NULL;
   ESP_ERROR_CHECK(esp_board_manager_get_device_config("lcd_touch", (void **)&cfg));

   const char *i2c_name = cfg->sub_cfg.i2c.i2c_name;
   size_t addr_count = cfg->sub_cfg.i2c.i2c_addr_count;
   const uint16_t *addr_candidates = cfg->sub_cfg.i2c.i2c_addr;

Note: ``cfg->sub_cfg.i2c.i2c_addr[]`` is the candidate address list, not necessarily the matched runtime address. If the application needs the actual selected touch address, use the effective-address query API ``esp_board_device_get_i2c_effective_addr()``.

Effective address migration
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Old code may read ``io_i2c_config.dev_addr`` or ``i2c_addr[0]`` from the legacy config to identify the touch chip. Migrate it to:

.. code-block:: c

   uint16_t touch_addr = 0;
   esp_err_t ret = esp_board_device_get_i2c_effective_addr("lcd_touch", &touch_addr);
   if (ret == ESP_OK) {
       // touch_addr is the selected 8-bit / left-shifted address
   }

Compile condition migration
^^^^^^^^^^^^^^^^^^^^^^^^^^^

All boards now use ``type: lcd_touch``, so guard the touch path with ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT`` and, it can also determine I2C subtypes by ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUB_I2C_SUPPORT`` sub-type switch. There is no longer a legacy ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_I2C_SUPPORT`` branch to fall back to.

Validation steps
~~~~~~~~~~~~~~~~

#. Run ``idf.py bmgr -b <board_name>`` to regenerate the board configuration.
#. Confirm no board YAML still uses ``type: lcd_touch_i2c``. The legacy type was removed in 0.6.0, so any remaining legacy YAML now makes ``idf.py bmgr`` fail with an unknown/unsupported device type error instead of a deprecation warning.
#. Check ``components/gen_bmgr_codes/board_manager.defaults`` contains:

   .. code-block:: ini

      CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT=y
      CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUB_I2C_SUPPORT=y

#. Confirm migrated application code does not use ``CONFIG_ESP_BOARD_DEV_LCD_TOUCH_I2C_SUPPORT`` or legacy ``dev_lcd_touch_i2c_*`` types, which are no longer defined.
#. Build the project and confirm there is no reference to the removed ``dev_lcd_touch_i2c.h`` header (such a reference now fails to compile).
#. If board factory code branches by touch-chip address, confirm ``esp_board_device_get_i2c_effective_addr()`` returns the expected 8-bit address.

Assisted migration tool
~~~~~~~~~~~~~~~~~~~~~~~~

The repository provides an optional AI Skill to help migrate from ``dev_lcd_touch_i2c`` to ``dev_lcd_touch``. The Skill guides an AI assistant through checking board YAML files, ``setup_device.c``, and application-side legacy type or macro usage, then follows the field mapping and validation steps described in this section.

Read this section first to understand the address format, compatibility macros, and application-code migration requirements. If you want an AI assistant to perform or review the migration, use the ``lcd-touch-i2c-migration`` AI Skill described in :doc:`/tools/ai-skill`.
