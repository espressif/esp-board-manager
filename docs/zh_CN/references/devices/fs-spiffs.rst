SPIFFS 文件系统 fs_spiffs
=========================

:link_to_translation:`en:[English]`

简介
------

``fs_spiffs`` 设备用于将 SPIFFS 分区挂载到 VFS 路径。初始化后，应用通过标准 C 文件接口访问 ``base_path`` 下的文件。

该设备不对应外接硬件，也不依赖 ``board_peripherals.yaml``。其前提是工程分区表中存在 SPIFFS 分区，``partition_label`` 用于选择分区。

支持的使用模式
---------------------

``fs_spiffs`` 只有一个使用模式：

- `SPIFFS 分区挂载`_

最小配置
------------

SPIFFS 分区挂载
^^^^^^^^^^^^^^^^^^^

无需新增 ``board_peripherals.yaml`` 条目。

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: fs_spiffs
        type: fs_spiffs
        config:
          base_path: "/spiffs"
          partition_label: NULL

``fs_spiffs`` 会使用 ``esp_vfs_spiffs_register()`` 挂载文件系统，并使用 ``esp_spiffs_info()`` 输出分区容量信息。设备句柄保存的是运行时分配的 ``esp_vfs_spiffs_conf_t``，卸载时用于调用 ``esp_vfs_spiffs_unregister()``。

完整字段
------------

SPIFFS 分区挂载完整字段
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example SPIFFS filesystem device configuration
    - name: fs_spiffs          # The name of the device, must be unique
      type: fs_spiffs          # The type of the device, must be unique
      version: 1.0.0
      config:
        base_path: "/spiffs"            # The base path for mounting the SPIFFS filesystem
        partition_label: NULL           # [TO_BE_CONFIRMED] The partition label for the SPIFFS filesystem
        max_files: 5                    # The maximum number of files that can be open simultaneously
        format_if_mount_failed: false   # Format the card if mount fails

组件依赖
------------

``fs_spiffs`` 使用 ESP-IDF 内置 ``spiffs`` 和 ``esp_spiffs`` 接口。当前设备模板未要求在 ``board_devices.yaml`` 中为该设备额外声明 ``dependencies``。

依赖外设
------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - 必选性
     - 用途
   * - 无
     - 无
     - 不需要
     - 使用工程分区表中的 SPIFFS 分区

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_fs_spiffs.c``
- ``esp_board_manager/devices/dev_fs_spiffs/dev_fs_spiffs.c``

板级参考
------------

- ``esp_friends_boards/esp32_s3_korvo_2l/board_devices.yaml``

注意事项
------------

- 公共 YAML 字段规则见 :doc:`/programming-guide/yaml-rules`。
- 工程分区表中必须存在与 ``partition_label`` 匹配的 SPIFFS 分区；``partition_label: NULL`` 表示使用默认 SPIFFS 分区。
- ``format_if_mount_failed: true`` 会在挂载失败时格式化文件系统，应结合数据保留需求设置。
- 修改 YAML 后需要重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
----------

``fs_spiffs`` 作为虚拟文件系统挂载，:cpp:func:`esp_board_manager_get_device_handle` 对该类型返回 ``NULL``；应用通过 POSIX 文件 API 访问 YAML 中配置的 ``base_path`` 挂载点。

设备配置结构 ``dev_fs_spiffs_config_t`` 及初始化函数见 ``esp_board_manager/devices/dev_fs_spiffs/dev_fs_spiffs.h``。
