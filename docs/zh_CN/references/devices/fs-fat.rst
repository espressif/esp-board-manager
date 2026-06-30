FAT 文件系统 fs_fat
===================

:link_to_translation:`en:[English]`

简介
------

``fs_fat`` 设备用于将 SD 卡挂载为 FAT 文件系统。初始化时调用 ESP-IDF FATFS 挂载接口，并返回 ``dev_fs_fat_handle_t``，应用通过标准 C 文件接口访问 ``mount_point`` 下的文件。

该设备支持 SDMMC 与 SDSPI 两种接入方式。SDMMC 直接配置 SDMMC host、slot 与引脚；SDSPI 复用板级 ``spi`` 外设，并在设备侧配置 SD 卡片选引脚。

支持的使用模式
---------------------

``fs_fat`` 按 ``sub_type`` 区分使用模式：

- `SDMMC`_
- `SPI`_

最小配置
------------

SDMMC
^^^^^^^^^

``sdmmc`` 模式使用 ``SDMMC_HOST_DEFAULT()`` 与 ``esp_vfs_fat_sdmmc_mount()``，适合直接连接到 SDMMC host 的 SD 卡，无需新增 ``board_peripherals.yaml`` 条目。``ldo_chan_id`` 仅在目标芯片支持 ``SOC_GP_LDO_SUPPORTED`` 时用于片上 LDO 供电控制，是否需要配置取决于开发板 SDMMC 供电电路。

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: fs_fat
        type: fs_fat
        sub_type: sdmmc
        config:
          mount_point: "/sdcard"
          vfs_config:
            format_if_mount_failed: false
          sub_config:
            bus_width: 1
            pins:
              clk: -1
              cmd: -1
              d0: -1

SPI
^^^^^^^

``spi`` 模式使用 ``SDSPI_HOST_DEFAULT()`` 与 ``esp_vfs_fat_sdspi_mount()``，必须引用一个 ``spi`` 外设，运行时从该外设获取 SPI host 端口。

``board_peripherals.yaml``：

.. code-block:: yaml

    peripherals:
      - name: spi_master
        type: spi
        role: master
        config:
          spi_bus_config:
            spi_port: 1
            mosi_io_num: 11
            miso_io_num: 13
            sclk_io_num: 12

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: fs_fat
        type: fs_fat
        sub_type: spi
        config:
          mount_point: "/sdcard"
          vfs_config:
            format_if_mount_failed: false
          sub_config:
            cs_gpio_num: 15
        peripherals:
          - name: spi_master

完整字段
------------

SDMMC 完整字段
^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example FS_FAT device with SDMMC sub device configuration
    - name: fs_fat          # The name of the device, must be unique
      type: fs_fat          # The type of the device, must be unique
      version: 1.0.0
      sub_type: sdmmc       # The sub type of the device, must be 'sdmmc' or 'spi'
      config:
        # Mount point configuration
        mount_point: "/sdcard"            # The mount point path for the SD card filesystem
        # VFS (Virtual File System) configuration
        vfs_config:
          format_if_mount_failed: false   # Format the card if mount fails
          max_files: 5                    # Maximum number of files that can be open simultaneously
          allocation_unit_size: 16384     # Allocation unit size in bytes (0 for default)

        sub_config:
          slot: SDMMC_HOST_SLOT_1           # SD card slot: SDMMC_HOST_SLOT_1 or SDMMC_HOST_SLOT_2
          frequency: SDMMC_FREQ_HIGHSPEED   # Clock frequency:
                                            #   SDMMC_FREQ_DEFAULT      (20000 Hz)
                                            #   SDMMC_FREQ_HIGHSPEED    (40000 Hz)
                                            #   SDMMC_FREQ_PROBING      (400 Hz)
                                            #   SDMMC_FREQ_52M          (52000 Hz)
                                            #   SDMMC_FREQ_26M          (26000 Hz)
                                            #   SDMMC_FREQ_DDR50        (50000 Hz)
                                            #   SDMMC_FREQ_SDR50        (100000 Hz)
          bus_width: 1                      # [TO_BE_CONFIRMED] Bus width: 1, 4, or 8 bits
          slot_flags: SDMMC_SLOT_FLAG_INTERNAL_PULLUP # Slot flags (SDMMC_SLOT_FLAG_INTERNAL_PULLUP, SDMMC_SLOT_FLAG_WP_ACTIVE_HIGH, SDMMC_SLOT_FLAG_UHS1)

          # GPIO pin configuration
          pins:
            clk: -1                         # [IO] Clock pin (-1 for not used)
            cmd: -1                         # [IO] Command pin (-1 for not used)
            d0: -1                          # [IO] Data line 0 pin (-1 for not used)
            d1: -1                          # [IO] Data line 1 pin (-1 for not used)
            d2: -1                          # [IO] Data line 2 pin (-1 for not used)
            d3: -1                          # [IO] Data line 3 pin (-1 for not used)
            d4: -1                          # [IO] Data line 4 pin (-1 for not used, 8-bit mode only)
            d5: -1                          # [IO] Data line 5 pin (-1 for not used, 8-bit mode only)
            d6: -1                          # [IO] Data line 6 pin (-1 for not used, 8-bit mode only)
            d7: -1                          # [IO] Data line 7 pin (-1 for not used, 8-bit mode only)
            cd: -1                          # [IO] Card detect pin (-1 for not used)
            wp: -1                          # [IO] Write protect pin (-1 for not used)

          # For SoCs where the SD power can be supplied both via an internal or external (e.g. on-board LDO) power supply.
          # When using specific IO pins (which can be used for ultra high-speed SDMMC) to connect to the SD card
          # and the internal LDO power supply, we need to initialize the power supply first.
          # This is an optional configuration, You can determine the available channel ID by checking the SoC datasheet.
          # whether it needs to be configured depends on your board.
          # Please check the schematic diagram or other documentation to determine if SDMMC is powered by LDO.
          # e.g., In ESP32-P4 Function-EV Board, VO4_LDO is connected to power the SDMMC IO. set it to 4 for VO4_LDO.
          ldo_chan_id: -1

SPI 完整字段
^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example FS_FAT with SPI sub device configuration
    - name: fs_fat          # The name of the device, must be unique
      type: fs_fat          # The type of the device, must be unique
      version: 1.0.0
      sub_type: spi         # The sub type of the device, must be 'sdmmc' or 'spi'
      config:
        # Mount point configuration
        mount_point: "/sdcard"            # The mount point path for the SD card filesystem
        # VFS (Virtual File System) configuration
        vfs_config:
          format_if_mount_failed: false   # Format the card if mount fails
          max_files: 5                    # Maximum number of files that can be open simultaneously
          allocation_unit_size: 16384     # Allocation unit size in bytes (0 for default)

        sub_config:
          cs_gpio_num: 15   # [IO] Chip select GPIO number
      # Peripherals must be defined at device level (consistent with other devices)
      # The SPI peripheral must have: type: spi and role: master
      peripherals:
        - name: spi_master  # [TO_BE_CONFIRMED] SPI bus name (can be any name, e.g., spi_0, spi_sdcard, etc.)

组件依赖
------------

``fs_fat`` 使用 ESP-IDF 内置 ``fatfs``、``sdmmc``、``driver`` 和 ``esp_vfs_fat`` 相关接口。当前设备模板未要求在 ``board_devices.yaml`` 中为该设备额外声明 ``dependencies``。

依赖外设
------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - 必选性
     - 用途
   * - ``spi``
     - ``master``
     - ``sub_type: spi`` 必选
     - 提供 SDSPI 总线
   * - 无
     - 无
     - ``sub_type: sdmmc`` 不需要板级外设引用
     - SDMMC host 和 slot 由设备配置直接初始化

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_fs_fat.c``
- ``esp_board_manager/devices/dev_fs_fat/dev_fs_fat.c``
- ``esp_board_manager/devices/dev_fs_fat/dev_fs_fat_sub_sdmmc.c``
- ``esp_board_manager/devices/dev_fs_fat/dev_fs_fat_sub_spi.c``
- ``esp_board_manager/examples/play_sdcard_music/main/play_sdcard_music.c``
- ``esp_board_manager/examples/record_to_sdcard/main/record_to_sdcard.c``

板级参考
------------

- ``esp_boards/esp32_p4_function_ev_board/board_devices.yaml``
- ``esp_boards/esp32_s3_korvo_2_3/board_devices.yaml``
- ``esp_friends_boards/esp32_s3_korvo_2l/board_devices.yaml``
- ``esp_boards/esp_vocat_1_2/board_devices.yaml``
- ``esp_boards/esp32_lyrat_mini_1_1/board_devices.yaml``
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``

注意事项
------------

- 公共 YAML 字段规则见 :doc:`/programming-guide/yaml-rules`。
- ``spi`` 模式必须在设备的 ``peripherals`` 中引用已有 SPI 外设；缺少 SPI 外设名时初始化会失败。
- ``sdmmc`` 模式的引脚、总线宽度和 slot 需要与芯片能力及板级连线一致。
- ``format_if_mount_failed: true`` 会在挂载失败时格式化文件系统，应结合数据保留需求设置。
- 修改 YAML 后需要重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，句柄类型为 ``dev_fs_fat_handle_t``：

.. code-block:: c

   typedef struct {
       sdmmc_card_t *card;         /*!< SD card card handle */
       sdmmc_host_t  host;         /*!< SD card host handle */
       char         *mount_point;  /*!< Mount point path */
   } dev_fs_fat_handle_t;

``card`` 可传入 ESP-IDF ``sdmmc`` 驱动接口查询卡信息；``mount_point`` 与 YAML 中的 ``mount_point`` 字段一致，应用通过 POSIX 文件 API 访问该路径。

相关声明位于 ``esp_board_manager/devices/dev_fs_fat/dev_fs_fat.h``。
