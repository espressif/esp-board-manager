# How to Customize Board

## Supported Board Paths

ESP Board Manager allows modular board customization, supporting the following three different paths for placing board configuration, providing flexibility for different deployment scenarios:

1. **Main Board Directory**: Built-in boards provided with the component, path like `esp_board_manager/boards`
2. **User Project Components**: Custom boards defined in project components, path like `{PROJECT_ROOT}/components/board_name`, refer to: [`esp_board_manager/test_apps/components/test_board_e/`](../test_apps/components/test_board_e/)
3. **Custom Path**: External board definitions in custom locations, such as [`esp_board_manager/test_apps/test_single_board`](../test_apps/test_single_board/), when running commands you need to specify the path via the `-c` parameter to set custom path `idf.py bmgr -c test_single_board -l` (legacy: `idf.py gen-bmgr-config -c test_single_board -l`)

A **common device configuration template document** is provided in the repository for quick reference and obtaining **YAML configuration snippets** for common devices: [Common Configuration Templates](./board_config_template.md)

> **Note:** If you use the ESP Board Manager component automatically downloaded from the component repository, it is not recommended to directly create custom boards in the motherboard-level directory to avoid accidentally deleting custom board configurations when cleaning components

## Creating a New Board

### 1. **Create Board Directory**

- It is recommended to place the board directory under the project's components folder `{PROJECT_ROOT}/components/XXXX`,

   ```bash
   mkdir components/<board_name>
   cd components/<board_name>
   ```

### 2. **Create Required Files**

There are three ways to create a new board:

#### 2.1 **Modify Based on Existing Configuration**

- Copy a similar board file from the motherboard-level directory to the project directory, and then **modify it according to the actual board being used** (recommended). You can then validate the new board by referring to [Validating and Using the New Board](#validating-and-using-the-new-board).

#### 2.2 **Use Script to Create**

- Use the script provided by ESP Board Manager to automatically create the board. The script will **automatically search and copy** required configurations based on the selected peripherals and devices, saving you the step of manually copying configuration information from corresponding folders. Usage is as follows:

```bash
# Create the board configuration files at the default path (default path is {PROJECT_ROOT}/components/<board_name>):
idf.py bmgr -n <board_name>

# Create the board configuration files at a custom path:
idf.py bmgr -n path/to/board/<board_name>
```

After running the script, you need to sequentially select **chip, device, and peripherals** according to the prompts. The script will automatically check the dependencies of devices on peripherals. If any peripheral is missing, the script will prompt you to add it until all dependencies are satisfied.

> **Note:**
> - Before proceeding with further operations, you need to manually **review and modify the `board_devices.yaml` and `board_peripherals.yaml` configuration files to meet the actual requirements of the board**, paying special attention to configuration items with `[IO]` and `[TO_BE_CONFIRMED]` keywords.
> - Please check the names of devices and peripherals to ensure there are no duplicate names for devices or peripherals.
> - Only devices and peripherals supported by the chip will appear in the script's options.
> - **Important: Devices find dependent peripherals by name**. Please ensure that all peripherals required by the devices are added to `board_peripherals.yaml`, and that the peripheral `name` filled in the device configuration in `board_devices.yaml` exactly matches the corresponding `name` in `board_peripherals.yaml`, and peripheral names **must start with the type**
>
> For example:
> ```yaml
> # board_devices.yaml
>  - name: led_green
>    type: gpio_ctrl
>    config:
>      ...
>    peripherals:
>      - name: gpio_led_g
>
> # board_peripherals.yaml
>  - name: gpio_led_g
>    type: gpio
>    config:
>      ...
> ```

**Usage demonstration**
![alt text](how_to_customize_board.gif)

#### 2.3 **Manually Create Files**

- Run the following script to manually create the required files for the board.

   ```bash
   touch board_peripherals.yaml
   touch board_devices.yaml
   touch board_info.yaml
   touch sdkconfig.defaults.board  # Optional: board-specific SDK configuration defaults
   ```

> **Note:**
>
> - If you choose to create board files manually or copy configuration files from another board for modification, please note that the development board name is primarily determined by the folder name where the configuration files are stored. Please ensure that the folder name is consistent with the `board` field in `board_info.yaml`.
>
> - **Important: Board names must follow naming conventions**, only **letters (a-z, A-Z), numbers (0-9), and underscores (_)** are allowed. Hyphens (-) or other special characters are not supported. If the name does not comply, this board will be unavailable.
>
> - Each development board must contain at least three files: `board_info.yaml`, `board_devices.yaml`, and `board_peripherals.yaml`, and the script uses the existence of these three files as the basis for determining whether a development board is available.
>
> - All methods rely on proper configuration in the device and peripheral YAML files. These YAML files closely mirror the original driver parameters, including:
>
>     - Which configuration fields are supported
>     - Valid enum values
>     - Allowed parameter ranges
>
>     In other words, the YAML configuration is designed to match the underlying driver API as closely as possible, so existing driver knowledge can be reused.
>
>     For the exact definitions and supported options, refer to:
>
>     - `esp_board_manager/devices/xxx/xxx.yaml`
>     - `esp_board_manager/peripherals/xxx/xxx.yaml`.

### 3. **The `version` Field: Parsing Contract (Schema Version)**

**Core meaning: `version` declares the schema-syntax version this YAML follows**—the **parsing contract** between the generator and your configuration. When Board Manager later changes directory layout or field semantics incompatibly, tooling can use this field to distinguish old vs new syntax, migrate configs, and avoid silently breaking projects.

**1. Board-level contract (`board_info.yaml`)**
- Indicates which **board description schema generation** applies (layout, meaning of `board_info` fields, etc.).
- **Long-term use:** when board-layer schema is refactored at scale, the generator can branch on this value and transition old vs new configs.

**2. Devices and peripherals (only the following form is supported)**
In **`board_peripherals.yaml`** and **`board_devices.yaml`**, use **`version` only inside each item** of the `peripherals` / `devices` list, as a **sibling** of **`name`** and **`type`** (peer fields in the same mapping), for example:

```yaml
peripherals:
  - name: xxx
    type: adc
    version: 1.0.0   # Optional; per entry, sibling of name/type

devices:
  - name: xxx
    type: gpio_ctrl
    version: 1.0.0   # Same
```

**Generation tag (not a permanent lock-in):** The current generation tag for board YAML in this repo is **`1.0.0`**—meaning the generation consistent with the current parser and field semantics. **Omitting `version` means the current generation** (equivalent to `1.0.0`). **`version: default`** (any casing) is equivalent to omitting or using `1.0.0`; generated metadata **resolves it to the current tag `1.0.0`**. **Breaking changes** will use **a new tag** (e.g. `2.0.0`); newer toolchains will recognize both. If you set a tag **this release does not recognize**, generation **emits a warning** (check Board Manager version or use the current tag / `default` / omit the field).

**3. Do not confuse with the following `version` keys**
- Under **`dependencies`** in `board_devices.yaml`, each component’s `version` (e.g. `"*"`, `~1.5`) is an **ESP-IDF Component Manager** dependency constraint; **not** the YAML parsing contract above.

### 4. **Configuration File Structure**

**Board Information**
   ```yaml
   # board_info.yaml
   board: <board_name>
   chip: <chip_type>
   version: <version>
   description: "<board description>"
   manufacturer: "<manufacturer_name>"
   ```

**Peripherals Configuration**
   - Refer to similar peripheral configuration YAML files in the `boards` directory
   - Configure based on the supported peripheral YAML files under `peripherals`
   The basic structure for each peripheral is as follows:
   ```yaml
   # board_peripherals.yaml
   peripherals:
     - name: <peripheral_name>
       type: <peripheral_type>
       version: <version>
       role: <peripheral_role>
       config:
         # Peripheral-specific configuration
   ```

**Devices Configuration**
   - Refer to similar device configuration YAML files in the `boards` directory
   - Configure based on the supported device YAML files under `devices`
   The basic structure for each device is as follows:
   ```yaml
   # board_devices.yaml
   devices:
     - name: <device_name>
       type: <device_type>
       version: <version>
       sub_type: <sub_type>   # Optional: sub-device type string, each device may have its own sub-type or none
       init_skip: false  # Optional: skip automatic initialization (default: false)
       dependencies:     # Optional: define component dependencies
         espressif/gmf_core:
            version: '*'  # Version from the Espressif component registry (IDF Component Manager), not YAML schema; see §3
            override_path: ${BOARD_PATH}/gmf_core
            # Optional: allows you to use a local component instead of the version downloaded from the component registry
            # You can specify:
            #   - Absolute path, or
            #   - Relative path under ${BOARD_PATH} for easier management
       depends_on: <device_name>  # Optional: Used to declare other devices that the current device depends on
       config:
         # Device-specific configuration
         sub_config:      # Optional: provide sub-configuration if sub_type exists
         # Sub-type specific configuration
       peripherals:
         - name: <peripheral_name>
   ```

> **⚙️ Notes on Using `dependencies`**
>
> - The `dependencies` field in `board_devices.yaml` allows you to specify component dependencies for each device. This is particularly useful for board levels that require custom or local versions of components.
> - These dependencies will be copied to the `idf_component.yml` file in the `gen_bmgr_codes` folder. If dependencies with the same name exist, only the last one is retained according to YAML order.
> - This field supports all component registry features, including `override_path` and `path` options. For more details, refer to [Component Dependencies](https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/manifest_file.html#component-dependencies).
> - When using relative paths as local paths, note that they are relative to the `gen_bmgr_codes` directory. If the user specifies a local path in the board directory, use `${BOARD_PATH}` to simplify the path. Refer to the example: `./test_apps/test_custom_boards/my_boards/test_board1`.
>
> **⚙️ `${BOARD_PATH}` Variable:**
> - `${BOARD_PATH}` is a special variable that always points to the root directory of the current board definition (i.e., the folder containing your `board_devices.yaml`).
> - When specifying local or board-specific component paths in the `override_path` or `path` fields, always use `${BOARD_PATH}`. For more details, refer to [Local Directory Dependencies](https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/manifest_file.html#local-directory-dependencies).
> - ❌ **Incorrect**: `{{BOARD_PATH}}` or `$BOARD_PATH`

### Device Dependencies (`depends_on`)

An optional list (or single string) of other devices that must be initialized before this device. Board Manager initializes dependencies first, deinitializes them last, and keeps a shared dependency alive while any active dependent still uses it.

```yaml
devices:
  - name: io_expander_aw9523
    type: gpio_expander
    # ... config ...

  - name: display_lcd
    type: display_lcd
    sub_type: i80
    depends_on:
      - io_expander_aw9523    # init expander before LCD; release it after the LCD is deinitialized
    # ... config ...

  - name: button_power
    type: button
    sub_type: custom
    depends_on: io_expander_aw9523    # single dependency may be written as a string
```

Semantics:

- During `esp_board_manager_init()` (and `esp_board_device_init(name)`), each entry of `depends_on` is initialized first. If any dependency fails, the partially initialized chain is rolled back automatically.
- During deinit, a device cannot be torn down while any other active device still lists it in `depends_on`; the runtime returns `ESP_BOARD_ERR_DEVICE_DEP_IN_USE` in that case.
- Cyclic `depends_on` chains are detected by the parser at generation time and reported as a configuration error.

### 5. **Board-Specific SDK Configuration (Optional)**

`sdkconfig.defaults.board` file is used to define board-specific default SDK configuration.

   - Create a `sdkconfig.defaults.board` file in the board directory to define board-specific SDK configuration defaults
   - When switching to this board, the script will automatically **write** these settings to the project's `board_manager.defaults` file
   - This ensures that board-specific configurations are automatically applied via `SDKCONFIG_DEFAULTS` environment variable during ESP-IDF build system operations (menuconfig, reconfigure, etc.)

   Example:
   ```bash
   # sdkconfig.defaults.board
   # Example: board with Octal PSRAM (8-line)
   CONFIG_SPIRAM_MODE_OCT=y
   CONFIG_SPIRAM_SPEED_80M=y
   ```
   - The file supports standard ESP-IDF sdkconfig format:
     - `CONFIG_XXX=y` enable
     - `CONFIG_XXX=n` or `# CONFIG_XXX is not set` disable
     - `CONFIG_XXX="value"` string value
   - When switching boards, the `board_manager.defaults` file is regenerated with new board configurations

   Configuration Priority:
1. `sdkconfig` (user's current configuration)
2. `sdkconfig.defaults` (project defaults)
3. `board_manager.defaults` (board-specific default fallback and Board Manager symbols; loaded before project defaults so user defaults can override ordinary board defaults)
4. Component's own defaults

   User defaults must not set selected-board, board-name, device-support, or peripheral-support symbols managed by ESP Board Manager. These symbols are checked before configuration.

### 6. **Custom Code in the `board` Directory**

Some devices require board-specific initialization logic that cannot be fully described through YAML configuration alone. Typical examples include `display_lcd`, `lcd_touch`, and so on.

To support flexible board adaptation, the board directory allows you to provide custom implementation code, enabling the board to:

- Execute appropriate initialization routines for devices
- Handle board-specific wiring, timing, or power-up sequences
- Override or extend the default device initialization behavior when necessary

For concrete examples of how this is done, see:
[setup_device.c](../boards/esp_vocat_board_v1_2/setup_device.c), which implements specific initialization flows for display_lcd and lcd_touch devices

### 7. Locally amending an existing board with `-a/--amend`

When you only need small differences on top of a built-in board (a single pin tweak, a swapped touch chip, an extra device the base board doesn't have), copying the whole board directory is overkill. Instead, prepare an **amend directory** containing a `board_amend.yaml` manifest and apply it with `-a/--amend <dir>` at generation time.

```bash
# The amend directory must contain board_amend.yaml; the path can be absolute or relative.
idf.py bmgr -b esp32_s3_korvo2_v3 -a path/to/my_amend
```

#### Amend directory layout

```text
my_amend/
  board_amend.yaml          # required: the manifest
  sdkconfig.defaults.board  # optional; must be listed in apply: to take effect
  Kconfig.projbuild         # optional; must be listed in apply: to take effect

  # Files explicitly referenced by the apply: list
  tweak.yaml
  strong_setup.c
  pack/
    pack_extra.yaml
    pack_setup.c
    sdkconfig.defaults.board  # must be listed as pack/sdkconfig.defaults.board
    Kconfig.projbuild         # must be listed as pack/Kconfig.projbuild
```

Notes:

- **All files** must be explicitly listed in `apply:` to take effect. If these files exist under the amend root but are not included in the manifest, the generator will emit an `info` log such as `"present at amend root but not listed in apply"`, and the files themselves will be ignored.
- **Directory items are not supported**: each item must point to a specific file. If multiple files under a directory need to be applied, they must be listed individually (including subdirectory prefixes, e.g. `pack/foo.yaml`).
- Paths in `apply` are resolved against the directory containing `board_amend.yaml`. Relative and absolute paths are allowed.

#### `board_amend.yaml` schema (v1.0)

```yaml
version: "1.0"
description: "Example amend"

apply:                       # required, ordered list; later entries override earlier ones
  - tweak.yaml               # YAML fragment: top-level must contain devices: or peripherals:
  - strong_setup.c           # C/C++ source: compiled into the generated component
  - pack/pack_extra.yaml     # specific YAML inside a sub-directory
  - pack/pack_setup.c        # specific C inside a sub-directory
  - pack/sdkconfig.defaults.board
  - pack/Kconfig.projbuild
  - sdkconfig.defaults.board # amend-root file; must be listed here to take effect
  - Kconfig.projbuild        # same as above
```

Recognised file types:

| Recognised by | Files | Role |
|---|---|---|
| Suffix `.yaml` / `.yml` | YAML fragment | Merged into base devices / peripherals |
| Suffix `.c` / `.cpp` / `.cc` / `.cxx` / `.S` | C/C++ source | Compiled into the generated component |
| Suffix `.h` / `.hpp` | Header | Its parent directory is added to `INCLUDE_DIRS` |
| Basename `sdkconfig.defaults.board` | Sdkconfig defaults | Appended to `board_manager.defaults` in `apply:` order |
| Basename `Kconfig.projbuild` | Kconfig fragment | Appended to the generated `Kconfig.projbuild` in `apply:` order |
| Anything else (incl. directories) | — | Aborts with an error |

#### YAML merge rules

Every YAML fragment must define `devices:` or `peripherals:` (or both) at the top level; otherwise generation fails. Fragments are applied in `apply:` order, "later overrides earlier":

- Same-name device / peripheral entries are merged field-by-field. `config` uses a recursive deep merge, `peripherals` sub-lists are merged by `name`, other fields are replaced wholesale.
- Names that don't already exist are appended to the list.

#### Priority order

`sdkconfig.defaults.board` is appended in this order (later sections override earlier same-symbol entries; the older line is rewritten with a `BMGR_CONFIG_OVERRIDE` marker so the trail stays auditable):

1. Auto-generated Board Manager defaults section
2. Selected board's own `sdkconfig.defaults.board`
3. **Every `apply:` item whose basename is `sdkconfig.defaults.board`**, in manifest order (later items override earlier ones)

`Kconfig.projbuild` is appended as plain text (no override detection; symbol clashes are caught by IDF's menuconfig/build stage):

1. Auto-generated Board Manager Kconfig section
2. Selected board's `Kconfig.projbuild`
3. **Every `apply:` item whose basename is `Kconfig.projbuild`**, in manifest order

#### C/C++ sources and weak-symbol overrides

`.c / .cpp / .cc / .cxx / .S` files listed in `apply:` are added to the generated component via `target_sources(${COMPONENT_LIB} PRIVATE ...)` (each entry annotated with its `apply[]` index). Every source / header file's parent directory is added to `INCLUDE_DIRS` (de-duplicated).

The generated component has `idf_component_set_property(... WHOLE_ARCHIVE TRUE)`, so any **strong symbol** supplied by an amend fragment wins over a **weak symbol** with the same name in the base board — a typical pattern for overriding board hooks defined in `setup_device.c`.

## YAML Configuration Rules

For detailed YAML configuration rules and format specifications, please refer to [Device and Peripheral Rules](device_and_peripheral_rules.md).

## Board Selection Priority

When boards with the same name exist in different paths, ESP Board Manager follows a specific priority order to determine which board configuration to use:

**Priority Order (highest to lowest):**

1. **Custom Customer Path** (highest priority)
   - Boards specified via the `-c` parameter
   - Example: `idf.py bmgr -b my_board -c /path/to/custom/boards`

2. **User Project Components** (medium priority)
   - Boards in project components: `{PROJECT_ROOT}/components/{component_name}/boards`
   - These override boards with the same name in the main board directory

3. **Main Board Directory** (lowest priority)
   - Built-in boards: `esp_board_manager/boards`
   - Used as a fallback when no other version exists

**Important Notes:**
- **No Duplicate Warning**: The system silently uses the higher-priority board without warning about duplicates

## Custom `custom` Device Explanation

For devices and peripherals not yet included in esp_board_manager, it is recommended to add them via the `custom` type device.

- Place the device's `init` and `deinit` function code in the development board path, and register the device with the board manager using `CUSTOM_DEVICE_IMPLEMENT("device_name", init_func, deinit_func)`.

- Add the configuration for the custom device in the development board path `board_devices.yaml`, where the `type` needs to be configured as `custom`.

- After executing the command `idf.py bmgr -b xxx` (legacy: `idf.py gen-bmgr-config -b xxx`) to generate the development board's configuration code, the `gen_board_device_custom.h` header file will be generated in the `components/gen_bmgr_codes` path for use by the application.

- You can refer to the implementation method in [`m5stack_cores3`](../boards/m5stack_cores3/power_manager.c). M5stack_cores3 defines a power management chip using the custom type and implementing the init and deinit functions.

## Validating and Using the New Board

**Validate Board Configuration**

   ```bash
   # Test whether your board configuration is valid
   # Ensure that IDF_EXTRA_ACTIONS_PATH is set and still valid
   # For main board directory and user project components (default paths)
   idf.py bmgr -b <board_name>

   # For custom customer paths (external locations)
   idf.py bmgr -b <board_name> -c /path/to/custom/boards

   # Or use the standalone script
   cd $YOUR_ESP_BOARD_MANAGER_PATH
   python gen_bmgr_config_codes.py -b <board_name>
   python gen_bmgr_config_codes.py -b <board_name> -c /path/to/custom/boards
   ```

   **Verify Successful Generation**: If the board configuration is valid, the following files will be generated in the project path:

   - `components/gen_bmgr_codes/gen_board_periph_handles.c` - Peripheral handle definitions
   - `components/gen_bmgr_codes/gen_board_periph_config.c` - Peripheral configuration structures
   - `components/gen_bmgr_codes/gen_board_device_handles.c` - Device handle definitions
   - `components/gen_bmgr_codes/gen_board_device_config.c` - Device configuration structures
   - `components/gen_bmgr_codes/gen_board_info.c` - Board metadata
   - `components/gen_bmgr_codes/CMakeLists.txt` - Build system configuration
   - `components/gen_bmgr_codes/idf_component.yml` - Component dependencies

   **If Errors Occur**: Check your YAML files for syntax errors, missing fields, or invalid configurations.

> **Note**: When you run `idf.py bmgr` (or `idf.py gen-bmgr-config`) for the first time, the script will automatically generate the Kconfig menu for the selected board.
