Code Generation and Build Integration
======================================

:link_to_translation:`zh_CN:[中文]`

Before compilation, BMGR parses the board's YAML description into a set of explicit configuration source files and build inputs. ``idf.py bmgr`` is the user-facing entry command.

When running ``idf.py bmgr -b <board>``, BMGR performs the following steps in order:

1. Scans board directories, collecting candidate boards from the default directory, custom directory, and component directories.
2. Determines the currently selected board based on the command-line arguments (name or index).
3. Locates the board's ``board_peripherals.yaml``, ``board_devices.yaml``, ``board_info.yaml``, ``sdkconfig.defaults.board``, and ``Kconfig.projbuild``.
4. Parses peripherals and devices, generating corresponding configuration structures, handle tables, and board-level metadata.
5. Generates the ``Kconfig.projbuild`` for the current board and appends the board directory's ``Kconfig.projbuild``.
6. Generates ``board_manager.defaults``, connecting the board's default configuration and capability symbols to the build.
7. Outputs the source files, build files, and tooling summary files under ``components/gen_bmgr_codes`` for compilation.

In BMGR's model, board configuration code comes from the YAML file description and the script's parsing and generation process, not from manually selecting devices or peripherals in ``menuconfig``. ``components/gen_bmgr_codes`` is not a cache or a view-only intermediate artifact; it is an actual component that participates in the ESP-IDF build.

Generated Output Files
-----------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - File
     - Description
   * - ``gen_board_periph_config.c``
     - Peripheral configuration structure definitions generated from ``board_peripherals.yaml``.
   * - ``gen_board_periph_handles.c``
     - Generated peripheral handle entry points, type mappings, and initialization function hooks.
   * - ``gen_board_device_config.c``
     - Device configuration structure definitions generated from ``board_devices.yaml``.
   * - ``gen_board_device_handles.c``
     - Generated device handle entry points, initialization/deinitialization function mappings, and device linked list.
   * - ``gen_board_info.c``
     - Generated board-level metadata, such as board name, chip, version, description, and manufacturer.
   * - ``gen_board_device_custom.h``
     - Configuration struct definitions for ``type: custom`` devices, used by application-side ``init`` and ``deinit``.
   * - ``board_manager.defaults``
     - ``sdkconfig`` default entries for the current board, along with the corresponding device and peripheral capability symbols.
   * - ``Kconfig.projbuild``
     - Kconfig symbol definitions and selection entries for the current board, projecting board-level capabilities into the project configuration system.
   * - ``idf_component.yml``
     - Component dependency manifest for the current board; device ``dependencies`` are reflected here.
   * - ``gen_board_metadata.yaml``
     - A unified board-level summary for tooling and debugging; useful for viewing the devices, peripherals, component dependencies, and occupied I/O of the current board.

BMGR does not organize compilation by manually selecting devices or peripherals one by one in ``menuconfig``; instead, it first generates ``board_manager.defaults`` from the board-level YAML, and the board capability macros contained therein take effect during the subsequent build. When running ``idf.py``, BMGR injects these settings into ``sdkconfig`` to drive BMGR's conditional compilation.

During debugging, it is recommended to triage by symptom:

- If the behavior does not match expectations, check ``gen_board_periph_config.c`` and ``gen_board_device_config.c`` first.
- If the symptom is a build failure or dependency resolution error, check whether the generated files under ``components/gen_bmgr_codes`` are complete, whether the capability symbols in ``board_manager.defaults`` match expectations, and whether ``sdkconfig`` is consistent with ``board_manager.defaults``.

Assembling and Overriding board_manager.defaults and Kconfig.projbuild
----------------------------------------------------------------------

``board_manager.defaults`` is the board-level defaults file through which BMGR connects board configuration to ESP-IDF build configuration. Based on the current board YAML, BMGR generates capability symbols such as ``CONFIG_ESP_BOARD_PERIPH_*_SUPPORT``, ``CONFIG_ESP_BOARD_DEV_*_SUPPORT``, and ``CONFIG_ESP_BOARD_DEV_<DEV>_SUB_<SUB>_SUPPORT``. These symbols participate in the subsequent build and control whether device, peripheral, and device sub-type code is compiled. Users should not manually write these BMGR-managed capability symbols in the project ``sdkconfig.defaults``; board-level differences should be managed through ``sdkconfig.defaults.board`` or amend.

When running ``idf.py bmgr -b <board> [-a <amend>]``, BMGR assembles the final ``board_manager.defaults`` and ``Kconfig.projbuild`` in the following order, with **later entries overriding earlier ones**:

1. BMGR auto-generated section: ``CONFIG_IDF_TARGET``, ``CONFIG_ESP_BOARD_<BOARD>=y``, ``CONFIG_ESP_BOARD_NAME``, and the capability symbols derived from YAML parsing: ``CONFIG_ESP_BOARD_PERIPH_*_SUPPORT``, ``CONFIG_ESP_BOARD_DEV_*_SUPPORT``, and ``CONFIG_ESP_BOARD_DEV_<DEV>_SUB_<SUB>_SUPPORT``.
2. The ``sdkconfig.defaults.board`` and ``Kconfig.projbuild`` in the board directory (if present).
3. The ``sdkconfig.defaults.board`` and ``Kconfig.projbuild`` fragments listed under ``apply:`` in the ``board_amend.yaml`` manifest, appended **strictly in the order they appear in** ``apply:``. To have one fragment override another amend fragment, place it later in the ``apply:`` list.

When duplicate ``CONFIG_*`` entries appear in ``board_manager.defaults``, BMGR keeps the last occurrence and rewrites earlier duplicate lines as comments in the form ``# BMGR_CONFIG_OVERRIDE by <section>: <original line>``, making override relationships easy to trace. ``Kconfig.projbuild`` is assembled by plain-text concatenation; a ``# --- <label>: <path> ---`` marker is inserted before each segment to indicate its source.

.. note::

   The ``sdkconfig.defaults.board`` and ``Kconfig.projbuild`` files listed in the ``board_amend.yaml`` manifest must be explicitly listed under ``apply:`` to be included in the merge. Files placed in the amend directory but not listed are ignored and an INFO log is emitted. See :doc:`/create-board/amend` for details.

Build Integration: SDKCONFIG_DEFAULTS Precedence
-------------------------------------------------

During the build, ESP-IDF reads a set of ``SDKCONFIG_DEFAULTS``. The files are declared by the ``SDKCONFIG_DEFAULTS`` environment variable or CMake variable, and separated by ``;``.

When the project ``sdkconfig`` does not exist, BMGR uses an ``idf.py`` global callback to assemble ``SDKCONFIG_DEFAULTS`` in the following order. Per the ESP-IDF rule, later entries take precedence:

1. Project ``sdkconfig.defaults`` (lowest)
2. ``components/gen_bmgr_codes/board_manager.defaults`` (board-level, including amend)
3. Environment ``SDKCONFIG_DEFAULTS``
4. ``-D SDKCONFIG_DEFAULTS`` (highest)

Therefore, **board-level defaults always take precedence over the project's** ``sdkconfig.defaults``. Project defaults can no longer override ordinary board defaults. For board-specific overrides, use amend (auto-amend or ``-a/--amend``).

.. warning::

   Use amend for board-level hardware/variant overrides; do not rely on the project ``sdkconfig.defaults`` to override the board. When the project ``sdkconfig.defaults`` sets a symbol also managed by the board, the board value wins, and BMGR prints a **non-blocking warning** when preparing ``SDKCONFIG_DEFAULTS`` (pointing to the board's precedence and recommending amend for board-level overrides). The previous behavior that raised a FatalError when project defaults touched BMGR-managed symbols has been removed. The project ``sdkconfig.defaults`` is best for cross-board policy not set by the board; for CI or temporary overrides use the environment variable or ``-D SDKCONFIG_DEFAULTS=``. Board-specific sdkconfig entries (PSRAM, Flash, partition tables, application-level switches, etc.) should be placed in ``sdkconfig.defaults.board`` under the board directory; BMGR will merge them uniformly.

When switching boards, ``idf.py bmgr -b <other_board>`` regenerates ``board_manager.defaults`` and ``Kconfig.projbuild``, and backs up and cleans up the capability macros written by the previous board from the old ``sdkconfig``.

.. note::

   In addition to explicit ``-a/--amend``, BMGR supports **auto-amend**: under the same scan paths used for boards (including ``-c`` paths), it automatically finds a directory whose name equals the currently selected board, that contains a ``board_amend.yaml``, and that is not itself a full board directory (convention ``<scan_root>/<board_name>/board_amend.yaml``), and applies it as that board's amend. ``-c/--customer-path`` now accepts multiple semicolon-separated paths (for example ``-c "overlays_a;overlays_b"``), where later paths take precedence. By placing each board's overlay under one root and passing a single ``-c``, every board can share the exact same command. Explicit ``-a/--amend`` has the highest precedence and overrides all auto-amend; set ``ESP_BOARD_MANAGER_DISABLE_AUTO_AMEND=1`` to disable auto-discovery. See :doc:`/create-board/amend` for details.
