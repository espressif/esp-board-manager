Ethernet (ethernet)
===================

:link_to_translation:`zh_CN:[中文]`

.. _ethernet-intro:

Overview
--------

The ``ethernet`` device initializes the on-chip EMAC and PHY through ESP-IDF ``esp_eth``. The RMII and RGMII subtypes each initialize and deinitialize their interface, using the generated MAC, PHY, clock, and GPIO configuration. It creates the MAC with ``esp_eth_mac_new_esp32`` and uses the PHY factory registered for ``chip``, or ``esp_eth_phy_new_generic`` when no factory is registered, before calling ``esp_eth_driver_install``. Applications obtain ``dev_ethernet_handles_t`` from :cpp:func:`esp_board_manager_get_device_handle` and pass its ``eth_handle`` to ``esp_eth`` APIs.

Device initialization does not create a netif, a default event loop, netif glue, or DHCP, and it does not call ``esp_eth_start``. The application owns the network stack.

.. _ethernet-usage-modes:

Supported Usage Modes
---------------------

``ethernet`` provides ``emac_rmii`` and ``emac_rgmii`` subtypes. The subtype selects the MAC-PHY interface, clock fields, and data GPIO layout. ``chip`` selects the PHY parser, driver factory, and optional chip-specific initialization. ``spi`` is reserved and rejected by the parser:

- `RGMII`_
- `RMII`_

``config.mac_config`` and ``config.phy_config`` expose the scalar MAC and PHY constructor settings. DMA burst length, interrupt priority, MDC frequency, and link polling are configured directly under ``config``. The full template identifies the defaults for optional tuning fields. An omitted PHY address or reset GPIO defaults to ``-1``. The generated configuration derives the native interface and union members from the subtype. MAC/PHY objects, network-stack callbacks, and other runtime resources remain managed by code.

.. _ethernet-min-config:

Minimal Configuration
---------------------

The device owns the GPIOs directly. Do not add a ``board_peripherals.yaml`` entry, and do not bind a BMGR ``gpio`` peripheral.

.. _ethernet-rgmii:

RGMII
^^^^^

See complete fields: :ref:`ethernet-rgmii-full`.

Use this layout on SoCs that mux the data plane through GPIO, such as ESP32-S31. ``dataif_gpio`` is required.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: ethernet
        type: ethernet
        sub_type: emac_rgmii
        chip: yt8531
        config:
          phy_config:
            phy_addr: -1
            reset_gpio_num: 7          # [IO]
          smi_gpio:
            mdc_num: 5                 # [IO]
            mdio_num: 6                # [IO]
          clock_config:
            clock_rx_gpio: 14          # [IO]
            clock_tx_gpio: 13          # [IO]
            clock_phy_ref_gpio: -1     # [IO]
          dataif_gpio:
            tx_ctl_num: 12             # [IO]
            txd0_num: 8                # [IO]
            txd1_num: 9                # [IO]
            txd2_num: 10               # [IO]
            txd3_num: 11               # [IO]
            rx_ctl_num: 15             # [IO]
            rxd0_num: 19               # [IO]
            rxd1_num: 18               # [IO]
            rxd2_num: 17               # [IO]
            rxd3_num: 16               # [IO]
          vendor_config:
            rxc_dly_en: true
            rx_delay_sel: 0
            tx_delay_sel: 13
            tx_delay_sel_fe: 13

.. _ethernet-rmii:

RMII
^^^^

See complete fields: :ref:`ethernet-rmii-full`.

Use this layout on classic ESP32 boards with fixed RMII pins. Classic ESP32 must not set ``dataif_gpio``.

The example uses the stock ESP32-Ethernet-Kit wiring: the PHY supplies the 50 MHz RMII reference clock to GPIO0, so select ``EMAC_CLK_EXT_IN``. Boards that drive the PHY clock from the ESP32 must select a clock output mode and pin supported by their hardware.

``board_devices.yaml``:

.. code-block:: yaml

    devices:
      - name: ethernet
        type: ethernet
        sub_type: emac_rmii
        chip: ip101
        config:
          phy_config:
            phy_addr: 1
            reset_gpio_num: 5          # [IO]
          smi_gpio:
            mdc_num: 23                # [IO]
            mdio_num: 18               # [IO]
          clock_config:
            clock_mode: EMAC_CLK_EXT_IN
            clock_gpio: 0              # [IO]

.. _ethernet-full-fields:

All Fields
----------

.. _ethernet-rgmii-full:

RGMII All Fields
^^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: ethernet
      type: ethernet
      sub_type: emac_rgmii
      chip: yt8531                   # PHY chip/model name (optional)
                                     # yt8531 automatically adds espressif/yt8531: ^0.2.0 (ESP-IDF v6.1+)
      config:
        mac_config:                  # Optional eth_mac_config_t fields
          sw_reset_timeout_ms: 100   # Software reset timeout in ms
          rx_task_stack_size: 4096   # Receive-task stack size in bytes
          rx_task_prio: 15           # Receive-task priority
          flags: 0                   # Integer mask or list: ETH_MAC_FLAG_WORK_WITH_CACHE_DISABLE,
                                     # ETH_MAC_FLAG_PIN_TO_CORE
        phy_config:                  # Optional eth_phy_config_t fields
          phy_addr: -1               # PHY address; -1 enables auto-detection
          reset_gpio_num: 7          # [IO] PHY reset GPIO; -1 if unused (BMGR default)
          reset_timeout_ms: 100      # PHY software reset timeout in ms; minimum 10 ms
          autonego_timeout_ms: 4000  # Auto-negotiation timeout in ms
          hw_reset_assert_time_us: 0 # Hardware reset pulse in us; 0 uses PHY-specific default
          post_hw_reset_delay_ms: 0  # Delay after hardware reset; 0 uses PHY default, -1 skips wait
        dma_burst_len: ETH_DMA_BURST_LEN_16 # ESP32-S31 default; 32 is unsupported on S31
        intr_priority: 0             # 0 or negative: automatic; explicit priority: 1, 2, or 3
        mdc_freq_hz: 0               # 0 or negative: default range up to 2.5 MHz;
                                     # positive: HAL selects the closest supported divider
        check_link_period_ms: 2000   # Driver link-status polling interval in ms
        smi_gpio:
          mdc_num: 5                 # [IO] SMI MDC GPIO
          mdio_num: 6                # [IO] SMI MDIO GPIO
        clock_config:
          clock_rx_gpio: 14          # [IO] RGMII RX clock GPIO
          clock_tx_gpio: 13          # [IO] RGMII TX clock GPIO
          clock_phy_ref_gpio: -1     # [IO] optional PHY ref clock GPIO; -1 if unused
        dataif_gpio:
          tx_ctl_num: 12             # [IO]
          txd0_num: 8                # [IO]
          txd1_num: 9                # [IO]
          txd2_num: 10               # [IO]
          txd3_num: 11               # [IO]
          rx_ctl_num: 15             # [IO]
          rxd0_num: 19               # [IO]
          rxd1_num: 18               # [IO]
          rxd2_num: 17               # [IO]
          rxd3_num: 16               # [IO]
        vendor_config:               # Optional PHY-specific fields, parsed by the chip plugin
                                     # Built-in yt8531 parser: emac_rgmii only; native delay type
                                     # Set all four members; omission preserves PHY delay settings.
                                     # Values below match ESP32-S31-Function-CoreBoard-1.
          rxc_dly_en: true           # Enable approximately 2 ns coarse RX delay
          rx_delay_sel: 0            # RX fine delay steps, 0..15 (approximately 150 ps per step)
          tx_delay_sel: 13           # Gigabit TX delay steps, 0..15
          tx_delay_sel_fe: 13        # 10/100 Mbps TX delay steps, 0..15

.. _ethernet-rmii-full:

RMII All Fields
^^^^^^^^^^^^^^^

.. code-block:: yaml

    - name: ethernet
      type: ethernet
      sub_type: emac_rmii
      chip: ip101                    # PHY chip/model name (optional)
      config:
        mac_config:                  # Optional eth_mac_config_t fields
          sw_reset_timeout_ms: 100   # Software reset timeout in ms
          rx_task_stack_size: 4096   # Receive-task stack size in bytes
          rx_task_prio: 15           # Receive-task priority
          flags: 0                   # Integer mask or list: ETH_MAC_FLAG_WORK_WITH_CACHE_DISABLE,
                                     # ETH_MAC_FLAG_PIN_TO_CORE
        phy_config:                  # Optional eth_phy_config_t fields
          phy_addr: 1                # PHY address; -1 enables auto-detection
          reset_gpio_num: 5          # [IO] PHY reset GPIO; -1 if unused (BMGR default)
          reset_timeout_ms: 100      # PHY software reset timeout in ms; minimum 10 ms
          autonego_timeout_ms: 4000  # Auto-negotiation timeout in ms
          hw_reset_assert_time_us: 0 # Hardware reset pulse in us; 0 uses PHY-specific default
          post_hw_reset_delay_ms: 0  # Delay after hardware reset; 0 uses PHY default, -1 skips wait
        dma_burst_len: ETH_DMA_BURST_LEN_32 # ESP32/ESP32-P4 default; use 16 for ESP32-S31
        intr_priority: 0             # 0 or negative: automatic; explicit priority: 1, 2, or 3
        mdc_freq_hz: 0               # 0 or negative: default range up to 2.5 MHz;
                                     # positive: HAL selects the closest supported divider
        check_link_period_ms: 2000   # Driver link-status polling interval in ms
        smi_gpio:
          mdc_num: 31                # [IO] SMI MDC GPIO
          mdio_num: 52               # [IO] SMI MDIO GPIO
        # ESP32-P4 RMII output-clock example: route GPIO23 output to GPIO50 input.
        # Match every GPIO and the external clock-return connection to the board schematic.
        # Classic ESP32: remove dataif_gpio and clock_config_out_in, then select
        # EMAC_CLK_EXT_IN with clock_gpio: 0 for PHY-supplied clock (as on Ethernet-Kit).
        # ESP32-S31: use DMA burst length 16; clock_config_out_in may be omitted or
        # use GPIO -1 to select its internal output-clock path.
        clock_config:
          clock_mode: EMAC_CLK_OUT
          clock_gpio: 23             # [IO] PHY reference-clock output
        clock_config_out_in:         # Only RMII OUT without internal output-to-input loopback
          clock_mode: EMAC_CLK_EXT_IN
          clock_gpio: 50             # [IO] External clock-return input; required on ESP32-P4
        dataif_gpio:                 # Required with configurable RMII data GPIOs
          tx_en_num: 49              # [IO]
          txd0_num: 34               # [IO]
          txd1_num: 35               # [IO]
          crs_dv_num: 28             # [IO]
          rxd0_num: 29               # [IO]
          rxd1_num: 30               # [IO]

.. _ethernet-deps:

Component Dependencies
----------------------

``ethernet`` uses the ESP-IDF built-in ``esp_eth`` component. For ``chip: yt8531``, the generator automatically adds ``espressif/yt8531: "^0.2.0"`` to ``components/gen_bmgr_codes/idf_component.yml``. This driver requires ESP-IDF v6.1 or later. An explicit entry in the device's ``dependencies`` overrides the default version and can specify a local driver path. Declare custom PHY driver dependencies in the Ethernet device's ``dependencies`` field in ``board_devices.yaml``, at the same level as ``chip`` and ``config``. For example:

.. code-block:: yaml

    dependencies:
      my_vendor/my_phy: "^1.0.0"

Internal EMAC also requires ``CONFIG_ETH_USE_ESP32_EMAC``.

.. _ethernet-peripherals:

Required Peripherals
--------------------

No BMGR peripheral is required. The device owns the EMAC, SMI, and data-plane GPIOs directly.

.. _ethernet-phy-extensions:

Ethernet Chip Extensions
------------------------

When an Ethernet chip needs additional configuration or initialization that ESP Board Manager does not provide, add a Python extension and, if needed, a C adapter. The extension parses chip-specific YAML fields and selects initialization code.

Place the extension in ``<board-directory>/ethernet_extra/`` under the selected board. For a custom board supplied by the application, use ``<project-directory>/components/<board-directory>/ethernet_extra/`` and select the board through the normal board-discovery mechanism.

When running ``idf.py bmgr -b <board>``, BMGR automatically loads Python modules from the selected board's ``ethernet_extra/`` directory. This does not require an amend or ``-a``. The C adapter can reside beside the Python module:

.. code-block:: text

    ethernet_extra/
    ├── my_phy.py
    └── my_phy_bmgr.c

To supply an extension through an amend, explicitly list its Python module in ``board_amend.yaml``:

.. code-block:: yaml

    apply:
      - ethernet_extra/my_phy.py

Use ``idf.py bmgr -b <board> -a <amend-dir>`` to select that amend. The loader does not scan amend directories for Python modules. The module's ``sources`` entry includes the C adapter; it does not need a separate ``apply`` entry.

A Python module exports ``get_phy_parsers()``, returning a mapping from YAML ``chip`` values to descriptors. A descriptor can provide ``parse`` and ``sources``. One module can register multiple chips. Descriptors are loaded from built-in ``devices/dev_ethernet/ethernet_extra/*.py``, then the selected board's ``ethernet_extra/`` directory, and finally explicitly applied amend modules in manifest order. A later descriptor replaces the earlier descriptor for the same chip. Registering the same chip more than once within one directory is an error.

``parse(name, vendor_config, context)`` validates the chip-specific YAML mapping and returns ``struct_type``, ``struct_init``, and ``includes``. ``context`` contains ``chip``, ``sub_type``, ``target``, and ``idf_version`` (a version tuple). The declared C type determines member widths. BMGR emits an independent constant object and fills the common ``vendor_config`` pointer and ``vendor_config_size``.

For example, save the following module as ``ethernet_extra/my_phy.py`` and select ``chip: my_phy`` in ``board_devices.yaml``. The example assumes a driver component provides ``my_phy.h`` and ``my_phy_vendor_config_t`` with a ``uint32_t timeout_ms`` member. Replace the component and interface names with those of the actual driver.

.. code-block:: python

    def parse_my_phy(name, vendor_config, context):
        if set(vendor_config) != {"timeout_ms"}:
            raise ValueError(f"{name}: vendor_config requires timeout_ms")
        timeout = vendor_config["timeout_ms"]
        if type(timeout) is not int or not 1 <= timeout <= 0xFFFFFFFF:
            raise ValueError(f"{name}: timeout_ms must be a positive uint32_t")
        return {
            "struct_type": "my_phy_vendor_config_t",
            "struct_init": {"timeout_ms": timeout},
            "includes": ["my_phy.h"],
        }

    def get_phy_parsers():
        return {
            "my_phy": {
                "parse": parse_my_phy,
                "sources": ["my_phy_bmgr.c"],
            }
        }

``sources`` is optional, and its paths are relative to the Python module. Selecting the chip includes these adapters even when ``vendor_config`` is omitted. BMGR compiles only the selected adapters in ``gen_bmgr_codes`` with the device's component dependencies.

In ``my_phy_bmgr.c``, include ``dev_ethernet.h`` and the driver header, implement the post-init callback, and register the operations:

.. code-block:: c

    ESP_BOARD_ETHERNET_PHY_REGISTER(my_phy, esp_eth_phy_new_my_phy, my_phy_post_init);

The constructor has signature ``esp_eth_phy_t *(const eth_phy_config_t *)``. The post-init callback has signature ``esp_err_t (esp_eth_handle_t, const dev_ethernet_config_t *)`` and runs after driver installation. It must validate ``vendor_config_size`` before reading ``vendor_config`` as ``my_phy_vendor_config_t``. A device supplying vendor configuration requires a registered post-init callback. Return an error to roll back initialization. Use one C registration per chip, and keep its identifier identical to the YAML ``chip``.

.. _ethernet-code:

Reference Code
--------------

- ``esp_board_manager/test_apps/main/test_dev_ethernet.c``
- ``esp_boards/esp32_s31_function_coreboard_1/board_devices.yaml``: RGMII, ``chip: yt8531``
- ``esp_boards/esp32_ethernet_kit/board_devices.yaml``: RMII, ``chip: ip101``, clock GPIO0
- ``esp_board_manager/test_apps/components/test_ethernet_extra/``: custom Python extension, C adapter, and local driver component. This compile-only fixture skips device initialization. The CI matrix runs ``idf.py bmgr -b test_ethernet_extra`` followed by a build of the test application.

.. _ethernet-notes:

Notes
-----

- ``emac_rgmii`` requires ESP-IDF v6.1 or later and ``SOC_EMAC_SUPPORT_1000M``. ``emac_rmii`` works on ESP-IDF v5.4 or later with ``CONFIG_ETH_USE_ESP32_EMAC``.
- Initialization returns after installing the driver. When the application needs a link or IP, it creates the netif, attaches glue, and calls ``esp_eth_start``.
- ``chip: yt8531`` selects the dedicated PHY driver. Registered PHY operations provide a constructor and an optional post-init callback. Without registered operations, the device uses the generic PHY driver.
- ``vendor_config`` requires a matching chip parser. The parser generates an independent constant object with its declared native C type. The common Ethernet configuration stores its pointer and size; vendor fields are not members of the common structure.
- PHY post-init callbacks receive the installed ``eth_handle`` and ``const dev_ethernet_config_t *config``. The YT8531 driver handles auto-negotiation. For ``chip: yt8531`` with ``emac_rgmii``, optional ``vendor_config`` supplies all four board-specific delay settings to ``YT8531_ETH_CMD_S_RGMII_CLK_DELAY``. Omitting this block leaves the PHY delay settings unchanged. The S31 board example enables the approximately 2 ns coarse RX delay, sets fine RX delay to zero, and sets both TX delay fields to 13 (approximately 1.95 ns).
- ``DEV_ETHERNET_IO_LIST`` has separate ``emac_rmii`` and ``emac_rgmii`` entries.
- Classic ESP32 must not set ``dataif_gpio``. SoCs with ``SOC_EMAC_USE_MULTI_IO_MUX`` or ``SOC_EMAC_MII_USE_GPIO_MATRIX`` (for example ESP32-S31) must provide ``dataif_gpio``.
- ``clock_config_out_in`` applies only to ``emac_rmii`` with ``EMAC_CLK_OUT`` on SoCs without ``SOC_EMAC_RMII_CLK_OUT_INTERNAL_LOOPBACK``. ESP32-P4 requires a valid external clock-return GPIO. ESP32-S31 can omit the block or use ``-1`` to select its internal output-clock path. Classic ESP32 does not support this field; RGMII rejects it.
- ``ethernet`` does not accept non-empty ``peripherals`` bindings.
- The application calls ``esp_eth_start`` and ``esp_eth_stop``. Before BMGR deinitialization, the application must stop a running driver and release application resources associated with it, such as glue/netif objects. BMGR only uninstalls the driver and deletes MAC/PHY.
- After modifying YAML, run ``idf.py bmgr -b <board>`` before building.

.. _ethernet-debug:

Debugging Tips
--------------

On a board that defines ``ethernet``, run the test application commands:

- ``case run eth.handle``: checks the handle, config, and MAC address without calling ``esp_eth_start``.
- ``case run eth.link``: creates a netif, starts the driver, waits for ``ETHERNET_EVENT_CONNECTED``, and always stops the driver before return.

Boards without an ``ethernet`` device skip these cases.

.. _ethernet-api:

API Reference
-------------

Use :cpp:func:`esp_board_manager_get_device_handle` to obtain ``dev_ethernet_handles_t``:

.. code-block:: c

    typedef struct {
        esp_eth_handle_t  eth_handle;  /*!< Native ESP-IDF Ethernet driver handle */
    } dev_ethernet_handles_t;

Pass ``eth_handle`` to ``esp_eth_start()``, ``esp_eth_ioctl()``, and other ESP-IDF Ethernet APIs. The declarations are in ``esp_board_manager/devices/dev_ethernet/dev_ethernet.h``.
