摄像头 camera
=============

:link_to_translation:`en:[English]`

简介
------

``camera`` 设备用于描述摄像头传感器及其数据接口，初始化后返回 ``dev_camera_handle_t``。该句柄保存视频设备路径，应用可通过 V4L2 路径采集图像数据。

该类型按 ``sub_type`` 选择摄像头接口。当前设备模板覆盖 ``dvp``、``csi`` 与 ``spi``；头文件中保留 ``usb_uvc`` 占位结构，但设备模板未给出 USB-UVC 的板级 YAML。

支持的使用模式
---------------------

``camera`` 以 ``sub_type`` 作为分类轴：

- :ref:`camera-dvp`
- :ref:`camera-csi`
- :ref:`camera-spi`

最小配置
------------

.. _camera-dvp:

DVP（``sub_type: dvp``）
^^^^^^^^^^^^^^^^^^^^^^^^^^

``dvp`` 模式使用 LCD_CAM DVP 控制器与并行数据线。``board_peripherals.yaml`` 至少需要可被摄像头 SCCB 控制接口引用的 ``i2c`` 外设。

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: camera
        type: camera
        sub_type: dvp
        config:
          dvp_config:
            reset_io: -1                 # [IO]
            pwdn_io: -1                  # [IO]
            vsync_io: 21                 # [IO]
            data_width: "CAM_CTLR_DATA_WIDTH_8"
            de_io: 38                    # [TO_BE_CONFIRMED]
            pclk_io: 11                  # [TO_BE_CONFIRMED]
            xclk_io: 40                  # [TO_BE_CONFIRMED]
            xclk_freq: 10000000          # [TO_BE_CONFIRMED]
            data_io:
              data_io_0: 13              # [IO]
              data_io_1: 47              # [IO]
              data_io_2: 14              # [IO]
              data_io_3: 3               # [IO]
              data_io_4: 12              # [IO]
              data_io_5: 42              # [IO]
              data_io_6: 41              # [IO]
              data_io_7: 39              # [IO]
        peripherals:
          - i2c_name: i2c_master
            frequency: 100000

.. _camera-csi:

CSI（``sub_type: csi``）
^^^^^^^^^^^^^^^^^^^^^^^^^^

``csi`` 模式使用 MIPI CSI 路径。``board_peripherals.yaml`` 至少需要 ``i2c`` 外设；当 ``dont_init_ldo: true`` 且由 BMGR 管理 MIPI LDO 时，还需配置 ``ldo`` 外设。

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: camera
        type: camera
        sub_type: csi
        config:
          csi_config:
            reset_io: 1                  # [IO]
            pwdn_io: 2                   # [IO]
            dont_init_ldo: true          # [TO_BE_CONFIRMED]
            xclk_config:
              xclk_pin: -1               # [IO]
              xclk_freq_hz: 20000000     # [TO_BE_CONFIRMED]
        peripherals:
          - i2c_name: i2c_master
            frequency: 100000
          - ldo_name: ldo_mipi

.. _camera-spi:

SPI 相机（``sub_type: spi``）
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``spi`` 模式使用 ``esp_video_init_spi_config_t`` 描述连接，初始化时由 ``i2c`` 外设提供 SCCB 句柄。``board_peripherals.yaml`` 至少需要可被 SCCB 控制接口引用的 ``i2c`` 外设；SPI 相机的数据接口 GPIO 写在设备 ``spi_config`` 中，不引用 ``spi`` 外设。``camera`` 初始化按 ``sub_type`` 查找子设备入口，``spi`` 模式在内部映射到 ``camera_spi`` 子入口以避免与其他设备的 ``spi`` 子入口重名。

``board_devices.yaml``：

.. code-block:: yaml

    devices:
      - name: camera
        type: camera
        sub_type: spi
        config:
          spi_config:
            intf: ESP_CAM_CTLR_SPI_CAM_INTF_SPI
            io_mode: ESP_CAM_CTLR_SPI_CAM_IO_MODE_1BIT
            spi_port: 1                  # [TO_BE_CONFIRMED]
            spi_cs_pin: 10               # [IO]
            spi_sclk_pin: 11             # [IO]
            spi_data0_io_pin: 12         # [IO]
            spi_data1_io_pin: -1         # [IO]
            reset_pin: -1                # [IO]
            pwdn_pin: -1                 # [IO]
            xclk_source: ESP_CAM_SENSOR_XCLK_LEDC
            xclk_freq: 20000000          # [TO_BE_CONFIRMED]
            xclk_pin: 13                 # [IO]
            xclk_ledc_cfg:
              timer: 0                   # [TO_BE_CONFIRMED]
              clk_cfg: LEDC_AUTO_CLK     # [TO_BE_CONFIRMED]
              channel: 0                 # [TO_BE_CONFIRMED]
        peripherals:
          - i2c_name: i2c_master
            frequency: 100000

完整字段
------------

DVP 完整字段
^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example board_devices.yaml configuration for camera device
    # This shows how to integrate the camera device into a board configuration
    # Please note that multiple cameras are not currently supported on the same board
    # All values shown are example values for DVP and CSI interface configuration

    # Example camera device configuration
    - name: camera         # The name of the device, must be unique
      type: camera         # The type of the camera, must be equal to "camera"
      sub_type: dvp        # The bus type: dvp
      config:
        # DVP interface configuration (used when sub_type is "dvp")
        dvp_config:
          reset_io: -1        # [IO] GPIO pin for reset signal (default: -1, not connected)
          pwdn_io: -1         # [IO] GPIO pin for power down signal (default: -1, not connected)
          vsync_io: 21        # [IO] GPIO pin for vertical sync signal (default: depends on implementation)
          data_width: "CAM_CTLR_DATA_WIDTH_8"  # [TO_BE_CONFIRMED] Data width configuration:
                                               # CAM_CTLR_DATA_WIDTH_8
                                               # CAM_CTLR_DATA_WIDTH_10
                                               # CAM_CTLR_DATA_WIDTH_12
                                               # CAM_CTLR_DATA_WIDTH_16
          de_io: 38            # [TO_BE_CONFIRMED] GPIO pin for data enable signal (default: depends on implementation)
          pclk_io: 11          # [TO_BE_CONFIRMED] GPIO pin for pixel clock signal (default: depends on implementation)
          xclk_io: 40          # [TO_BE_CONFIRMED] GPIO pin for external clock output (default: depends on implementation)
          xclk_freq: 10000000  # [TO_BE_CONFIRMED] External clock frequency in Hz (default: 10MHz)
          data_io:
            data_io_0: 13      # [IO] GPIO pin for data bit 0 (default: depends on implementation)
            data_io_1: 47      # [IO] GPIO pin for data bit 1 (default: depends on implementation)
            data_io_2: 14      # [IO] GPIO pin for data bit 2 (default: depends on implementation)
            data_io_3: 3       # [IO] GPIO pin for data bit 3 (default: depends on implementation)
            data_io_4: 12      # [IO] GPIO pin for data bit 4 (default: depends on implementation)
            data_io_5: 42      # [IO] GPIO pin for data bit 5 (default: depends on implementation)
            data_io_6: 41      # [IO] GPIO pin for data bit 6 (default: depends on implementation)
            data_io_7: 39      # [IO] GPIO pin for data bit 7 (default: depends on implementation)
            data_io_8: -1      # [IO] GPIO pin for data bit 8 (default: -1, not used in 8-bit mode)
            data_io_9: -1      # [IO] GPIO pin for data bit 9 (default: -1, not used in 8-bit mode)
            data_io_10: -1     # [IO] GPIO pin for data bit 10 (default: -1, not used in 8-bit mode)
            data_io_11: -1     # [IO] GPIO pin for data bit 11 (default: -1, not used in 8-bit mode)
            data_io_12: -1     # [IO] GPIO pin for data bit 12 (default: -1, not used in 8-bit mode)
            data_io_13: -1     # [IO] GPIO pin for data bit 13 (default: -1, not used in 8-bit mode)
            data_io_14: -1     # [IO] GPIO pin for data bit 14 (default: -1, not used in 8-bit mode)
            data_io_15: -1     # [IO] GPIO pin for data bit 15 (default: -1, not used in 8-bit mode)

      # I2C configuration for camera control (For the USB-UVC camera, this configuration is not required)
      peripherals:
        - i2c_name: i2c_master # [TO_BE_CONFIRMED] I2C bus name for camera control (default: depends on implementation)
          frequency: 100000    # I2C frequency in Hz (default: 400kHz)

CSI 完整字段
^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example CSI camera configuration
    - name: camera             # The name of the device, must be unique
      type: camera             # The type of the camera, must be equal to "camera"
      sub_type: "csi"          # The bus type: csi
      config:
        # CSI interface configuration (used when sub_type is "csi")
        csi_config:
          reset_io: 1           # [IO] GPIO pin for reset signal (example value)
          pwdn_io: 2            # [IO] GPIO pin for power down signal (example value)
          dont_init_ldo: true   # [TO_BE_CONFIRMED] If true, MIPI-CSI video device will not initialize the LDO (default: true)
                                # If using the periph_ldo in esp_board_manager to manage the ldo peripheral, it needs to be set to true
          xclk_config:
            xclk_pin: -1        # [IO] GPIO pin for camera XCLK signal (default: -1, not connected)
            xclk_freq_hz: 20000000 # [TO_BE_CONFIRMED] XCLK frequency in Hz (default: 20MHz)

      # I2C configuration for camera control
      peripherals:
        - i2c_name: i2c_master  # [TO_BE_CONFIRMED] I2C bus name for camera control
          frequency: 100000     # I2C frequency in Hz
        - ldo_name: ldo_mipi    # [TO_BE_CONFIRMED] LDO peripheral for csi power management

SPI 相机完整字段
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: yaml

    # Example SPI camera configuration
    # CONFIG_CAMERA_XCLK_USE_LEDC: required when xclk_source is ESP_CAM_SENSOR_XCLK_LEDC (xclk_ledc_cfg used).
    - name: camera
      type: camera
      sub_type: spi
      config:
        spi_config:
          # esp_video_init_spi_config_t (esp_video_init.h), excluding sccb_config (filled at init from periph_i2c).
          # SPI CAM interface type: ESP_CAM_CTLR_SPI_CAM_INTF_SPI or ESP_CAM_CTLR_SPI_CAM_INTF_PARLIO
          intf: ESP_CAM_CTLR_SPI_CAM_INTF_SPI
          # SPI CAM data I/O mode (SPI interface only supports 1-bit): ESP_CAM_CTLR_SPI_CAM_IO_MODE_1BIT / _2BIT
          io_mode: ESP_CAM_CTLR_SPI_CAM_IO_MODE_1BIT
          spi_port: 1                         # [TO_BE_CONFIRMED] SPI port
          spi_cs_pin: 10                      # [IO] SPI CS pin
          spi_sclk_pin: 11                    # [IO] SPI SCLK pin
          spi_data0_io_pin: 12                # [IO] SPI data0 I/O pin
          # SPI data1 I/O pin (only required when io_mode is ESP_CAM_CTLR_SPI_CAM_IO_MODE_2BIT, set to -1 if not used)
          spi_data1_io_pin: -1                # [IO] SPI data1 I/O pin
          # SPI interface camera sensor reset pin, if hardware has no reset pin, set reset_pin to be -1
          reset_pin: -1                       # [IO] SPI interface camera sensor reset pin
          # SPI interface camera sensor power down pin, if hardware has no power down pin, set pwdn_pin to be -1
          pwdn_pin: -1                       # [IO] SPI interface camera sensor power down pin
          # Output clock resource / frequency / pin for SPI interface camera sensor
          xclk_source: ESP_CAM_SENSOR_XCLK_LEDC
          xclk_freq: 20000000                # [TO_BE_CONFIRMED] XCLK frequency in Hz (default: 20MHz)
          xclk_pin: 13                       # [IO] XCLK pin
          # Used when xclk_source is ESP_CAM_SENSOR_XCLK_LEDC (ledc_timer_t / ledc_clk_cfg_t / ledc_channel_t)
          xclk_ledc_cfg:
            timer: 0                          # [TO_BE_CONFIRMED] The timer source of channel
            clk_cfg: LEDC_AUTO_CLK            # [TO_BE_CONFIRMED] LEDC source clock from ledc_clk_cfg_t
            channel: 0                        # [TO_BE_CONFIRMED] LEDC channel used for XCLK
      peripherals:
        - i2c_name: i2c_master               # Must match a periph_i2c name; provides SCCB i2c_handle to esp_video
          frequency: 100000                   # SCCB I2C frequency (Hz)

组件依赖
------------

``dev_camera.yaml`` 未在设备条目中声明 ``dependencies``。摄像头子类型依赖 ESP-IDF 与 ``esp_video`` 相关的头文件、驱动及目标芯片能力，板级无需在该设备 YAML 中额外补写不存在的依赖字段。

依赖外设
------------

.. list-table::
   :header-rows: 1

   * - peripheral type
     - role / format
     - 必选性
     - 用途
   * - ``i2c``
     - ``master``
     - ``dvp``、``csi`` 和 ``spi`` 模式需要
     - 摄像头 SCCB 控制接口
   * - ``ldo``
     - 无
     - ``csi`` 使用 BMGR 管理 MIPI LDO 时需要
     - MIPI CSI 供电管理

参考代码
------------

- ``esp_board_manager/test_apps/main/test_dev_camera.c``：获取 ``camera`` 设备句柄并通过 V4L2 设备路径采集图像。
- ``esp_board_manager/devices/dev_camera/dev_camera_sub_dvp.c``：DVP 子类型初始化实现。
- ``esp_board_manager/devices/dev_camera/dev_camera_sub_csi.c``：CSI 子类型初始化实现。
- ``esp_board_manager/devices/dev_camera/dev_camera_sub_spi.c``：SPI 子类型初始化实现。

板级参考
------------

- ``esp_boards/esp32_s31_korvo_1/board_devices.yaml``：``dvp`` 摄像头配置。
- ``esp_boards/esp32_s3_korvo_2_3/board_devices.yaml``：``dvp`` 摄像头配置。
- ``esp_boards/esp32_p4_function_ev_board/board_devices.yaml``：``csi`` 摄像头配置。
- ``esp_boards/esp32_p4_function_ev_board/board_peripherals.yaml``：``csi`` 摄像头使用的 ``i2c`` 和 ``ldo`` 外设。
- ``m5stack_boards/m5stack_tab5/board_devices.yaml``：摄像头配置。

注意事项
------------

- 当前设备模板注释说明同一块开发板暂不支持配置多个摄像头。
- ``i2c`` 外设仅提供 SCCB 控制句柄；DVP、CSI 或 SPI 数据线在 ``camera`` 设备配置中声明。
- ``spi`` 模式使用 ``ESP_CAM_SENSOR_XCLK_LEDC`` 时，模板要求启用 ``CONFIG_CAMERA_XCLK_USE_LEDC``。
- 修改摄像头设备、I2C 外设或 LDO 外设配置后，需重新执行 ``idf.py bmgr -b <board>``。

调试技巧
------------

API 参考
----------

使用 :cpp:func:`esp_board_manager_get_device_handle` 获取设备句柄，句柄类型为 ``dev_camera_handle_t``：

.. code-block:: c

   typedef struct {
       const char *dev_path;   /*!< Camera device path or identifier */
       const char *meta_path;  /*!< Camera metadata path or identifier (if applicable) */
                               /*!< For CSI camera, meta_path is ISP path */
   } dev_camera_handle_t;

``dev_path`` 和 ``meta_path`` 是 esp_video 框架注册的设备路径（如 ``/dev/video0``），通过 ``open()`` 等 POSIX 接口访问。

相关声明位于 ``esp_board_manager/devices/dev_camera/dev_camera.h``。
