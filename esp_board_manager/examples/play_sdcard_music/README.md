# Play Music from microSD Card

- [中文版](./README_CN.md)
- Example difficulty: ⭐

## Example Brief

- This example plays a WAV file from the microSD card root (`/sdcard/test.wav`).
- Technically it demonstrates `esp_board_manager` initialization for Audio DAC and FAT/SD card (`fs_sdcard`), then `esp_codec_dev` for playback using format information from the WAV header (shared helper in `common/wav_header`).

### Typical Scenarios

- Local music playback, voice prompt playback from removable storage, or audio assets shipped on SD.

### Run Flow

Mount SD card → open WAV → `read_wav_header` → `esp_codec_dev_open` / `write` in a loop → unmount and deinit.

### File Structure

```
├── common
│   ├── CMakeLists.txt
│   ├── wav_header.c
│   └── include/wav_header.h
├── main
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   └── play_sdcard_music.c
├── CMakeLists.txt               Project: bmgr_play_sdcard_music (EXTRA_COMPONENT_DIRS ../common)
├── partitions.csv
├── sdkconfig.defaults           Includes FatFs / LFN settings
├── sdkconfig.defaults.esp32
├── sdkconfig.defaults.esp32s3
├── sdkconfig.defaults.esp32p4
├── README.md
└── README_CN.md
```

## Environment Setup

### Default IDF Branch

This example supports IDF release/v5.4 (>= v5.4.3) and release/v5.5 (>= v5.5.2).

### Hardware Required

- A development board with an SD card; put `test.wav` in the card root.

### Software Requirements

- WAV file named `test.wav` on the SD card root.

## Build and Flash

### Build Preparation

Before building this example, make sure ESP-IDF is set up. If it is already configured, you can skip this step; otherwise, run the following scripts in the ESP-IDF root directory to set up the build environment. For the complete steps of configuring and using ESP-IDF, see the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/index.html):

```shell
./install.sh
. ./export.sh
```

This example uses [ESP Board Manager](https://github.com/espressif/esp-board-manager) to manage board-level resources. The [`esp-bmgr-assist`](https://pypi.org/project/esp-bmgr-assist/) helper tool is recommended as the default entry point.

Install it in the activated ESP-IDF Python environment (only needed once per environment):

```bash
pip install esp-bmgr-assist
pip install --upgrade esp-bmgr-assist  # run this command when an update is requested
```

- Navigate to this example's project directory:

```shell
cd $YOUR_GMF_PATH/packages/esp_board_manager/examples/play_sdcard_music
```

### Build and Flash Commands

- List the currently visible boards:

```bash
idf.py bmgr -l
```

Example output:

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

The example output above is based on the board list and ordering from `esp_boards` 0.5.2. Different `esp_boards` versions or custom board dependencies may change the list and indexes. Use the actual output of `idf.py bmgr -l` when selecting a board.

- Select a board:

```bash
idf.py bmgr -b <board_index|board_name>
```

For example, to select `esp32_s3_korvo_2_3`:

```bash
idf.py bmgr -b 10
# or
idf.py bmgr -b esp32_s3_korvo_2_3
```

On first invocation of `idf.py bmgr`, the component is downloaded automatically based on the `espressif/esp_board_manager` dependency declared in `main/idf_component.yml`.

> [!NOTE]
> To switch to a different board supported by `esp_board_manager`, repeat the same steps with the new board name or index.
> For a custom board, see [Creating a Board Guide](https://docs.espressif.com/projects/esp-board-manager/en/latest/create-board/index.html).
> For more information about `esp_board_manager`, see the [ESP Board Manager Getting Started Guide](https://github.com/espressif/esp-board-manager/blob/main/esp_board_manager/README.md).

- Compile the example code:

```shell
idf.py build
```

Flash the program and run the monitor tool to view serial output (replace PORT with your port name):

```shell
idf.py -p PORT flash monitor
```

To exit the monitor, use `Ctrl-]`.

## How to Use the Example

### Functionality and Usage

- Power on with SD inserted; firmware plays `/sdcard/test.wav` once through the DAC.

### Log Output

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

### Audio file not found

If logs show failure to open `/sdcard/test.wav`, ensure the file exists at the card root with the exact name.
