"""
Tests for parser edge cases and generation behavior regressions.
"""

from pathlib import Path
import re
import sys

import pytest

def test_adc_continuous_patterns_reject_single_unit_conv_mode_for_mixed_units(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_adc import periph_adc as mod

    with pytest.raises(ValueError, match='conv_mode cannot be single-unit'):
        mod.parse(
            'adc_audio_in',
            {
                'role': 'continuous',
                'config': {
                    'patterns': [
                        {
                            'unit': 'ADC_UNIT_1',
                            'channel': 4,
                            'atten': 'ADC_ATTEN_DB_0',
                            'bit_width': 'ADC_BITWIDTH_DEFAULT',
                        },
                        {
                            'unit': 'ADC_UNIT_2',
                            'channel': 0,
                            'atten': 'ADC_ATTEN_DB_0',
                            'bit_width': 'ADC_BITWIDTH_DEFAULT',
                        },
                    ],
                    'conv_mode': 'ADC_CONV_SINGLE_UNIT_1',
                },
            },
        )

def test_adc_oneshot_rejects_list_channel_id(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_adc import periph_adc as mod

    with pytest.raises(ValueError, match='must be a single integer'):
        mod.parse(
            'adc_oneshot',
            {
                'role': 'oneshot',
                'config': {
                    'unit_id': 'ADC_UNIT_1',
                    'channel_id': [4],
                },
            },
        )

def test_adc_oneshot_accepts_numeric_bit_width(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_adc import periph_adc as mod

    result = mod.parse(
        'adc_oneshot',
        {
            'role': 'oneshot',
            'config': {
                'unit_id': 'ADC_UNIT_1',
                'channel_id': 4,
                'bit_width': 17,
            },
        },
    )

    chan_cfg = result['struct_init']['cfg']['oneshot']['chan_cfg']
    assert chan_cfg['bitwidth'] == 17


@pytest.mark.parametrize('bit_width', [True, -1])
def test_adc_oneshot_rejects_invalid_numeric_bit_width(bmgr_root, bit_width):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_adc import periph_adc as mod

    with pytest.raises(ValueError, match='Invalid ADC bit width value'):
        mod.parse(
            'adc_oneshot',
            {
                'role': 'oneshot',
                'config': {
                    'unit_id': 'ADC_UNIT_1',
                    'channel_id': 4,
                    'bit_width': bit_width,
                },
            },
        )

def test_adc_continuous_single_unit_accepts_matching_explicit_conv_mode(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_adc import periph_adc as mod

    result = mod.parse(
        'adc_audio_in',
        {
            'role': 'continuous',
            'config': {
                'unit_id': 'ADC_UNIT_1',
                'atten': 'ADC_ATTEN_DB_0',
                'bit_width': 'ADC_BITWIDTH_DEFAULT',
                'channel_list': [4],
                'conv_mode': 'ADC_CONV_SINGLE_UNIT_1',
            },
        },
    )

    assert result['struct_init']['cfg']['continuous']['conv_mode'] == 'ADC_CONV_SINGLE_UNIT_1'


def test_adc_continuous_accepts_numeric_bit_widths(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_adc import periph_adc as mod

    single_unit_result = mod.parse(
        'adc_audio_in',
        {
            'role': 'continuous',
            'config': {
                'unit_id': 'ADC_UNIT_1',
                'channel_list': [4],
                'bit_width': 17,
            },
        },
    )
    single_unit_cfg = single_unit_result['struct_init']['cfg']['continuous']['cfg']['single_unit']
    assert single_unit_cfg['bit_width'] == 17

    pattern_result = mod.parse(
        'adc_audio_in',
        {
            'role': 'continuous',
            'config': {
                'patterns': [
                    {
                        'unit': 'ADC_UNIT_1',
                        'channel': 4,
                        'bit_width': 17,
                    },
                ],
            },
        },
    )
    patterns_cfg = pattern_result['struct_init']['cfg']['continuous']['cfg']['patterns']
    assert patterns_cfg[0]['bit_width'] == 17


def test_fs_fat_sdmmc_accepts_zero_slot_flags(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_fs_fat import dev_fs_fat as mod

    result = mod.parse(
        'fs_sdcard',
        {
            'type': 'fs_fat',
            'sub_type': 'sdmmc',
            'config': {
                'sub_config': {
                    'slot_flags': 0,
                },
            },
        },
    )

    assert result['struct_init']['sub_cfg']['sdmmc']['slot_flags'] == 0


def test_littlefs_flash_parses_native_vfs_config(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_littlefs import dev_littlefs as mod

    result = mod.parse(
        'littlefs',
        {
            'type': 'littlefs',
            'sub_type': 'flash',
            'config': {
                'vfs_config': {
                    'base_path': '/littlefs',
                    'partition_label': 'storage',
                    'format_if_mount_failed': False,
                    'read_only': False,
                    'dont_mount': False,
                    'grow_on_mount': True,
                },
            },
        },
    )

    littlefs_cfg = result['struct_init']['vfs_config']
    assert littlefs_cfg['base_path'] == '/littlefs'
    assert littlefs_cfg['partition_label'] == 'storage'
    assert littlefs_cfg['partition'] is None
    assert littlefs_cfg['grow_on_mount'] is True


def test_littlefs_sdmmc_accepts_zero_slot_flags(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_littlefs import dev_littlefs as mod

    result = mod.parse(
        'littlefs',
        {
            'type': 'littlefs',
            'sub_type': 'sdmmc',
            'config': {
                'vfs_config': {
                    'base_path': '/littlefs',
                    'partition_label': 'ignored_for_sd',
                },
                'sub_config': {
                    'frequency': 'SDMMC_FREQ_DEFAULT',
                    'slot_flags': 0,
                },
            },
        },
    )

    littlefs_cfg = result['struct_init']['vfs_config']
    assert littlefs_cfg['partition_label'] is None
    assert result['struct_init']['sub_cfg']['sdmmc']['slot_flags'] == 0


def test_littlefs_spi_resolves_spi_master_peripheral(bmgr_root):
    from types import SimpleNamespace

    sys.path.insert(0, str(bmgr_root))
    from devices.dev_littlefs import dev_littlefs as mod

    result = mod.parse(
        'littlefs',
        {
            'type': 'littlefs',
            'sub_type': 'spi',
            'peripherals': [
                {
                    'name': 'sd_spi_bus',
                },
            ],
            'config': {
                'sub_config': {
                    'cs_gpio_num': 10,
                    'frequency': 'SDMMC_FREQ_DEFAULT',
                },
            },
        },
        {
            'sd_spi_bus': SimpleNamespace(type='spi', role='master'),
        },
    )

    spi_cfg = result['struct_init']['sub_cfg']['spi']
    assert spi_cfg['cs_gpio_num'] == 10
    assert spi_cfg['spi_bus_name'] == 'sd_spi_bus'


def test_littlefs_spi_requires_peripheral_context(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_littlefs import dev_littlefs as mod

    with pytest.raises(ValueError, match='peripheral validation context'):
        mod.parse(
            'littlefs',
            {
                'type': 'littlefs',
                'sub_type': 'spi',
                'peripherals': [{'name': 'sd_spi_bus'}],
                'config': {},
            },
        )


def test_littlefs_spi_reports_bad_peripheral_reference(bmgr_root):
    from types import SimpleNamespace

    sys.path.insert(0, str(bmgr_root))
    from devices.dev_littlefs import dev_littlefs as mod

    with pytest.raises(ValueError, match='checked references'):
        mod.parse(
            'littlefs',
            {
                'type': 'littlefs',
                'sub_type': 'spi',
                'peripherals': [{'name': 'i2c_master'}],
                'config': {},
            },
            {
                'i2c_master': SimpleNamespace(type='i2c', role='master'),
            },
        )


def test_littlefs_spi_reports_missing_peripheral_reference(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_littlefs import dev_littlefs as mod

    with pytest.raises(ValueError, match='missing references'):
        mod.parse(
            'littlefs',
            {
                'type': 'littlefs',
                'sub_type': 'spi',
                'peripherals': [{'name': 'sd_spi_bus'}],
                'config': {},
            },
            {},
        )


def test_littlefs_rejects_runtime_handles(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_littlefs import dev_littlefs as mod

    with pytest.raises(ValueError, match='runtime handle fields'):
        mod.parse(
            'littlefs',
            {
                'type': 'littlefs',
                'sub_type': 'flash',
                'config': {
                    'vfs_config': {
                        'base_path': '/littlefs',
                        'sdcard': 'card',
                    },
                },
            },
        )


def test_littlefs_rejects_read_only_format_or_grow(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_littlefs import dev_littlefs as mod

    with pytest.raises(ValueError, match='format_if_mount_failed'):
        mod.parse(
            'littlefs',
            {
                'type': 'littlefs',
                'sub_type': 'flash',
                'config': {
                    'vfs_config': {
                        'read_only': True,
                        'format_if_mount_failed': True,
                    },
                },
            },
        )

    with pytest.raises(ValueError, match='grow_on_mount'):
        mod.parse(
            'littlefs',
            {
                'type': 'littlefs',
                'sub_type': 'flash',
                'config': {
                    'vfs_config': {
                        'read_only': True,
                        'grow_on_mount': True,
                    },
                },
            },
        )


def test_peripheral_yaml_rejects_duplicate_names(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from generators.peripheral_parser import PeripheralParser
    from generators.utils.yaml_utils import BoardConfigYamlError

    periph_yaml = tmp_path / 'board_peripherals.yaml'
    periph_yaml.write_text(
        '''
peripherals:
  - name: gpio_power
    type: gpio
    config: {}
  - name: gpio_power
    type: gpio
    config: {}
''',
        encoding='utf-8',
    )

    parser = PeripheralParser(bmgr_root)
    with pytest.raises(BoardConfigYamlError, match="duplicate peripheral name 'gpio_power'"):
        parser.parse_peripherals_yaml(str(periph_yaml))


def test_device_yaml_rejects_duplicate_names(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from generators.device_parser import DeviceParser
    from generators.utils.yaml_utils import BoardConfigYamlError

    device_yaml = tmp_path / 'board_devices.yaml'
    device_yaml.write_text(
        '''
devices:
  - name: display_lcd
    type: display_lcd
    config: {}
  - name: display_lcd
    type: display_lcd
    config: {}
''',
        encoding='utf-8',
    )

    parser = DeviceParser(bmgr_root)
    with pytest.raises(BoardConfigYamlError, match="duplicate device name 'display_lcd'"):
        parser.parse_devices_yaml_legacy(str(device_yaml), peripherals_dict={})


def test_adc_continuous_single_unit_accepts_legacy_channel_id(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_adc import periph_adc as mod

    result = mod.parse(
        'adc_audio_in',
        {
            'role': 'continuous',
            'config': {
                'unit_id': 'ADC_UNIT_1',
                'channel_id': [4],
            },
        },
    )

    single_unit = result['struct_init']['cfg']['continuous']['cfg']['single_unit']
    assert single_unit['channel_id'] == [4]


def test_adc_continuous_single_unit_rejects_channel_list_and_legacy_channel_id(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_adc import periph_adc as mod

    with pytest.raises(ValueError, match='channel_list.*legacy.*channel_id'):
        mod.parse(
            'adc_audio_in',
            {
                'role': 'continuous',
                'config': {
                    'unit_id': 'ADC_UNIT_1',
                    'channel_list': [4],
                    'channel_id': [4],
                },
            },
        )


def test_adc_schema_demotes_cross_role_fields_to_debug(bmgr_root, caplog):
    """Cross-role config keys (e.g. oneshot's `channel_id` in a continuous ADC)
    must NOT surface as user-facing WARNINGs — they are part of the type-wide
    union of valid ADC keys, so :class:`PeripheralSchemaValidator` reports
    them via a debug log instead. This avoids noisy false positives when an
    amend swaps role and keeps real typos visible as WARNINGs.
    """
    import logging

    sys.path.insert(0, str(bmgr_root))
    from generators.schema_validator import PeripheralSchemaValidator

    validator = PeripheralSchemaValidator(bmgr_root / 'peripherals')

    # `channel_id` belongs to oneshot's role-specific schema; supplying it for
    # role=continuous used to raise a WARNING. With the unified type-wide
    # validation policy we demote it to a debug log on the validator's logger.
    with caplog.at_level(logging.DEBUG, logger=validator.logger.name):
        continuous_issues = validator.validate_config(
            'adc',
            {'unit_id': 'ADC_UNIT_1', 'channel_id': [4]},
            'adc_audio_in',
            role='continuous',
        )
    assert continuous_issues == []
    debug_msgs = [r.getMessage() for r in caplog.records
                  if r.levelno == logging.DEBUG and 'channel_id' in r.getMessage()]
    assert debug_msgs, (
        f'Expected a debug message mentioning channel_id, got records: '
        f'{[(r.levelname, r.getMessage()) for r in caplog.records]}'
    )

    # `channel_id` is legitimate for role=oneshot — no warning, no demotion.
    oneshot_issues = validator.validate_config(
        'adc',
        {'unit_id': 'ADC_UNIT_1', 'channel_id': 4},
        'adc_oneshot',
        role='oneshot',
    )
    assert oneshot_issues == []

    # A genuinely unknown key (in neither role's schema) must still WARN with a fuzzy suggestion.
    typo_issues = validator.validate_config(
        'adc',
        {'unit_id': 'ADC_UNIT_1', 'channel_id_typoz': [4]},
        'adc_typoz',
        role='continuous',
    )
    assert len(typo_issues) == 1
    assert typo_issues[0].key_path == 'channel_id_typoz'

def test_adc_channel_mapper_rejects_zero_unit_string(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from generators.adc_channel_mapper import _normalize_unit

    with pytest.raises(ValueError, match='Unsupported ADC unit value: ADC_UNIT_0'):
        _normalize_unit('ADC_UNIT_0')

def test_i2c_rejects_lp_port_with_regular_clk_source(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_i2c import periph_i2c as mod

    with pytest.raises(ValueError, match='incompatible with LP I2C port'):
        mod.parse(
            'i2c_master',
            {
                'config': {
                    'port': 'LP_I2C_NUM_0',
                    'clk_source': 'I2C_CLK_SRC_DEFAULT',
                    'pins': {
                        'sda': 1,
                        'scl': 2,
                    },
                },
            },
        )

def test_i2c_rejects_regular_port_with_lp_clk_source(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_i2c import periph_i2c as mod

    with pytest.raises(ValueError, match='incompatible with regular I2C port'):
        mod.parse(
            'i2c_master',
            {
                'config': {
                    'port': 0,
                    'clk_source': 'LP_I2C_SCLK_DEFAULT',
                    'pins': {
                        'sda': 1,
                        'scl': 2,
                    },
                },
            },
        )

def test_i2c_basic_parse_returns_i2c_master_bus_config(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_i2c import periph_i2c as mod

    result = mod.parse(
        'i2c_master',
        {
            'config': {
                'port': 0,
                'pins': {
                    'sda': 18,
                    'scl': 23,
                },
            },
        },
    )

    assert result['struct_type'] == 'i2c_master_bus_config_t'
    assert result['struct_init']['i2c_port'] == 'I2C_NUM_0'
    assert result['struct_init']['sda_io_num'] == 18
    assert result['struct_init']['scl_io_num'] == 23

def test_spi_bus_dma_burst_size_is_idf61_only(bmgr_root, monkeypatch, capsys):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_spi import periph_spi as mod

    cfg = {
        'config': {
            'spi_bus_config': {
                'spi_port': 1,
                'mosi_io_num': 1,
                'miso_io_num': 2,
                'sclk_io_num': 3,
                'dma_burst_size': 64,
            },
        },
    }

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (6, 1, 0))
    result = mod.parse('spi_master', cfg)
    assert result['struct_init']['spi_bus_config']['dma_burst_size'] == 64

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (6, 0, 1))
    result = mod.parse('spi_master', cfg)
    assert 'dma_burst_size' not in result['struct_init']['spi_bus_config']
    assert 'requires ESP-IDF v6.1' in capsys.readouterr().out

def test_dsi_clock_lane_force_hs_is_idf6_only(bmgr_root, monkeypatch, capsys):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_dsi import periph_dsi as mod

    cfg = {
        'config': {
            'bus_id': 0,
            'data_lanes': 2,
            'phy_clk_src': 0,
            'lane_bit_rate_mbps': 1000,
            'flags': {
                'clock_lane_force_hs': True,
            },
        },
    }

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (6, 0, 0))
    result = mod.parse('dsi_panel', cfg)
    assert result['struct_init']['flags']['clock_lane_force_hs'] is True

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (5, 5, 4))
    result = mod.parse('dsi_panel', cfg)
    assert 'flags' not in result['struct_init']
    assert 'requires ESP-IDF v6.0' in capsys.readouterr().out

def test_dependency_manager_treats_null_devices_as_empty(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from generators.dependency_manager import DependencyManager

    dev_yaml = tmp_path / 'board_devices.yaml'
    dev_yaml.write_text('# Devices Configuration\ndevices:\n', encoding='utf-8')

    manager = DependencyManager(bmgr_root)
    assert manager.extract_device_dependencies(str(dev_yaml)) == {}

def test_m5stack_tab5_display_dependencies_include_st7121(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from generators.dependency_manager import DependencyManager

    dev_yaml = bmgr_root / 'boards' / 'm5stack_tab5' / 'board_devices.yaml'

    manager = DependencyManager(bmgr_root)
    dependencies = manager.extract_device_dependencies(str(dev_yaml))

    assert dependencies['espressif/esp_lcd_st7121'] == '*'

def test_process_peripherals_rejects_unsupported_role_from_public_enum_list(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    generator = BoardConfigGenerator(bmgr_root)
    generator.set_project_dir(str(tmp_path))

    board_yaml = bmgr_root / 'test_apps' / 'components' / 'test_component_2' / 'path1' / 'test_board_c' / 'board_peripherals.yaml'
    with pytest.raises(ValueError, match="Unsupported peripheral role 'console'"):
        generator.process_peripherals(str(board_yaml))

def test_process_devices_codegen_emits_chip_in_descriptors_and_handles(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    periph_yaml = tmp_path / 'board_peripherals.yaml'
    periph_yaml.write_text('peripherals: []\n', encoding='utf-8')

    dev_yaml = tmp_path / 'board_devices.yaml'
    dev_yaml.write_text(
        """
devices:
  - name: motor_driver
    type: custom
    chip: mx16161
    config:
      test_int: 123
""".strip()
        + '\n',
        encoding='utf-8',
    )

    generator = BoardConfigGenerator(bmgr_root)
    generator.set_project_dir(str(tmp_path))

    peripherals_dict, periph_name_map, _ = generator.process_peripherals(str(periph_yaml))
    generator.process_devices(str(dev_yaml), peripherals_dict, periph_name_map)

    config_c = (tmp_path / 'components' / 'gen_bmgr_codes' / 'gen_board_device_config.c').read_text(encoding='utf-8')
    handles_c = (tmp_path / 'components' / 'gen_bmgr_codes' / 'gen_board_device_handles.c').read_text(encoding='utf-8')

    assert '.chip = "mx16161",' in config_c
    assert '.chip = "mx16161",' in handles_c

def test_camera_dvp_requires_i2c_peripheral(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_camera import dev_camera as mod

    with pytest.raises(ValueError, match='requires an i2c peripheral'):
        mod.parse(
            'camera',
            {
                'type': 'camera',
                'sub_type': 'dvp',
                'config': {
                    'dvp_config': {},
                },
                'peripherals': [],
            },
        )

def test_button_gpio_rejects_legacy_gpio_name_and_events_keys(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_button import dev_button as mod

    with pytest.raises(ValueError, match="Legacy 'events' is no longer supported"):
        mod.parse(
            'gpio_button_0',
            {
                'type': 'button',
                'sub_type': 'gpio',
                'config': {
                    'events': {
                        'press_down': True,
                    },
                    'gpio_name': 'gpio_button',
                },
                'peripherals': [
                    {'name': 'gpio_button'},
                ],
            },
        )

def test_button_gpio_requires_top_level_gpio_peripheral(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_button import dev_button as mod

    with pytest.raises(ValueError, match="Legacy 'config.gpio_name' is no longer supported"):
        mod.parse(
            'gpio_button_0',
            {
                'type': 'button',
                'sub_type': 'gpio',
                'config': {
                    'events_cfg': {
                        'press_down': True,
                    },
                    'gpio_name': 'gpio_button',
                },
            },
        )

def test_button_adc_requires_top_level_adc_peripheral(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_button import dev_button as mod

    with pytest.raises(ValueError, match="Legacy 'config.adc_name' is no longer supported"):
        mod.parse(
            'adc_button_0',
            {
                'type': 'button',
                'sub_type': 'adc_single',
                'config': {
                    'adc_name': 'adc_oneshot',
                    'button_index': 0,
                    'min_voltage': 0,
                    'max_voltage': 500,
                },
            },
        )

def test_camera_csi_allows_missing_ldo_when_dont_init_ldo_false(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_camera import dev_camera as mod

    result = mod.parse(
        'camera',
        {
            'type': 'camera',
            'sub_type': 'csi',
            'config': {
                'csi_config': {
                    'dont_init_ldo': False,
                },
            },
            'peripherals': [
                {'name': 'i2c_master', 'frequency': 400000},
            ],
        },
    )

    assert result['struct_init']['sub_cfg']['csi']['ldo_name'] == ''

def test_camera_csi_requires_ldo_when_dont_init_ldo_true(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_camera import dev_camera as mod

    with pytest.raises(ValueError, match='requires an ldo peripheral'):
        mod.parse(
            'camera',
            {
                'type': 'camera',
                'sub_type': 'csi',
                'config': {
                    'csi_config': {
                        'dont_init_ldo': True,
                    },
                },
                'peripherals': [
                    {'name': 'i2c_master', 'frequency': 400000},
                ],
            },
        )

def test_display_lcd_dsi_finds_dsi_and_ldo_without_order_dependency(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_display_lcd import dev_display_lcd as mod

    peripherals_dict = {
        'ldo_panel': object(),
        'dsi_panel': object(),
    }

    result = mod.parse(
        'display_lcd',
        {
            'name': 'display_lcd',
            'type': 'display_lcd',
            'sub_type': 'dsi',
            'config': {
                'dbi_config': {},
                'dpi_config': {},
            },
            'peripherals': [
                {'name': 'ldo_panel'},
                {'name': 'dsi_panel'},
            ],
        },
        peripherals_dict=peripherals_dict,
    )

    dsi_cfg = result['struct_init']['sub_cfg']['dsi']
    assert dsi_cfg['ldo_name'] == 'ldo_panel'
    assert dsi_cfg['dsi_name'] == 'dsi_panel'
    assert dsi_cfg['dpi_config']['in_color_format'] == 'LCD_COLOR_FMT_RGB565'

def test_display_lcd_spi_fallback_uses_generic_spi_prefix(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_display_lcd import dev_display_lcd as mod

    peripherals_dict = {
        'spi_bus_custom': object(),
    }

    result = mod.parse(
        'display_lcd',
        {
            'name': 'display_lcd',
            'type': 'display_lcd',
            'sub_type': 'spi',
            'config': {
                'io_spi_config': {},
            },
        },
        peripherals_dict=peripherals_dict,
    )

    assert result['struct_init']['sub_cfg']['spi']['spi_name'] == 'spi_bus_custom'

def test_display_lcd_spi_psram_dma_direct_is_idf61_only(bmgr_root, monkeypatch, capsys):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_display_lcd import dev_display_lcd as mod

    cfg = {
        'name': 'display_lcd',
        'type': 'display_lcd',
        'sub_type': 'spi',
        'config': {
            'io_spi_config': {
                'flags': {
                    'psram_dma_direct': True,
                },
            },
        },
        'peripherals': [
            {'name': 'spi_master'},
        ],
    }

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (6, 1, 0))
    result = mod.parse('display_lcd', cfg, peripherals_dict={'spi_master': object()})
    flags = result['struct_init']['sub_cfg']['spi']['io_spi_config']['flags']
    assert flags['psram_dma_direct'] is True

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (6, 0, 1))
    result = mod.parse('display_lcd', cfg, peripherals_dict={'spi_master': object()})
    flags = result['struct_init']['sub_cfg']['spi']['io_spi_config']['flags']
    assert 'psram_dma_direct' not in flags
    assert 'requires ESP-IDF v6.1' in capsys.readouterr().out

def test_display_lcd_i80_allow_pd_is_idf6_only(bmgr_root, monkeypatch, capsys):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_display_lcd import dev_display_lcd as mod

    cfg = {
        'name': 'display_lcd',
        'type': 'display_lcd',
        'sub_type': 'i80',
        'config': {
            'bus_config': {
                'flags': {
                    'allow_pd': True,
                },
            },
        },
    }

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (6, 0, 0))
    result = mod.parse('display_lcd', cfg)
    bus_cfg = result['struct_init']['sub_cfg']['i80']['bus_config']
    assert bus_cfg['flags']['allow_pd'] is True

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (5, 5, 4))
    result = mod.parse('display_lcd', cfg)
    bus_cfg = result['struct_init']['sub_cfg']['i80']['bus_config']
    assert 'flags' not in bus_cfg
    assert 'requires ESP-IDF v6.0' in capsys.readouterr().out

def test_display_lcd_parlio_rejects_known_idf_before_5_5(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_display_lcd import dev_display_lcd as mod

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (5, 4, 3))

    with pytest.raises(ValueError, match='parlio requires ESP-IDF v5.5'):
        mod.parse(
            'display_lcd',
            {
                'name': 'display_lcd',
                'type': 'display_lcd',
                'sub_type': 'parlio',
                'config': {},
            },
        )

def test_display_lcd_rgb_idf6_uses_color_formats_and_user_fbs_func(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_display_lcd import dev_display_lcd as mod

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (6, 0, 0))

    result = mod.parse(
        'display_lcd',
        {
            'name': 'display_lcd',
            'type': 'display_lcd',
            'sub_type': 'rgb',
            'config': {
                'rgb_panel_config': {
                    'data_width': 16,
                    'in_color_format': 'LCD_COLOR_FMT_RGB565',
                    'out_color_format': 'LCD_COLOR_FMT_RGB565',
                    'user_fbs_func': 'lcd_rgb_user_fbs',
                    'data_gpio_nums': [8, 9, 10, 11],
                    'timings': {
                        'h_res': 800,
                        'v_res': 480,
                    },
                },
            },
        },
    )

    rgb_cfg = result['struct_init']['sub_cfg']['rgb']
    panel_cfg = rgb_cfg['panel_config']
    assert rgb_cfg['user_fbs_func'] == 'lcd_rgb_user_fbs'
    assert panel_cfg['in_color_format'] == 'LCD_COLOR_FMT_RGB565'
    assert panel_cfg['out_color_format'] == 'LCD_COLOR_FMT_RGB565'
    assert 'bits_per_pixel' not in panel_cfg
    assert panel_cfg['data_gpio_nums'] == [8, 9, 10, 11]
    assert result['struct_init']['lcd_width'] == 800
    assert result['struct_init']['lcd_height'] == 480

def test_display_lcd_rgb_idf5_rejects_user_fbs_func(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_display_lcd import dev_display_lcd as mod

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (5, 5, 0))

    with pytest.raises(ValueError, match='user_fbs_func requires ESP-IDF v6.0'):
        mod.parse(
            'display_lcd',
            {
                'name': 'display_lcd',
                'type': 'display_lcd',
                'sub_type': 'rgb',
                'config': {
                    'rgb_panel_config': {
                        'user_fbs_func': 'lcd_rgb_user_fbs',
                    },
                },
            },
        )

def test_display_lcd_rgb_3wire_spi_parses_line_unions_and_reuses_rgb_config(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_display_lcd import dev_display_lcd as mod

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (5, 5, 0))

    result = mod.parse(
        'display_lcd',
        {
            'name': 'display_lcd',
            'type': 'display_lcd',
            'sub_type': 'rgb_3wire_spi',
            'config': {
                'io_3wire_spi_config': {
                    'io_expander_name': 'io_expander',
                    'line_config': {
                        'cs_io_type': 'IO_TYPE_EXPANDER',
                        'cs_expander_pin': 1,
                        'cs_gpio_num': 99,
                        'scl_io_type': 'IO_TYPE_GPIO',
                        'scl_gpio_num': 2,
                        'sda_io_type': 'IO_TYPE_GPIO',
                        'sda_gpio_num': 3,
                    },
                },
                'rgb_panel_config': {
                    'data_gpio_nums': [8, 9, 10, 11],
                    'timings': {
                        'h_res': 480,
                        'v_res': 480,
                    },
                },
                'lcd_panel_config': {
                    'vendor_config': 'auto',
                    'auto_del_panel_io': True,
                },
            },
        },
    )

    cfg = result['struct_init']['sub_cfg']['rgb_3wire_spi']
    line_config = cfg['io_3wire_spi_config']['line_config']
    assert cfg['io_expander_name'] == 'io_expander'
    assert line_config['cs_io_type'] == 'IO_TYPE_EXPANDER'
    assert line_config['cs_expander_pin'] == 1
    assert 'cs_gpio_num' not in line_config
    assert line_config['scl_gpio_num'] == 2
    assert line_config['sda_gpio_num'] == 3
    assert line_config['io_expander'] is None
    assert cfg['rgb_panel_config']['data_gpio_nums'] == [8, 9, 10, 11]
    assert cfg['panel_config']['vendor_config'] is None
    assert cfg['auto_del_panel_io'] is True
    assert result['struct_init']['lcd_width'] == 480
    assert result['struct_init']['lcd_height'] == 480

def test_lcd_touch_i2c_uses_8bit_addrs_and_i2c_sub_config(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_lcd_touch import dev_lcd_touch as mod

    result = mod.parse(
        'lcd_touch',
        {
            'type': 'lcd_touch',
            'chip': 'gt911',
            'sub_type': 'i2c',
            'config': {
                'io_i2c_config': {
                    'scl_speed_hz': 400000,
                },
                'touch_config': {
                    'x_max': 480,
                    'y_max': 320,
                },
            },
            'peripherals': [
                {'name': 'i2c_master', 'i2c_addr': [0xBA, 0x2C]},
            ],
        },
        peripherals_dict={'i2c_master': object()},
    )

    assert result['struct_type'] == 'dev_lcd_touch_config_t'
    assert result['struct_init']['sub_type'] == 'i2c'
    assert result['struct_init']['touch_config']['x_max'] == 480
    i2c_cfg = result['struct_init']['sub_cfg']['i2c']
    assert i2c_cfg['i2c_name'] == 'i2c_master'
    assert i2c_cfg['i2c_addr_count'] == 2
    assert i2c_cfg['i2c_addr'] == ['0xba', '0x2c', '0x00', '0x00']
    assert i2c_cfg['io_i2c_config']['dev_addr'] == 0
    assert i2c_cfg['io_i2c_config']['scl_speed_hz'] == 400000

def test_lcd_touch_i2c_transaction_timeout_is_idf61_only(bmgr_root, monkeypatch, capsys):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_lcd_touch import dev_lcd_touch as mod

    cfg = {
        'type': 'lcd_touch',
        'chip': 'gt911',
        'sub_type': 'i2c',
        'config': {
            'io_i2c_config': {
                'transaction_timeout_ms': 50,
            },
        },
        'peripherals': [
            {'name': 'i2c_master', 'i2c_addr': 0xBA},
        ],
    }

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (6, 1, 0))
    result = mod.parse('lcd_touch', cfg, peripherals_dict={'i2c_master': object()})
    io_cfg = result['struct_init']['sub_cfg']['i2c']['io_i2c_config']
    assert io_cfg['transaction_timeout_ms'] == 50

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (6, 0, 1))
    result = mod.parse('lcd_touch', cfg, peripherals_dict={'i2c_master': object()})
    io_cfg = result['struct_init']['sub_cfg']['i2c']['io_i2c_config']
    assert 'transaction_timeout_ms' not in io_cfg
    assert 'requires ESP-IDF v6.1' in capsys.readouterr().out

def test_display_lcd_dsi_deinit_disables_dma2d_before_panel_delete(bmgr_root):
    source = (
        bmgr_root
        / 'devices'
        / 'dev_display_lcd'
        / 'dev_display_lcd_sub_dsi.c'
    ).read_text(encoding='utf-8')
    deinit = source[
        source.index('int dev_display_lcd_sub_dsi_deinit_with_config'):
        source.index('int dev_display_lcd_sub_dsi_deinit(void *device_handle)')
    ]

    disable_pos = deinit.index('esp_lcd_dpi_panel_disable_dma2d')
    delete_pos = deinit.index('esp_lcd_panel_del')
    assert disable_pos < delete_pos

def test_lcd_touch_i2c_rejects_7bit_addr(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_lcd_touch import dev_lcd_touch as mod

    with pytest.raises(ValueError, match='8-bit left-shifted address'):
        mod.parse(
            'lcd_touch',
            {
                'type': 'lcd_touch',
                'chip': 'gt911',
                'sub_type': 'i2c',
                'config': {},
                'peripherals': [
                    {'name': 'i2c_master', 'i2c_addr': 0x5D},
                ],
            },
            peripherals_dict={'i2c_master': object()},
        )

def test_lcd_touch_i2c_requires_explicit_addr(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_lcd_touch import dev_lcd_touch as mod

    with pytest.raises(ValueError, match='I2C peripheral requires i2c_addr'):
        mod.parse(
            'lcd_touch',
            {
                'type': 'lcd_touch',
                'chip': 'gt911',
                'sub_type': 'i2c',
                'config': {
                    'io_i2c_config': {},
                },
                'peripherals': [
                    {'name': 'i2c_master'},
                ],
            },
            peripherals_dict={'i2c_master': object()},
        )

def test_lcd_touch_i2c_requires_i2c_peripheral_entry(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_lcd_touch import dev_lcd_touch as mod

    with pytest.raises(ValueError, match='requires one I2C peripheral entry in peripherals'):
        mod.parse(
            'lcd_touch',
            {
                'type': 'lcd_touch',
                'chip': 'gt911',
                'sub_type': 'i2c',
                'config': {},
            },
            peripherals_dict={'i2c_master': object()},
        )

def test_lcd_touch_i2c_requires_i2c_peripheral_name(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_lcd_touch import dev_lcd_touch as mod

    with pytest.raises(ValueError, match='I2C peripheral requires name'):
        mod.parse(
            'lcd_touch',
            {
                'type': 'lcd_touch',
                'chip': 'gt911',
                'sub_type': 'i2c',
                'config': {},
                'peripherals': [
                    {'i2c_addr': 0xBA},
                ],
            },
            peripherals_dict={'i2c_master': object()},
        )

def test_lcd_touch_spi_is_reserved_for_now(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_lcd_touch import dev_lcd_touch as mod

    with pytest.raises(ValueError, match='reserved but not implemented yet'):
        mod.parse(
            'lcd_touch_spi',
            {
                'type': 'lcd_touch',
                'chip': 'xpt2046',
                'sub_type': 'spi',
                'config': {},
            },
        )

def test_generate_kconfig_allows_external_only_boards(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    generator = BoardConfigGenerator(bmgr_root)
    generator.gen_codes_dir = tmp_path / 'gen_codes'

    external_board = tmp_path / 'custom_board'
    external_board.mkdir()

    result = generator.generate_kconfig({
        'custom_board': str(external_board),
    })

    assert result is True

    kconfig_content = (generator.gen_codes_dir / 'Kconfig.in').read_text(encoding='utf-8')
    assert 'menu "Board Selection"' not in kconfig_content
    assert 'config ESP_BOARD_NAME' not in kconfig_content
    assert 'config ESP_BOARD_CUSTOM_BOARD' not in kconfig_content
    assert 'menu "Peripheral Support"' in kconfig_content

@pytest.mark.parametrize('selected_board', ['esp_box_3', 'custom_board'])
def test_generate_selected_board_kconfig_projbuild_defines_current_board_only(bmgr_root, tmp_path, selected_board):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    generator = BoardConfigGenerator(bmgr_root)

    result = generator.generate_selected_board_kconfig_projbuild(
        selected_board=selected_board,
        project_root=str(tmp_path),
    )

    assert result is True

    projbuild_path = tmp_path / 'components' / 'gen_bmgr_codes' / 'Kconfig.projbuild'
    projbuild_content = projbuild_path.read_text(encoding='utf-8')
    board_macro = f'ESP_BOARD_{selected_board.upper().replace("-", "_")}'

    assert f'config {board_macro}' in projbuild_content
    assert '    bool\n    default y' in projbuild_content
    assert 'config ESP_BOARD_NAME' in projbuild_content
    assert f'    default "{selected_board}"' in projbuild_content


def test_generate_selected_board_kconfig_projbuild_appends_board_and_amend_kconfig(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator
    from generators.amend import AmendPlan, AmendFragment, KIND_KCONFIG_PROJBUILD

    board_dir = tmp_path / 'board'
    amend_dir = tmp_path / 'amend'
    board_dir.mkdir()
    amend_dir.mkdir()
    (board_dir / 'Kconfig.projbuild').write_text(
        'menu "board options"\nconfig BOARD_LOCAL_FEATURE\n    bool "Board local feature"\nendmenu\n',
        encoding='utf-8',
    )
    amend_kconfig = amend_dir / 'Kconfig.projbuild'
    amend_kconfig.write_text(
        'menu "amend options"\nconfig AMEND_LOCAL_FEATURE\n    bool "Amend local feature"\nendmenu\n',
        encoding='utf-8',
    )

    generator = BoardConfigGenerator(bmgr_root)
    # Inject a minimal AmendPlan whose only fragment is the amend-root
    # Kconfig.projbuild; this is the same shape build_amend_plan() would
    # produce for ``apply: [Kconfig.projbuild]``.
    generator.amend_plan = AmendPlan(
        amend_dir=amend_dir,
        manifest_path=amend_dir / 'board_amend.yaml',
        fragments=[
            AmendFragment(
                kind=KIND_KCONFIG_PROJBUILD,
                resolved_path=amend_kconfig,
                raw_item='Kconfig.projbuild',
                item_index=0,
            ),
        ],
    )

    result = generator.generate_selected_board_kconfig_projbuild(
        selected_board='custom_board',
        project_root=str(tmp_path),
        board_path=str(board_dir),
    )

    assert result is True

    projbuild_path = tmp_path / 'components' / 'gen_bmgr_codes' / 'Kconfig.projbuild'
    projbuild_content = projbuild_path.read_text(encoding='utf-8')

    assert 'config ESP_BOARD_CUSTOM_BOARD' in projbuild_content
    assert '# --- Board Kconfig.projbuild:' in projbuild_content
    assert 'config BOARD_LOCAL_FEATURE' in projbuild_content
    assert '# --- Amend Kconfig (apply[0] = ' in projbuild_content
    assert 'config AMEND_LOCAL_FEATURE' in projbuild_content
    assert projbuild_content.index('config ESP_BOARD_CUSTOM_BOARD') < projbuild_content.index('config BOARD_LOCAL_FEATURE')
    assert projbuild_content.index('config BOARD_LOCAL_FEATURE') < projbuild_content.index('config AMEND_LOCAL_FEATURE')


def test_generate_selected_board_kconfig_projbuild_skips_empty_extra_kconfigs(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    board_dir = tmp_path / 'board'
    board_dir.mkdir()
    (board_dir / 'Kconfig.projbuild').write_text('\n', encoding='utf-8')

    generator = BoardConfigGenerator(bmgr_root)

    result = generator.generate_selected_board_kconfig_projbuild(
        selected_board='custom_board',
        project_root=str(tmp_path),
        board_path=str(board_dir),
    )

    assert result is True

    projbuild_path = tmp_path / 'components' / 'gen_bmgr_codes' / 'Kconfig.projbuild'
    projbuild_content = projbuild_path.read_text(encoding='utf-8')

    assert 'config ESP_BOARD_CUSTOM_BOARD' in projbuild_content
    assert 'Kconfig.projbuild:' not in projbuild_content


def test_lyrat_mini_peripheral_generation_keeps_structs_aligned(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    generator = BoardConfigGenerator(bmgr_root)
    generator.project_root = str(tmp_path)

    board_yaml = bmgr_root / 'boards' / 'lyrat_mini_v1_1' / 'board_peripherals.yaml'
    generator.process_peripherals(str(board_yaml))

    generated = tmp_path / 'components' / 'gen_bmgr_codes' / 'gen_board_periph_config.c'
    content = generated.read_text(encoding='utf-8')

    assert 'const static i2c_master_bus_config_t esp_bmgr_i2c_master_cfg' in content
    assert 'const static periph_i2s_config_t esp_bmgr_i2s_audio_out_cfg' in content
    assert 'const static periph_i2s_config_t esp_bmgr_i2s_audio_in_cfg' in content
    assert 'const static periph_adc_config_t esp_bmgr_adc_button_cfg' in content

    descriptor_block = content.split('// Peripheral descriptor array', 1)[1]
    descriptor_pairs = re.findall(
        r'\.name = "([^"]+)",.*?\.cfg = &(esp_bmgr_[^,]+),',
        descriptor_block,
        re.S,
    )
    assert descriptor_pairs == [
        ('i2c_master', 'esp_bmgr_i2c_master_cfg'),
        ('i2s_audio_out', 'esp_bmgr_i2s_audio_out_cfg'),
        ('i2s_audio_in', 'esp_bmgr_i2s_audio_in_cfg'),
        ('gpio_sd_power', 'esp_bmgr_gpio_sd_power_cfg'),
        ('gpio_headphone_detection', 'esp_bmgr_gpio_headphone_detection_cfg'),
        ('gpio_power_amp', 'esp_bmgr_gpio_power_amp_cfg'),
        ('gpio_green_led', 'esp_bmgr_gpio_green_led_cfg'),
        ('gpio_blue_led', 'esp_bmgr_gpio_blue_led_cfg'),
        ('gpio_sd_detect', 'esp_bmgr_gpio_sd_detect_cfg'),
        ('adc_button', 'esp_bmgr_adc_button_cfg'),
    ]

def test_esp32_lyrat_v4_3_audio_sdcard_generation_matches_schematic(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    generator = BoardConfigGenerator(bmgr_root)
    generator.project_root = str(tmp_path)

    board_dir = bmgr_root / 'boards' / 'esp32_lyrat_v4_3'
    peripherals_dict, periph_name_map, _ = generator.process_peripherals(str(board_dir / 'board_peripherals.yaml'))
    generator.process_devices(str(board_dir / 'board_devices.yaml'), peripherals_dict, periph_name_map)

    periph_content = (tmp_path / 'components' / 'gen_bmgr_codes' / 'gen_board_periph_config.c').read_text(
        encoding='utf-8'
    )
    device_content = (tmp_path / 'components' / 'gen_bmgr_codes' / 'gen_board_device_config.c').read_text(
        encoding='utf-8'
    )

    assert '.sda_io_num = 18' in periph_content
    assert '.scl_io_num = 23' in periph_content
    assert '.mclk = 0' in periph_content
    assert '.bclk = 5' in periph_content
    assert '.ws = 25' in periph_content
    assert '.dout = 26' in periph_content
    assert '.din = 35' in periph_content
    assert '.pin_bit_mask = BIT64(21)' in periph_content
    assert '.pin_bit_mask = BIT64(12)' in periph_content
    assert '.pin_bit_mask = BIT64(34)' in periph_content

    assert '.port = 21' in device_content
    assert '.active_level = 1' in device_content
    assert '.gpio_name = "gpio_sd_power"' in device_content
    assert '.chip = "es8388"' in device_content
    assert '.address = 32' in device_content
    assert '.bus_width = 1' in device_content
    assert '.clk = 14' in device_content
    assert '.cmd = 15' in device_content
    assert '.d0 = 2' in device_content
    assert '.d1 = -1' in device_content
    assert '.d2 = -1' in device_content
    assert '.d3 = -1' in device_content
    assert '.cd = 34' in device_content

def test_device_descriptor_generation_emits_desc_level_sub_type(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    generator = BoardConfigGenerator(bmgr_root)
    generator.project_root = str(tmp_path)

    board_dir = bmgr_root / 'test_apps' / 'components' / 'test_board_e'
    periph_yaml = board_dir / 'board_peripherals.yaml'
    dev_yaml = board_dir / 'board_devices.yaml'

    peripherals_dict, periph_name_map, _ = generator.process_peripherals(str(periph_yaml))
    generator.process_devices(str(dev_yaml), peripherals_dict, periph_name_map)

    generated = tmp_path / 'components' / 'gen_bmgr_codes' / 'gen_board_device_config.c'
    content = generated.read_text(encoding='utf-8')

    sdcard_desc = re.search(
        r'\.name = "sdcard_fat",.*?\.type = "fs_fat",.*?\.sub_type = "spi",',
        content,
        re.S,
    )
    motor_desc = re.search(
        r'\.name = "motor_driver",.*?\.type = "custom",.*?\.sub_type = NULL,',
        content,
        re.S,
    )

    assert sdcard_desc is not None
    assert motor_desc is not None

def test_process_peripherals_fails_fast_when_parser_is_missing(bmgr_root, tmp_path, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    import gen_bmgr_config_codes as bmgr_module

    generator = bmgr_module.BoardConfigGenerator(bmgr_root)
    generator.project_root = str(tmp_path)

    real_load_parsers = bmgr_module.load_parsers

    def _load_parsers_without_i2c(*args, **kwargs):
        parsers = real_load_parsers(*args, **kwargs)
        parsers.pop('i2c', None)
        return parsers

    monkeypatch.setattr(bmgr_module, 'load_parsers', _load_parsers_without_i2c)

    board_yaml = bmgr_root / 'boards' / 'lyrat_mini_v1_1' / 'board_peripherals.yaml'
    with pytest.raises(RuntimeError, match=r"i2c_master'.*type: i2c"):
        generator.process_peripherals(str(board_yaml))

    generated = tmp_path / 'components' / 'gen_bmgr_codes' / 'gen_board_periph_config.c'
    assert not generated.exists()

def test_parser_loader_raises_on_import_failure(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from generators.parser_loader import load_parsers

    periph_dir = tmp_path / 'peripherals'
    parser_dir = periph_dir / 'periph_bad'
    parser_dir.mkdir(parents=True)
    (parser_dir / 'periph_bad.py').write_text(
        'raise RuntimeError("boom during import")\n',
        encoding='utf-8',
    )

    with pytest.raises(RuntimeError, match='periph_bad'):
        load_parsers([], prefix='periph_', base_dir=str(periph_dir))

def test_audio_codec_rejects_simultaneous_adc_and_dac_enablement(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_audio_codec import dev_audio_codec as mod

    with pytest.raises(ValueError, match='cannot enable both adc_enabled and dac_enabled'):
        mod.parse(
            'audio_codec',
            {
                'type': 'audio_codec',
                'chip': 'es8311',
                'config': {
                    'adc_enabled': True,
                    'dac_enabled': True,
                },
                'peripherals': [
                    {'name': 'i2s_audio_out'},
                    {'name': 'i2c_master', 'address': 0x30},
                ],
            },
        )

def test_audio_codec_without_pa_uses_disabled_pin_sentinel(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_audio_codec import dev_audio_codec as mod

    result = mod.parse(
        'audio_codec',
        {
            'type': 'audio_codec',
            'chip': 'es8311',
            'config': {
                'dac_enabled': True,
            },
            'peripherals': [
                {'name': 'i2s_audio_out'},
                {'name': 'i2c_master', 'address': 0x30},
            ],
        },
    )

    pa_cfg = result['struct_init']['pa_cfg']
    assert pa_cfg['name'] is None
    assert pa_cfg['port'] == -1


def test_new_board_device_options_hide_internal_dedupe_keys(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from create_new_board import BoardCreator

    creator = BoardCreator(bmgr_root)
    devices = creator.get_available_devices()

    assert 'audio_codec::audio_adc_mic' not in devices
    assert 'lcd_touch_i2c__2' not in devices
    assert 'audio_adc_mic_single_unit' in devices
    assert 'lcd_touch_i2c' in devices
    assert not any('::' in device or '__' in device for device in devices)
