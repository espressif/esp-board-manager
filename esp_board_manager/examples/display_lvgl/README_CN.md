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

本示例使用 [ESP Board Manager](https://github.com/espressif/esp-board-manager) 管理板级资源。推荐安装辅助工具 [`esp-bmgr-assist`](https://pypi.org/project/esp-bmgr-assist/) 作为默认入口。

在已激活的 ESP-IDF Python 环境下安装（同一环境只需安装一次）：

```bash
pip install esp-bmgr-assist
pip install --upgrade esp-bmgr-assist  # 当提示需要更新时执行此命令
```

- 进入基于 LVGL 驱动屏幕的测试工程存放位置

```shell
cd $YOUR_GMF_PATH/packages/esp_board_manager/examples/display_lvgl
```

### 编译与烧录

- 列出当前可见的开发板

```bash
idf.py bmgr -l
```

输出示例：

```text
ℹ️  Board Components:
  espressif/esp_boards:
    [1] esp32_c3_lyra
    [2] esp32_lyrat_4_3
    [3] esp32_lyrat_mini_1_1
    [4] esp32_p4_eye
    [5] esp32_p4_function_ev_board
    [6] esp32_s31_function_coreboard_1
    [7] esp32_s31_korvo_1
    [8] esp32_s3_box_3
    [9] esp32_s3_box_lite
    [10] esp32_s3_korvo_2_3
    [11] esp32_s3_lcd_ev_board
    [12] esp_vocat_1_0
    [13] esp_vocat_1_2
```

以上输出示例基于 `esp_boards` 0.5.2 的开发板列表和排序。不同 `esp_boards` 版本或自定义开发板依赖可能会使列表和序号变化，使用时以 `idf.py bmgr -l` 的实际输出为准。

- 选择开发板：

```bash
idf.py bmgr -b <board_index|board_name>
```

例如选择 `esp32_s3_korvo_2_3`：

```bash
idf.py bmgr -b 10
# 或
idf.py bmgr -b esp32_s3_korvo_2_3
```

首次执行 `idf.py bmgr` 时，组件会根据本工程 `main/idf_component.yml` 中声明的 `espressif/esp_board_manager` 依赖自动下载。

> [!NOTE]
> 如果切换为其他 `esp_board_manager` 支持的开发板，请按相同步骤执行并替换板型名称/索引。
> 自定义开发板请参考 [创建开发板指南](https://docs.espressif.com/projects/esp-board-manager/zh_CN/latest/create-board/index.html)。
> `esp_board_manager` 更多信息请参考 [ESP_BOARD_MANAGER 入门指南](https://github.com/espressif/esp-board-manager/blob/main/esp_board_manager/README_CN.md)

- 编译例程代码

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
