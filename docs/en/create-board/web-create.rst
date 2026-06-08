Create a Board Using the Web Configurator
==========================================================================

:link_to_translation:`zh_CN:[中文]`

The ESP Board Manager Web Configurator creates a board configuration in the browser, exports board-level YAML files, and validates them in a BMGR project. For the tool's feature scope and usage boundaries, see :doc:`/tools/web-config`.

Preparation
--------------

- Prepare the target board's schematic and relevant device datasheets so that pin numbers, addresses, clocks, and timing parameters can be confirmed.
- Confirm the target main chip and the ESP-IDF version you plan to use.

Workflow
--------------

Open the Web Configurator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Visit https://board-manager.espressif.com/ in the browser to reach the main page.

Fill in Basic Information
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the home form, fill in the board name, manufacturer, and description, then select the target ESP-IDF version and main chip. The board name allows only lowercase letters, digits, and underscores, and serves as the ``board`` field in ``board_info.yaml`` as well as the exported file name. If the selected chip is unsupported on the target ESP-IDF version, the form prompts you to choose a higher version.

Besides filling in the basic information manually, you can also import an existing configuration and modify it. The following import sources are supported:

- Import a local board directory: select a local board directory, which must contain at least ``board_info.yaml``, ``board_devices.yaml``, and ``board_peripherals.yaml``.
- Import an official board: select an official board from the ``esp_boards`` component, which BMGR declares as a dependency by default, so no extra declaration is needed.
- Import a board component: select a board component, by default from the ``esp_friends_boards`` and ``m5stack_boards`` components. You can also search and load other board components before selecting and importing.

Once filled in, enter the configuration table.

Add Devices and Peripherals
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Click items in the left device and peripheral libraries to add them, then fill in each configuration card by group. The main groups are:

- **Mode and options**: select fields such as ``sub_type``, ``role``, and ``format`` according to the actual hardware path.
- **IO pin assignment**: fill in real pin numbers from the board's schematic.
- **To-be-confirmed parameters**: fill in values such as clock, address, and timing after consulting device datasheets; these are usually directly related to whether the hardware works correctly.
- **Power control** and **Device dependencies**: configure the device's ``power_ctrl_device`` and ``depends_on`` respectively.
- **IDF component dependencies**: search and select components from the component registry, exported as the device's ``dependencies``.
- **Dependent peripheral configuration**: bind a device to the peripheral instance it references.

For device and peripheral field specifications, see :doc:`/references/index`.

Validation and Correction
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The configuration table validates the selected chip and ESP-IDF version and summarizes issues in the validation results panel. Click an issue row to locate the corresponding configuration card.

Common messages include:

- **Chip capability errors**: the current configuration exceeds what the selected chip supports. These errors block export. Check whether the target chip and ESP-IDF version were selected correctly, or adjust unsupported device, peripheral, and mode combinations.
- **IO pin reuse warnings**: multiple configurations occupy the same pin. These warnings do not block export, but usually require returning to "IO pin assignment" in the relevant cards to reassign pins according to the schematic, or checking whether a peripheral instance was selected incorrectly.

Web validation helps detect obvious issues early, but it does not replace ``idf.py bmgr -b``, compilation, or bring-up on real hardware.

Export Board Files
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

After configuration, export a zip archive named after the board. After extraction, you get:

- ``board_info.yaml``
- ``board_devices.yaml``
- ``board_peripherals.yaml``
- ``setup_device.c``\ (if present)

When a selected device needs board-level initialization logic that pure YAML cannot describe, such as a display factory function, the tool also generates ``setup_device.c``. This file is only a code template generated against the factory function interface; it cannot be used as is. Complete the initialization logic (reset timing, command sequences, parameters, and so on) against the actual hardware and debug it on the board.

Place into the Project
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Place the extracted board directory into the project's ``components/`` directory, with the directory name matching the board name:

.. code-block:: text

   components/
   └── my_board/
       ├── board_info.yaml
       ├── board_devices.yaml
       ├── board_peripherals.yaml
       └── setup_device.c   # if present

Confirm that the ``board`` field in ``board_info.yaml`` matches the directory name.

Generate and Validate
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   idf.py bmgr -b my_board

After successful generation, ``components/gen_bmgr_codes/`` should contain: ``gen_board_periph_config.c``, ``gen_board_device_config.c``, ``gen_board_info.c``, ``board_manager.defaults``, ``Kconfig.projbuild``, ``idf_component.yml``, and so on.

Before formal use, confirm each item:

- The board name in ``board_info.yaml`` matches the directory name.
- All pin numbers have been verified against the schematic.
- Parameters that require device datasheets to confirm, such as clock, address, and timing, have been filled with real values.
- All peripheral names referenced by devices exist in ``board_peripherals.yaml``.
- The components in ``dependencies`` can be resolved correctly.
- If a ``setup_device.c`` was generated, its factory functions have been completed against the actual hardware and debugged successfully.

After passing the checks above, run an example project or test app on the new board to further verify that peripheral and device initialization works:

- **Example projects**: the ``esp_board_manager/examples/`` directory provides scenarios such as audio playback, recording, and LVGL display, which can be switched to the new board and run to verify actual hardware behavior.
- **Test apps**: ``esp_board_manager/test_apps/`` provides system tests for the board initialization flow, suitable for thoroughly verifying the correctness of the generated code.
