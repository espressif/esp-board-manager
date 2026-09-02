Device Reference
================

:link_to_translation:`zh_CN:[中文]`

A device (device) is a board-level object in BMGR that is oriented toward functionality. It describes hardware or software capabilities that can be used directly by the application, such as audio codecs, displays, touch controllers, buttons, cameras, and filesystems. Device entries are written in ``board_devices.yaml``; after parsing, BMGR generates device configurations, component dependencies, and initialization code. The application retrieves device handles by name via :cpp:func:`esp_board_manager_get_device_handle`.

The device reference pages are used to look up the configuration method for a specific ``type``. Common field rules (``name``, ``type``, ``sub_type``, ``chip``, ``version``, ``[IO]``, ``[TO_BE_CONFIRMED]``, ``${BOARD_PATH}``, ``dependencies``, etc.) are described in :doc:`/programming-guide/board-directory` and :doc:`/programming-guide/yaml-rules`. This section only covers the device-type-specific fields, required peripherals, component dependencies, and adaptation constraints for each device type.

.. toctree::
   :maxdepth: 1
   :caption: Device List

   audio-codec
   display-lcd
   lcd-touch
   button
   knob
   led-strip
   ledc-ctrl
   gpio-ctrl
   gpio-expander
   power-ctrl
   camera
   fs-fat
   fs-spiffs
   littlefs
   custom

Configuration Sources
---------------------

BMGR device configurations come from the following sources:

- Device YAML template: ``esp_board_manager/devices/dev_<type>/dev_<type>.yaml``.
- Device header and implementation: ``esp_board_manager/devices/dev_<type>/dev_<type>.h`` and the corresponding ``.c``.

All configurable parameters for each device are listed in the YAML template, including default values and valid ranges. The device header defines the device handle type and configuration type; the device implementation provides the initialization and deinitialization logic. Some devices have special behavior, which is described on the corresponding device page.

BMGR device implementations are typically based on IDF drivers or components. When designing devices, BMGR reuses IDF driver functionality and configuration items as much as possible. Therefore, fields in the device YAML typically correspond one-to-one with IDF driver API parameters, and the device implementation calls the corresponding IDF driver API to complete initialization and deinitialization.

Notes
-----

- After modifying device YAML, re-run ``idf.py bmgr -b <board>`` and rebuild the project.
- ``[IO]`` indicates a pin or hardware resource that must be replaced according to the schematic; ``[TO_BE_CONFIRMED]`` indicates a value that board maintainers must confirm.
- The peripheral ``name`` referenced by a device must match the instance name in ``board_peripherals.yaml``.
- When custom initialization, panel factory functions, or runtime registration logic is needed, first check the corresponding device page to see whether ``setup_device.c`` or a ``custom`` device is required. Board-level C that includes chip-driver headers must be wrapped with ``__has_include``, and factory entries must be weak symbols, so ``gen_skip`` and amend still compile. See :doc:`/programming-guide/board-directory`.
