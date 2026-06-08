# Audio Loopback (Record and Play)

- [中文版](./README_CN.md)
- Example difficulty: ⭐⭐

## Example Brief

- This example runs real-time audio loopback: samples from the ADC (mic) are immediately played on the DAC (speaker) for a fixed time (`LOOPBACK_DURATION_SEC`).
- Technically it demonstrates simultaneous `esp_board_manager` use of `audio_dac` and `audio_adc`, two independent `esp_codec_dev_open` calls with the same `esp_codec_dev_sample_info_t`, and a read→write loop without SD storage.

### Typical Scenarios

- Quick validation that both capture and playback paths work on the target board.

### Run Flow

Init DAC → init ADC → open both codecs with same rate/channels/bits → optional gain/volume → repeat read buffer / write buffer until timeout → close codecs and deinit devices.

### File Structure

```
├── main
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   └── record_and_play.c
├── CMakeLists.txt               Project: bmgr_record_and_play
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

- Board with both Audio ADC and Audio DAC definitions (`audio_adc`, `audio_dac`), e.g. ESP32-S3-Korvo-2 V3.
- Microphone and speaker per board design.

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
cd $YOUR_GMF_PATH/packages/esp_board_manager/examples/record_and_play
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

- Tune `DEFAULT_SAMPLE_RATE`, `DEFAULT_CHANNELS`, `DEFAULT_BITS_PER_SAMPLE`, `DEFAULT_PLAY_VOL`, `DEFAULT_REC_GAIN`, and `LOOPBACK_DURATION_SEC` in `record_and_play.c`.
- Reduce playback volume or mic gain if you hear feedback or howling.
- For development boards using ADC microphones, PDM speakers, or other speakers that do not rely on Codec chips, enable the relevant options in `menuconfig`:

    - `Component config` -> `Audio Codec Device Data Interface Configuration` -> `Support ADC continuous data interface`

    - `Component config` -> `Audio Codec Device Configuration` -> `Support Dummy Codec Chip`

## How to Use the Example

### Functionality and Usage

- After boot, loopback runs for the configured duration; you should hear the microphone signal on the speaker.

### Log Output

```text
I (xxx) BMGR_RECORD_AND_PLAY: Audio loopback: record from ADC and play through DAC for 30 seconds
I (xxx) BMGR_RECORD_AND_PLAY: Audio loopback started: 16000 Hz, 2 ch, 16 bit
I (xxx) BMGR_RECORD_AND_PLAY: Loopback running... 1/30s, total 131072 bytes
...
I (xxx) BMGR_RECORD_AND_PLAY: Audio loopback completed. Total bytes transferred: ...
```

(Exact timestamps vary; adjust duration in source if needed.)

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

### Echo or feedback

Lower `DEFAULT_PLAY_VOL` or `DEFAULT_REC_GAIN` in `record_and_play.c`, or increase physical isolation between mic and speaker.

### Only one of ADC/DAC exists on board

This example requires both devices in the board YAML; choose a full audio reference board or extend your custom board definition.
