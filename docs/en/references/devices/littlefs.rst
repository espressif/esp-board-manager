LittleFS Filesystem (``littlefs``)
==================================

:link_to_translation:`zh_CN:[中文]`

Overview
--------

``littlefs`` is a filesystem device based on the ``joltwallet/littlefs`` component, used to mount LittleFS to a VFS path. After BMGR initializes the device, the application accesses files under the mount path through POSIX file APIs such as ``fopen()``, ``fread()``, and ``fwrite()``.

``littlefs`` selects the mount backend with ``sub_type``:

- ``flash``: mount LittleFS on an on-chip flash partition.
- ``sdmmc``: initialize an SDMMC SD card and mount LittleFS on that card.
- ``spi``: reuse a BMGR SPI master peripheral to initialize an SDSPI SD card and mount LittleFS on that card.

Supported Usage Modes
---------------------

``littlefs`` distinguishes mount backends with ``sub_type``:

- `Flash Partition Mount`_
- `SDMMC Card Mount`_
- `SPI SD Card Mount`_

Minimal Configuration
---------------------

Flash Partition Mount
^^^^^^^^^^^^^^^^^^^^^^

The ``flash`` mode calls ``esp_vfs_littlefs_register`` to mount a flash partition; no new ``board_peripherals.yaml`` entry is required. The project partition table must provide a ``data`` type partition; ``partition_label`` must match the partition name in the partition table.

Partition table example:

.. code-block:: none

    littlefs, data, littlefs, , 0x50000,

In this example, the first ``littlefs`` is the partition name, ``data`` is the partition type, and the second ``littlefs`` is the partition sub-type. ``partition_label`` can be changed to another name but must match the partition name in the partition table.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: littlefs
        type: littlefs
        sub_type: flash
        version: default
        config:
          vfs_config:
            base_path: "/littlefs"
            partition_label: "littlefs"
            format_if_mount_failed: false
            read_only: false
            dont_mount: false
            grow_on_mount: false

SDMMC Card Mount
^^^^^^^^^^^^^^^^

The ``sdmmc`` mode has BMGR initialize the SDMMC host and SD card handle, then passes the SD card handle to ``esp_vfs_littlefs_register``. This mode requires no new ``board_peripherals.yaml`` entry.

Both the SDMMC and SDSPI SD backends depend on the SD card support of the ``joltwallet/littlefs`` component and must be enabled in ``sdkconfig.defaults.board``:

.. code-block:: none

    CONFIG_LITTLEFS_SDMMC_SUPPORT=y

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: littlefs
        type: littlefs
        sub_type: sdmmc
        version: default
        config:
          vfs_config:
            base_path: "/littlefs"
            format_if_mount_failed: false
            read_only: false
            dont_mount: false
            grow_on_mount: false
          sub_config:
            slot: SDMMC_HOST_SLOT_1
            frequency: SDMMC_FREQ_HIGHSPEED
            bus_width: 1
            slot_flags: SDMMC_SLOT_FLAG_INTERNAL_PULLUP
            pins:
              clk: -1
              cmd: -1
              d0: -1

SPI SD Card Mount
^^^^^^^^^^^^^^^^^

The ``spi`` mode reuses a SPI master peripheral already defined in ``board_peripherals.yaml`` and initializes an SDSPI SD card on that bus. The device-level ``peripherals`` must reference a ``spi`` peripheral whose ``role`` is ``master``.

Both the SDMMC and SDSPI SD backends depend on the SD card support of the ``joltwallet/littlefs`` component and must be enabled in ``sdkconfig.defaults.board``:

.. code-block:: none

    CONFIG_LITTLEFS_SDMMC_SUPPORT=y

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: littlefs
        type: littlefs
        sub_type: spi
        version: default
        config:
          vfs_config:
            base_path: "/littlefs"
            format_if_mount_failed: false
            read_only: false
            dont_mount: false
            grow_on_mount: false
          sub_config:
            cs_gpio_num: 15
            frequency: SDMMC_FREQ_DEFAULT
        peripherals:
          - spi_name: spi_master

All Fields
----------

Flash Partition All Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example LittleFS device mounted from flash partition.
    - name: littlefs                          # Device name, must be unique
      type: littlefs                          # Device type, must be littlefs
      version: 1.0.0                          # Schema version
      sub_type: flash                         # Backend subtype: flash, sdmmc, or spi
      config:
        # vfs_config maps to the YAML-safe fields of esp_vfs_littlefs_conf_t
        vfs_config:
          base_path: "/littlefs"              # VFS mount point (default: /littlefs)
          partition_label: "storage"          # Flash partition name in the partition table; flash mode only (default: storage)
          format_if_mount_failed: false       # Format the filesystem if mount fails (default: false; cannot be used with read_only)
          read_only: false                    # Mount as read-only (default: false; cannot be used with format_if_mount_failed or grow_on_mount)
          dont_mount: false                   # Register the filesystem without mounting (default: false)
          grow_on_mount: false                # Grow filesystem to match partition size on mount (default: false; cannot be used with read_only)

SDMMC Card All Fields
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example LittleFS device mounted from SDMMC card.
    # Enable CONFIG_LITTLEFS_SDMMC_SUPPORT=y in sdkconfig.defaults.board for SD backends.
    - name: littlefs                          # Device name, must be unique
      type: littlefs                          # Device type, must be littlefs
      version: 1.0.0                          # Schema version
      sub_type: sdmmc                         # Backend subtype: flash, sdmmc, or spi
      config:
        vfs_config:
          base_path: "/littlefs"              # VFS mount point (default: /littlefs)
          format_if_mount_failed: false       # Format the SD card if mount fails (default: false; cannot be used with read_only)
          read_only: false                    # Mount as read-only (default: false; cannot be used with format_if_mount_failed or grow_on_mount)
          dont_mount: false                   # Register the filesystem without mounting (default: false)
          grow_on_mount: false                # Grow filesystem to match card size on mount (default: false; cannot be used with read_only)
          # partition_label is ignored for SD card backends (sdmmc/spi)
        sub_config:
          slot: SDMMC_HOST_SLOT_1             # SDMMC slot (default: SDMMC_HOST_SLOT_1):
                                              #   SDMMC_HOST_SLOT_0 / SDMMC_HOST_SLOT_1 / SDMMC_HOST_SLOT_2
          frequency: SDMMC_FREQ_HIGHSPEED     # SD clock frequency (default: SDMMC_FREQ_HIGHSPEED):
                                              #   SDMMC_FREQ_DEFAULT   (20 MHz)
                                              #   SDMMC_FREQ_HIGHSPEED (40 MHz)
                                              #   SDMMC_FREQ_PROBING   (400 kHz)
                                              #   SDMMC_FREQ_52M / SDMMC_FREQ_26M
                                              #   SDMMC_FREQ_DDR50 / SDMMC_FREQ_SDR50
          bus_width: 1                        # SDMMC bus width: 1, 4, or 8 bits (default: 1)
          slot_flags: SDMMC_SLOT_FLAG_INTERNAL_PULLUP  # Slot flags (default: SDMMC_SLOT_FLAG_INTERNAL_PULLUP):
                                              #   0 / SDMMC_SLOT_FLAG_INTERNAL_PULLUP /
                                              #   SDMMC_SLOT_FLAG_WP_ACTIVE_HIGH / SDMMC_SLOT_FLAG_UHS1
          pins:
            clk: -1     # [IO] Clock pin (-1 if unused)
            cmd: -1     # [IO] Command pin (-1 if unused)
            d0: -1      # [IO] Data line 0 pin (-1 if unused)
            d1: -1      # [IO] Data line 1 pin (-1 if unused)
            d2: -1      # [IO] Data line 2 pin (-1 if unused)
            d3: -1      # [IO] Data line 3 pin (-1 if unused)
            d4: -1      # [IO] Data line 4 pin, 8-bit mode only (-1 if unused)
            d5: -1      # [IO] Data line 5 pin, 8-bit mode only (-1 if unused)
            d6: -1      # [IO] Data line 6 pin, 8-bit mode only (-1 if unused)
            d7: -1      # [IO] Data line 7 pin, 8-bit mode only (-1 if unused)
            cd: -1      # [IO] Card-detect pin (-1 if unused)
            wp: -1      # [IO] Write-protect pin (-1 if unused)
          ldo_chan_id: -1                     # On-chip LDO channel ID for SDMMC IO power (-1 when unused)

SPI SD Card All Fields
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example LittleFS device mounted from SDSPI card.
    # Enable CONFIG_LITTLEFS_SDMMC_SUPPORT=y in sdkconfig.defaults.board for SD backends.
    - name: littlefs                          # Device name, must be unique
      type: littlefs                          # Device type, must be littlefs
      version: 1.0.0                          # Schema version
      sub_type: spi                           # Backend subtype: flash, sdmmc, or spi
      config:
        vfs_config:
          base_path: "/littlefs"              # VFS mount point (default: /littlefs)
          format_if_mount_failed: false       # Format the SD card if mount fails (default: false; cannot be used with read_only)
          read_only: false                    # Mount as read-only (default: false; cannot be used with format_if_mount_failed or grow_on_mount)
          dont_mount: false                   # Register the filesystem without mounting (default: false)
          grow_on_mount: false                # Grow filesystem to match card size on mount (default: false; cannot be used with read_only)
        sub_config:
          cs_gpio_num: 15                     # [IO] SD card chip-select pin (default: 15)
          frequency: SDMMC_FREQ_DEFAULT       # SD clock frequency (default: SDMMC_FREQ_DEFAULT); same enum values as the SDMMC frequency field
      peripherals:
        - spi_name: spi_master                 # Referenced SPI master peripheral (type: spi, role: master)

In ``esp_vfs_littlefs_conf_t``, the ``partition``, ``sdcard``, and ``blockdev`` fields are runtime mount-source fields and cannot be written in YAML. BMGR sets the corresponding mount source during initialization according to ``sub_type``.

Component Dependencies
----------------------

``littlefs`` introduces ``joltwallet/littlefs`` (version ``7b72caff1de089598a9b5b0b15a7226b790f3c96``) via ``esp_board_manager/idf_component.yml`` when ``CONFIG_ESP_BOARD_DEV_LITTLEFS_SUPPORT`` is enabled. Board YAML does not need to re-declare ``dependencies`` for this common component.

The SDMMC and SDSPI SD backends require ``CONFIG_LITTLEFS_SDMMC_SUPPORT`` to be enabled; otherwise the SD card support fields in ``esp_vfs_littlefs_conf_t`` and the related APIs are not compiled.

Required Peripherals
--------------------

.. list-table::
   :header-rows: 1

   * - ``sub_type``
     - peripheral type
     - Required
     - Purpose
   * - ``flash``
     - None
     - No BMGR peripheral required
     - Mounts a flash partition
   * - ``sdmmc``
     - None
     - No BMGR peripheral required
     - Creates the SDMMC host and SD card handle during device initialization
   * - ``spi``
     - ``spi``
     - Required
     - Reuses the SPI master peripheral to initialize the SDSPI SD card

Reference Code
--------------

- ``esp_board_manager/test_apps/main/test_dev_littlefs.c``
- ``esp_board_manager/devices/dev_littlefs/dev_littlefs.c``
- ``esp_board_manager/devices/dev_littlefs/dev_littlefs_sub_flash.c``
- ``esp_board_manager/devices/dev_littlefs/dev_littlefs_sub_sdmmc.c``
- ``esp_board_manager/devices/dev_littlefs/dev_littlefs_sub_spi.c``

Board Reference
---------------

- ``esp_board_manager/test_apps/components/test_board_littlefs_flash/board_devices.yaml``: flash partition mount test configuration.
- ``esp_board_manager/test_apps/partitions_test_app.csv``: the ``littlefs`` partition configuration in the test app.

Notes
-----

- ``partition_label`` is the partition name in the partition table and can be changed to another name; after changing it, update the partition table accordingly. When generating the image with ``littlefs_create_partition_image``, also update the partition name of the image generation target.
- When the ``flash`` mode does not explicitly configure ``partition_label``, BMGR uses ``storage`` as the partition name by default. A ``data`` partition with the same name must exist in the partition table.
- A ``littlefs`` partition sub-type is recommended. When no mount source is specified, the upstream component looks for the first ``data,littlefs`` partition; the current BMGR flash mode generates a ``partition_label``, so the actual mount is resolved by partition name.
- The SDMMC and SDSPI modes mount LittleFS on the SD card and do not use the LittleFS partition in the flash partition table.
- ``format_if_mount_failed: true`` formats the target filesystem when mounting fails. In SD card modes, this operation erases the existing filesystem content on the card.
- After modifying YAML, re-run ``idf.py bmgr -b <board>``.

Debugging Tips
--------------

- After SD card mode initialization succeeds, use ``sdmmc_card_print_info(stdout, handle->card)`` to print SD card information.
- When the SD card first mounts existing FAT or unformatted media, a LittleFS mount failure is expected. With ``format_if_mount_failed`` enabled, the component formats the SD card.
- In SDSPI mode, check the SPI master peripheral name, the CS GPIO, power control, and bus sharing with other SPI devices.

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain the device handle. The handle type is ``dev_littlefs_handle_t``:

.. code-block:: c

   typedef struct {
       sdmmc_card_t *card;
       sdmmc_host_t  host;
       char         *mount_point;
   } dev_littlefs_handle_t;

``mount_point`` is the VFS mount path. ``card`` and ``host`` are valid only in the ``sdmmc`` and ``spi`` modes, and can be used by the application for SD card diagnostics.

The related declarations are located in ``esp_board_manager/devices/dev_littlefs/dev_littlefs.h``.
