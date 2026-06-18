spi
============

:link_to_translation:`zh_CN:[中文]`

Overview
--------

The ``spi`` peripheral describes an ESP-IDF SPI master bus. BMGR converts an ``spi`` entry in ``board_peripherals.yaml`` into a ``periph_spi_config_t`` and calls ``spi_bus_initialize`` to create the SPI bus during initialization.

``spi`` is commonly used for SPI LCDs, some SPI cameras, or other devices that share an SPI host. Chip-select, D/C, LCD pixel clock, device SPI mode, and other device-private parameters belong to the device-side configuration; the peripheral side only describes the SPI bus host, data lines, and transfer capability.

Supported Operating Modes
--------------------------

``spi`` distinguishes configuration by bus data-line format. The BMGR SPI peripheral initialization entry only creates the SPI master bus and does not create specific SPI devices.

- :ref:`Standard SPI bus <spi-standard-bus>`
- :ref:`Quad / Octal SPI bus <spi-wide-bus>`

Minimal Configuration
---------------------

.. _spi-standard-bus:

Standard SPI Bus
^^^^^^^^^^^^^^^^

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: spi_display
        type: spi
        config:
          spi_bus_config:
            spi_port: 1
            mosi_io_num: 11     # [IO]
            miso_io_num: -1     # [IO]
            sclk_io_num: 12     # [IO]

.. _spi-wide-bus:

Quad / Octal SPI Bus
^^^^^^^^^^^^^^^^^^^^

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: spi_display
        type: spi
        config:
          spi_bus_config:
            spi_port: SPI2_HOST
            data0_io_num: 11    # [IO]
            data1_io_num: 13    # [IO]
            sclk_io_num: 12     # [IO]
            data2_io_num: 14    # [IO]
            data3_io_num: 15    # [IO]

Mode Description
----------------

The standard SPI bus uses ``mosi_io_num``, ``miso_io_num``, and ``sclk_io_num``. For write-only display output, ``miso_io_num`` can remain ``-1``.

The Quad / Octal SPI bus uses the data-line fields of the ESP-IDF ``spi_bus_config_t``. ``data0_io_num`` and ``mosi_io_num`` are the same union field; ``data1_io_num`` and ``miso_io_num`` are the same union field; ``data2_io_num`` and ``quadwp_io_num`` are the same union field; ``data3_io_num`` and ``quadhd_io_num`` are the same union field. Fields within the same union must not be set simultaneously.

The SPI LCD's ``cs_gpio_num``, ``dc_gpio_num``, ``spi_mode``, ``pclk_hz``, and transaction queue depth belong to the ``display_lcd`` device-side ``io_spi_config`` and are not written into the ``spi`` peripheral ``config``.

Full Field Reference
--------------------

Standard SPI Bus / Quad / Octal SPI Bus — Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # SPI Peripheral Default Configuration
    # This file shows the default values used by the SPI peripheral parser
    # Based on periph_spi.py parsing script

    - name: spi_master
      type: spi
      config:
        # SPI bus configuration
        spi_bus_config:
          # SPI port (default: 0, maps to SPI1_HOST)
          spi_port: 0
          # Valid values: 0 (SPI1_HOST), 1 (SPI2_HOST), 2 (SPI3_HOST)
          # Or use string: "SPI1_HOST", "SPI2_HOST", "SPI3_HOST"

          # Standard SPI pins (union fields - use either dataX_io_num or traditional names)
          # MOSI pin (default: -1, not set)
          mosi_io_num: -1                     # [IO] MOSI pin
          # Alternative: data0_io_num: -1

          # MISO pin (default: -1, not set)
          miso_io_num: -1                     # [IO] MISO pin
          # Alternative: data1_io_num: -1

          # SCLK pin (default: -1, not set)
          sclk_io_num: -1                     # [IO] SCLK pin

          # Quad SPI pins (union fields)
          # Quad Write Protect pin (default: -1, not set)
          quadwp_io_num: -1                   # [IO] Quad Write Protect pin
          # Alternative: data2_io_num: -1

          # Quad Hold pin (default: -1, not set)
          quadhd_io_num: -1                   # [IO] Quad Hold pin
          # Alternative: data3_io_num: -1

          # Octal mode pins (optional)
          # Data pin 4 (default: -1, not set)
          data4_io_num: -1                    # [IO] Data pin 4
          # Data pin 5 (default: -1, not set)
          data5_io_num: -1                    # [IO] Data pin 5
          # Data pin 6 (default: -1, not set)
          data6_io_num: -1                    # [IO] Data pin 6
          # Data pin 7 (default: -1, not set)
          data7_io_num: -1                    # [IO] Data pin 7

          # Data IO default level (default: false)
          data_io_default_level: false

          # Maximum transfer size (default: 4092)
          max_transfer_sz: 4092

          # DMA burst size in bytes (IDF v6.1+; 0 uses driver default)
          dma_burst_size: 0

          # Flags (default: 0)
          flags: 0

          # ISR CPU affinity (default: ESP_INTR_CPU_AFFINITY_AUTO)
          isr_cpu_id: "ESP_INTR_CPU_AFFINITY_AUTO"
          # Valid values:
          # - ESP_INTR_CPU_AFFINITY_AUTO
          # - ESP_INTR_CPU_AFFINITY_0
          # - ESP_INTR_CPU_AFFINITY_1

          # Interrupt flags (default: 0)
          intr_flags: 0

Applicable Devices
------------------

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Notes
   * - ``display_lcd``
     - Devices with ``sub_type: spi`` reference ``spi`` in ``peripherals``
     - LCD CS, D/C, reset, pixel clock, and panel parameters belong to device-side configuration
   * - ``camera``
     - SPI camera devices configure ``spi_config`` on the device side
     - In existing board configurations, SPI camera parameters are described directly on the device side without reusing an ``spi`` peripheral instance

Reference Code
--------------

- ``esp_board_manager/peripherals/periph_spi/periph_spi.c``
- ``esp_board_manager/peripherals/periph_spi/periph_spi.h``

Board-Level Reference
---------------------

- ``esp_board_manager/boards/esp_box_3/board_peripherals.yaml``: SPI LCD bus configuration.
- ``esp_board_manager/boards/esp_vocat_board_v1_2/board_peripherals.yaml``: SPI LCD bus configuration.
- ``esp_board_manager/boards/esp32_s3_korvo2_v3/board_peripherals.yaml``: SPI LCD bus configuration.
- ``esp_board_manager/boards/m5stack_cores3/board_peripherals.yaml``: SPI master bus configuration.
- ``esp_board_manager/boards/dual_eyes_board_v1_0/board_peripherals.yaml``: two LCD devices sharing the same SPI bus.

Notes
-----

- For common YAML field rules, see :doc:`/programming-guide/yaml-rules`.
- A numeric ``spi_port`` maps to ``SPI1_HOST``, ``SPI2_HOST``, or ``SPI3_HOST``. The number of available hosts on the target chip follows the ESP-IDF SoC capabilities.
- Within the same union field, only the traditional name or the ``dataX`` name may be used. For example, do not fill in both ``mosi_io_num`` and ``data0_io_num``.
- SPI peripheral initialization only creates the bus. The chip-select, command/data pin, and device clock of specific devices belong to the corresponding device configuration.
- After modifying the SPI peripheral configuration, re-run ``idf.py bmgr -b <board>``.

Debugging Tips
--------------

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to retrieve the SPI peripheral handle. The handle type is ``periph_spi_handle_t``:

.. code-block:: c

   typedef struct {
       spi_host_device_t  spi_port;  /*!< SPI port number */
   } periph_spi_handle_t;

``spi_port`` is the SPI bus number, which can be passed to ``spi_bus_add_device`` to attach an SPI device, or to drivers such as ``esp_lcd_*`` that use the SPI bus.

Related declarations are in ``esp_board_manager/peripherals/periph_spi/periph_spi.h``.

Underlying ESP-IDF driver documentation: `SPI Master Driver <https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/spi_master.html>`__.
