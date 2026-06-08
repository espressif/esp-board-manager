# 从 microSD 卡播放音乐

- [English Version](./README.md)
- 例程难度：⭐

## 例程简介

- 从 microSD 卡根目录播放 WAV 文件（默认 `/sdcard/test.wav`）。
- 技术上演示 `esp_board_manager` 初始化 Audio DAC 与 FAT/SD 卡（`fs_sdcard`），配合 `common/wav_header` 解析 WAV 头，并使用 `esp_codec_dev` 流式播放。

### 典型场景

- 本地音乐播放、外置存储上的提示音或音频资源演示。

### 运行机制

挂载 SD → 打开 WAV → `read_wav_header` → `esp_codec_dev_open` / 循环 `write` → 卸载并释放设备。

### 文件结构

```
├── common
│   ├── CMakeLists.txt
│   ├── wav_header.c
│   └── include/wav_header.h
├── main
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   └── play_sdcard_music.c
├── CMakeLists.txt               工程：bmgr_play_sdcard_music（EXTRA_COMPONENT_DIRS ../common）
├── partitions.csv
├── sdkconfig.defaults           含 FatFs / 长文件名等
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

- 具备 SD 卡的开发板，将 `test.wav` 置于卡根目录。

### 软件要求

- SD 卡根目录下的 `test.wav` 文件。

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
cd $YOUR_GMF_PATH/packages/esp_board_manager/examples/play_sdcard_music
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

- 插入已放好 `test.wav` 的 SD 卡上电，固件自动播放该文件一次。

### 日志输出

```text
I (732) BMGR_PLAY_SDCARD_MUSIC: Playing music from /sdcard/test.wav
I (739) PERIPH_I2S: I2S[0] TDM,  TX, ws: 45, bclk: 9, dout: 8, din: 10
I (745) PERIPH_I2S: I2S[0] initialize success: 0x3c096c58
I (750) DEV_AUDIO_CODEC: DAC is ENABLED
I (754) DEV_AUDIO_CODEC: Init audio_dac, i2s_name: i2s_audio_out, i2s_rx_handle:0x0, i2s_tx_handle:0x3c096c58, data_if: 0x3fcea804
I (765) PERIPH_I2C: i2c_new_master_bus initialize success
I (775) ES8311: Work in Slave mode
I (778) DEV_AUDIO_CODEC: Successfully initialized codec: audio_dac
I (779) DEV_AUDIO_CODEC: Create esp_codec_dev success, dev:0x3fceaa68, chip:es8311
I (787) BOARD_MANAGER: Device audio_dac initialized
I (791) DEV_FS_FAT: slot_config: cd=-1, wp=-1, clk=15, cmd=7, d0=4, d1=-1, d2=-1, d3=-1, d4=-1, d5=-1, d6=-1, d7=-1, width=1, flags=0x0
I (873) DEV_FS_FAT: Filesystem mounted, base path: /sdcard
Name: SD64G
Type: SDHC
Speed: 40.00 MHz (limit: 40.00 MHz)
Size: 60906MB
CSD: ver=2, sector_size=512, capacity=124735488 read_bl_len=9
SSR: bus_width=1
I (881) BOARD_MANAGER: Device fs_sdcard initialized
I (886) BOARD_DEVICE: Device handle audio_dac found, Handle: 0x3fce9a7c TO: 0x3fce9a7c
I (900) WAV_HEADER: WAV file: 48000 Hz, 2 channels, 16 bits
I (900) BMGR_PLAY_SDCARD_MUSIC: Play WAV file info: 48000 Hz, 2 channels, 16 bits
I (907) I2S_IF: channel mode 2 bits:16/16 channel:2 mask:3
I (912) I2S_IF: TDM Mode 1 bits:16/16 channel:2 sample_rate:48000 mask:3
I (933) Adev_Codec: Open codec device OK
I (5185) BMGR_PLAY_SDCARD_MUSIC: Play WAV file completed
I (5185) BOARD_DEVICE: Deinit device audio_dac ref_count: 0 device_handle:0x3fce9a7c
I (5197) BOARD_DEVICE: Device audio_dac config found: 0x3c064ebc (size: 92)
I (5198) BOARD_PERIPH: Deinit peripheral i2s_audio_out ref_count: 0
E (5200) i2s_common: i2s_channel_disable(1217): the channel has not been enabled yet
W (5208) PERIPH_I2S: Caution: Releasing TX (0x0).
W (5212) PERIPH_I2S: Caution: RX (0x3c096e0c) forced to stop.
E (5217) i2s_common: i2s_channel_disable(1217): the channel has not been enabled yet
I (5225) BOARD_PERIPH: Deinit peripheral i2c_master ref_count: 0
I (5231) PERIPH_I2C: i2c_del_master_bus deinitialize
I (5235) BOARD_MANAGER: Device audio_dac deinitialized
I (5240) BOARD_DEVICE: Deinit device fs_sdcard ref_count: 0 device_handle:0x3fceaa98
I (5248) BOARD_MANAGER: Device fs_sdcard deinitialized
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

### 找不到音频文件

若日志提示无法打开 `/sdcard/test.wav`，请确认音频文件是否存在，命名是否正确。
