FAQ
===

:link_to_translation:`zh_CN:[中文]`

The FAQ targets users who can already run BMGR normally but encounter problems. It consolidates troubleshooting approaches, files to inspect, and recommended actions for frequently reported errors. It is recommended to first identify the phase of the problem, then continue troubleshooting based on the specific error, rather than diving directly into business code.

- **Generation phase**: Wrong board selection, incomplete YAML, incorrect dependency description, missing generated artifacts.
- **Compilation phase**: Missing macros, missing components, missing generated files, link failure.
- **Runtime phase**: Initialization order, power supply, callback, and configuration timing anomalies.

General troubleshooting order: first confirm the currently selected board and whether the most recent ``idf.py bmgr`` executed successfully; then check whether ``gen_board_*.c``, ``board_manager.defaults``, ``Kconfig.projbuild``, and ``gen_board_metadata.yaml`` under ``components/gen_bmgr_codes`` are all present; then confirm whether the generated output actually participates in compilation and linking; finally troubleshoot runtime initialization, handle acquisition, power supply, and timing issues.

Generation Phase
----------------

``idf.py bmgr`` or ``idf.py gen-bmgr-config`` Command Not Found
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Possible Causes**

- ESP-IDF has not discovered ``idf_ext.py`` in the ``esp_board_manager`` component.
- In ESP-IDF versions before v6.0, ``IDF_EXTRA_ACTIONS_PATH`` does not point to the correct path.
- The current shell has not reloaded the ESP-IDF environment.

**Recommended Actions**

1. Confirm that the main component manifest declares the ``espressif/esp_board_manager`` dependency, and that ``idf.py menuconfig`` or ``idf.py build`` has been run to let the component manager download the component to ``managed_components/``.
2. Restart the terminal session or re-run ``source export.sh`` to apply the ESP-IDF environment variables.
3. ESP-IDF v6.0 and above uses the auto-discovery mechanism with the component's own ``idf_ext.py``; the project must be able to recognize the ``esp_board_manager`` component. Versions below v6.0 require ``IDF_EXTRA_ACTIONS_PATH`` to be set correctly.

``esp_board_manager`` Component Path Not Found
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Possible Causes**

- The main component manifest ``idf_component.yml`` does not declare the dependency.
- No command has been run to trigger the component manager download after adding the dependency.

**Recommended Actions**

1. Check the project's main ``idf_component.yml`` and confirm it contains the ``espressif/esp_board_manager`` dependency.
2. After adding the dependency, run ``idf.py menuconfig`` or ``idf.py build``; the component manager will download ``esp_board_manager`` to ``<project>/managed_components/``.

Board Name Not Found or Misspelled
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Possible Causes**

- The board name is not among those scanned in the current project.
- The board directory name contains uppercase letters, hyphens, or other disallowed characters, and is ignored during the scan phase.
- In the current version (0.5.x), official boards are built into BMGR and require no additional dependency declaration. Starting from 0.6, official boards have been migrated to independent components; some non-official boards require manual dependency declaration. See :doc:`/references/boards/index` for specific components.

**Recommended Actions**

1. Run ``idf.py bmgr -l`` to view all boards actually recognized and their source labels.
2. Board names must contain only lowercase letters, digits, and underscores, and must match the ``board`` field in ``board_info.yaml``.
3. To temporarily verify an external board directory, pass the path directly: ``idf.py bmgr -b /abs/path/to/board``.

Generated Result Unchanged After YAML Modification
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Possible Causes**

- ``idf.py bmgr -b <board>`` has not been re-run; BMGR does not automatically regenerate due to YAML modifications.
- The file being modified is in an amend directory but is not listed under ``apply:`` in ``board_amend.yaml``.
- The file being modified belongs to another board, the currently selected board is not that board, or the directory containing the modified file is overridden by a higher-priority board of the same name. See :doc:`/programming-guide/board-path-priority` for priority rules.

**Recommended Actions**

1. Re-run ``idf.py bmgr -b <board> [-a <amend>]`` and check whether ``components/gen_bmgr_codes`` has been refreshed.
2. Check whether the amend ``apply:`` list includes the modified file. Files not listed will only produce an INFO log and will not participate in the merge.
3. Confirm the actual directory of the currently selected board via ``idf.py bmgr -b <board>``; see the ``Board path: <path>`` information in the log.

Compilation Phase
-----------------

undefined reference to g_esp_board_*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Typical symbols: ``g_esp_board_devices``, ``g_esp_board_device_handles``, ``g_esp_board_peripherals``.

**Possible Causes**

1. ``idf.py bmgr -b <board>`` did not complete successfully; ``components/gen_bmgr_codes`` is missing required files. In addition to the generated ``.c`` and ``.h`` files, it **must** also contain ``CMakeLists.txt`` (and usually ``idf_component.yml`` and ``board_manager.defaults`` generated at the same time). If the directory only contains some ``.c`` files or is missing ``CMakeLists.txt``, ESP-IDF will not register that directory as a component, the generated source will not be compiled, and this type of undefined reference will appear at link time.
2. The project has enabled minimal or stripped builds, such as ``idf_build_set_property(MINIMAL_BUILD ON)`` or ``set(COMPONENTS main)``. The former retains only minimal common components; the latter only builds explicitly listed components and their dependencies. In either case, if ``gen_bmgr_codes`` is not explicitly included in the build scope, the board-level generated code will not be compiled.

**Recommended Actions**

1. First confirm that all files under ``components/gen_bmgr_codes`` are present. The minimum set should include: ``gen_board_info.c``, ``gen_board_device_config.c``, ``gen_board_device_handles.c``, ``gen_board_periph_config.c``, ``gen_board_periph_handles.c``, ``gen_board_device_custom.h``, ``CMakeLists.txt``, ``idf_component.yml``, ``board_manager.defaults``, ``Kconfig.projbuild``.
2. If any files are missing, re-run ``idf.py bmgr -b <board>``.
3. In stripped build scenarios, explicitly add ``gen_bmgr_codes`` to the build scope (add the component to the ``set(COMPONENTS ...)`` list, or confirm that the dependency chain can reach that component).

Header File, Macro, or Generated Symbol Not Found
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Many BMGR issues are rooted not in the application layer, but in misaligned YAML, generated artifacts, or default configuration. It is recommended to check the following in order, rather than troubleshooting only in the business code:

1. Whether ``components/gen_bmgr_codes`` is correctly generated and all files are present (same as above).
2. ``gen_board_metadata.yaml``: Whether the parsed devices and peripherals match expectations.
3. ``gen_board_device_config.c`` and ``gen_board_periph_config.c``: Whether specific field values match expectations.
4. ``board_manager.defaults``: Whether capability macros such as ``CONFIG_ESP_BOARD_DEV_*_SUPPORT``, ``CONFIG_ESP_BOARD_PERIPH_*_SUPPORT``, and ``CONFIG_ESP_BOARD_DEV_*_SUB_*_SUPPORT`` are all present.
5. ``Kconfig.projbuild``: Whether ``CONFIG_ESP_BOARD_<BOARD>`` corresponding to the currently selected board is declared.
6. ``sdkconfig``: Whether the above capability macros are actually written into sdkconfig.

Component Dependency Resolution Failed / Version Solving Failed
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Typical errors:

.. code-block:: text

   ERROR: Because project depends on xxxxx which
   doesn't match any versions, version solving failed.

.. code-block:: text

   Failed to resolve component 'esp_board_manager' required by component
     'gen_bmgr_codes': unknown name.

**Possible Causes**

- The ``idf_component.yml`` in ``gen_bmgr_codes`` from the last run still contains dependencies that are no longer applicable.
- The component name or version constraint in ``dependencies`` within the YAML is incorrect.
- The dependency source (repository component, local component, private component) is inconsistent. For example, a dependency was originally a component repository package but has been replaced by a local path without cleaning the same-named dependency.

**Recommended Actions**

1. First use ``idf.py bmgr -x`` (equivalent to the old command ``idf.py gen-bmgr-config -x`` or ``python gen_bmgr_config_codes.py -x``) to clean generated artifacts. This command deletes the generated ``.c`` and ``.h`` files, resets ``gen_bmgr_codes/CMakeLists.txt`` and ``idf_component.yml``, and removes ``board_manager.defaults``.
2. Then run ``idf.py bmgr -b <board>`` to regenerate.
3. If errors persist, refer to the `IDF Component Manager Manifest documentation <https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/manifest_file.html#dependencies>`_ to check the syntax of the ``dependencies`` field in the device YAML.

Runtime Phase
-------------

Abnormal Behavior or sdkconfig Inconsistency After Switching Boards
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Possible Causes**

- The switch was performed via ``idf.py menuconfig`` or by manually modifying ``sdkconfig``, without triggering cleanup and regeneration through ``idf.py bmgr -b <board>``.
- Capability macros from the previous board are still in ``sdkconfig``.
- ``CONFIG_ESP_BOARD_DEV_*_SUPPORT``, ``CONFIG_ESP_BOARD_PERIPH_*_SUPPORT``, ``CONFIG_ESP_BOARD_DEV_*_SUB_*_SUPPORT``, or other BMGR-managed symbols were manually written into the project's ``sdkconfig.defaults``, interfering with the board-switching logic.

**Recommended Actions**

1. Switching boards must use ``idf.py bmgr -b <other_board>`` or an equivalent script entry; ``idf.py menuconfig`` should not be used for switching. BMGR automatically backs up the old ``sdkconfig`` as ``components/gen_bmgr_codes/sdkconfig.bmgr_board.old`` and deletes the original file when switching boards, preventing residual ``CONFIG_IDF_TARGET`` and device enable configurations from the old board.
2. Do not manually write BMGR device, peripheral, or device sub-type capability symbols into the project ``sdkconfig.defaults``; these symbols should come only from the BMGR-generated ``board_manager.defaults``. Board-specific regular sdkconfig items (PSRAM, Flash, partition, etc.) should be placed in ``sdkconfig.defaults.board`` under the board directory.
3. To return to the current board's default values, delete the project ``sdkconfig`` and re-run ``idf.py build`` to let ``board_manager.defaults`` take effect again.

No Runtime Change After YAML Modification
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Possible Causes**

- ``idf.py bmgr`` has not been run.
- The modification is to YAML versus runtime configuration; the two take effect at different phases.
- The device has already been initialized; runtime modification of config usually requires a deinit followed by init to let the driver re-read the configuration.

**Recommended Actions**

1. First re-run ``idf.py bmgr -b <board>`` and confirm that ``components/gen_bmgr_codes`` has been refreshed.
2. If the modification is to a runtime override, call :cpp:func:`esp_board_manager_deinit_device_by_name` once for the relevant device, then call :cpp:func:`esp_board_manager_init_device_by_name`.

Cannot Get Handle or Configuration at Runtime
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Possible Causes**

- The current board has not enabled the device or peripheral; it is not declared in the YAML.
- ``init_skip: true`` causes BMGR to skip the device during the automatic initialization phase.
- :cpp:func:`esp_board_manager_init` did not execute successfully (a prior error or ESP_FAIL return).
- The ``name`` is misspelled and does not match the YAML.

**Recommended Actions**

1. Call :cpp:func:`esp_board_manager_print` after startup to confirm that the expected devices and peripherals are all correctly registered and initialized.
2. Check the startup log to confirm whether :cpp:func:`esp_board_manager_init` returned successfully; any device initialization failure will leave an error log.
3. For devices explicitly declared with ``init_skip: true``, call :cpp:func:`esp_board_manager_init_device_by_name` at the appropriate business timing before a handle can be obtained.

Device Malfunction (Power / Reset / Timing)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Possible Causes**

- The factory function or power-on sequence implementation in the board-level ``setup_device.c`` is incorrect.
- The ``power_ctrl_device`` reference is wrong, causing the target device to not actually be powered before initialization.
- Key ``[IO]`` or ``[TO_BE_CONFIRMED]`` fields in the YAML still use template default values instead of being filled in according to the schematic.

**Recommended Actions**

1. Cross-check the ``[IO]`` and ``[TO_BE_CONFIRMED]`` fields in the YAML related to that device against the schematic and device datasheet.
2. For devices requiring chip-specific timing (such as LCD displays, touchscreens, and cameras), confirm that the corresponding factory functions (``lcd_panel_factory_entry_t``, ``lcd_touch_factory_entry_t``, etc.) in the board-level ``setup_device.c`` are correctly implemented.
3. When a device is powered by another device, confirm that the device entry has ``power_ctrl_device`` correctly filled in, and that the referenced ``power_ctrl`` device itself can initialize successfully.
