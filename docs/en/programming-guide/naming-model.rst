Key Concepts and Naming
=======================

:link_to_translation:`zh_CN:[中文]`

When working with BMGR, the most common source of confusion is not the API itself but the meaning of each naming and configuration layer.

- ``peripherals``: Low-level hardware resource instances on the board, defined in ``board_peripherals.yaml``—for example, a specific ``i2c``, ``spi``, ``i2s``, or ``gpio`` bus.
- ``devices``: Functional device instances, defined in ``board_devices.yaml``—for example, ``audio_codec``, ``display_lcd``, or ``button``.
- ``name``: The actual instance name used within the current board; it is the primary key in board-level configuration. Applications use this name to retrieve handles, for example ``esp_board_manager_get_device_handle("display_lcd", ...)``; devices also use this name to reference specific peripherals.
- ``type``: The category of a device or peripheral, which determines the matching parsing rules and runtime implementation—for example, ``audio_codec``, ``button``, or ``i2c``.
- ``sub_type``: The sub-type of certain devices, used to distinguish different implementation paths within the same type—for example, ``display_lcd`` is divided into ``spi``, ``i80``, ``dsi``, ``rgb``, ``rgb_3wire_spi``, and ``parlio`` based on interface.
- ``role``: The operating mode of certain peripherals—for example, ``adc`` distinguishes between ``oneshot`` and ``continuous`` modes.
- ``format``: The data format of certain peripherals; typical cases include ``i2s`` formats such as ``std-out``, ``std-in``.
- ``dependencies``: Additional component dependency information required by a device; written into ``components/gen_bmgr_codes/idf_component.yml`` during generation.
- ``power_ctrl_device``: Used for devices that require controlled power supply. Referencing a ``power_ctrl``-type device via ``power_ctrl_device`` enables automatic power-on during initialization.
- ``depends_on``: Declares initialization dependencies between devices. During initialization, BMGR automatically checks and initializes the declared dependencies first; the type is not restricted, and a device may declare multiple dependencies.
- ``init_skip``: Marks a device as excluded from :cpp:func:`esp_board_manager_init` automatic initialization; the application must manually initialize it at the appropriate time.
- ``gen_skip``: Typically used to override board defaults, indicating that the current project does not use the device or peripheral; it is skipped during parsing and generation.

In summary, there are two layers: ``peripherals`` describes how low-level resources are connected, and ``devices`` describes the functional capabilities composed from those low-level resources. ``name`` is the in-board instance name; ``type``, ``sub_type``, ``role``, and ``format`` determine the specific implementation path. ``depends_on`` and ``power_ctrl_device`` describe dependency relationships between devices and affect the runtime initialization order; see :doc:`runtime-lifecycle` for details.
