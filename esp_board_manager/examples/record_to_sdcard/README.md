# Record Audio to microSD Card

- [中文版](./README_CN.md)
- Example difficulty: ⭐⭐

## Example Brief

- This example captures audio from the microphone (Audio ADC), encodes it as PCM in a WAV container, and saves to `/sdcard/test_rec.wav` for a fixed duration (see `DEFAULT_DURATION_SECONDS` in source).
- Technically it demonstrates `esp_board_manager` for Audio ADC and `fs_sdcard`, plus `esp_codec_dev` read path and `write_wav_header` from the shared `common` component.

### Typical Scenarios

- Voice memos, acoustic logging to SD, or verifying microphone and SD paths on a board.

### Run Flow

init ADC → mount SD → write WAV header → loop `esp_codec_dev_read` and `fopen` write → close file → deinit.

### File Structure

```
├── common
│   ├── CMakeLists.txt
│   ├── wav_header.c
│   └── include/wav_header.h
├── main
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   └── record_to_sdcard.c
├── CMakeLists.txt               Project: bmgr_record_to_sdcard
├── partitions.csv
├── sdkconfig.defaults
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

- Board with Audio ADC (`audio_adc`) and SD card support (`fs_sdcard`).

### Software Requirements

- Sufficient free space on the SD card for the WAV clip.

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
cd $YOUR_GMF_PATH/packages/esp_board_manager/examples/record_to_sdcard
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

### Project Configuration

- Recording duration, sample rate, path, and gain: edit the macros in `record_to_sdcard.c` (`DEFAULT_DURATION_SECONDS`, `DEFAULT_SAMPLE_RATE`, `DEFAULT_REC_URL`, `DEFAULT_REC_GAIN`, etc.).

## How to Use the Example

### Functionality and Usage

- Insert SD card, flash, and run; after boot the device records for the configured duration and writes `/sdcard/test_rec.wav`.

### Log Output

```text
I (732) BMGR_RECORD_TO_SDCARD: Record to /sdcard/test_rec.wav
I (738) DEV_AUDIO_CODEC: ADC is ENABLED
I (760) PERIPH_I2S: I2S[0] TDM, RX, ws: 45, bclk: 9, dout: 8, din: 10
I (766) PERIPH_I2S: I2S[0] initialize success: 0x3c096ebc
I (788) DEV_AUDIO_CODEC: Init audio_adc, i2s_name: i2s_audio_in, i2s_rx_handle:0x3c096ebc, i2s_tx_handle:0x3c096d08, data_if: 0x3fcee2dc
I (800) PERIPH_I2C: i2c_new_master_bus initialize success
I (808) ES7210: Work in Slave mode
I (814) ES7210: Enable ES7210_INPUT_MIC1
I (817) ES7210: Enable ES7210_INPUT_MIC2
I (820) ES7210: Enable ES7210_INPUT_MIC3
I (823) ES7210: Enable TDM mode
I (826) DEV_AUDIO_CODEC: Successfully initialized codec: audio_adc
I (828) DEV_AUDIO_CODEC: Create esp_codec_dev success, dev:0x3fcee514, chip:es7210
I (835) BOARD_MANAGER: Device audio_adc initialized
I (840) DEV_FS_FAT: slot_config: cd=-1, wp=-1, clk=15, cmd=7, d0=4, d1=-1, d2=-1, d3=-1, d4=-1, d5=-1, d6=-1, d7=-1, width=1, flags=0x0
I (922) DEV_FS_FAT: Filesystem mounted, base path: /sdcard
Name: SD64G
Type: SDHC
Speed: 40.00 MHz (limit: 40.00 MHz)
Size: 60906MB
CSD: ver=2, sector_size=512, capacity=124735488 read_bl_len=9
SSR: bus_width=1
I (930) BOARD_MANAGER: Device fs_sdcard initialized
I (935) BOARD_DEVICE: Device handle audio_adc found, Handle: 0x3fce9a7c TO: 0x3fce9a7c
I (949) I2S_IF: channel mode 2 bits:32/32 channel:2 mask:1
I (949) I2S_IF: TDM Mode 0 bits:32/32 channel:2 sample_rate:48000 mask:1
I (954) I2S_IF: channel mode 2 bits:32/32 channel:2 mask:1
I (959) I2S_IF: TDM Mode 1 bits:32/32 channel:2 sample_rate:48000 mask:1
I (966) ES7210: Bits 16
I (975) ES7210: Enable ES7210_INPUT_MIC1
I (977) ES7210: Enable ES7210_INPUT_MIC2
I (980) ES7210: Enable ES7210_INPUT_MIC3
I (983) ES7210: Enable TDM mode
I (988) ES7210: Unmuted
I (988) Adev_Codec: Open codec device OK
I (991) BMGR_RECORD_TO_SDCARD: Record WAV file info: 48000 Hz, 1 channels, 32 bits
I (995) BMGR_RECORD_TO_SDCARD: Starting I2S recording...
I (11012) BMGR_RECORD_TO_SDCARD: I2S recording completed. Total bytes recorded: 1925120
I (11018) BOARD_DEVICE: Deinit device audio_adc ref_count: 0 device_handle:0x3fce9a7c
I (11021) BOARD_DEVICE: Device audio_adc config found: 0x3c064f04 (size: 92)
I (11024) BOARD_PERIPH: Deinit peripheral i2s_audio_in ref_count: 0
E (11030) i2s_common: i2s_channel_disable(1217): the channel has not been enabled yet
W (11038) PERIPH_I2S: Caution: Releasing RX (0x0).
I (11042) BOARD_PERIPH: Deinit peripheral i2c_master ref_count: 0
I (11048) PERIPH_I2C: i2c_del_master_bus deinitialize
I (11053) BOARD_MANAGER: Device audio_adc deinitialized
I (11058) BOARD_DEVICE: Deinit device fs_sdcard ref_count: 0 device_handle:0x3fcee544
I (11065) BOARD_MANAGER: Device fs_sdcard deinitialized
```

(Exact sample rate or channel count may match your codec/board configuration.)

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

### SD or recording failures

Check the card is FAT, mounted at `/sdcard`, and not write-protected. Verify the board supports `audio_adc` and `fs_sdcard`.
