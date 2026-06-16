# esp-bmgr-assist

PyPI package: `esp-bmgr-assist`
Python package: `esp_bmgr_py`

`esp-bmgr-assist` injects into the `idf.py` startup flow through a `.pth` hook, then discovers and loads the [ESP Board Manager](https://components.espressif.com/components/espressif/esp_board_manager) extension automatically.

PyPI 包名：`esp-bmgr-assist`
Python 包目录：`esp_bmgr_py`

`esp-bmgr-assist` 通过 `.pth` 注入 `idf.py` 启动流程，帮助工程自动发现并加载 [ESP Board Manager](https://components.espressif.com/components/espressif/esp_board_manager) 扩展。

## Features / 功能

- Automatically appends the detected board manager path to `IDF_EXTRA_ACTIONS_PATH`.
- Supports local and managed component discovery:
  - `components/esp_board_manager`
  - `components/espressif__esp_board_manager`
  - `managed_components/espressif__esp_board_manager`
- Supports first-use bootstrap on `idf.py bmgr ...` and the legacy `idf.py gen-bmgr-config ...`.
- Resolves the real project directory correctly for `-C` / `--project-dir`.
- Can locate board manager through direct dependencies or transitive dependencies resolved by the project dependency graph.

- 自动把可用的 board manager 路径加入 `IDF_EXTRA_ACTIONS_PATH`。
- 支持本地组件和托管组件发现：
  - `components/esp_board_manager`
  - `components/espressif__esp_board_manager`
  - `managed_components/espressif__esp_board_manager`
- 支持在 `idf.py bmgr ...` 和兼容旧命令 `idf.py gen-bmgr-config ...` 首次使用时自动自举。
- 支持正确处理 `-C` / `--project-dir` 等项目目录参数。
- 支持通过直接依赖或项目依赖图解析出的传递依赖定位 board manager。

## Installation / 安装

Install this package into the Python environment used by ESP-IDF.

请安装在 ESP-IDF 对应的 Python 虚拟环境中。

Install from PyPI:

```bash
pip install esp-bmgr-assist
```

Install from source:

```bash
pip install --no-build-isolation /path/to/esp-bmgr-py
```

Upgrade an existing installation from PyPI:

```bash
pip install --upgrade esp-bmgr-assist
```

Upgrade from a local checkout after new changes:

```bash
pip install --upgrade --no-build-isolation /path/to/esp-bmgr-py
```

Using `--no-build-isolation` avoids forcing a fresh setuptools/wheel download when you install
from a local checkout inside an existing ESP-IDF Python environment.

## Usage / 使用方法

Recommended command:

```bash
idf.py bmgr
```

Legacy command kept for compatibility:

```bash
idf.py gen-bmgr-config
```

Debug mode:

```bash
export ESP_BMGR_DEBUG=1
idf.py bmgr
```

Assist preflight is enabled by default for build-like commands and now blocks the command on
detected issues:

```bash
idf.py build
```

Change the level with a command-line parameter when needed:

```bash
idf.py --bmgr-preflight-level warning build
idf.py --bmgr-preflight-level silent build
```

You can also set a persistent environment variable:

```bash
export ESP_BMGR_ASSIST_PREFLIGHT=1
idf.py build
```

Supported levels:
- `2` / `error` / `abort`: stop the command immediately when issues are found (default)
- `1` / `warning` / `warn` / `true`: print warnings only
- `0` / `silent` / `off` / `false`: stay quiet

The source default lives in `esp_bmgr_py/preflight.py` as `DEFAULT_PREFLIGHT_LEVEL`, with
matching constants `PREFLIGHT_LEVEL_SILENT`, `PREFLIGHT_LEVEL_WARNING`, and
`PREFLIGHT_LEVEL_ERROR`.

Assist preflight can report:
- missing `components/gen_bmgr_codes/*` generated artifacts
- `CONFIG_IDF_TARGET` mismatch between `sdkconfig` and `board_manager.defaults`

The assist-side preflight currently focuses on build-like commands and intentionally skips
interactive/config-only commands such as `set-target`, `menuconfig`, `confserver`, and
`config-report`.

推荐命令：

```bash
idf.py bmgr
```

兼容旧命令：

```bash
idf.py gen-bmgr-config
```

调试模式：

```bash
export ESP_BMGR_DEBUG=1
idf.py bmgr
```

assist 侧预检查默认会在 build 一类命令上启用，发现问题时默认直接打断命令：

```bash
idf.py build
```

如需调整等级，可直接通过参数设置：

```bash
idf.py --bmgr-preflight-level warning build
idf.py --bmgr-preflight-level silent build
```

也可以通过环境变量长期配置：

```bash
export ESP_BMGR_ASSIST_PREFLIGHT=1
idf.py build
```

支持的等级如下：
- `2` / `error` / `abort`：发现问题立即中断命令，默认值
- `1` / `warning` / `warn` / `true`：只输出 warning
- `0` / `silent` / `off` / `false`：静默跳过输出

源码默认值在 `esp_bmgr_py/preflight.py` 里的 `DEFAULT_PREFLIGHT_LEVEL`，同时保留了
`PREFLIGHT_LEVEL_SILENT`、`PREFLIGHT_LEVEL_WARNING`、`PREFLIGHT_LEVEL_ERROR` 这几个固定常量，
后续改默认策略会比较直接。

开启时，assist 会额外检查：
- `components/gen_bmgr_codes/*` 生成物缺失
- `sdkconfig` 与 `board_manager.defaults` 的 `CONFIG_IDF_TARGET` 不匹配

当前 assist 预检查只针对 build 一类命令，不会对 `set-target`、`menuconfig`、
`confserver`、`config-report` 这类交互/配置命令输出这些 warning。

## Compatibility And Known Issues / 兼容与已知问题

### Older package behavior / 旧版本行为

Starting from `0.3.1`, this package no longer declares core dependencies managed by ESP-IDF itself, such as `idf-component-manager`. This avoids accidentally upgrading key packages inside the IDF environment during `pip install`.

从 `0.3.1` 起，本包不再声明 `idf-component-manager` 等由 ESP-IDF 自身管理的核心依赖，避免在 `pip install` 时误升级 IDF 环境中的关键包。

If you installed `0.3.0` before and later saw `check-python-dependencies` failures, repair the current IDF environment first, then reinstall a newer `esp-bmgr-assist`.

如果之前安装过 `0.3.0`，之后出现 `check-python-dependencies` 失败，请先修复当前 IDF 环境，再重新安装更新版本的 `esp-bmgr-assist`。

### Local Source Install / 本地源码安装

For local checkout installs, prefer:

```bash
pip install --no-build-isolation /path/to/esp-bmgr-py
```

This reuses the current ESP-IDF Python environment instead of asking pip to create an isolated
build environment and download a newer setuptools/wheel pair first.

本地源码安装建议优先使用：

```bash
pip install --no-build-isolation /path/to/esp-bmgr-py
```

这样会直接复用当前 ESP-IDF Python 环境，避免 `pip` 先创建隔离构建环境并额外下载新的
`setuptools` / `wheel`。

### `InvalidVersion` / `4.0.0-unsupported`

This is usually caused by broken third-party package metadata already present in the current Python environment, often in mixed or Conda-based environments. It is not caused by `esp-bmgr-assist` itself.

这通常是当前 Python 环境里已有第三方包的元数据不符合 PEP 440，常见于混用或 Anaconda 环境，不是 `esp-bmgr-assist` 自身逻辑导致的。

Recommended actions:

1. Use the official ESP-IDF Python environment.
2. Reinstall or remove the package named in the error log.
3. Create a clean virtual environment if needed.

建议按下面顺序处理：

1. 优先使用 ESP-IDF 官方 Python 虚拟环境。
2. 重装或卸载报错中点名的包。
3. 必要时新建干净的虚拟环境后再安装。

### `IDF_EXTRA_ACTIONS_PATH` separator / 多路径分隔符

Use `;` as the separator for multiple `IDF_EXTRA_ACTIONS_PATH` entries, matching ESP-IDF behavior on all platforms. Do not use `:`.

多个 `IDF_EXTRA_ACTIONS_PATH` 路径请使用分号 `;` 分隔，与 ESP-IDF 在各平台上的行为保持一致。不要使用 `:`。

### Transitive bmgr dependency with no target info / 间接依赖 bmgr 且缺少 target 信息

When the project depends on `esp_board_manager` **transitively** (i.e. `main/idf_component.yml` does not declare `espressif/esp_board_manager` directly, but some upstream package pulls it in), the assist has to resolve the full project dependency graph to discover `esp_board_manager`. That resolution is driven by `IDF_TARGET`.

If the assist cannot detect a target from the environment or the project files (no `IDF_TARGET` / `ESP_BMGR_BOOTSTRAP_TARGET` env, no `CONFIG_IDF_TARGET` in `sdkconfig` / `sdkconfig.defaults` / `components/gen_bmgr_codes/board_manager.defaults`), it falls back to `esp32`. If any other component in the project graph is incompatible with `esp32` (for example, target-gated packages that only support `esp32s3` or `esp32p4`), version solving fails here even though `esp_board_manager` itself is target-agnostic.

Two workarounds when you know the project actually depends on `esp_board_manager`:

1. Run `idf.py set-target <target>` first. This populates `sdkconfig` with your real target and downloads all components under it; subsequent assist runs will pick the correct target up from `sdkconfig` instead of falling back to `esp32`.
2. Or pass a valid target inline for a single invocation:

   ```bash
   IDF_TARGET=esp32s3 idf.py bmgr -l
   IDF_TARGET=esp32s3 idf.py bmgr -b <board_name>
   IDF_TARGET=esp32p4 idf.py gen-bmgr-config -b <board_name>
   ```

   `ESP_BMGR_BOOTSTRAP_TARGET` works the same way if you prefer a name that is scoped to the assist only.

If the project declares `espressif/esp_board_manager` **directly** in `main/idf_component.yml`, the assist uses a minimal-manifest download path that only fetches bmgr; that path is target-agnostic and is not affected by this issue.

当项目是**间接依赖** `esp_board_manager`（即 `main/idf_component.yml` 没有直接声明 `espressif/esp_board_manager`，而是通过某个上游包把 bmgr 传进来）时，assist 需要先把整个项目依赖图解出来，才能识别出 `esp_board_manager`。这一步解析受 `IDF_TARGET` 影响。

如果 assist 从环境变量和项目文件里都拿不到 target（即：没设 `IDF_TARGET` / `ESP_BMGR_BOOTSTRAP_TARGET`，`sdkconfig` / `sdkconfig.defaults` / `components/gen_bmgr_codes/board_manager.defaults` 里也没有 `CONFIG_IDF_TARGET`），就会回退到 `esp32`。如果项目依赖图里还有**别的组件**跟 `esp32` 不兼容（例如只支持 `esp32s3` 或 `esp32p4` 的目标受限包），解依赖就会在这里挂掉——即便 `esp_board_manager` 本身不挑 target。

当你确定项目确实依赖 `esp_board_manager` 时，两种绕过办法：

1. 先跑一次 `idf.py set-target <target>`，让项目真实的 target 写进 `sdkconfig` 并把组件都下载下来；后续 assist 会从 `sdkconfig` 读到正确的 target，不再回退到 `esp32`。
2. 或者在执行命令时临时带上有效的 target：

   ```bash
   IDF_TARGET=esp32s3 idf.py bmgr -l
   IDF_TARGET=esp32s3 idf.py bmgr -b <board_name>
   IDF_TARGET=esp32p4 idf.py gen-bmgr-config -b <board_name>
   ```

   也可以用 assist 专用的 `ESP_BMGR_BOOTSTRAP_TARGET`，作用一致，但仅对 assist 生效。

如果项目在 `main/idf_component.yml` 里**直接声明了** `espressif/esp_board_manager`，assist 会走"只含 bmgr 一条的临时 manifest"的最小化下载路径，该路径对 target 不敏感，不会踩到这个问题。

### Options not recognized after switching IDF versions / 切换 IDF 版本后选项不识别

After switching to a different IDF version, `idf.py bmgr` may report unrecognized options, for example:

```
>> idf.py bmgr -l
Usage: idf.py bmgr [OPTIONS]
Try 'idf.py bmgr --help' for help.

Error: No such option: -l
```

Each ESP-IDF version uses an isolated Python virtual environment. After switching versions, `esp-bmgr-assist` may not be installed in the new environment.

Check whether the package is present:

```bash
pip show esp-bmgr-assist
```

If no package information is shown, reinstall or upgrade it in the current IDF environment:

```bash
python3 -m pip install --upgrade esp-bmgr-assist
```

切换 IDF 版本后，`idf.py bmgr` 可能出现选项不识别的报错，例如：

```
>> idf.py bmgr -l
Usage: idf.py bmgr [OPTIONS]
Try 'idf.py bmgr --help' for help.

Error: No such option: -l
```

ESP-IDF 各版本使用独立的 Python 虚拟环境，切换版本后新环境中可能未安装 `esp-bmgr-assist`。

检查当前 IDF Python 环境中是否存在该包：

```bash
pip show esp-bmgr-assist
```

若未显示包信息，在当前 IDF 环境中重新安装或升级：

```bash
python3 -m pip install --upgrade esp-bmgr-assist
```
