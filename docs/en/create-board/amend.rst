Using -a/--amend
================

:link_to_translation:`zh_CN:[中文]`

When making minor differences on an existing board (such as modifying a pin or replacing a touch chip) or adding devices not included by default, there is no need to copy the entire board directory. Prepare an **amend directory** containing a ``board_amend.yaml`` manifest, then apply changes as patches on top of the selected board during generation using ``-a/--amend <dir>``:

.. code-block:: bash

   # The amend directory is an absolute or relative path and must contain board_amend.yaml
   idf.py bmgr -b esp32_s3_korvo_2_3 -a path/to/my_amend

   # When the amend directory is placed under the selected board directory, pass the subdirectory name directly
   # For example: esp_boards/esp32_s3_lcd_ev_board/sub_board_800_480_lcd
   idf.py bmgr -b esp32_s3_lcd_ev_board -a sub_board_800_480_lcd

``-a`` accepts absolute or relative paths; relative paths are resolved from the current working directory (``cwd``). **If the amend directory is placed under the selected board directory, only the subdirectory name needs to be passed, and BMGR will look for the corresponding subdirectory inside that board directory.**

Different sub-boards, screen modules, or minor hardware variants of the same main board are recommended to be placed uniformly under that board's subdirectory (for example, ``esp_boards/esp32_s3_lcd_ev_board/sub_board_800_480_lcd/``); only the subdirectory name needs to be passed when using it, and it is not affected by the project path.

.. note::

   Besides the explicit ``-a`` above, BMGR can also **auto-discover and apply an amend by board name** (auto-amend), which is especially handy for keeping a single command across many boards in a multi-board example project. See :ref:`auto-amend` below.

Basic structure of the amend directory:

.. code-block:: text

   my_amend/
     board_amend.yaml          # Required: manifest file
     tweak.yaml                # YAML fragment, must be listed under apply:
     extra_setup.c             # Optional source file, must be listed under apply:
     sdkconfig.defaults.board  # Optional, must be listed under apply:

Manifest File
-------------

``board_amend.yaml`` format:

.. code-block:: yaml

   version: "1.0"
   description: "Add external sensor power control"

   apply:                        # Ordered list; later entries override earlier ones
     - tweak.yaml
     - extra_setup.c
     - sdkconfig.defaults.board

Files at the amend root (including ``sdkconfig.defaults.board`` and ``Kconfig.projbuild``) **must be explicitly listed in** ``apply:`` **to take effect**. Files that are placed but not listed will be ignored and an info log will be output. Directory entries are not supported; files in subdirectories must be listed with their full relative paths, such as ``pack/extra.yaml``.

Supported path formats for each entry in ``apply:``:

- **Relative path**: Resolved relative to the directory containing ``board_amend.yaml``, for example ``tweak.yaml``, ``pack/extra.yaml``, ``../shared/extra_setup.c``.
- **Absolute path**: Used directly, for example ``/abs/path/to/extra_setup.c``.

Regardless of the format, directory entries are not supported. Files in subdirectories must be listed with their full filenames expanded (``pack/extra.yaml``, not ``pack``).

YAML Fragment Merge Rules
-------------------------

Each YAML fragment must contain ``devices:`` or ``peripherals:`` at the top level. Merging is performed in the order of ``apply:``. Devices or peripherals with the same name undergo field-level merging (``config`` uses deep merge); names that do not exist are appended to the end of the list.

.. code-block:: yaml

   # Example tweak.yaml: add a peripheral and the corresponding power control device
   peripherals:
     - name: gpio_sensor_power
       type: gpio
       role: io
       version: default
       config:
         pin: 4                  # [IO]
         mode: GPIO_MODE_OUTPUT

   devices:
     - name: sensor_power
       type: power_ctrl
       sub_type: gpio
       version: default
       peripherals:
         - name: gpio_sensor_power
           active_level: 1

Source File Override
--------------------

``.c``, ``.cpp``, ``.cc``, ``.cxx``, and ``.S`` files listed in ``apply:`` are compiled into the generated component. The generated component sets ``WHOLE_ARCHIVE``, so strong symbols provided by the amend override weak symbol functions of the same name in the base board. The typical usage is to rewrite initialization hooks in ``setup_device.c``. Factory functions, ``custom`` implementations, and chip-driver ``DEVICE_EXTRA_FUNC_REGISTER`` hooks on the base board should uniformly use ``__attribute__((weak))`` (for factory entries with external linkage) together with ``__has_include``, so an amend can replace them and a downstream ``gen_skip`` can still compile. See the ``setup_device.c`` section in :doc:`/programming-guide/board-directory`.

Cross-Board Module Reuse
------------------------

``apply:`` supports relative paths, resolved from the directory containing ``board_amend.yaml``, allowing references to files outside the amend directory at other locations in the project. Using this feature, common peripheral and device configurations can be split into independent YAML and source code fragments, stored centrally in a shared directory, and then referenced on demand by different boards' amend files—effectively assembling a complete board-level configuration from reusable feature modules.

For example, to split the gas sensor adaptation into an independent fragment and place it in the project shared directory:

.. code-block:: text

   sensors/
     gas_sensor/
       gas_sensor.yaml   # Peripheral and device declarations
       gas_sensor.c      # Initialization implementation (optional)

A board's ``board_amend.yaml`` references it via a relative path:

.. code-block:: yaml

   version: "1.0"
   description: "Board A: base board + gas sensor"

   apply:
     - ../sensors/gas_sensor/gas_sensor.yaml
     - ../sensors/gas_sensor/gas_sensor.c

Another board that needs the same sensor directly reuses the same files without needing to maintain them again:

.. code-block:: yaml

   version: "1.0"
   description: "Board B: base board + gas sensor + extra periph"

   apply:
     - ../sensors/gas_sensor/gas_sensor.yaml
     - ../sensors/gas_sensor/gas_sensor.c
     - extra_periph.yaml   # Board-specific additional tweaks

As feature modules accumulate in the shared directory, adapting new boards can rely increasingly on existing modules—combining the required fragments in ``apply:`` rather than writing repetitive YAML content from scratch.

.. _auto-amend:

Auto-amend
----------

Besides the explicit ``-a``, BMGR can **auto-discover and apply an amend by board name**. Under the scan paths (including directories given via ``-c/--customer-path``), if there is a directory **whose name matches the selected board, contains** ``board_amend.yaml`` **, and is not itself a complete board directory**, BMGR applies it as that board's amend automatically, without ``-a``.

The expected layout is ``<scan_root>/<board_name>/board_amend.yaml``:

.. code-block:: text

   board_overlays/                       # point -c here
     esp32_s3_box_3/
       board_amend.yaml
       box_tweak.yaml
     esp32_p4_function_ev_board/
       board_amend.yaml
       p4_tweak.yaml

The ``board_amend.yaml`` inside an auto-amend directory uses the same syntax, fragment merge rules, source override, and cross-board reuse as the explicit ``-a`` (``apply:`` is still the single source of truth).

Key points:

- ``-c/--customer-path`` accepts multiple semicolon-separated paths, for example ``-c "overlays_a;overlays_b"``; they are scanned in order and later paths take precedence.
- Explicit ``-a`` and auto-amend can be combined; explicit ``-a`` has the highest priority (applied last, overriding auto-amend).
- A complete board directory is treated as a board even if it contains ``board_amend.yaml``; it is never auto-applied to itself.
- Auto-discovery recurses up to 3 levels below each scan root.
- Set the environment variable ``ESP_BOARD_MANAGER_DISABLE_AUTO_AMEND=1`` to disable auto-discovery and keep only the explicit ``-a``.

Two ways to use amend in a demo
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When writing an example project that supports multiple boards, the biggest value of auto-amend is **keeping a single command across all boards**.

**Option 1: All boards share the same amend**

When every target board needs the same patch set (for example, enabling a debug option or adding the same external module), point the explicit ``-a`` at the same amend directory; the amend part of each command stays identical:

.. code-block:: bash

   idf.py bmgr -b esp32_s3_box_3             -a path/to/common_amend
   idf.py bmgr -b esp32_p4_function_ev_board -a path/to/common_amend

**Option 2: Different boards need different amends, with consistent commands**

When each board needs its own patch, the old approach required a different ``-a`` path per board, so commands varied and were easy to confuse. With auto-amend, simply collect every board's overlay under one root, with a subdirectory per board name:

.. code-block:: text

   board_overlays/
     esp32_s3_box_3/board_amend.yaml                # box-specific patch
     esp32_p4_function_ev_board/board_amend.yaml    # p4-specific patch

Then all boards use the **exact same** command, changing only the ``-b`` board name while ``-c`` always points to the same overlay root; BMGR matches and applies the right amend by board name:

.. code-block:: bash

   idf.py bmgr -b esp32_s3_box_3             -c board_overlays
   idf.py bmgr -b esp32_p4_function_ev_board -c board_overlays

When no overlay exists for a board, the command generates the base board normally without error. In CI or multi-board demo scripts, you can loop the same command over a list of board names without maintaining per-board amend arguments.
