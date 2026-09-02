SPIFFS Filesystem (``fs_spiffs``)
==================================

:link_to_translation:`zh_CN:[中文]`

.. _fs-spiffs-intro:

Overview
--------

The ``fs_spiffs`` device mounts a SPIFFS partition to a VFS path. After initialization, the application accesses files under ``base_path`` through the standard C file API.

This device does not correspond to external hardware and does not depend on ``board_peripherals.yaml``. The prerequisite is that a SPIFFS partition exists in the project's partition table; ``partition_label`` is used to select the partition.

.. _fs-spiffs-usage-modes:

Supported Usage Modes
---------------------

``fs_spiffs`` has a single usage mode:

- `SPIFFS Partition Mount`_

.. _fs-spiffs-min-config:

Minimal Configuration
---------------------

.. _fs-spiffs-mount:

SPIFFS Partition Mount
^^^^^^^^^^^^^^^^^^^^^^

See complete fields: :ref:`fs-spiffs-mount-full`.

No new ``board_peripherals.yaml`` entry is required.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: fs_spiffs
        type: fs_spiffs
        config:
          base_path: "/spiffs"
          partition_label: NULL

``fs_spiffs`` mounts the filesystem using ``esp_vfs_spiffs_register()`` and prints partition capacity information via ``esp_spiffs_info()``. The device handle stores the runtime-allocated ``esp_vfs_spiffs_conf_t``, which is used to call ``esp_vfs_spiffs_unregister()`` during unmounting.

.. _fs-spiffs-full-fields:

All Fields
----------

.. _fs-spiffs-mount-full:

SPIFFS Partition Mount All Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example SPIFFS filesystem device configuration
    - name: fs_spiffs          # The name of the device, must be unique
      type: fs_spiffs          # The type of the device, must be unique
      version: 1.0.0
      config:
        base_path: "/spiffs"            # The base path for mounting the SPIFFS filesystem
        partition_label: NULL           # [TO_BE_CONFIRMED] The partition label for the SPIFFS filesystem
        max_files: 5                    # The maximum number of files that can be open simultaneously
        format_if_mount_failed: false   # Format the card if mount fails

.. _fs-spiffs-deps:

Component Dependencies
----------------------

``fs_spiffs`` uses the ESP-IDF built-in ``spiffs`` and ``esp_spiffs`` interfaces. The current device template does not require additional ``dependencies`` declarations in ``board_devices.yaml``.

.. _fs-spiffs-peripherals:

Required Peripherals
--------------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - Required
     - Purpose
   * - None
     - None
     - Not required
     - Uses the SPIFFS partition from the project partition table

.. _fs-spiffs-code:

Reference Code
--------------

- ``esp_board_manager/test_apps/main/test_dev_fs_spiffs.c``
- ``esp_board_manager/devices/dev_fs_spiffs/dev_fs_spiffs.c``

.. _fs-spiffs-boards:

Board Reference
---------------

- ``esp_friends_boards/esp32_s3_korvo_2l/board_devices.yaml``

.. _fs-spiffs-notes:

Notes
-----

- For common YAML field rules, see :doc:`/programming-guide/yaml-rules`.
- A SPIFFS partition matching ``partition_label`` must exist in the project partition table; ``partition_label: NULL`` selects the default SPIFFS partition.
- ``format_if_mount_failed: true`` formats the filesystem when mounting fails; set this according to your data retention requirements.
- After modifying YAML, re-run ``idf.py bmgr -b <board>``.

.. _fs-spiffs-debug:

Debugging Tips
--------------

.. _fs-spiffs-api:

API Reference
-------------

``fs_spiffs`` is mounted as a virtual filesystem; :cpp:func:`esp_board_manager_get_device_handle` returns ``NULL`` for this type. The application accesses the ``base_path`` mount point configured in YAML through the POSIX file API.

The device configuration struct ``dev_fs_spiffs_config_t`` and initialization functions are located in ``esp_board_manager/devices/dev_fs_spiffs/dev_fs_spiffs.h``.
