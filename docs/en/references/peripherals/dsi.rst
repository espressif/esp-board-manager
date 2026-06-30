dsi
=======

:link_to_translation:`zh_CN:[中文]`

Overview
--------

The ``dsi`` peripheral type describes MIPI DSI bus resources. BMGR uses this configuration to create an ``esp_lcd_dsi_bus_handle_t``, which is referenced by the ``dsi`` sub-type of a ``display_lcd`` device.

This peripheral describes only the DSI bus itself, such as bus ID, number of data lanes, PHY clock source, and per-lane bit rate. Panel reset, DBI and DPI parameters, pixel format, resolution, and timing belong to the ``display_lcd`` device configuration and must not be written into the ``dsi`` peripheral ``config``.

Supported Operating Modes
--------------------------

``dsi`` is currently configured as an ESP-IDF MIPI DSI bus mode and does not split operating modes via ``role`` or ``sub_type``.

- `DSI Bus`_

Minimal Configuration
---------------------

DSI Bus
^^^^^^^

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: dsi_display
        type: dsi
        config:
          bus_id: 0
          data_lanes: 2
          phy_clk_src: 0
          lane_bit_rate_mbps: 1000

Mode Description
----------------

The ``dsi`` peripheral creates a DSI bus handle. When using a DSI display, set the ``display_lcd`` device's ``sub_type`` to ``dsi`` and reference this peripheral name in the device-side ``peripherals``. If the board configuration also requires MIPI power control, define an ``ldo`` or other power peripheral separately and reference it by name on the device side.

Full Field Reference
--------------------

DSI Bus — Full Fields
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # DSI Display Configuration
    # This configuration sets up a MIPI DSI display interface
    # Based on common DSI display parameters

    - name: dsi_display
      type: dsi
      config:
        # DSI bus identifier (default: 0 for primary bus)
        # Valid values depending on available DSI controllers, esp32p4 support only 1 MIPI DSI bus
        bus_id: 0

        # Number of data lanes (default: 2 for dual-lane mode)
        # esp32p4 support up to 2 data lanes
        data_lanes: 2

        # Physical layer clock source (default: MIPI_DSI_PHY_CLK_SRC_DEFAULT)
        phy_clk_src: 0
        # Valid values:
        # - 0 (Set it to 0 and then let the driver configure it)
        # - MIPI_DSI_PHY_CLK_SRC_DEFAULT
        # - MIPI_DSI_PHY_CLK_SRC_PLL_F20M
        # - MIPI_DSI_PHY_CLK_SRC_PLL_F25M
        # - MIPI_DSI_PHY_CLK_SRC_RC_FAST

        # Bit rate per data lane in Mbps (default: 1000 for 1Gbps per lane)
        # Common values between 80 to 1500 Mbps depending on display capabilities
        lane_bit_rate_mbps: 1000          # [TO_BE_CONFIRMED] Bit rate per data lane in Mbps

Applicable Devices
------------------

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Notes
   * - ``display_lcd``
     - When ``sub_type`` is ``dsi``, reference the ``dsi`` peripheral in the device-side ``peripherals``
     - DBI/DPI configuration, color format, panel timing, and LCD component dependencies are written in the device configuration

Reference Code
--------------

- ``esp_board_manager/peripherals/periph_dsi/periph_dsi.c``
- ``esp_board_manager/devices/dev_display_lcd/dev_display_lcd.c``

Board-Level Reference
---------------------

- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``: defines ``dsi_display``, used together with ``ldo_mipi`` and a DSI LCD.
- ``esp_boards/esp32_p4_function_ev_board/board_devices.yaml``: ``display_lcd`` references ``dsi_display`` with ``sub_type: dsi``.
- ``m5stack_boards/m5stack_tab5/board_peripherals.yaml``: defines ``dsi_display`` and LCD backlight LEDC peripheral.
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``: ``display_lcd`` references ``dsi_display`` with ``sub_type: dsi``.

Notes
-----

- The ``dsi`` peripheral configuration only covers the DSI bus. Panel driver component dependencies, initialization commands, and DPI timing belong to the ``display_lcd`` device.
- In ESP32-P4 board configurations, MIPI power supply is described separately via the ``ldo`` peripheral; the ``dsi`` peripheral does not configure the supply voltage.
- ``lane_bit_rate_mbps`` must match the display, number of lanes, and pixel clock budget; ``[TO_BE_CONFIRMED]`` is retained in the template.
- After modifying the DSI peripheral configuration, re-run ``idf.py bmgr -b <board>``.

Debugging Tips
--------------

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to retrieve the DSI peripheral handle. The handle type is the ESP-IDF native ``esp_lcd_dsi_bus_handle_t``, which can be passed directly to ``esp_lcd_*`` APIs.

Related declarations are in ``esp_board_manager/peripherals/periph_dsi/periph_dsi.h``.

Underlying ESP-IDF driver documentation: `LCD on MIPI DSI Interface <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32p4/api-reference/peripherals/lcd/dsi_lcd.html>`__.
