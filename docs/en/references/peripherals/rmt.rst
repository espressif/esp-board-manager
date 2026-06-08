rmt
=======

:link_to_translation:`zh_CN:[中文]`

Overview
--------

The ``rmt`` peripheral type is used to describe an ESP-IDF RMT channel. BMGR creates a TX or RX channel based on ``role`` and returns a ``periph_rmt_handle_t``.

This peripheral is suitable for board configurations that need to directly expose an RMT channel handle. LED strip devices can also use RMT, but the ``rmt`` parameters for ``led_strip`` are written in the device-side configuration; an ``rmt`` peripheral in ``board_peripherals.yaml`` is only needed when a device explicitly references a defined ``rmt`` peripheral.

Supported Operating Modes
--------------------------

``rmt`` distinguishes between transmit and receive modes by ``role``.

- `TX Channel`_
- `RX Channel`_

Minimal Configuration
---------------------

TX Channel
^^^^^^^^^^

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: rmt_tx
        type: rmt
        role: tx
        config:
          gpio_num: 18
          clk_src: RMT_CLK_SRC_DEFAULT
          resolution_hz: 10000000
          mem_block_symbols: 64
          trans_queue_depth: 4
          intr_priority: 1
          flags:
            invert_out: false
            with_dma: false
            io_od_mode: false
            allow_pd: false
            init_level: 0

RX Channel
^^^^^^^^^^

``board_peripherals.yaml``:

.. code-block:: yaml

    peripherals:
      - name: rmt_rx
        type: rmt
        role: rx
        config:
          gpio_num: 19
          clk_src: RMT_CLK_SRC_DEFAULT
          resolution_hz: 1000000
          mem_block_symbols: 64
          intr_priority: 1
          flags:
            invert_in: false
            with_dma: false
            allow_pd: false

Mode Notes
----------

``role: tx`` uses ``rmt_tx_channel_config_t`` fields, including ``trans_queue_depth`` and TX output-related flags. ``role: rx`` uses ``rmt_rx_channel_config_t`` fields, which do not include TX queue depth. After the RMT channel is created, encoders, receive buffers, and transmit or receive actions are configured by the device or application code using the handle.

The RMT YAML templates for IDF 5 and IDF 6 are mostly identical, with a difference in flags: the IDF 5 template includes ``io_loop_back``, while the IDF 6 template moves open-drain related configuration into BMGR extra flags and no longer lists ``io_loop_back`` in the template.

Full Field Reference
--------------------

IDF 5 TX Channel Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # RMT Peripheral Default Configuration
    # This file shows the default values used by the RMT peripheral parser
    # Based on periph_rmt.py parsing script
    # Note: Configuration now uses ESP-IDF native structures via union

    # -----------------------------------------------------------------------------
    # RMT TX example - Using rmt_tx_channel_config_t structure
    # -----------------------------------------------------------------------------
    - name: rmt_tx
      type: rmt
      role: tx
      config:
        # Basic channel configuration
        gpio_num: 18                     # [IO] GPIO number used by RMT channel. Set to -1 if unused
        clk_src: RMT_CLK_SRC_DEFAULT     # Clock source of RMT channel, channels in same group must use same clock source
                                          # Please check the 'rmt_clock_source_t' in 'driver/rmt_types.h' for valid values
        resolution_hz: 10000000          # [TO_BE_CONFIRMED] Channel clock resolution, in Hz (10MHz = 0.1us precision)
        mem_block_symbols: 64            # Size of memory block in number of rmt_symbol_word_t (even number required)
                                          # In DMA mode: controls DMA buffer size
                                          # In normal mode: controls number of RMT memory blocks
        trans_queue_depth: 4             # Depth of internal transfer queue (more transfers pending in background)
        intr_priority: 1                 # RMT interrupt priority, if set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)

        # TX channel flags
        flags:
          invert_out: false              # Whether to invert RMT channel signal before output to GPIO pad
          with_dma: false                # If set, allocate RMT channel with DMA capability
                                          # Please check the macro definition SOC_RMT_SUPPORT_DMA in soc/soc_caps.h
                                          # to determine whether the chip supports
          io_loop_back: false            # For debug: signal output from GPIO fed back to input path
          io_od_mode: false              # Configure GPIO as open-drain mode
          allow_pd: false                # If set, allow power domain to power off during sleep (saves power, uses more RAM)
          init_level: 0                  # Set initial level of RMT channel signal (0=low, 1=high)

IDF 5 RX Channel Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # -----------------------------------------------------------------------------
    # RMT RX example - Using rmt_rx_channel_config_t structure
    # -----------------------------------------------------------------------------
    - name: rmt_rx
      type: rmt
      role: rx
      config:
        # Basic channel configuration
        gpio_num: 19                     # [IO] GPIO number used by RMT channel. Set to -1 if unused
        clk_src: RMT_CLK_SRC_DEFAULT     # Clock source of RMT channel, channels in same group must use same clock source
                                          # Please check the 'rmt_clock_source_t' in 'driver/rmt_types.h' for valid values
        resolution_hz: 1000000           # [TO_BE_CONFIRMED] Channel clock resolution, in Hz (1MHz = 1us precision, good for 38kHz IR)
        mem_block_symbols: 64            # Size of memory block in number of rmt_symbol_word_t (even number required)
                                          # In DMA mode: controls DMA buffer size
                                          # In normal mode: controls number of RMT memory blocks
        intr_priority: 1                 # RMT interrupt priority, if set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)

        # RX channel flags
        flags:
          invert_in: false               # Whether to invert the incoming RMT channel signal
          with_dma: false                # If set, allocate RMT channel with DMA capability
                                          # Please check the macro definition SOC_RMT_SUPPORT_DMA in soc/soc_caps.h
                                          # to determine whether the chip supports
          io_loop_back: false            # For debug: signal output from GPIO fed back to input path
          allow_pd: false                # If set, allow power domain to power off during sleep (saves power, uses more RAM)

IDF 6 TX Channel Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # RMT Peripheral Default Configuration
    # This file shows the default values used by the RMT peripheral parser
    # Based on periph_rmt.py parsing script
    # Note: Configuration now uses ESP-IDF native structures via union

    # -----------------------------------------------------------------------------
    # RMT TX example - Using rmt_tx_channel_config_t structure
    # -----------------------------------------------------------------------------
    - name: rmt_tx
      type: rmt
      role: tx
      config:
        # Basic channel configuration
        gpio_num: 18                     # [IO] GPIO number used by RMT channel. Set to -1 if unused
        clk_src: RMT_CLK_SRC_DEFAULT     # Clock source of RMT channel, channels in same group must use same clock source
                                          # Please check the 'rmt_clock_source_t' in 'driver/rmt_types.h' for valid values
        resolution_hz: 10000000          # [TO_BE_CONFIRMED] Channel clock resolution, in Hz (10MHz = 0.1us precision)
        mem_block_symbols: 64            # Size of memory block in number of rmt_symbol_word_t (even number required)
                                          # In DMA mode: controls DMA buffer size
                                          # In normal mode: controls number of RMT memory blocks
        trans_queue_depth: 4             # Depth of internal transfer queue (more transfers pending in background)
        intr_priority: 1                 # RMT interrupt priority, if set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)

        # TX channel flags
        flags:
          invert_out: false              # Whether to invert RMT channel signal before output to GPIO pad
          with_dma: false                # If set, allocate RMT channel with DMA capability
                                          # Please check the macro definition SOC_RMT_SUPPORT_DMA in soc/soc_caps.h
                                          # to determine whether the chip supports
          io_od_mode: false              # Configure GPIO as open-drain mode
          allow_pd: false                # If set, allow power domain to power off during sleep (saves power, uses more RAM)
          init_level: 0                  # Set initial level of RMT channel signal (0=low, 1=high)

IDF 6 RX Channel Full Fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # -----------------------------------------------------------------------------
    # RMT RX example - Using rmt_rx_channel_config_t structure
    # -----------------------------------------------------------------------------
    - name: rmt_rx
      type: rmt
      role: rx
      config:
        # Basic channel configuration
        gpio_num: 19                     # [IO] GPIO number used by RMT channel. Set to -1 if unused
        clk_src: RMT_CLK_SRC_DEFAULT     # Clock source of RMT channel, channels in same group must use same clock source
                                          # Please check the 'rmt_clock_source_t' in 'driver/rmt_types.h' for valid values
        resolution_hz: 1000000           # [TO_BE_CONFIRMED] Channel clock resolution, in Hz (1MHz = 1us precision, good for 38kHz IR)
        mem_block_symbols: 64            # Size of memory block in number of rmt_symbol_word_t (even number required)
                                          # In DMA mode: controls DMA buffer size
                                          # In normal mode: controls number of RMT memory blocks
        intr_priority: 1                 # RMT interrupt priority, if set to 0, the driver will try to allocate an interrupt with a relative low priority (1,2,3)

        # RX channel flags
        flags:
          invert_in: false               # Whether to invert the incoming RMT channel signal
          with_dma: false                # If set, allocate RMT channel with DMA capability
                                          # Please check the macro definition SOC_RMT_SUPPORT_DMA in soc/soc_caps.h
                                          # to determine whether the chip supports
          allow_pd: false                # If set, allow power domain to power off during sleep (saves power, uses more RAM)

Applicable Devices
------------------

.. list-table::
   :header-rows: 1

   * - device type
     - Usage
     - Description
   * - ``led_strip``
     - Uses RMT driver capability when ``sub_type`` is ``rmt``
     - The current ``led_strip`` device template places ``rmt`` parameters in the device-side ``config.rmt``; referencing a standalone ``rmt`` peripheral is not required
   * - ``custom``
     - The device side can reference a defined ``rmt`` peripheral
     - Encoder, transmit queue usage, and receive buffer are determined by the custom device or application code

Reference Code
--------------

- ``esp_board_manager/peripherals/periph_rmt/idf5/periph_rmt.c``
- ``esp_board_manager/peripherals/periph_rmt/idf6/periph_rmt.c``
- ``esp_board_manager/test_apps/main/periph/test_periph_rmt.c``

Board Examples
--------------

- ``esp_board_manager/test_apps/components/board_customer/boards/esp32_s3_devkitc/board_peripherals.yaml``: Defines the ``rmt_tx`` test peripheral.
- ``esp_boards/esp32_s31_korvo_1/board_devices.yaml``: ``led_strip`` using ``sub_type: rmt``.
- ``esp_boards/esp32_s31_function_coreboard_1/board_devices.yaml``: ``led_strip`` using ``sub_type: rmt``.

Notes
-----

- The ``clk_src`` of channels within the same RMT group must be consistent; this constraint is noted in the template comments.
- ``with_dma`` is only available on targets that support RMT DMA.
- ``role`` determines whether TX or RX configuration is parsed. TX-specific fields must not be written into an RX peripheral, and RX-specific fields must not be written into a TX peripheral.
- After modifying RMT peripheral configuration, re-run ``idf.py bmgr -b <board>``.

Debugging Tips
--------------

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_periph_handle` to obtain the RMT peripheral handle. The handle type is ``periph_rmt_handle_t``:

.. code-block:: c

   typedef struct {
       rmt_channel_handle_t  channel;  /*!< Generic RMT channel handle */
   } periph_rmt_handle_t;

``channel`` can be passed to ``rmt_transmit``, ``rmt_receive``, and other ESP-IDF RMT APIs; the specific available interfaces depend on whether ``role`` is configured as TX or RX in ``board_peripherals.yaml``.

The relevant declarations are in ``esp_board_manager/peripherals/periph_rmt/periph_rmt.h``.

Underlying ESP-IDF driver documentation: `Remote Control Transceiver (RMT) <https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/esp32s3/api-reference/peripherals/rmt.html>`__\ .
