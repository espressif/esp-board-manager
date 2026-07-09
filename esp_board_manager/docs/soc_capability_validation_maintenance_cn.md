# BMGR SoC 能力校验维护指南

本文说明 ESP Board Manager 当前的 SoC 能力校验实现，以及新增或调整校验规则时需要维护的文件。SoC 能力校验只处理芯片硬件事实，例如外设是否存在、普通 SoC GPIO 是否可用、实例数量和硬件字段上限。

## 实现路径

BMGR 的 SoC 能力校验分为四个环节：

1. 维护能力源文件：`private_inc/esp_board_soc_requirements.yml`。
2. 生成静态 catalog：`private_inc/soc_capability_catalog/index.json` 和 `idf_*.json`。
3. 在 parser dispatch 前执行 YAML 级统一校验。
4. 在 metadata 写出前执行普通 SoC GPIO 校验。

生成流程启动时，`gen_bmgr_config_codes.py` 会读取板级 `chip` 和当前 ESP-IDF 版本，并通过 `configure_soc_capabilities()` 选择唯一 catalog profile。随后：

- `process_peripherals()` 在调用具体 `periph_*.py` 前校验展开后的 peripheral YAML。
- `process_devices()` 在调用具体 `dev_*.py` 前校验展开后的 device YAML。
- `BoardMetadataGenerator.write_metadata_file()` 在写出 metadata 前校验 parser 提取出的 `io` 字段。

## 能力源文件

`private_inc/esp_board_soc_requirements.yml` 是 SoC 能力的人工维护入口。该文件包含以下分节：

- `devices`：定义 device 能力 key、所需 SoC 宏，以及匹配哪些 board device 字段。
- `peripherals`：定义 peripheral 能力 key、所需 SoC 宏，以及匹配哪些 board peripheral 字段。
- `capabilities`：定义独立布尔能力 key，例如 `i2s.supports_pdm_rx_hp_filter`。
- `gpio`：声明推导合法输入、输出 GPIO 的 SoC 宏名。
- `hardware_limits`：定义数值型 SoC 事实，例如实例数量、通道数量、总线宽度上限。

`devices` 和 `peripherals` 使用 board YAML 里的字段做匹配。`type` 是保留字段，`sub_type`、`role`、`format` 等字段是选择条件。

```yaml
peripherals:
  - i2s_master_pdm-in:
      requires: SOC_I2S_SUPPORTS_PDM_RX
      match: { type: i2s, format: [pdm-in] }
```

`requires` 可以是字符串或列表。列表表示所有宏都必须满足。需要组合条件时使用 `allOf` 和 `anyOf`。

```yaml
devices:
  - display_lcd_parlio:
      allOf:
        - SOC_PARLIO_SUPPORTED
      anyOf:
        - SOC_PARLIO_LCD_SUPPORTED
        - SOC_PARLIO_SUPPORT_SPI_LCD
        - SOC_PARLIO_SUPPORT_I80_LCD
      match: { type: display_lcd, sub_type: parlio }
```

## Catalog 生成

`export_soc_capability_catalog.py` 读取 `esp_board_soc_requirements.yml` 和 ESP-IDF 头文件，生成 catalog。常用命令如下：

```bash
python3 esp_board_manager/export_soc_capability_catalog.py \
  --idf-profile 5.4=/path/to/esp-idf-v5.4 \
  --idf-profile 5.5=/path/to/esp-idf-v5.5 \
  --output esp_board_manager/private_inc/soc_capability_catalog \
  --strict
```

如果没有传入 `--chip`，脚本会从各个 IDF profile 的 `components/soc` 目录发现芯片。修改能力源文件后应重新生成 catalog，并检查以下内容：

- `index.json` 包含预期 profile，且 profile version 可被当前 ESP-IDF 版本选中。
- `idf_*.json` 的 `schemaVersion` 为当前 schema。
- 新增的 device 或 peripheral 能力进入 `capabilityDefs`。
- 新增的数值校验绑定进入 `hardwareLimitDefs`。
- 目标芯片的 `chips.<chip>.capabilities`、`chips.<chip>.hardwareLimits`、`gpio` 和（若支持 ADC）`adcChannelMap` 结果符合预期。

## ADC 通道到 GPIO 映射

`export_soc_capability_catalog.py` 会从每个 profile 对应 ESP-IDF 树中的 `components/soc/{chip}/include/soc/adc_channel.h` 解析 `ADC{unit}_CHANNEL_{channel}_GPIO_NUM` 宏，并写入 `chips.<chip>.adcChannelMap`。该字段是查找表，不是 `hardwareLimitDefs` 规则；`esp_board_soc_requirements.yml` 中无需单独配置。

运行时 `generators/adc_channel_mapper.py` 通过 `configure_soc_capabilities()` 已选中的 catalog profile 查询映射，供 `periph_adc` / `dev_audio_codec` 的 metadata IO 提取使用。生成流程必须先完成 catalog 配置；不再在运行时读取 `IDF_PATH` 下的 `adc_channel.h`。

## YAML 级统一校验

`generators/utils/soc_capability_validator.py` 是 parser 前置统一校验器。输入不是 parser 生成后的结构体，而是 include、amend、flatten 后的 board YAML 条目。

统一校验器当前处理三类规则：

- 能力可用性：根据 `capabilityDefs` 匹配 YAML 条目，再检查目标芯片是否支持对应 capability。
- 数值上限：根据 `hardwareLimitDefs.*.appliesTo` 校验字段值、数组长度或实例数量。
- 带 `[IO]` 标签的字段：校验普通 SoC GPIO 是否在目标芯片输入或输出集合内。实际主路径仍是 metadata 级 IO 校验。

如果板级配置有意使用暂未建模的硬件能力，或者确认错误为误报，可临时跳过 parser 前置的 YAML 级 SoC 能力校验：

```bash
idf.py bmgr -b <board> --skip-soc-capability-check
export ESP_BOARD_MANAGER_SKIP_SOC_CAPABILITY_CHECK=1
```

该跳过选项只影响 `process_peripherals()` 和 `process_devices()` 调用 parser 前的统一 validator，不会跳过 metadata 级 IO 校验、parser 内语义校验或 sdkconfig 一致性检查。发布前仍应修正 board YAML、`esp_board_soc_requirements.yml` 或 catalog。

`hardware_limits` 中的每个 key 可以包含 `sources` 和 `applies_to`。`sources` 负责从 ESP-IDF 中提取归一化后的数值；`applies_to` 负责说明这个数值如何用于 board YAML 自动校验。

```yaml
lcd.rgb_data_width:
  sources:
    - kind: soc_caps_macro
      symbol: SOC_LCDCAM_RGB_DATA_WIDTH
  applies_to:
    - { kind: device, type: display_lcd, sub_type: [rgb], path: [data_width], check: value }
```

`sources` 常用字段如下：

- `kind: soc_caps_macro`：从 `soc_caps.h` 中读取宏值。
- `kind: header_define`：从 `path` 指向的头文件读取 `#define`。`path` 可包含 `{chip}`。
- `value`：`HardwareLimitSource` 的固定数值回退字段。它属于 `source` 项本身，不属于 `sources` 列表外层。

`applies_to` 的保留字段如下：

- `kind`：`device` 或 `peripheral`。
- `type`：BMGR device 或 peripheral 类型。
- `path`：以该 YAML 条目的 `config` 为根的字段路径。
- `check`：校验模式。
- `compare`：实际值与 limit 的比较关系。

除保留字段外，其余字段都是选择条件，例如 `sub_type`、`role`、`format`。

`check` 当前只支持以下取值：

- `value`：`path` 指向一个或多个数值。校验器接受整数、十进制字符串，以及最后一段为 `_<数字>` 的枚举字符串。
- `arrayLength`：`path` 指向数组本身，校验数组长度。
- `instanceCount`：统计匹配 `kind`、`type` 和选择条件的 YAML 条目数量。该模式不需要 `path`，也不支持按字段去重。

I2C 和 I2S 的实例数量不使用通用 `instanceCount` 语义。I2S 按 `config.port` 去重，缺省 `port` 按 `0` 处理；I2C 需要根据 `i2c.instance_count`、`i2c.hp_instance_count` 和 `i2c.lp_instance_count` 解析 HP/LP 端口并分别计数。这类外设应在 `generators/utils/soc_capability_validator.py` 的特殊校验逻辑中处理，不应通过 `applies_to` 增加通用计数规则。

`compare` 支持以下取值：

- `le`：实际值小于等于 limit。默认值。
- `lt`：实际值小于 limit。常用于从 0 开始编号的 unit、channel、slot。
- `ge`：实际值大于等于 limit。
- `gt`：实际值大于 limit。
- `eq`：实际值等于 limit。
- `ne`：实际值不等于 limit。

`unitIndex` 和 `arrayUnitIndex` 已废弃，不能再新增使用。需要校验 unit、channel、slot 这类从 0 开始编号的字段时，使用 `check: value` 和 `compare: lt`。

## Metadata 级 IO 校验

parser 生成 `struct_init` 后，`BoardMetadataGenerator` 会根据每个模块的 IO 描述或自定义 extractor 提取 metadata 中的 `devices.*.io` 与 `peripherals.*.io`。写出 metadata 前，`validate_metadata_io()` 会调用当前 SoC catalog 的 GPIO 查询接口校验普通 SoC GPIO。

该阶段适合处理最终生成代码实际使用到的普通 SoC GPIO。以下内容不属于普通 SoC GPIO 校验：

- `-1`、`GPIO_NUM_NC` 等未连接值。
- GPIO expander 上的引脚编号。
- ADC channel 到 GPIO 的映射。
- 板级固定连接、LDO 或外部芯片内部信号。

## Parser 内保留逻辑

统一 validator 只接收可由 board YAML 字段和 catalog 规则表达的 SoC 校验。parser 内仍应保留以下逻辑：

- 非 SoC 语义校验，例如字段互斥、引用关系、默认值推导、枚举合法性。
- 生成布局决策，例如 `i2s.hw_version` 控制不同 ESP-IDF 结构体字段布局。
- 字段是否生成或参数如何生成的专属逻辑，例如 `i2s.supports_pdm_rx_hp_filter` 控制 PDM RX HP filter 字段。
- ESP-IDF API 或组件版本兼容规则。
- 不能用简单数值上限表达的 SoC 规则。ADC channel 到 GPIO 的映射已导出到 catalog 的 `adcChannelMap`，由 metadata 提取阶段消费，不进入统一 YAML validator 的 `hardwareLimitDefs`。

没有 `applies_to` 的 `hardware_limits` 只进入 `chips.<chip>.hardwareLimits`，不会触发自动 YAML 校验。这类值可以继续由 parser 查询，用于生成布局或专属逻辑。若后续确认某个值是纯 board YAML 上限，应补充 `applies_to` 后再删除 parser 内重复校验。

## 新增规则流程

新增或调整 SoC 校验时，按以下顺序维护：

1. 确认规则是否属于芯片硬件事实。ESP-IDF API 字段、组件特性、板级业务语义不要放入 SoC catalog。
2. 在 `esp_board_soc_requirements.yml` 中新增或调整 `devices` / `peripherals` / `capabilities` / `hardware_limits`。
3. 如果是数值型自动校验，给对应 `hardware_limits` 增加 `applies_to`。路径必须以目标 YAML 条目的 `config` 为根。
4. 重新运行 `export_soc_capability_catalog.py` 生成 catalog。
5. 增加或更新测试。
6. 确认统一 validator 已覆盖对应行为后，再删除 parser 内重复的纯 SoC 校验。

测试建议如下：

- `generators/tests/test_soc_capabilities.py`：覆盖 catalog 生成、profile 选择、schema 和 source 解析。
- `generators/tests/test_adc_channel_map_parser.py`：覆盖 `adc_channel.h` 解析与 `adcChannelMap` round-trip。
- `generators/tests/test_adc_channel_mapper.py`：覆盖 catalog 驱动的 ADC channel 映射。
- `generators/tests/test_soc_capability_validator.py`：覆盖 `capabilityDefs`、`hardwareLimitDefs`、path 解析、`check` 和 `compare` 行为。
- `test_apps/test_scripts/test_soc_capability_availability.py`：覆盖生成流程中的 parser 前置 SoC 能力错误。
- 具体外设或设备已有兼容测试：确认删除 parser 重复校验后，错误仍在统一阶段被捕获。

常用回归命令：

```bash
PYTHONDONTWRITEBYTECODE=1 pytest -q \
  esp_board_manager/generators/tests/test_soc_capabilities.py \
  esp_board_manager/generators/tests/test_adc_channel_map_parser.py \
  esp_board_manager/generators/tests/test_adc_channel_mapper.py \
  esp_board_manager/generators/tests/test_soc_capability_validator.py \
  esp_board_manager/generators/tests/test_soc_capability_query.py \
  esp_board_manager/test_apps/test_scripts/test_soc_capability_availability.py \
  -p no:cacheprovider
```

## 迁移边界

删除 parser 内 SoC 校验前，需要满足以下条件：

- 对应 capability 或 hardware limit 已进入 catalog。
- 对应 `applies_to` 能准确匹配 board YAML 字段。
- 测试覆盖了合法值、越界值和不支持芯片。
- 错误发生在 parser dispatch 前，且错误信息能定位到 board YAML 条目。

以下内容不要为了收敛而迁入统一 validator：

- codegen layout 决策。
- ESP-IDF API 版本门禁。
- 设备、外设之间的引用关系。
- 非普通 SoC GPIO。
- 需要专用结构化数据才能表达的 SoC 规则。
