Copy an Existing Board
======================

:link_to_translation:`zh_CN:[中文]`

When the new board is hardware-compatible with an existing supported board, copying that board's directory and modifying it is the fastest integration path. If the differences are limited to a small number of pins or components, use :doc:`amend` instead to avoid copying the entire directory.

Steps
-----

**Step 1: Find a Reference Board**

Check the supported board list on the :doc:`/references/boards/index` page to review available hardware combinations (chip, peripherals, devices), and identify the board most similar to your target board.

**Step 2: Copy the Directory and Update the Name**

Ensure the project root contains a ``components/`` directory (standard IDF projects already have one), then copy the reference board directory into it. The destination directory name becomes the new board name:

.. code-block:: bash

   cp -r managed_components/espressif__esp_boards/esp32_s3_korvo_2_3 components/my_board

The example assumes that the reference board comes from the ``espressif/esp_boards`` component downloaded by the component manager. If you use a board component directory from a source checkout, replace the source path with the corresponding ``esp_boards/<board_name>``, ``esp_friends_boards/<board_name>``, or ``m5stack_boards/<board_name>`` directory.

This creates a ``components/my_board/`` directory containing ``board_info.yaml`` and the other board files directly. If ``components/my_board`` already exists, ``cp -r`` will create an extra subdirectory level inside it, resulting in an incorrect structure. Verify the destination does not exist before running the command.

After copying, update the ``board`` field in ``board_info.yaml`` to match the new directory name:

.. code-block:: yaml

   board: my_board   # must match the directory name

Board names may only contain lowercase letters, digits, and underscores. Hyphens are not supported.

**Step 3: Update the Configuration from the Schematic**

Compare the new board's schematic against the reference board and update all differing pins and configuration fields. Common changes include:

- GPIO numbers, bus IDs, and other parameters in ``board_peripherals.yaml``.
- GPIO numbers, chip model (``chip`` field), component dependencies (``dependencies``), I2C addresses, and other settings in ``board_devices.yaml``.
- Metadata fields such as ``chip``, ``description``, and ``manufacturer`` in ``board_info.yaml``.
- If devices or peripherals are added or removed, update the corresponding entries following the field specifications in :doc:`/references/devices/index` and :doc:`/references/peripherals/index`.

For the complete file structure and field descriptions, see :doc:`/programming-guide/board-directory`.

**Step 4: Verify**

.. code-block:: bash

   idf.py bmgr -b my_board

After a successful run, ``components/gen_bmgr_codes/`` should contain: ``gen_board_periph_config.c``, ``gen_board_device_config.c``, ``gen_board_info.c``, ``board_manager.defaults``, ``Kconfig.projbuild``, ``idf_component.yml``, and related files.

Before using the board in production, verify the following:

- The board name in ``board_info.yaml`` matches the directory name.
- All pin numbers have been verified against the schematic.
- All peripheral names referenced by devices exist in ``board_peripherals.yaml``.
- All components listed in ``dependencies`` can be resolved.
- Devices requiring special initialization sequences (LCD display, touchscreen, camera, etc.) have been assessed to determine whether ``setup_device.c`` is needed.

After passing the checks above, run an example or test app on the new board to further verify that peripheral and device initialization works correctly:

- **Examples**: ``esp_board_manager/examples/`` provides audio playback, recording, and LVGL display examples that can be run directly on the new board to validate actual hardware behavior.
- **Test app**: ``esp_board_manager/test_apps/`` provides system-level tests covering the board initialization flow, suitable for comprehensive validation of the generated code.
