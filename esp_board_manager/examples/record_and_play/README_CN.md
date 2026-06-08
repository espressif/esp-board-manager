# 音频回环（边录边播）

- [English Version](./README.md)
- 例程难度：⭐⭐

## 例程简介

- 实时音频回环：从 ADC（麦克风）读取的采样数据立即经 DAC（扬声器）播放，持续时长由 `LOOPBACK_DURATION_SEC` 控制。
- 技术上演示在同一工程中同时使用 `audio_dac` 与 `audio_adc`，对两个 codec 分别以相同 `esp_codec_dev_sample_info_t` 执行 `esp_codec_dev_open`，并在循环中 read→write，无需 SD 存储。

### 典型场景

- 快速验证板级采集与播放链路是否正常。

### 运行机制

初始化 DAC → 初始化 ADC → 以相同采样格式分别 open → 设置音量/增益 → 循环读缓冲并写缓冲直至超时 → close codec 并释放设备。

### 文件结构

```
├── main
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   └── record_and_play.c
├── CMakeLists.txt               工程：bmgr_record_and_play
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

- 板级描述中同时包含 Audio ADC 与 Audio DAC（`audio_adc`、`audio_dac`），例如 ESP32-S3-Korvo-2 V3。
- 按硬件连接麦克风与扬声器。

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
cd $YOUR_GMF_PATH/packages/esp_board_manager/examples/record_and_play
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

### 项目配置

- 可在 `record_and_play.c` 中调整 `DEFAULT_SAMPLE_RATE`、`DEFAULT_CHANNELS`、`DEFAULT_BITS_PER_SAMPLE`、`DEFAULT_PLAY_VOL`、`DEFAULT_REC_GAIN`、`LOOPBACK_DURATION_SEC`。
- 出现啸叫或回声时可降低播放音量或录音增益。
- 对于使用 ADC 麦克风、PDM 扬声器或是其他不依赖 Codec 芯片的扬声器的开发板，需要在 `menuconfig` 中打开相关的配置：

    - `Component config` -> `Audio Codec Device Data Interface Configuration` -> `Support ADC continuous data interface`

    - `Component config` -> `Audio Codec Device Configuration` -> `Support Dummy Codec Chip`

## 如何使用例程

### 功能和用法

- 上电后自动运行设定时长的回环，麦克风拾取的声音应从扬声器听到。

### 日志输出

```text
I (xxx) BMGR_RECORD_AND_PLAY: Audio loopback: record from ADC and play through DAC for 30 seconds
I (xxx) BMGR_RECORD_AND_PLAY: Audio loopback started: 16000 Hz, 2 ch, 16 bit
I (xxx) BMGR_RECORD_AND_PLAY: Loopback running... 1/30s, total 131072 bytes
...
I (xxx) BMGR_RECORD_AND_PLAY: Audio loopback completed. Total bytes transferred: ...
```

（时间戳与总字节数以实际运行环境为准。）

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

### 啸叫或反馈

降低 `DEFAULT_PLAY_VOL` / `DEFAULT_REC_GAIN`，或加大麦克风与扬声器的物理隔离。

### 板子仅有 ADC 或仅有 DAC

本例程需板级同时定义两种设备；请选用全功能音频参考板或扩展自定义板 YAML。
