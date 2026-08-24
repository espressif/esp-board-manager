LittleFS 文件系统 littlefs
===========================

:link_to_translation:`en:[English]`

简介
------

``littlefs`` 是基于 ``joltwallet/littlefs`` 组件的文件系统 device，用于将 LittleFS 挂载到 VFS 路径。BMGR 初始化该设备后，应用可通过 ``fopen()``、``fread()``、``fwrite()`` 等 POSIX 文件接口访问挂载路径下的文件。

``littlefs`` 按 ``sub_type`` 选择挂载后端：

- ``flash``：在片内 flash 分区上挂载 LittleFS。
- ``sdmmc``：初始化 SDMMC SD 卡，并在该卡上挂载 LittleFS。
- ``spi``：复用 BMGR SPI master 外设初始化 SDSPI SD 卡，并在该卡上挂载 LittleFS。

支持的使用模式
---------------------

``littlefs`` 按 ``sub_type`` 区分挂载后端：

- `Flash 分区挂载`_
- `SDMMC SD 卡挂载`_
- `SPI SD 卡挂载`_

最小配置
------------

Flash 分区挂载
^^^^^^^^^^^^^^^^^^^

``flash`` 模式调用 ``esp_vfs_littlefs_register`` 挂载 flash 分区，无需新增 ``board_peripherals.yaml`` 条目。工程分区表需要提供一个 ``data`` 类型分区；``partition_label`` 应与分区表的分区名称一致。

分区表示例：

.. code-block:: none

    littlefs, data, littlefs, , 0x50000,

该示例中，第一个 ``littlefs`` 是分区名称，``data`` 是分区类型，第二个 ``littlefs`` 是分区子类型。``partition_label`` 可改为其他名称，但必须与分区表中的分区名称一致。

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: littlefs
        type: littlefs
        sub_type: flash
        version: default
        config:
          vfs_config:
            base_path: "/littlefs"
            partition_label: "littlefs"
            format_if_mount_failed: false
            read_only: false
            dont_mount: false
            grow_on_mount: false

SDMMC SD 卡挂载
^^^^^^^^^^^^^^^^^^^

``sdmmc`` 模式由 BMGR 初始化 SDMMC host 和 SD 卡句柄，再将 SD 卡句柄传给 ``esp_vfs_littlefs_register``。该模式无需新增 ``board_peripherals.yaml`` 条目。

SDMMC 和 SDSPI 两种 SD 后端都依赖 ``joltwallet/littlefs`` 组件的 SD card 支持，需要在 ``sdkconfig.defaults.board`` 中启用：

.. code-block:: none

    CONFIG_LITTLEFS_SDMMC_SUPPORT=y

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: littlefs
        type: littlefs
        sub_type: sdmmc
        version: default
        config:
          vfs_config:
            base_path: "/littlefs"
            format_if_mount_failed: false
            read_only: false
            dont_mount: false
            grow_on_mount: false
          sub_config:
            slot: SDMMC_HOST_SLOT_1
            frequency: SDMMC_FREQ_HIGHSPEED
            bus_width: 1
            slot_flags: SDMMC_SLOT_FLAG_INTERNAL_PULLUP
            pins:
              clk: -1
              cmd: -1
              d0: -1

SPI SD 卡挂载
^^^^^^^^^^^^^^^^^^^

``spi`` 模式复用 ``board_peripherals.yaml`` 中已定义的 SPI master 外设，并在该总线上初始化 SDSPI SD 卡。设备级 ``peripherals`` 需引用一个 ``role`` 为 ``master`` 的 ``spi`` 外设。

SDMMC 和 SDSPI 两种 SD 后端都依赖 ``joltwallet/littlefs`` 组件的 SD card 支持，需要在 ``sdkconfig.defaults.board`` 中启用：

.. code-block:: none

    CONFIG_LITTLEFS_SDMMC_SUPPORT=y

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: littlefs
        type: littlefs
        sub_type: spi
        version: default
        config:
          vfs_config:
            base_path: "/littlefs"
            format_if_mount_failed: false
            read_only: false
            dont_mount: false
            grow_on_mount: false
          sub_config:
            cs_gpio_num: 15
            frequency: SDMMC_FREQ_DEFAULT
        peripherals:
          - spi_name: spi_master

完整字段
------------

Flash 分区完整字段
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example LittleFS device mounted from flash partition.
    - name: littlefs                          # Device name, must be unique
      type: littlefs                          # Device type, must be littlefs
      version: 1.0.0                          # Schema version
      sub_type: flash                         # Backend subtype: flash, sdmmc, or spi
      config:
        # vfs_config maps to the YAML-safe fields of esp_vfs_littlefs_conf_t
        vfs_config:
          base_path: "/littlefs"              # VFS mount point (default: /littlefs)
          partition_label: "storage"          # Flash partition name in the partition table; flash mode only (default: storage)
          format_if_mount_failed: false       # Format the filesystem if mount fails (default: false; cannot be used with read_only)
          read_only: false                    # Mount as read-only (default: false; cannot be used with format_if_mount_failed or grow_on_mount)
          dont_mount: false                   # Register the filesystem without mounting (default: false)
          grow_on_mount: false                # Grow filesystem to match partition size on mount (default: false; cannot be used with read_only)

SDMMC 卡完整字段
^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example LittleFS device mounted from SDMMC card.
    # Enable CONFIG_LITTLEFS_SDMMC_SUPPORT=y in sdkconfig.defaults.board for SD backends.
    - name: littlefs                          # Device name, must be unique
      type: littlefs                          # Device type, must be littlefs
      version: 1.0.0                          # Schema version
      sub_type: sdmmc                         # Backend subtype: flash, sdmmc, or spi
      config:
        vfs_config:
          base_path: "/littlefs"              # VFS mount point (default: /littlefs)
          format_if_mount_failed: false       # Format the SD card if mount fails (default: false; cannot be used with read_only)
          read_only: false                    # Mount as read-only (default: false; cannot be used with format_if_mount_failed or grow_on_mount)
          dont_mount: false                   # Register the filesystem without mounting (default: false)
          grow_on_mount: false                # Grow filesystem to match card size on mount (default: false; cannot be used with read_only)
          # partition_label is ignored for SD card backends (sdmmc/spi)
        sub_config:
          slot: SDMMC_HOST_SLOT_1             # SDMMC slot (default: SDMMC_HOST_SLOT_1):
                                              #   SDMMC_HOST_SLOT_0 / SDMMC_HOST_SLOT_1 / SDMMC_HOST_SLOT_2
          frequency: SDMMC_FREQ_HIGHSPEED     # SD clock frequency (default: SDMMC_FREQ_HIGHSPEED):
                                              #   SDMMC_FREQ_DEFAULT   (20 MHz)
                                              #   SDMMC_FREQ_HIGHSPEED (40 MHz)
                                              #   SDMMC_FREQ_PROBING   (400 kHz)
                                              #   SDMMC_FREQ_52M / SDMMC_FREQ_26M
                                              #   SDMMC_FREQ_DDR50 / SDMMC_FREQ_SDR50
          bus_width: 1                        # SDMMC bus width: 1, 4, or 8 bits (default: 1)
          slot_flags: SDMMC_SLOT_FLAG_INTERNAL_PULLUP  # Slot flags (default: SDMMC_SLOT_FLAG_INTERNAL_PULLUP):
                                              #   0 / SDMMC_SLOT_FLAG_INTERNAL_PULLUP /
                                              #   SDMMC_SLOT_FLAG_WP_ACTIVE_HIGH / SDMMC_SLOT_FLAG_UHS1
          pins:
            clk: -1     # [IO] Clock pin (-1 if unused)
            cmd: -1     # [IO] Command pin (-1 if unused)
            d0: -1      # [IO] Data line 0 pin (-1 if unused)
            d1: -1      # [IO] Data line 1 pin (-1 if unused)
            d2: -1      # [IO] Data line 2 pin (-1 if unused)
            d3: -1      # [IO] Data line 3 pin (-1 if unused)
            d4: -1      # [IO] Data line 4 pin, 8-bit mode only (-1 if unused)
            d5: -1      # [IO] Data line 5 pin, 8-bit mode only (-1 if unused)
            d6: -1      # [IO] Data line 6 pin, 8-bit mode only (-1 if unused)
            d7: -1      # [IO] Data line 7 pin, 8-bit mode only (-1 if unused)
            cd: -1      # [IO] Card-detect pin (-1 if unused)
            wp: -1      # [IO] Write-protect pin (-1 if unused)
          ldo_chan_id: -1                     # On-chip LDO channel ID for SDMMC IO power (-1 when unused)

SPI SD 卡完整字段
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example LittleFS device mounted from SDSPI card.
    # Enable CONFIG_LITTLEFS_SDMMC_SUPPORT=y in sdkconfig.defaults.board for SD backends.
    - name: littlefs                          # Device name, must be unique
      type: littlefs                          # Device type, must be littlefs
      version: 1.0.0                          # Schema version
      sub_type: spi                           # Backend subtype: flash, sdmmc, or spi
      config:
        vfs_config:
          base_path: "/littlefs"              # VFS mount point (default: /littlefs)
          format_if_mount_failed: false       # Format the SD card if mount fails (default: false; cannot be used with read_only)
          read_only: false                    # Mount as read-only (default: false; cannot be used with format_if_mount_failed or grow_on_mount)
          dont_mount: false                   # Register the filesystem without mounting (default: false)
          grow_on_mount: false                # Grow filesystem to match card size on mount (default: false; cannot be used with read_only)
        sub_config:
          cs_gpio_num: 15                     # [IO] SD card chip-select pin (default: 15)
          frequency: SDMMC_FREQ_DEFAULT       # SD clock frequency (default: SDMMC_FREQ_DEFAULT); same enum values as the SDMMC frequency field
      peripherals:
        - spi_name: spi_master                 # Referenced SPI master peripheral (type: spi, role: master)

``esp_vfs_littlefs_conf_t`` 中的 ``partition``、``sdcard`` 和 ``blockdev`` 是运行期挂载源字段，不能写入 YAML。BMGR 根据 ``sub_type`` 在初始化过程中设置相应挂载源。

组件依赖
------------

``littlefs`` 会通过 ``esp_board_manager/idf_component.yml`` 在启用 ``CONFIG_ESP_BOARD_DEV_LITTLEFS_SUPPORT`` 时引入 ``joltwallet/littlefs``，版本为 ``7b72caff1de089598a9b5b0b15a7226b790f3c96``。板级 YAML 不需要为该通用组件重复声明 ``dependencies``。

SDMMC 和 SDSPI 两种 SD 后端需要启用 ``CONFIG_LITTLEFS_SDMMC_SUPPORT``，否则 ``esp_vfs_littlefs_conf_t`` 中的 SD card 支持字段与相关 API 不会参与编译。

依赖外设
------------

.. list-table::
   :header-rows: 1

   * - ``sub_type``
     - peripheral type
     - 必选性
     - 用途
   * - ``flash``
     - 无
     - 不需要 BMGR peripheral
     - 挂载 flash 分区
   * - ``sdmmc``
     - 无
     - 不需要 BMGR peripheral
     - 设备初始化时创建 SDMMC host 与 SD 卡句柄
   * - ``spi``
     - ``spi``
     - 必选
     - 复用 SPI master 外设初始化 SDSPI SD 卡

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_littlefs.c``
- ``esp_board_manager/devices/dev_littlefs/dev_littlefs.c``
- ``esp_board_manager/devices/dev_littlefs/dev_littlefs_sub_flash.c``
- ``esp_board_manager/devices/dev_littlefs/dev_littlefs_sub_sdmmc.c``
- ``esp_board_manager/devices/dev_littlefs/dev_littlefs_sub_spi.c``

板级参考
------------

- ``esp_board_manager/test_apps/components/test_board_littlefs_flash/board_devices.yaml``：flash 分区挂载测试配置。
- ``esp_board_manager/test_apps/partitions_test_app.csv``：测试应用中的 ``littlefs`` 分区配置。

注意事项
------------

- ``partition_label`` 表示分区表中的分区名称，可改为其他名称；修改后需要同步修改分区表。使用 ``littlefs_create_partition_image`` 生成镜像时，还需要同步修改镜像生成目标对应的分区名称。
- ``flash`` 模式未显式配置 ``partition_label`` 时，BMGR 默认使用 ``storage`` 作为分区名称。分区表中必须存在同名 ``data`` 分区。
- 分区子类型建议使用 ``littlefs``。上游组件在未指定挂载源时会查找第一个 ``data,littlefs`` 分区；BMGR 当前 flash 模式会生成 ``partition_label``，因此实际挂载按分区名称查找。
- SDMMC 和 SDSPI 模式会在 SD 卡上挂载 LittleFS，不使用 flash 分区表中的 LittleFS 分区。
- ``format_if_mount_failed: true`` 会在挂载失败时格式化目标文件系统。SD 卡模式下该操作会清除卡上原有文件系统内容。
- 修改 YAML 后需要重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

- SD 卡模式初始化成功后，可使用 ``sdmmc_card_print_info(stdout, handle->card)`` 打印 SD 卡信息。
- SD 卡首次挂载已有 FAT 或未格式化介质时，LittleFS 挂载失败属于预期现象。启用 ``format_if_mount_failed`` 后，组件会格式化 SD 卡。
- SDSPI 模式需检查 SPI master 外设名称、CS GPIO、电源控制和与其他 SPI 设备的总线占用关系。

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，句柄类型为 ``dev_littlefs_handle_t``：

.. code-block:: c

   typedef struct {
       sdmmc_card_t *card;
       sdmmc_host_t  host;
       char         *mount_point;
   } dev_littlefs_handle_t;

``mount_point`` 是 VFS 挂载路径。``card`` 和 ``host`` 仅在 ``sdmmc`` 与 ``spi`` 模式下有效，应用可用于 SD 卡诊断。

相关声明位于 ``esp_board_manager/devices/dev_littlefs/dev_littlefs.h``。
