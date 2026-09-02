uart
============

:link_to_translation:`zh_CN:[中文]`

.. _uart-intro:

Overview
--------

The ``uart`` peripheral describes an ESP-IDF UART driver instance. BMGR converts ``uart`` entries in ``board_peripherals.yaml`` into ``periph_uart_config_t``, then during initialization installs the UART driver, configures serial parameters, sets TX/RX/RTS/CTS pins, and sets the UART operating mode as needed.

``uart`` is suitable for serial resources that need to be declared uniformly by the board configuration. No existing board configuration in the current repository references a ``uart`` peripheral device example; therefore this page only lists the configurable operating modes and source code entry points for the peripheral itself.

.. _uart-working-modes:

Supported Operating Modes
--------------------------

``uart`` distinguishes configurations by ESP-IDF ``uart_mode_t`` and whether event queues are enabled. The ``uart_config`` field in the template describes the serial frame format and clock source.

- :ref:`Basic UART Mode <uart-basic>`
- :ref:`RS485 / IrDA and Other UART Modes <uart-alt-mode>`
- :ref:`Enabling Event Queue <uart-queue>`

.. _uart-min-config:

Minimal Configuration
---------------------

.. _uart-basic:

Basic UART Mode
^^^^^^^^^^^^^^^

See complete fields: :ref:`uart-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: uart_1
        type: uart
        config:
          uart_num: 1
          tx_io_num: 17     # [IO]
          rx_io_num: 18     # [IO]
          uart_config:
            baud_rate: 115200

.. _uart-alt-mode:

RS485 / IrDA and Other UART Modes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See complete fields: :ref:`uart-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: uart_rs485
        type: uart
        config:
          uart_num: 1
          tx_io_num: 17     # [IO]
          rx_io_num: 18     # [IO]
          rts_io_num: 19    # [IO]
          uart_mode: UART_MODE_RS485_HALF_DUPLEX
          uart_config:
            baud_rate: 115200

.. _uart-queue:

Enabling Event Queue
^^^^^^^^^^^^^^^^^^^^

See complete fields: :ref:`uart-full`.

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: uart_1
        type: uart
        config:
          uart_num: 1
          tx_io_num: 17     # [IO]
          rx_io_num: 18     # [IO]
          use_queue: true
          queue_size: 10
          uart_config:
            baud_rate: 115200

.. _uart-mode-notes:

Mode Notes
----------

Basic mode uses ``UART_MODE_UART``. RS485, IrDA, and other UART modes are selected via ``uart_mode``; BMGR only calls ``uart_set_mode`` during initialization when this field is not ``UART_MODE_UART``.

``use_queue`` controls whether ``uart_driver_install`` creates an event queue. When the queue is enabled, the returned ``periph_uart_handle_t`` includes ``uart_queue``; when the queue is disabled, that member is not used for receiving events.

``source_clk`` and ``lp_source_clk`` are mutually exclusive (union), and cannot be configured simultaneously. When using LP UART, port and GPIO selection must comply with the LP UART capability of the target chip.

.. _uart-full-fields:

Full Field Reference
--------------------

.. _uart-full:

Basic Mode / Extended Mode / Event Queue Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # UART Peripheral Configuration
    # This section configures UART peripheral settings

    - name: uart_1
      type: uart
      config:
        # UART port number, valid values depend on SOC
        # Can be configured as UART_NUM_0, UART_NUM_1, UART_NUM_2, ... or 0,1,2,... directly
        # Valid values depend on SOC, please check soc/soc_caps.h
        uart_num: 1

        # RX buffer size in bytes (default: 256)
        rx_buffer_size: 256

        # TX buffer size in bytes (default: 256)
        tx_buffer_size: 256

        # GPIO number for TX pin (default: -1 for not used)
        tx_io_num: -1                     # [IO] GPIO number for TX pin

        # GPIO number for RX pin (default: -1 for not used)
        rx_io_num: -1                     # [IO] GPIO number for RX pin

        # GPIO number for RTS pin (default: -1 for not used)
        rts_io_num: -1                    # [IO] GPIO number for RTS pin

        # GPIO number for CTS pin (default: -1 for not used)
        cts_io_num: -1                    # [IO] GPIO number for CTS pin

        # UART operating mode (default: UART_MODE_UART)
        uart_mode: UART_MODE_UART
        # Valid values:
        # - UART_MODE_UART
        # - UART_MODE_RS485_HALF_DUPLEX
        # - UART_MODE_IRDA
        # - UART_MODE_RS485_COLLISION_DETECT
        # - UART_MODE_RS485_APP_CTRL

        # Flag to enable queue for UART events (default: false)
        use_queue: false

        # Size of the event queue, only used when use_queue is true (default: 10)
        queue_size: 10

        # Interrupt allocation flags (default: 0)
        intr_alloc_flags: 0

        # UART configuration parameters
        uart_config:
          # Baud rate in bits per second (default: 115200)
          baud_rate: 115200               # [TO_BE_CONFIRMED] Baud rate in bits per second

          # Number of data bits (default: UART_DATA_8_BITS)
          data_bits: UART_DATA_8_BITS
          # Valid values:
          # - UART_DATA_5_BITS
          # - UART_DATA_6_BITS
          # - UART_DATA_7_BITS
          # - UART_DATA_8_BITS

          # Parity setting (default: UART_PARITY_DISABLE)
          parity: UART_PARITY_DISABLE
          # Valid values:
          # - UART_PARITY_DISABLE
          # - UART_PARITY_EVEN
          # - UART_PARITY_ODD

          # Number of stop bits (default: UART_STOP_BITS_1)
          stop_bits: UART_STOP_BITS_1
          # Valid values:
          # - UART_STOP_BITS_1
          # - UART_STOP_BITS_1_5
          # - UART_STOP_BITS_2

          # Flow control mode (default: UART_HW_FLOWCTRL_DISABLE)
          flow_ctrl: UART_HW_FLOWCTRL_DISABLE
          # Valid values:
          # - UART_HW_FLOWCTRL_DISABLE
          # - UART_HW_FLOWCTRL_RTS
          # - UART_HW_FLOWCTRL_CTS
          # - UART_HW_FLOWCTRL_CTS_RTS

          # RX flow control threshold (default: 0)
          # This is a optional configuration, used to control the hardware flow control threshold.
          # Should be used together with the configuration flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS
          rx_flow_ctrl_thresh: 0

          # Source clock for UART (default: UART_SCLK_DEFAULT)
          # Valid values depend on SOC, please check soc/clk_tree_defs.h
          source_clk: UART_SCLK_DEFAULT

          # Source clock for LP UART (default: not used)
          # This is a configuration related to the SOC, which requires the chip to have an LP UART controller
          # You can check the ESP-IDF programming Guide for more details, or check
          # the macro definition of SOC_UART_LP_NUM in the file soc/soc_caps.h to see if the chip supports LP UART directly
          # If supported, you need to set the uart_num to a specific port, or directly set uart_num to LP_UART_NUM_0
          # And the GPIO pins of the LP UART controller can only be selected from the LP GPIO pins
          # This configuration and source_clk are mutually exclusive; please do not configure both options at the same time
          # Valid values depend on SOC, please check soc/clk_tree_defs.h
          lp_source_clk: NULL

          flags:
            # If set, driver allows the power domain to be powered off when system enters sleep mode.
            # This can save power, but at the expense of more RAM being consumed to save register context.
            allow_pd: 0

            # @deprecated, same meaning as allow_pd
            backup_before_sleep: 0

.. _uart-devices:

Applicable Devices
------------------

No existing device YAML template in the current repository requires referencing a ``uart`` peripheral. Custom devices can obtain the UART peripheral handle through a ``custom`` device or board-level code.

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Description
   * - ``custom``
     - The device can obtain the ``uart`` peripheral handle in custom initialization code
     - UART port, pins, and serial parameters are written in the ``uart`` peripheral ``config``

.. _uart-code:

Reference Code
--------------

- ``esp_board_manager/peripherals/periph_uart/periph_uart.c``
- ``esp_board_manager/peripherals/periph_uart/periph_uart.h``

.. _uart-boards:

Board Examples
--------------

.. _uart-notes:

Notes
-----

- Common YAML field rules: see :doc:`/programming-guide/yaml-rules`.
- ``uart_num``, the number of available UART ports, LP UART capability, and available clock sources depend on the target chip.
- ``source_clk`` and ``lp_source_clk`` cannot be configured simultaneously.
- Setting an RTS/CTS pin to ``-1`` means that hardware flow control pin is not used; if hardware flow control is enabled, ``flow_ctrl`` must also be configured accordingly.
- After modifying UART peripheral configuration, re-run ``idf.py bmgr -b <board>``.

.. _uart-debug:

Debugging Tips
--------------

.. _uart-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to obtain the UART peripheral handle. The handle type is ``periph_uart_handle_t``:

.. code-block:: c

   typedef struct {
       uart_port_t    uart_num;    /*!< UART port number */
       QueueHandle_t  uart_queue;  /*!< Queue handle for UART events if set use_queue */
   } periph_uart_handle_t;

``uart_num`` can be passed to ``uart_read_bytes``, ``uart_write_bytes``, and other ESP-IDF UART APIs; if ``use_queue`` is enabled in ``board_peripherals.yaml``, ``uart_queue`` is valid and can be used to monitor serial port events.

The relevant declarations are in ``esp_board_manager/peripherals/periph_uart/periph_uart.h``.

Underlying ESP-IDF driver documentation: `Universal Asynchronous Receiver/Transmitter (UART) <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/uart.html>`__\ .
