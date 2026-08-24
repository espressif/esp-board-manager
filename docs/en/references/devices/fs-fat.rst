FAT Filesystem (``fs_fat``)
===========================

:link_to_translation:`zh_CN:[中文]`

Overview
--------

The ``fs_fat`` device mounts an SD card as a FAT filesystem. During initialization it calls the ESP-IDF FATFS mount interface and returns a ``dev_fs_fat_handle_t``; the application accesses files under ``mount_point`` through the standard C file API.

This device supports two access modes: SDMMC and SDSPI. SDMMC configures the SDMMC host, slot, and pins directly; SDSPI reuses a board-level ``spi`` peripheral and configures the SD card chip-select pin on the device side.

Supported Usage Modes
---------------------

``fs_fat`` distinguishes usage modes with ``sub_type``:

- `SDMMC`_
- `SPI`_

Minimal Configuration
---------------------

SDMMC
^^^^^

``sdmmc`` mode uses ``SDMMC_HOST_DEFAULT()`` and ``esp_vfs_fat_sdmmc_mount()``. It is suitable for SD cards connected directly to the SDMMC host and does not require a new ``board_peripherals.yaml`` entry. ``ldo_chan_id`` is used for on-chip LDO power control only when the target SoC supports ``SOC_GP_LDO_SUPPORTED``; whether it needs to be configured depends on the board's SDMMC power circuit.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: fs_fat
        type: fs_fat
        sub_type: sdmmc
        config:
          mount_point: "/sdcard"
          vfs_config:
            format_if_mount_failed: false
          sub_config:
            bus_width: 1
            pins:
              clk: -1
              cmd: -1
              d0: -1

SPI
^^^

``spi`` mode uses ``SDSPI_HOST_DEFAULT()`` and ``esp_vfs_fat_sdspi_mount()``. A ``spi`` peripheral must be referenced; at runtime the SPI host port is obtained from that peripheral.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: spi_master
        type: spi
        role: master
        config:
          spi_bus_config:
            spi_port: 1
            mosi_io_num: 11
            miso_io_num: 13
            sclk_io_num: 12

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: fs_fat
        type: fs_fat
        sub_type: spi
        config:
          mount_point: "/sdcard"
          vfs_config:
            format_if_mount_failed: false
          sub_config:
            cs_gpio_num: 15
        peripherals:
          - spi_name: spi_master

All Fields
----------

SDMMC All Fields
^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example FS_FAT device with SDMMC sub device configuration
    - name: fs_fat          # The name of the device, must be unique
      type: fs_fat          # The type of the device, must be unique
      version: 1.0.0
      sub_type: sdmmc       # The sub type of the device, must be 'sdmmc' or 'spi'
      config:
        # Mount point configuration
        mount_point: "/sdcard"            # The mount point path for the SD card filesystem
        # VFS (Virtual File System) configuration
        vfs_config:
          format_if_mount_failed: false   # Format the card if mount fails
          max_files: 5                    # Maximum number of files that can be open simultaneously
          allocation_unit_size: 16384     # Allocation unit size in bytes (0 for default)

        sub_config:
          slot: SDMMC_HOST_SLOT_1           # SD card slot: SDMMC_HOST_SLOT_1 or SDMMC_HOST_SLOT_2
          frequency: SDMMC_FREQ_HIGHSPEED   # Clock frequency:
                                            #   SDMMC_FREQ_DEFAULT      (20000 Hz)
                                            #   SDMMC_FREQ_HIGHSPEED    (40000 Hz)
                                            #   SDMMC_FREQ_PROBING      (400 Hz)
                                            #   SDMMC_FREQ_52M          (52000 Hz)
                                            #   SDMMC_FREQ_26M          (26000 Hz)
                                            #   SDMMC_FREQ_DDR50        (50000 Hz)
                                            #   SDMMC_FREQ_SDR50        (100000 Hz)
          bus_width: 1                      # [TO_BE_CONFIRMED] Bus width: 1, 4, or 8 bits
          slot_flags: SDMMC_SLOT_FLAG_INTERNAL_PULLUP # Slot flags (SDMMC_SLOT_FLAG_INTERNAL_PULLUP, SDMMC_SLOT_FLAG_WP_ACTIVE_HIGH, SDMMC_SLOT_FLAG_UHS1)

          # GPIO pin configuration
          pins:
            clk: -1                         # [IO] Clock pin (-1 for not used)
            cmd: -1                         # [IO] Command pin (-1 for not used)
            d0: -1                          # [IO] Data line 0 pin (-1 for not used)
            d1: -1                          # [IO] Data line 1 pin (-1 for not used)
            d2: -1                          # [IO] Data line 2 pin (-1 for not used)
            d3: -1                          # [IO] Data line 3 pin (-1 for not used)
            d4: -1                          # [IO] Data line 4 pin (-1 for not used, 8-bit mode only)
            d5: -1                          # [IO] Data line 5 pin (-1 for not used, 8-bit mode only)
            d6: -1                          # [IO] Data line 6 pin (-1 for not used, 8-bit mode only)
            d7: -1                          # [IO] Data line 7 pin (-1 for not used, 8-bit mode only)
            cd: -1                          # [IO] Card detect pin (-1 for not used)
            wp: -1                          # [IO] Write protect pin (-1 for not used)

          # For SoCs where the SD power can be supplied both via an internal or external (e.g. on-board LDO) power supply.
          # When using specific IO pins (which can be used for ultra high-speed SDMMC) to connect to the SD card
          # and the internal LDO power supply, we need to initialize the power supply first.
          # This is an optional configuration, You can determine the available channel ID by checking the SoC datasheet.
          # whether it needs to be configured depends on your board.
          # Please check the schematic diagram or other documentation to determine if SDMMC is powered by LDO.
          # e.g., In ESP32-P4 Function-EV Board, VO4_LDO is connected to power the SDMMC IO. set it to 4 for VO4_LDO.
          ldo_chan_id: -1

SPI All Fields
^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example FS_FAT with SPI sub device configuration
    - name: fs_fat          # The name of the device, must be unique
      type: fs_fat          # The type of the device, must be unique
      version: 1.0.0
      sub_type: spi         # The sub type of the device, must be 'sdmmc' or 'spi'
      config:
        # Mount point configuration
        mount_point: "/sdcard"            # The mount point path for the SD card filesystem
        # VFS (Virtual File System) configuration
        vfs_config:
          format_if_mount_failed: false   # Format the card if mount fails
          max_files: 5                    # Maximum number of files that can be open simultaneously
          allocation_unit_size: 16384     # Allocation unit size in bytes (0 for default)

        sub_config:
          cs_gpio_num: 15   # [IO] Chip select GPIO number
      # Peripherals must be defined at device level (consistent with other devices)
      # The SPI peripheral must have: type: spi and role: master
      peripherals:
        - spi_name: spi_master  # [TO_BE_CONFIRMED] SPI bus name (can be any name, e.g., spi_0, spi_sdcard, etc.)

Component Dependencies
----------------------

``fs_fat`` uses the ESP-IDF built-in ``fatfs``, ``sdmmc``, ``driver``, and ``esp_vfs_fat`` interfaces. The current device template does not require additional ``dependencies`` declarations in ``board_devices.yaml``.

Required Peripherals
--------------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - Required
     - Purpose
   * - ``spi``
     - ``master``
     - Required for ``sub_type: spi``
     - Provides the SDSPI bus
   * - None
     - None
     - Not required for ``sub_type: sdmmc``
     - SDMMC host and slot are initialized directly from device configuration

Reference Code
--------------

- ``esp_board_manager/test_apps/main/test_dev_fs_fat.c``
- ``esp_board_manager/devices/dev_fs_fat/dev_fs_fat.c``
- ``esp_board_manager/devices/dev_fs_fat/dev_fs_fat_sub_sdmmc.c``
- ``esp_board_manager/devices/dev_fs_fat/dev_fs_fat_sub_spi.c``
- ``esp_board_manager/examples/play_sdcard_music/main/play_sdcard_music.c``
- ``esp_board_manager/examples/record_to_sdcard/main/record_to_sdcard.c``

Board Reference
---------------

- ``esp_boards/esp32_p4_function_ev_board/board_devices.yaml``
- ``esp_boards/esp32_s3_korvo_2_3/board_devices.yaml``
- ``esp_friends_boards/esp32_s3_korvo_2l/board_devices.yaml``
- ``esp_boards/esp_vocat_1_2/board_devices.yaml``
- ``esp_boards/esp32_lyrat_mini_1_1/board_devices.yaml``
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``

Notes
-----

- For common YAML field rules, see :doc:`/programming-guide/yaml-rules`.
- In ``spi`` mode, an existing SPI peripheral must be referenced in the device's ``peripherals``; initialization will fail if the SPI peripheral name is missing.
- In ``sdmmc`` mode, the pin assignments, bus width, and slot must match the SoC capabilities and board wiring.
- ``format_if_mount_failed: true`` formats the filesystem when mounting fails; set this according to your data retention requirements.
- After modifying YAML, re-run ``idf.py bmgr -b <board>``.

Debugging Tips
--------------

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain the device handle. The handle type is ``dev_fs_fat_handle_t``:

.. code-block:: c

   typedef struct {
       sdmmc_card_t *card;         /*!< SD card card handle */
       sdmmc_host_t  host;         /*!< SD card host handle */
       char         *mount_point;  /*!< Mount point path */
   } dev_fs_fat_handle_t;

``card`` can be passed to the ESP-IDF ``sdmmc`` driver API to query card information; ``mount_point`` matches the ``mount_point`` field in YAML and is the path used for POSIX file API access.

The related declarations are located in ``esp_board_manager/devices/dev_fs_fat/dev_fs_fat.h``.
