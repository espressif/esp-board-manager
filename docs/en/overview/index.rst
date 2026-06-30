Getting Started
===============

:link_to_translation:`zh_CN:[中文]`

Install ``esp-bmgr-assist``
---------------------------

It is recommended to use ``esp-bmgr-assist`` as the default entry point. This tool integrates with the ``idf.py`` startup process, automatically discovers ESP Board Manager components in the current project, and adds them to ``IDF_EXTRA_ACTIONS_PATH``. Once installed in an activated ESP-IDF Python environment (run ``./install.sh`` and ``. ./export.sh`` in the IDF directory), the ``idf.py bmgr`` command is available for all subsequent projects in the same environment.

.. code-block:: bash

   pip install esp-bmgr-assist

.. note::

   - Usually you only need to install it once; run ``pip install --upgrade esp-bmgr-assist`` only when an update is requested.
   - ``esp-bmgr-assist`` is only used to avoid manually configuring ``IDF_EXTRA_ACTIONS_PATH`` and does not replace the ``esp_board_manager`` component itself. You still need to add the ``esp_board_manager`` dependency to your project as described below. See :doc:`/tools/esp-bmgr-assist` for details.

Add BMGR Dependency
-------------------

Add the dependency quickly using ``idf.py add-dependency`` (recommended):

.. code-block:: bash

   idf.py add-dependency "espressif/esp_board_manager"

This command automatically updates the project ``main/idf_component.yml``.

You can also add it manually to ``main/idf_component.yml``:

.. code-block:: yaml

   espressif/esp_board_manager:
     version: "*"
     require: public

If using a local repository path:

.. code-block:: yaml

   espressif/esp_board_manager:
     override_path: /PATH/TO/esp_board_manager
     version: "*"
     require: public

Basic ``idf.py bmgr`` Usage
---------------------------

For getting started, only two commands are needed:

.. code-block:: bash

   # List the boards currently visible
   idf.py bmgr -l

   # Select the target board and generate board-level code
   idf.py bmgr -b <board>

Recommended workflow:

1. Run ``idf.py bmgr -l`` first to confirm that the BMGR path is configured correctly and that the target board can be scanned.

   The board list and indexes printed by ``idf.py bmgr -l`` may change with board dependency versions and custom paths. Use the output from the current project when selecting a board.

2. Then run ``idf.py bmgr -b <board>`` to generate the ``components/gen_bmgr_codes`` board-level code. During generation, the log outputs the following key information to confirm that the selected board matches expectations:

   - ``Resolved board: <board>``: The actual board name matched.
   - ``Board path: <path>``: The source directory of the board configuration files.
   - ``Board configuration generation completed successfully for board: <board>``: Confirms generation is complete.

3. Then proceed with the normal build, flash, or run.

When switching boards, BMGR also handles the following board-bound state:

- Regenerates C source files and build files under ``components/gen_bmgr_codes``.
- Updates ``board_manager.defaults``.
- Processes the ``Kconfig.projbuild`` for the current board.
- Backs up and clears the old ``sdkconfig`` to avoid residual ``CONFIG_IDF_TARGET`` or device configuration from the previous board.

Basic API Usage
---------------

The typical application usage path is: initialize BMGR, get device or peripheral handles by name, query configuration as needed, and finally deinitialize.

.. code-block:: c

   #include "esp_board_manager_includes.h"

   void app_main(void)
   {
       ESP_ERROR_CHECK(esp_board_manager_init());

       void *lcd_handle = NULL;
       ESP_ERROR_CHECK(esp_board_manager_get_device_handle("display_lcd", &lcd_handle));

       void *lcd_config = NULL;
       ESP_ERROR_CHECK(esp_board_manager_get_device_config("display_lcd", &lcd_config));

       ESP_ERROR_CHECK(esp_board_manager_deinit());
   }

:cpp:func:`esp_board_manager_init` initializes peripherals first and then devices in the order specified in ``board_peripherals.yaml`` and ``board_devices.yaml``.

Common API entry points:

- Initialize and deinitialize: :cpp:func:`esp_board_manager_init`, :cpp:func:`esp_board_manager_deinit`.
- Get runtime handles: :cpp:func:`esp_board_manager_get_periph_handle`, :cpp:func:`esp_board_manager_get_device_handle`.
- Query generated configuration: :cpp:func:`esp_board_manager_get_periph_config`, :cpp:func:`esp_board_manager_get_device_config`.

Common Getting-Started Issues
------------------------------

This section lists common issues encountered when integrating BMGR for the first time, to help with quick troubleshooting. For complete troubleshooting guidance, see :doc:`/faq`.

``idf.py bmgr`` Command Not Found
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Under ESP-IDF v5.x, first check whether ``IDF_EXTRA_ACTIONS_PATH`` correctly points to the BMGR directory.
- ``idf.py bmgr`` is the simplified command introduced after BMGR v0.5.8; for older versions, use the old command ``idf.py gen-bmgr-config``, or upgrade BMGR.
- Ensure the path where the command is executed is a valid IDF project.

Missing ``dev_xxx.h`` or ``periph_xxx.h`` at Compile Time
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- In most cases, ``idf.py bmgr -b <board>`` has not been run yet, or the last generation result is outdated or incomplete.
- Normally, ``components/gen_bmgr_codes`` should contain not only several ``.c`` files, but also ``CMakeLists.txt``, ``idf_component.yml``, ``board_manager.defaults``, and ``Kconfig.projbuild``.

YAML Changes Not Taking Effect
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

BMGR does not automatically refresh old generated artifacts when board-level YAML is modified. After modification, run ``idf.py bmgr -b <board>`` again, then continue with the build.

Examples
--------

``esp_board_manager/examples/`` provides standalone buildable examples that demonstrate the complete workflow: selecting a board, generating board-level code, initializing the board, and obtaining device handles by name. Each example directory includes a ``README.md`` with hardware requirements and build instructions.

.. list-table::
   :header-rows: 1
   :widths: 22 34 32 12

   * - Example
     - Typical Use Case
     - Primary Devices Involved
     - Registry
   * - ``play_embed_music``
     - Play a WAV prompt tone embedded in firmware
     - ``audio_codec`` (DAC)
     - `Link <https://components.espressif.com/components/espressif/esp_board_manager/examples/play_embed_music>`__
   * - ``play_sdcard_music``
     - Play a WAV file from a microSD card
     - ``audio_codec``, ``fs_fat``
     - `Link <https://components.espressif.com/components/espressif/esp_board_manager/examples/play_sdcard_music>`__
   * - ``record_to_sdcard``
     - Record microphone input and write to SD card as WAV
     - ``audio_codec`` (ADC), ``fs_fat``
     - `Link <https://components.espressif.com/components/espressif/esp_board_manager/examples/record_to_sdcard>`__
   * - ``record_and_play``
     - Capture microphone input and play it back in real time (loopback)
     - ``audio_codec`` (ADC + DAC)
     - `Link <https://components.espressif.com/components/espressif/esp_board_manager/examples/record_and_play>`__
   * - ``display_lvgl``
     - Initialize LCD and touchscreen, run the LVGL test UI
     - ``display_lcd``, ``lcd_touch``
     - `Link <https://components.espressif.com/components/espressif/esp_board_manager/examples/display_lvgl>`__

``examples/common/`` provides shared utilities such as WAV header parsing, used by SD card-related examples.

In addition to the examples above, ``esp_board_manager/test_apps/`` provides test applications for CI and local validation, covering devices and peripherals already supported by Board Manager. After completing a new board integration, use this test application to verify the configuration correctness.

SDK Resources
-------------

The following open-source projects use BMGR for board-level initialization, providing board adaptation as independent components, and can serve as reference for complete projects.

- `ESP-ADF <https://github.com/espressif/esp-adf>`_: Espressif's official advanced application development framework, targeting audio, video, and IoT product development. v3.0 refactors the core media pipeline with ESP-GMF, providing modular product services such as audio/video playback, battery management, OTA, and Wi-Fi services, with MCP protocol support; Board Manager will be integrated as the official board initialization solution.
- `ESP-GMF <https://github.com/espressif/esp-gmf>`_: Espressif's lightweight, modular software framework for IoT multimedia applications, supporting audio, image, video processing, and arbitrary data stream products, with a minimum memory footprint of 7 KB. The framework is organized in four layers—GMF-Core, Elements, and Packages—with Board Manager integrated as the official board initialization solution.
- `ESP-Claw <https://github.com/espressif/esp-claw>`_: A Chat Coding AI Agent framework based on ESP32 series chips. It defines device behavior through conversation, completes the closed loop of perception, decision-making, and execution on the device side, uses BMGR for board initialization, and interacts with external tools via the MCP protocol.
- `ESP-Brookesia <https://github.com/espressif/esp-brookesia>`_: A human-machine interaction development framework for AIoT devices. Based on ESP-IDF, it provides audio, display, storage, Wi-Fi, SNTP, video, and LLM adapters (OpenAI, Coze, Xiaozhi, etc.) as three-layer components (HAL, Services, AI Agent), with board-level configuration handled by ``brookesia_hal_boards`` calling BMGR.
