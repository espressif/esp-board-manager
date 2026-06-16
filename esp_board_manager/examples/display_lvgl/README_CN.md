# 驱动屏幕显示 LVGL

- [English](./README.md)

## 例程简介

本例程介绍了如何使用 `esp_board_manager` 初始化屏幕，获取设备的句柄，并结合 `esp_lvgl_adapter` 组件在开发板上运行 LVGL 测试 UI 的例子。

## 环境配置

### 默认 IDF 分支

本例程仅支持 IDF release/v5.5 (>=5.5.2) 及 IDF release/v5.4 (>=5.4.3) 分支。

### 硬件要求

- LCD
- 可选：LCD Touch，LEDC 背光控制

## 编译和下载

### 编译准备

编译本例程前需要先确保已配置 ESP-IDF ，如果已配置可跳过，未配置需要先在 ESP-IDF 根目录运行下面脚本设置编译环境，有关配置和使用 ESP-IDF 完整步骤，请参阅 [《ESP-IDF 编程指南》](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/index.html)：

```shell
./install.sh
. ./export.sh
```

在已激活的 ESP-IDF Python 环境中安装 `esp-bmgr-assist`：

```shell
pip install esp-bmgr-assist
```

- 进入基于 LVGL 驱动屏幕的测试工程存放位置

```shell
cd $YOUR_GMF_PATH/packages/esp_board_manager/examples/display_lvgl
```

- 列出当前可见的开发板

```shell
idf.py bmgr -l
```

- 选择使用的开发板

```shell
idf.py bmgr -b esp32_s3_korvo2_v3
```

### 编译与烧录

编译例程代码：

```shell
idf.py build
```

烧录程序并运行 monitor 工具来查看串口输出 (替换 PORT 为端口名称)：

```shell
idf.py -p PORT flash monitor
```

退出调试界面使用 `Ctrl-]`。

## 如何使用例程

### 功能和用法

- 例程开始运行后，将自动初始化显示屏并在屏幕上运行 LVGL 测试 UI（包含多个维度的压力测试等）。输出日志如下：

```c
I (829) BMGR_DISPLAY_LVGL: Starting LVGL Example
...
I (889) DEV_DISPLAY_LCD: Initializing LCD display: display_lcd, chip: st7789, sub_type: i80
I (899) DEV_DISPLAY_LCD_SUB_I80: Initializing I80 LCD display: display_lcd, chip: st7789
I (1029) DEV_DISPLAY_LCD: Successfully initialized LCD display: display_lcd (sub_type: i80), panel: 0x3fce9f40, io: 0x3c0b09e8
I (1029) BOARD_MANAGER: Device display_lcd initialized
...
I (1049) BMGR_DISPLAY_LVGL: Initializing LVGL adapter...
I (1059) LVGL: Starting LVGL task
I (1059) BOARD_DEVICE: Device handle display_lcd found, Handle: 0x3fce9b14 TO: 0x3fce9b14
I (1069) BOARD_DEVICE: Device display_lcd config found: 0x3c0a4ba0 (size: 184)
I (1079) BMGR_DISPLAY_LVGL: Running LVGL Test UI
I (1079) lvgl_test_ui: Starting LCD LVGL test sequence
I (1089) lvgl_test_ui: Testing Magenta screen
I (1109) lvgl_test_ui: cleanup_ui_elements done
I (4219) lvgl_test_ui: cleanup_ui_elements done
I (4719) lvgl_test_ui: Testing Cyan screen
I (4719) lvgl_test_ui: cleanup_ui_elements done
I (7819) lvgl_test_ui: cleanup_ui_elements done
I (8319) lvgl_test_ui: Testing Blue screen
I (8319) lvgl_test_ui: cleanup_ui_elements done
I (11419) lvgl_test_ui: cleanup_ui_elements done
I (11919) lvgl_test_ui: Testing White screen
I (11919) lvgl_test_ui: cleanup_ui_elements done
I (15019) lvgl_test_ui: cleanup_ui_elements done
I (15519) lvgl_test_ui: show_test_results start
I (15519) lvgl_test_ui: show_test_results UI update done
I (18519) lvgl_test_ui: show_test_results done
I (18519) lvgl_test_ui: Test sequence completed. Result: FAIL.
I (18519) BMGR_DISPLAY_LVGL: Example Finished. Exiting app_main...
...
I (18629) main_task: Returned from app_main()
```

## 故障排除

### `idf.py bmgr` 命令未找到

- 确认已在当前 ESP-IDF Python 环境中安装 `esp-bmgr-assist`。
- 确认工程 `main/idf_component.yml` 中已包含 `esp_board_manager` 依赖。
- 如果使用旧入口，请确认 `IDF_EXTRA_ACTIONS_PATH` 指向 `esp_board_manager`。

```shell
# Linux / macOS:
echo $IDF_EXTRA_ACTIONS_PATH

# Windows PowerShell:
echo $env:IDF_EXTRA_ACTIONS_PATH

# Windows CMD:
echo %IDF_EXTRA_ACTIONS_PATH%
```

### 自定义开发板

如果需要使用自定义的开发板，请参考 [README_CN.md](../../../README_CN.md) 中关于 **自定义板级** 的说明。
