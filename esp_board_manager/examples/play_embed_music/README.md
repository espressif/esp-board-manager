# Play Embedded WAV Music

- [中文版](./README_CN.md)
- Example difficulty: ⭐

## Example Brief

- This example plays a WAV file bundled with the firmware from flash, without using a microSD card or network.
- Technically it demonstrates `esp_board_manager` device initialization for the audio DAC, retrieval of `dev_audio_codec_handles_t`, and playback through `esp_codec_dev` (open, set volume, write). Audio is linked via CMake `EMBED_TXTFILES`.

### Typical Scenarios

- Firmware-included prompt tones, boot jingles, or small demonstration clips where no external storage is required.

### Prerequisites

The WAV file is built into the application using ESP-IDF [embedded binary data](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/build-system.html#cmake-embed-data). Place your file under `main/audio_files/` (default: `test.wav`) and ensure `main/CMakeLists.txt` lists it in `EMBED_TXTFILES`. The C symbols are `_binary_<name>_start` / `_binary_<name>_end` (object name derived from the file path).

### Run Flow

`app_main` initializes the DAC via the board manager, opens the codec with parameters parsed from the WAV header in flash, pass the data stream through `esp_codec_dev_write` for output, then deinitializes the device.

### File Structure

```
├── main
│   ├── audio_files              Place test.wav (or your WAV) here
│   ├── CMakeLists.txt           EMBED_TXTFILES for WAV
│   ├── idf_component.yml
│   └── play_embed_music.c
├── CMakeLists.txt               Project: bmgr_play_embed_music
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

- Speaker as required by the board.

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
cd $YOUR_GMF_PATH/packages/esp_board_manager/examples/play_embed_music
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

- After flashing, the device plays the embedded `test.wav` once through the DAC, then exits `app_main`.

### Log Output

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

### Embedded WAV missing or build error

Ensure `main/audio_files/test.wav` exists and `main/CMakeLists.txt` contains `EMBED_TXTFILES` for that path.
