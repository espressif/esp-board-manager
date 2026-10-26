以太网 ethernet
====================

:link_to_translation:`en:[English]`

.. _ethernet-intro:

简介
------

``ethernet`` 设备通过 ESP-IDF ``esp_eth`` 初始化片内 EMAC 与 PHY。RMII 与 RGMII 子类型各自完成对应接口的初始化和反初始化，使用生成的 MAC、PHY、时钟与引脚配置。设备通过 ``esp_eth_mac_new_esp32`` 创建 MAC，使用 ``chip`` 对应的已注册 PHY 工厂；未注册工厂时使用 ``esp_eth_phy_new_generic``\ ，随后调用 ``esp_eth_driver_install``\ 。应用通过 :cpp:func:`esp_board_manager_get_device_handle` 获取 ``dev_ethernet_handles_t``\ ，并将其中的 ``eth_handle`` 传入 ``esp_eth`` API。

设备初始化不创建 netif、不创建默认事件循环、不安装 netif glue、不启动 DHCP，也不调用 ``esp_eth_start``\ 。网络协议栈由应用自行挂接。

.. _ethernet-usage-modes:

支持的使用模式
-----------------

``ethernet`` 提供 ``emac_rmii`` 和 ``emac_rgmii`` 两种子类型，分别确定 MAC-PHY 接口、时钟字段和数据引脚配置。``chip`` 选择 PHY 解析器、驱动工厂及可选的芯片专用初始化。``spi`` 为预留值，解析器会拒绝：

- `RGMII`_
- `RMII`_

``config.mac_config`` 和 ``config.phy_config`` 提供 MAC 与 PHY 构造参数中的标量配置。DMA 突发长度、中断优先级、MDC 频率和链路轮询周期直接配置在 ``config`` 下。可选调节字段的默认值见完整模板。省略 PHY 地址或复位 GPIO 时默认值为 ``-1``\ 。生成配置根据子类型确定原生接口和联合体成员。MAC/PHY 对象、网络栈回调及其他运行期资源由代码管理。

.. _ethernet-min-config:

最小配置
------------

GPIO 由设备直接管理，无需新增 ``board_peripherals.yaml`` 条目，也不应绑定 BMGR ``gpio`` peripheral。

.. _ethernet-rgmii:

RGMII
^^^^^^^

完整字段见 :ref:`ethernet-rgmii-full`。

适用于数据平面经 GPIO 复用的 SoC，例如 ESP32-S31。必须填写 ``dataif_gpio``\ 。

``board_devices.yaml``\ ：

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
^^^^^^

完整字段见 :ref:`ethernet-rmii-full`。

适用于经典 ESP32 等固定 RMII 引脚的开发板。经典 ESP32 不得设置 ``dataif_gpio``\ 。

示例采用 ESP32-Ethernet-Kit 原装硬件连接，PHY 向 GPIO0 提供 50 MHz RMII 参考时钟，因此选择 ``EMAC_CLK_EXT_IN``。由 ESP32 向 PHY 提供时钟的开发板需按硬件连接选择支持的时钟输出模式和引脚。

``board_devices.yaml``\ ：

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

完整字段
------------

.. _ethernet-rgmii-full:

RGMII 完整字段
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

RMII 完整字段
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

组件依赖
------------

``ethernet`` 使用 ESP-IDF 内置的 ``esp_eth``\ 。当 ``chip: yt8531`` 时，生成器自动将 ``espressif/yt8531: "^0.2.0"`` 写入 ``components/gen_bmgr_codes/idf_component.yml``\ 。该驱动要求 ESP-IDF v6.1 或更高版本。在设备的 ``dependencies`` 中显式声明此依赖可覆盖默认版本，也可指定本地驱动路径。自定义 PHY 驱动依赖写在 ``board_devices.yaml`` 对应 Ethernet 设备的 ``dependencies`` 字段中，与 ``chip`` 和 ``config`` 同级。例如：

.. code-block:: yaml

    dependencies:
      my_vendor/my_phy: "^1.0.0"

片内 EMAC 还要求启用 ``CONFIG_ETH_USE_ESP32_EMAC``\ 。

.. _ethernet-peripherals:

依赖外设
------------

不需要 BMGR peripheral。EMAC、SMI 与数据平面 GPIO 由该设备直接管理。

.. _ethernet-phy-extensions:

以太网芯片扩展
------------------------------

当所使用的以太网芯片需要额外的配置或初始化操作，而 ESP Board Manager 尚未提供对应支持时，可以编写 Python 扩展脚本，并按需提供 C 适配代码。扩展负责解析芯片专有 YAML 字段并选择初始化代码。

将扩展放在所选开发板的 ``<开发板目录>/ethernet_extra/`` 下。应用工程提供自定义开发板时，可使用 ``<工程目录>/components/<开发板目录>/ethernet_extra/``\ ，并按现有开发板发现规则选择该板。

执行 ``idf.py bmgr -b <board>`` 时，BMGR 自动加载所选开发板 ``ethernet_extra/`` 目录中的 Python 模块，无需编写 amend 或传入 ``-a``\ 。C 适配文件可以与 Python 模块放在同一目录：

.. code-block:: text

    ethernet_extra/
    ├── my_phy.py
    └── my_phy_bmgr.c

如需通过 amend 提供扩展，在 ``board_amend.yaml`` 中显式列出 Python 模块：

.. code-block:: yaml

    apply:
      - ethernet_extra/my_phy.py

可通过 ``idf.py bmgr -b <board> -a <amend-dir>`` 指定该 amend。加载器不扫描 amend 目录中的 Python 模块。模块通过 ``sources`` 引入 C 适配文件，无需为该文件另写 ``apply`` 条目。

Python 模块通过 ``get_phy_parsers()`` 返回 YAML ``chip`` 值到描述符的映射。描述符可提供 ``parse`` 和 ``sources``\ ，同一模块可注册多个芯片。加载顺序为内置 ``devices/dev_ethernet/ethernet_extra/*.py``\ 、所选开发板的 ``ethernet_extra/`` 目录，最后按清单顺序加载 amend 显式应用的模块。后一级的同名芯片描述符完整替换前一级描述符。同一目录中重复注册同名芯片会报错。

``parse(name, vendor_config, context)`` 校验芯片专有 YAML 配置，返回 ``struct_type``\ 、``struct_init`` 和 ``includes``\ 。``context`` 包含 ``chip``\ 、``sub_type``\ 、``target`` 和版本元组 ``idf_version``\ 。成员宽度由显式 C 类型确定。BMGR 生成独立常量对象，并填写公共配置的 ``vendor_config`` 指针和 ``vendor_config_size``\ 。

例如，将以下模块保存为 ``ethernet_extra/my_phy.py``\ ，并在 ``board_devices.yaml`` 中选择 ``chip: my_phy``\ 。示例假定驱动组件提供 ``my_phy.h``\ ，其中 ``my_phy_vendor_config_t`` 定义了 ``uint32_t timeout_ms`` 成员。组件名和接口名需替换为实际驱动的定义。

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

``sources`` 为可选项，路径相对于 Python 模块解析。选择芯片后，即使省略 ``vendor_config`` 也会包含对应适配源码。BMGR 仅在 ``gen_bmgr_codes`` 中编译所选适配源码，并使用该设备的组件依赖。

在 ``my_phy_bmgr.c`` 中包含 ``dev_ethernet.h`` 和驱动头文件，实现安装后初始化回调，并注册操作：

.. code-block:: c

    ESP_BOARD_ETHERNET_PHY_REGISTER(my_phy, esp_eth_phy_new_my_phy, my_phy_post_init);

构造函数签名为 ``esp_eth_phy_t *(const eth_phy_config_t *)``\ 。安装后初始化回调签名为 ``esp_err_t (esp_eth_handle_t, const dev_ethernet_config_t *)``\ ，在驱动安装后执行。回调必须先检查 ``vendor_config_size``\ ，再将 ``vendor_config`` 作为 ``my_phy_vendor_config_t`` 读取。提供专有配置的设备必须有已注册的安装后初始化回调。回调返回错误时，设备初始化执行回滚。每个芯片仅保留一处 C 注册，标识符须与 YAML ``chip`` 一致。

.. _ethernet-code:

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_ethernet.c``
- ``esp_boards/esp32_s31_function_coreboard_1/board_devices.yaml``\ ：RGMII、``chip: yt8531``
- ``esp_boards/esp32_ethernet_kit/board_devices.yaml``\ ：RMII、``chip: ip101``\ 、时钟 GPIO0
- ``esp_board_manager/test_apps/components/test_ethernet_extra/``\ ：包含自定义 Python 扩展、C 适配代码和本地驱动组件的编译测试板。该测试板跳过设备初始化；CI 矩阵执行 ``idf.py bmgr -b test_ethernet_extra`` 后编译测试应用。

.. _ethernet-notes:

注意事项
------------

- ``emac_rgmii`` 需要 ESP-IDF v6.1 或更高版本及 ``SOC_EMAC_SUPPORT_1000M``\ 。``emac_rmii`` 在 ESP-IDF v5.4 及以上、且启用 ``CONFIG_ETH_USE_ESP32_EMAC`` 时可用。
- 初始化安装驱动后即返回。应用需要链路或 IP 时，自行创建 netif、安装 glue 并调用 ``esp_eth_start``\ 。
- ``chip: yt8531`` 选择专用 PHY 驱动。已注册的 PHY 操作提供构造函数和可选的安装后初始化回调。未注册操作时，设备使用通用 PHY 驱动。
- ``vendor_config`` 要求存在对应芯片的解析器。解析器生成采用显式原生 C 类型的独立常量对象。公共以太网配置保存对象指针和大小，不包含芯片专有字段。
- PHY 安装后初始化回调接收 ``eth_handle`` 和 ``const dev_ethernet_config_t *config``\ 。YT8531 驱动负责自动协商。``chip: yt8531`` 与 ``emac_rgmii`` 组合可通过可选的 ``vendor_config`` 提供完整的四项板级延迟设置，BMGR 使用 ``YT8531_ETH_CMD_S_RGMII_CLK_DELAY`` 下发。省略该配置块时保留 PHY 延迟设置。S31 开发板示例启用约 2 ns 的 RX 粗延迟、将 RX 细延迟设为零，并将两个 TX 延迟字段均设为 13（约 1.95 ns）。
- ``DEV_ETHERNET_IO_LIST`` 分别以 ``emac_rmii`` 和 ``emac_rgmii`` 为键。
- 经典 ESP32 不得设置 ``dataif_gpio``\ 。具备 ``SOC_EMAC_USE_MULTI_IO_MUX`` 或 ``SOC_EMAC_MII_USE_GPIO_MATRIX`` 的 SoC（例如 ESP32-S31）必须填写 ``dataif_gpio``\ 。
- ``clock_config_out_in`` 仅适用于 ``emac_rmii`` 的 ``EMAC_CLK_OUT`` 模式，且 SoC 不具备 ``SOC_EMAC_RMII_CLK_OUT_INTERNAL_LOOPBACK``\ 。ESP32-P4 必须配置有效的外部时钟回送 GPIO。ESP32-S31 可省略该配置块，或将 GPIO 设为 ``-1`` 以选择内部输出时钟路径。经典 ESP32 不支持该字段，RGMII 会拒绝该字段。
- ``ethernet`` 不接受非空 ``peripherals`` 绑定。
- 应用负责调用 ``esp_eth_start`` 和 ``esp_eth_stop``\ 。调用 BMGR 反初始化前，应用须停止已启动的驱动，并释放与该驱动关联的 glue/netif 等应用资源。BMGR 仅卸载驱动并删除 MAC/PHY。
- 修改 YAML 后，构建前需执行 ``idf.py bmgr -b <board>``\ 。

.. _ethernet-debug:

调试技巧
------------

在定义了 ``ethernet`` 的开发板上执行测试应用命令：

- ``case run eth.handle``\ ：检查句柄、配置与 MAC 地址，不调用 ``esp_eth_start``\ 。
- ``case run eth.link``\ ：创建 netif 并启动驱动，等待 ``ETHERNET_EVENT_CONNECTED``\ ，返回前始终停止驱动。

未声明 ``ethernet`` 的开发板会跳过上述用例。

.. _ethernet-api:

API 参考
------------

通过 :cpp:func:`esp_board_manager_get_device_handle` 获取 ``dev_ethernet_handles_t``\ ：

.. code-block:: c

    typedef struct {
        esp_eth_handle_t  eth_handle;  /*!< Native ESP-IDF Ethernet driver handle */
    } dev_ethernet_handles_t;

将 ``eth_handle`` 传入 ``esp_eth_start()``\ 、``esp_eth_ioctl()`` 等 ESP-IDF Ethernet API。声明位于 ``esp_board_manager/devices/dev_ethernet/dev_ethernet.h``\ 。
