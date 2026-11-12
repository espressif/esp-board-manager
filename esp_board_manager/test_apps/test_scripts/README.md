# ESP Board Manager Test Suite

This directory contains comprehensive pytest-based test suite for the ESP Board Manager Configuration Generator.

## Host C tests

`../host_tests/` contains the host C tests and substitutes for hardware APIs.
Python test entry points remain in this directory and `generators/tests/`.
The tests compile production BMGR sources with the real headers from `include/`,
`private_inc/`, and the relevant device or peripheral directory. They do not
maintain copies of BMGR descriptors, error codes, registration macros, or lookup
functions.

```text
host_tests/
├── runtime_override/  # Manager lifecycle, runtime overrides, and revision cleanup
├── ethernet/          # RMII/RGMII runtime fixtures and EMAC substitutes
├── gpio_expander/     # Expander mapping lifecycle fixture
├── gpio/              # Generated native and virtual GPIO configuration fixture
├── gpio_ctrl/         # Initial-level selection, validation, and cleanup fixture
└── support/           # Shared host linker sections
```

- `test_dev_ethernet.py` checks RMII/RGMII configuration forwarding, PHY hooks,
  and cleanup against headers from `IDF_PATH`. MAC/PHY hardware calls are simulated.
- `test_gpio_expander_mapping.py` checks mapping registration, cleanup failures,
  and generated virtual GPIO configurations, with UBSan enabled for its C fixtures.
- `test_dev_gpio_ctrl_runtime.py` checks initial-level selection, invalid configurations,
  reinitialization, and GPIO-write failure cleanup against the real device source with UBSan.
- `test_interactive_controls.py` checks button selection, callback cleanup, IO
  dispatch, and console argument handling. Its board/driver boundary functions
  are simulated, but their BMGR declarations come from the production headers.
- `../host_tests/runtime_override/run_host_test.sh` compiles the real manager, device,
  peripheral, runtime-state, and custom power-controller sources. The revision
  generator suite invokes this runner with generated revision-apply code.

Tests that exercise registration use `../host_tests/support/esp_board_sections.ld`
with a GNU-compatible ELF host linker. This supplies the section boundaries
normally created by the ESP-IDF linker; registration and lookup still use the
production BMGR implementation. The standalone runtime runner requires Bash and
`cc`. Registration tests skip hosts whose compiler does not target ELF; the
standalone runner returns exit code 77 on those hosts. Ethernet host tests also
require `IDF_PATH` and skip targets absent from that IDF checkout.

These tests verify software behavior. Simulated GPIO/I2C/EMAC calls and RTOS/timer
scheduling do not verify electrical behavior, real concurrency, network traffic,
or operation on a physical board.

## Interactive hardware controls

### LCD LVGL touch input

Run `case run lcd.lvgl` to inspect each color screen and select PASS or FAIL.
With LVGL 9 and an available touch device, the test app installs its own touch
read callback while retaining the port's interrupt handling and cleanup. This
case uses a single pointer for its buttons; use `touch.multitouch` below to
validate contact IDs and multiple simultaneous contacts.

`ESP_ERR_NOT_FINISHED` preserves the last valid pointer position and pressed or
released state. Only a valid zero-contact report releases the pointer. Other
acquisition errors stop touch reads, end the manual test, and return the original
error to the case runner without aborting the firmware. LVGL 8 retains its
existing port callback.

`test_lcd_lvgl_touch.py` replays valid, skipped, and failed reports through the
actual test-app callback on the host. It does not validate the controller's I2C
or ACK protocol on hardware.

### Multi-touch tracking

On a board with the generic `lcd_touch` device and a driver that provides native
track IDs, run:

```text
case run touch.multitouch
```

Within 30 seconds, place two fingers on the panel, move them, lift one while
keeping the other pressed, then lift the remaining finger. The case prints
`DOWN`, `MOVE`, and `UP` with each point's native ID and coordinates. IDs are
matched across valid frames, so a different point order does not become a new
press. `MOVE` is printed when coordinates change; `UP` uses the last valid
coordinates. The case passes only after observing simultaneous contacts, a
partial release that retains an existing contact, and full release in the same
gesture. No input, one-finger input, or a missing step times out and fails.
Movement is shown for inspection but is not required for a pass.

When LCD support is enabled and the configured display is present, the same
case also shows colored contact circles, native IDs, coordinates, and short
trails (when LVGL's `LV_USE_LINE` is enabled). A released contact disappears;
the other contacts keep their colors and trails even if the report order
changes. Skipped reports leave the last valid view intact. Touch coordinates
are scaled and clipped to the display size without applying touch mirroring
or swapping a second time. The view briefly shows PASS or FAIL before cleanup.
It only consumes snapshots from the test and does not register an LVGL touch
reader. Without a display, the case keeps using the console alone. A configured
display that fails to initialize causes the case to fail.

The console framework acquires and releases `BMGR_TEST_NAME_LCD_TOUCH`, plus the
optional display and backlight when LCD support is enabled; a prior
`bmgr init` is unnecessary. Use the test app's touch device-name override if the
board uses a different name. Enable `ESP_BOARD_DEV_LCD_TOUCH_SUPPORT` and set
`ESP_LCD_TOUCH_MAX_POINTS` to at least 2. Drivers without a native `get_track_id`
callback and valid frames containing duplicate IDs are rejected. A driver that
supports only one point cannot complete this test.

Run this case with other touch readers stopped. An active LVGL/touch consumer
recorded in the test context is rejected to avoid concurrent controller reads.
`ESP_ERR_NOT_FINISHED` skips a report without producing release events; an
actual I/O error fails the case. Passing this manual test confirms the observed
touch sequence on that board, not the controller's maximum supported point count.

The automated replay for this case runs on the host:

```sh
python3 -m pytest -q esp_board_manager/test_apps/test_scripts/test_lcd_touch_multitouch.py
```

It tests event association, incomplete gestures, skipped reports, invalid IDs,
and read failures using simulated touch and clock boundaries. It also checks
that the case compiles when touch support is disabled. It does not exercise
physical I2C, controller firmware, or concurrent tasks.

### Buttons and IO

The test application's console can monitor one button by its YAML device name,
or enumerate every `button` device, including GPIO, ADC, and custom buttons:

```text
case run button.read
case run button.read key0_button
case run button.read all
```

Each run monitors events for 10 seconds and prints the device name and button
handle, including single/double clicks and long-press events. It acquires and
releases its own device references and removes its test callbacks on exit.
If a monitored event already has callbacks, the test reports an error; release
that consumer before testing, because the button API cannot remove a callback
by its function pointer. Existing application callbacks are preserved.

Physical IO levels can be changed without rebuilding:

```text
io set gpio 1 0
io get gpio 1
bmgr init xl9555
io set xl9555 2 0
io get xl9555 2
case run speaker.tone.1khz
bmgr deinit xl9555
```

`gpio` addresses native chip GPIOs and, when the GPIO wrapper is enabled,
registered expander GPIOs. Native `set` leaves the pad in input/output mode;
native `get` requires its input to be enabled. Mapped `set` selects output mode,
and mapped `get` reads the expander input register. Initialize the expander first
(for ATK: `bmgr init xl9555`, then `io set gpio 51 0`). These operations do not
automatically restore earlier pin configuration.

Device IO requires the device to be initialized first. A board-local driver can
register `<device>_io_get` and `<device>_io_set` with
`DEVICE_EXTRA_FUNC_REGISTER`; their signatures are documented in
`main/commands/cmd_io.h`. The ATK XL9555 adapters take zero-based pin indices,
reject writes to input pins, and update the driver's cached output state. A
native GPIO number does not refer to an expander pin. Device IO state is left
in place for subsequent tests; a later device initialization may restore its
configured defaults.

## 📁 Directory Structure

```
test_scripts/
├── conftest.py                    # Pytest configuration and fixtures
├── pytest.ini                     # Pytest settings
├── test_board_selection.py        # Board selection tests (name, index, priority)
├── test_list_boards.py            # Board listing tests
├── test_generation_modes.py       # Generation mode tests (kconfig-only, etc.)
├── test_error_handling.py         # Error handling and edge cases
├── test_log_levels.py             # Log level functionality tests
├── run_pytest.sh                  # Test execution script
├── validate_tests.py              # Test validation utility
├── self_test.py                   # Self-test utility
├── diagnose_pytest.py             # Pytest diagnostics tool
└── README.md                      # This file
```

## 🎯 Test Suite

- **Type**: pytest-based tests
- **Tests**: 99 test cases
- **Requirements**: pytest (and optional plugins)
- **Usage**: `./run_pytest.sh`
- **Features**: Detailed reporting, coverage analysis, parallel execution

## 🚀 Quick Start

### Automatic Dependency Installation

The script automatically installs required packages if they are missing:
- **pytest**: Automatically installed if not present
- **pytest-html**: Automatically installed when using `--html` option
- **pytest-cov**: Automatically installed when using `--cov` option

You can also install them manually if preferred:
```bash
pip3 install pytest pytest-html pytest-cov
```

### Run All Tests

```bash
# Basic run
./run_pytest.sh

# Verbose output
./run_pytest.sh -v

# With HTML report (default path: test_reports/report.html)
./run_pytest.sh --html

# With HTML report (custom filename)
./run_pytest.sh --html=report.html

# With coverage report
./run_pytest.sh --cov

# Generate both HTML and coverage reports
./run_pytest.sh --html --cov
```

### Run Specific Test Cases

```bash
# Board selection tests
pytest test_board_selection.py -v

# List boards tests
pytest test_list_boards.py -v

# Generation modes
pytest test_generation_modes.py -v

# Error handling
pytest test_error_handling.py -v

# Log levels
pytest test_log_levels.py -v
```

## 📊 Test Coverage

The test suite covers:

- ✅ Board selection (by name and index)
- ✅ Positional argument support
- ✅ Parameter priority (-b overrides positional)
- ✅ Board listing with numbering
- ✅ Generation modes (kconfig-only, peripherals-only, devices-only)
- ✅ Error handling and boundary conditions
- ✅ Log level functionality
- ✅ Command-line argument validation
- ✅ Special characters and edge cases

## 📈 Test Results

Current Status: **🟢 All Tests Passing**

- Total Tests: 99 ✅
- Passed: 99
- Failed: 0
- Success Rate: 100% 🎊

## 🔧 Test Files

### test_board_selection.py
Tests board selection functionality:
- Board selection by name (positional and -b)
- Board selection by index (positional and -b)
- Parameter priority (-b overrides positional)
- Invalid board names and indices
- Special characters and edge cases

### test_list_boards.py
Tests board listing functionality:
- Basic listing with -l option
- Board numbering display
- Output format validation

### test_generation_modes.py
Tests different generation modes:
- --kconfig-only
- --peripherals-only
- --devices-only

### test_error_handling.py
Tests error handling:
- Invalid board names
- Invalid indices
- Missing configuration files
- Empty parameters

### test_log_levels.py
Tests logging functionality:
- DEBUG, INFO, WARNING, ERROR levels
- Log output validation

## 🛠️ Utilities

### run_pytest.sh
Main test execution script with preset options for convenience.

### validate_tests.py
Validates test suite integrity and coverage.

### self_test.py
Self-test utility to verify test framework functionality.

### diagnose_pytest.py
Diagnostic tool for troubleshooting pytest issues.

## 🔧 Maintenance

When adding new features to ESP Board Manager:

1. Add corresponding test cases
2. Run test suite to verify: `./run_pytest.sh -v`
3. Ensure 100% pass rate
4. Update documentation as needed

## 🐛 Troubleshooting

If tests fail:

1. **Check test output** for detailed error messages
2. **Run individual test** to isolate the issue:
   ```bash
   pytest test_board_selection.py::TestBoardSelectionByName::test_valid_board_name -v
   ```
3. **Use diagnostics**:
   ```bash
   python diagnose_pytest.py
   ```
4. **Review recent changes** to the main script

### Common Issues

**Automatic Installation Fails**
- **When**: Script tries to auto-install packages but fails
- **Possible Causes**:
  - No internet connection
  - pip3 not available
  - Permission issues
- **Solution**: Install manually:
  ```bash
  pip3 install pytest pytest-html pytest-cov
  ```
- **Note**: The script automatically installs missing packages. If auto-installation fails, you'll see an error message with manual installation instructions.

**Permission Errors During Installation**
- **When**: pip3 requires sudo/admin privileges
- **Solution**: Use `pip3 install --user` or install in a virtual environment:
  ```bash
  python3 -m venv venv
  source venv/bin/activate  # On Windows: venv\Scripts\activate
  pip install pytest pytest-html pytest-cov
  ```

## 📝 Requirements

- Python 3.7+
- pytest
- pytest-html (optional, for HTML reports)
- pytest-cov (optional, for coverage reports)
