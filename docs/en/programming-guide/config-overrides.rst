Configuration Extensions and Runtime Overrides
==============================================

:link_to_translation:`zh_CN:[中文]`

BMGR supports changing a board's default configuration, using the same firmware across hardware revisions, and adjusting device or peripheral configuration before initialization. The mechanisms below differ in when and how configuration is applied.

Extension mechanisms
--------------------

Select a mechanism based on what needs to change and when the change is applied:

.. list-table:: Mechanism selection by requirement
   :header-rows: 1

   * - Use case
     - Recommended mechanism
     - Configuration and timing
   * - Override the default configuration, connect a known sub-board, or add a reusable functional module.
     - :ref:`Generation-time configuration changes (amend) <board-amend>`
     - Write the differences in configuration fragments, merge them into one configuration during generation, then build the firmware.
   * - One board has multiple hardware revisions that must share the same firmware, and each revision's differences can be written in YAML ahead of time.
     - :ref:`Runtime configuration selection (board_revisions) <board-revisions-runtime>`
     - Retain the base configuration and generate revision-specific differences. Detect the hardware and call the matching revision apply function before resource initialization.
   * - Adjust device or peripheral configuration from application code at runtime, or use a custom device initialization function.
     - :ref:`Application runtime overrides <app-runtime-overrides>`
     - Call ``override_config`` or ``set_ops`` by resource name before initializing the target resource.
   * - Device types or connections must be determined dynamically at runtime. New device nodes and bus bindings must be constructed, or existing resources removed, and these changes are difficult to describe in YAML ahead of time.
     - :ref:`Writable resource tables and C nodes <writable-tables>`
     - Generate writable tables with ``--writable``. C code maintains resource nodes, links, and the required capacity.
   * - Every project using the board needs the same board-level setup before hardware resources are initialized.
     - :ref:`Board pre-init hook <board-pre-init-runtime>`
     - Put the logic in the board's ``esp_board_runtime_pre_init()``. BMGR runs it automatically before normal resource initialization.

These mechanisms can be combined. ``board_revisions`` describes revision differences known ahead of time; the pre-init hook runs board hardware detection and adaptation. The hook can select a revision or call override APIs to adjust existing resources.

.. _board-amend:

Generation-time configuration changes (``-a/--amend``)
------------------------------------------------------

Amend uses the ``apply:`` list in ``board_amend.yaml`` to merge configuration fragments into the selected board's base configuration in order. It can modify existing fields, add devices or peripherals, and include source files, Kconfig, and sdkconfig defaults.

Select an amend directory with ``-a/--amend``, or use the directory convention in :ref:`auto-amend` for automatic discovery. Fragments are merged during code generation, and subsequent builds use the merged configuration. The original YAML configuration in the board directory does not need to be modified directly. See :doc:`/create-board/amend` for directory layout and merge rules.

Usage examples
~~~~~~~~~~~~~~

- To adjust LCD parameters, skip automatic initialization of a device, or add project-specific sdkconfig defaults, write the differences in amend fragments and reference them from the project's ``board_amend.yaml``.
- To add a device or peripheral whose resources are known before code generation, add the corresponding ``devices:`` or ``peripherals:`` entries in amend.
- To connect different sub-boards to the same main board, prepare an amend directory for each sub-board and select the appropriate directory during generation. The `800 × 480 display sub-board for ESP32-S3-LCD-EV-Board <https://github.com/espressif/esp-board-manager/tree/main/esp_boards/esp32_s3_lcd_ev_board/sub_board_800_480_lcd>`_ provides configuration fragments and panel initialization sources.
- To reuse a functional module across boards, put its configuration and source files in a shared directory and reference them through ``apply:`` in each board's amend. The `MAX30102 module <https://github.com/espressif/esp-health/blob/main/esp_health/examples/heart_rate_detect/components/amend/max30102/README.md>`_ reuses the target board's existing I2C bus and adds an INT GPIO and a custom device. When connecting it to another board, adjust GPIO and bus bindings to match the actual wiring.

If an example project supports multiple boards, prepare an automatically discovered amend directory for each board name. The board can still be selected with ``idf.py bmgr -b <board>``, without passing ``-a`` each time. See :ref:`auto-amend` for the directory convention.

.. _board-revisions-runtime:

Runtime configuration selection (``board_revisions``)
------------------------------------------------------

When a board has multiple hardware revisions that must be detected and supported by one board definition and firmware image, declare ``board_revisions`` in ``board_amend.yaml`` at the board directory root. The base configuration and revision-specific differences are all generated and built. Board code selects the revision to use at runtime.

``board_revisions`` supports declaring multiple revisions. Each revision derives independently from the base configuration and merges its difference files in list order. Later files override matching fields from earlier files. For example:

.. code-block:: yaml

   board_revisions:
     v1_2:
       - board_revision_diff_v1_2.yaml
     v1_3:
       - board_revision_diff_v1_2.yaml
       - board_revision_diff_v1_3.yaml

If a new revision depends on differences from an earlier revision, explicitly list the required difference files. In this example, v1_3 applies the v1_2 differences first, then the v1_3 differences. If its list contains only ``board_revision_diff_v1_3.yaml``, that file is applied directly to the base configuration; v1_2 configuration is not inherited automatically.

The generator creates the following C files for each revision, where ``<name>`` is the revision name, such as ``v1_2``:

* ``gen_board_device_config_<name>.c``
* ``gen_board_periph_config_<name>.c``
* ``gen_board_revision_<name>.c``

The generated ``gen_board_revisions.h`` declares ``gen_board_revision_apply_<name>()``. After detecting the hardware revision, board code calls the corresponding function to update device and peripheral configuration tables. Declaring ``board_revisions`` alone does not select a configuration; without an apply call, BMGR uses the base configuration.

Revision difference files follow the amend YAML merge rules described in :doc:`/create-board/amend`. They also support ``unused: true`` to indicate that a revision does not use a resource. Calling the corresponding apply function removes that resource from the active resource table.

``board_revisions`` can be combined with generation-time amend. Amend is merged after revision fragments and applies to the base configuration and all revisions; for the same field, the application amend value wins. Application amend can modify resources present in the base configuration, but cannot modify resources added only by a revision. Application amend with ``gen_skip: true`` excludes the resource from the base configuration and all revisions during generation.

Usage examples
~~~~~~~~~~~~~~

If revision v1.2 changes I2C wiring or adds or removes devices, write these differences in ``board_revision_diff_v1_2.yaml``, then detect the revision and call its apply function in the pre-init hook. The `ESP-Mosaico revision difference file <https://github.com/espressif/esp-board-manager/blob/main/esp_boards/esp_mosaico/board_revision_diff_v1_2.yaml>`_ provides an example.

Board code calls the generated API as follows. ``board_is_v1_2()`` represents hardware detection logic that must be implemented by board code:

.. code-block:: c

   #include "gen_board_revisions.h"

   esp_err_t esp_board_runtime_pre_init(void)
   {
       if (board_is_v1_2()) {
           return gen_board_revision_apply_v1_2();
       }
       return ESP_OK;
   }

.. _app-runtime-overrides:

Application runtime overrides
-----------------------------

Before initializing a device or peripheral, an application can change its configuration or replace its initialization and deinitialization functions by resource name. The target resource must already exist in BMGR's resource tables.

* Configuration: ``esp_board_device_override_config`` and ``esp_board_periph_override_config``.
* Initialization and deinitialization functions: ``esp_board_device_set_ops`` and ``esp_board_periph_set_ops``.
* Restore configuration or functions: the corresponding ``restore_config`` and ``restore_ops`` APIs.

Modification and restore operations return ``ESP_ERR_INVALID_STATE`` while a resource is initializing or remains unreleased. Init can be retried after an init failure if resources have been released. Init failure with a non-NULL handle blocks subsequent init/deinit for that resource. A normal deinit failure after successful initialization can be retried. See :ref:`bmgr-init-failure` for the scenarios.

``override_config`` copies the configuration structure but does not copy data referenced by its pointer fields. This data must remain valid while the override is active. See :doc:`/programming-guide/advanced-runtime` for complete API rules.

Override priority and restoration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A resource can have the following configuration sources. BMGR selects the effective configuration by priority:

* Base configuration: generated device or peripheral configuration. After revision apply or direct code changes to a configuration table, the modified table is used.
* Board overrides (BOARD): configuration installed by ``override_config`` or ``set_ops`` in the pre-init hook.
* Application overrides (APP): configuration installed by the application through the same APIs.

The priority is base configuration < BOARD < APP. Initialization and deinitialization functions follow the same priority order and are tracked separately from configuration. Changing only configuration preserves the current function selection; replacing only functions preserves the current configuration selection.

``restore_config`` clears only the APP configuration override. BMGR then uses the BOARD configuration override if one exists, or the configuration in the current base table otherwise. ``restore_ops`` follows the same restoration order for initialization and deinitialization functions.

For example, if the board hook has changed a device's configuration and the application then overrides the same device, calling ``restore_config`` returns to the configuration set by the board hook. Restore does not undo revision apply or C code changes to the base configuration table. Deinitializing a device or BMGR does not clear installed overrides either.

Usage examples
~~~~~~~~~~~~~~

If parameters need to be calculated at runtime according to application requirements, perform these steps before initializing the target resource:

#. Query the current effective configuration through the configuration query API.
#. Copy the configuration structure and modify the required fields.
#. Call the corresponding ``override_config`` API, then initialize the resource.

If the application needs to create an existing device with its own initialization function, call ``set_ops`` before initialization and provide both initialization and deinitialization functions.

Keep changes that affect only the current application in application code. Put changes that every project using the board must apply after hardware detection in the :ref:`board-pre-init-runtime`.

.. _writable-tables:

Generating writable descriptor tables and C nodes
-------------------------------------------------

A descriptor is a generated device or peripheral description containing its name, type, configuration, and other information. Generated descriptor tables are read-only by default. To add, remove, or relink resource nodes from C code at runtime, generate writable tables with ``--writable``:

.. code-block:: bash

   idf.py bmgr -b <board> --writable

The short option for ``--writable`` is ``-w``. When enabled, ``g_esp_board_devices[]`` and ``g_esp_board_peripherals[]`` become writable. C code can modify fields such as ``next``, name, type, ``cfg``, and ``cfg_size``.

Edit tables in the prepare-owner task before initializing resources, and serialize any other table-editing path against initialization and cleanup. Generated revision apply functions check resource state internally and return ``ESP_ERR_INVALID_STATE`` without editing tables if resources are active, initialization left unreleased resources, or prepare requires a restart.

With ``--writable`` alone, ``ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM`` and ``ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM`` default to the number of generated devices and peripherals, respectively. No capacity is reserved for new resources. To add nodes, increase the corresponding project Kconfig capacity to the number of generated nodes plus the maximum number of new nodes.

A new device requires descriptor and handle nodes with matching names and types. A new peripheral requires a descriptor node and operations installed through ``esp_board_periph_set_ops``. Nodes, configurations, and referenced data must remain valid while in use. See :doc:`/programming-guide/runtime-lifecycle` for node-linking and initialization requirements.

Declaring ``board_revisions`` also generates writable tables and sets a minimum capacity based on the resources required by each revision. When revision differences can be written in YAML, use ``board_revisions`` to describe and manage the hardware differences. See :ref:`board-revisions-runtime`.

.. _board-pre-init-runtime:

Board pre-init hook
-------------------

Board code can define ``esp_board_runtime_pre_init()`` in a source file such as ``setup_device.c`` to replace BMGR's default implementation, detect hardware, and adjust the configuration required for initialization.

BMGR automatically checks board runtime preparation during whole-board initialization, individual device or peripheral initialization, configuration or handle queries, and override updates and restores. If the board runtime is unprepared, BMGR runs the hook and runtime validation before continuing the requested operation. Applications can use these APIs directly without explicitly calling ``esp_board_manager_prepare()`` to trigger the hook.

Configuration queries can be used before resource initialization. Handle queries also trigger preparation, but do not automatically initialize the target resource. A handle query returns an error if the target resource's handle has not been created.

BMGR records prepare state to avoid repeated calls to ``esp_board_runtime_pre_init()``. If the hook or runtime validation fails without leaving resources, BMGR immediately restores its saved tables and BOARD-layer overrides and returns to the unprepared state. A later call can retry preparation.

The hook can initialize hardware resources. If it returns an error, it must first deinitialize the hardware resources it created; BMGR runtime rollback does not perform hardware cleanup. Overrides installed through ``override_config`` or ``set_ops`` in the hook belong to the BOARD layer. See :ref:`app-runtime-overrides` for priority and restoration rules.

If prepare fails with unreleased resources, BMGR enters ``FAILED``. Normal runtime operations are then rejected for every resource, including resources that initialized successfully, until the system restarts. The current tables and required config remain allocated, while the old table backups are freed. BMGR logs ``restart required`` without restarting automatically; the application decides whether to restart. See :ref:`bmgr-prepare-failure` for a scenario table and example. Apply board revisions before initializing resources.
