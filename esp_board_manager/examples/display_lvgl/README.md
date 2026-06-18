# Display LVGL Example

- [中文版](./README_CN.md)

## Example Brief

This example demonstrates how to use the `esp_board_manager` with LVGL on supported development boards. It initializes the board manager, sets up the LVGL adapter, adds the LCD display and touch input (if available), and runs the LVGL test UI.

## Environment Setup

### Default IDF Branch

This example supports IDF release/v5.5 (>=5.5.2) and IDF release/v5.4 (>=5.4.3) branches.

### Hardware Required

- LCD
- Optional：LCD Touch，LEDC brightness ctrl

## Build and Flash

### Build Preparation

Before compiling this example, ensure that the ESP-IDF environment is properly set up. If not, run the following script in the root directory of ESP-IDF to set up the build environment. For detailed steps on configuring and using ESP-IDF, please refer to the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/index.html):

```shell
./install.sh
. ./export.sh
```

Install `esp-bmgr-assist` in the activated ESP-IDF Python environment:

```shell
pip install esp-bmgr-assist
```

- Navigate to the test project directory for driving display with LVGL:

```shell
cd $YOUR_GMF_PATH/packages/esp_board_manager/examples/display_lvgl
```

- List visible boards:

```shell
idf.py bmgr -l
```

- Select the development board to use:

```shell
idf.py bmgr -b esp32_s3_korvo2_v3
```

### Build and Flash Commands

Compile the example code:

```shell
idf.py build
```

Flash the program and run the monitor tool to view serial output (replace PORT with your port name):

```shell
idf.py -p PORT flash monitor
```

To exit the debugging interface, use `Ctrl-]`.

## How to Use the Example

### Functionality and Usage

- After the example starts, it will automatically initialize the display and run the LVGL test UI. The output will be as follows:

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

## Troubleshooting

### `idf.py bmgr` command not found

- Make sure `esp-bmgr-assist` is installed in the current ESP-IDF Python environment.
- Make sure `main/idf_component.yml` contains the `esp_board_manager` dependency.
- If using the legacy entry point, make sure `IDF_EXTRA_ACTIONS_PATH` points to `esp_board_manager`.

```shell
# Linux / macOS:
echo $IDF_EXTRA_ACTIONS_PATH

# Windows PowerShell:
echo $env:IDF_EXTRA_ACTIONS_PATH

# Windows CMD:
echo %IDF_EXTRA_ACTIONS_PATH%
```

### Custom Board

If you need to use a custom development board, refer to the instructions on **custom boards** in [README.md](../../../README.md).
