# 播放嵌入式 WAV 音乐

- [English Version](./README.md)
- 例程难度：⭐

## 例程简介

- 本例程从固件内置 Flash 中的 WAV 数据播放音频，无需 microSD 卡或网络。
- 技术上演示通过 `esp_board_manager` 初始化音频 DAC、获取 `dev_audio_codec_handles_t`，并使用 `esp_codec_dev`（open、音量、write）播放；音频通过 CMake `EMBED_TXTFILES` 嵌入镜像。

### 典型场景

- 开机提示音、小体积演示音频等不依赖外置存储的场景。

### 预备知识

音频文件通过 ESP-IDF [嵌入式二进制](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/api-guides/build-system.html#cmake-embed-data)随固件编译。将 WAV 放在 `main/audio_files/`（默认 `test.wav`），并在 `main/CMakeLists.txt` 的 `EMBED_TXTFILES` 中注册。C 侧符号为 `_binary_<名称>_start` / `_binary_<名称>_end`（名称由路径规则生成）。

### 运行机制

`app_main` 经板级管理初始化 DAC，根据 Flash 中 WAV 头解析格式并打开 codec，将采样数据通过 `esp_codec_dev_write` 写入 ，最后释放设备。

### 文件结构

```
├── main
│   ├── audio_files              放置 test.wav 等
│   ├── CMakeLists.txt           EMBED_TXTFILES 嵌入 WAV
│   ├── idf_component.yml
│   └── play_embed_music.c
├── CMakeLists.txt               工程名：bmgr_play_embed_music
├── partitions.csv
├── sdkconfig.defaults
├── sdkconfig.defaults.esp32
├── sdkconfig.defaults.esp32s3
├── sdkconfig.defaults.esp32p4
├── README.md
└── README_CN.md
```

## 环境配置

### 默认 IDF 分支

本例程支持 IDF release/v5.4 (>= v5.4.3) 与 release/v5.5 (>= v5.5.2) 分支。

### 硬件要求

- 连接扬声器

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

- 进入本例程目录：

```shell
cd $YOUR_GMF_PATH/packages/esp_board_manager/examples/play_embed_music
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

- 烧录后设备自动播放嵌入的 `test.wav` 一遍，随后 `app_main` 结束。

### 日志输出

```text
I (918) BMGR_EMBED_MUSIC: Playing embedded music
I (923) PERIPH_I2S: I2S[0] TDM,  TX, ws: 45, bclk: 9, dout: 8, din: 10
I (929) PERIPH_I2S: I2S[0] initialize success: 0x3c1629b0
I (934) DEV_AUDIO_CODEC: DAC is ENABLED
I (938) DEV_AUDIO_CODEC: Init audio_dac, i2s_name: i2s_audio_out, i2s_rx_handle:0x0, i2s_tx_handle:0x3c1629b0, data_if: 0x3fcea7f4
I (950) PERIPH_I2C: I2C master bus initialized successfully
I (960) ES8311: Work in Slave mode
I (963) DEV_AUDIO_CODEC: Successfully initialized codec: audio_dac
I (964) DEV_AUDIO_CODEC: Create esp_codec_dev success, dev:0x3fceaa48, chip:es8311
I (971) BOARD_MANAGER: Device audio_dac initialized
I (976) BOARD_DEVICE: Device handle audio_dac found, Handle: 0x3fce9a7c TO: 0x3fce9a7c
I (983) BMGR_EMBED_MUSIC: Embedded WAV file size: 818920 bytes
I (989) BMGR_EMBED_MUSIC: WAV file info: 48000 Hz, 2 channels, 16 bits
I (996) I2S_IF: channel mode 2 bits:16/16 channel:2 mask:3
I (1002) I2S_IF: TDM Mode 1 bits:16/16 channel:2 sample_rate:48000 mask:3
I (1023) Adev_Codec: Open codec device OK
I (5273) BMGR_EMBED_MUSIC: Embedded WAV file playback completed
I (5273) BOARD_DEVICE: Deinit device audio_dac ref_count: 0 device_handle:0x3fce9a7c
I (5286) BOARD_DEVICE: Device audio_dac config found: 0x3c12f0e4 (size: 92)
I (5286) BOARD_PERIPH: Deinit peripheral i2s_audio_out ref_count: 0
E (5288) i2s_common: i2s_channel_disable(1217): the channel has not been enabled yet
W (5296) PERIPH_I2S: Caution: Releasing TX (0x0).
W (5300) PERIPH_I2S: Caution: RX (0x3c162b64) forced to stop.
E (5306) i2s_common: i2s_channel_disable(1217): the channel has not been enabled yet
I (5313) BOARD_PERIPH: Deinit peripheral i2c_master ref_count: 0
I (5319) PERIPH_I2C: I2C master bus deinitialized successfully
I (5324) BOARD_MANAGER: Device audio_dac deinitialized
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

### 嵌入式 WAV 缺失或编译失败

确认 `main/audio_files/test.wav` 存在，且 `main/CMakeLists.txt` 中 `EMBED_TXTFILES` 已包含该文件。
