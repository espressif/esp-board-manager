"""
Tests for entrance-level SoC capability availability checks.
"""

import json
import sys
from pathlib import Path

import pytest
import yaml


def _write_catalog(root: Path, capabilities, hardware_limit_defs=None, hardware_limits=None) -> Path:
    catalog_dir = root / 'soc_capability_catalog'
    catalog_dir.mkdir()
    (catalog_dir / 'index.json').write_text(
        json.dumps({
            'schemaVersion': 1,
            'catalogSchemaVersion': 3,
            'profiles': [
                {'id': '5.5', 'version': '5.5.0', 'path': 'idf_5_5.json'},
            ],
        }),
        encoding='utf-8',
    )
    (catalog_dir / 'idf_5_5.json').write_text(
        json.dumps({
            'schemaVersion': 3,
            'capabilityDefs': {
                'peripheral.i2s_master_pdm-in': {
                    'kind': 'peripheral',
                    'type': 'i2s',
                    'format': ['pdm-in'],
                },
                'device.display_lcd_rgb': {
                    'kind': 'device',
                    'type': 'display_lcd',
                    'sub_type': ['rgb'],
                },
            },
            'hardwareLimitDefs': hardware_limit_defs or {},
            'profile': {'id': '5.5'},
            'chips': {
                'esp32c3': {
                    'capabilities': capabilities,
                    'gpio': {'validInput': [], 'validOutput': []},
                    'hardwareLimits': hardware_limits or {},
                },
            },
        }),
        encoding='utf-8',
    )
    return catalog_dir


def _generator(bmgr_root, tmp_path, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    gen = BoardConfigGenerator(Path(bmgr_root), project_dir=str(tmp_path))
    monkeypatch.setattr(gen, 'write_periph_c', lambda *args, **kwargs: None)
    monkeypatch.setattr(gen, 'write_periph_handles', lambda *args, **kwargs: None)
    monkeypatch.setattr(gen, 'write_device_custom_h', lambda *args, **kwargs: None)
    monkeypatch.setattr(gen, 'write_device_c', lambda *args, **kwargs: None)
    monkeypatch.setattr(gen, 'write_device_handles', lambda *args, **kwargs: None)
    monkeypatch.setattr(gen.dependency_manager, 'validate_extra_dev_usage', lambda *args, **kwargs: None)
    return gen


def test_unsupported_peripheral_fails_before_parser_dispatch(tmp_path, bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils import soc_capability_query
    import gen_bmgr_config_codes as gen_mod

    catalog_dir = _write_catalog(tmp_path, {'peripheral.i2s_master_pdm-in': False})
    soc_capability_query.configure_soc_capabilities(catalog_dir, '5.5.0', 'esp32c3')

    periph_yaml = tmp_path / 'board_peripherals.yaml'
    periph_yaml.write_text(
        yaml.safe_dump({
            'peripherals': [
                {
                    'name': 'i2s_mic',
                    'type': 'i2s',
                    'format': 'pdm-in',
                    'config': {},
                },
            ],
        }),
        encoding='utf-8',
    )

    parser_calls = []

    def parse_func(name, config):
        parser_calls.append((name, config))
        return {'struct_type': 'dummy_t', 'struct_var': 'dummy', 'struct_init': {}}

    monkeypatch.setattr(
        gen_mod,
        'load_parsers',
        lambda *args, **kwargs: {'i2s': ('v1', parse_func, lambda: [])},
    )
    gen = _generator(bmgr_root, tmp_path, monkeypatch)

    try:
        with pytest.raises(ValueError, match='peripheral.i2s_master_pdm-in'):
            gen.process_peripherals(str(periph_yaml))
    finally:
        soc_capability_query.clear_soc_capabilities()

    assert parser_calls == []


def test_catalog_missing_chip_fails_before_parser_dispatch(tmp_path, bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils import soc_capability_query
    import gen_bmgr_config_codes as gen_mod

    catalog_dir = _write_catalog(tmp_path, {'peripheral.i2s_master_pdm-in': True})
    soc_capability_query.configure_soc_capabilities(catalog_dir, '5.5.4', 'esp32s31')

    periph_yaml = tmp_path / 'board_peripherals.yaml'
    periph_yaml.write_text(
        yaml.safe_dump({
            'peripherals': [
                {
                    'name': 'i2s_mic',
                    'type': 'i2s',
                    'format': 'pdm-in',
                    'config': {},
                },
            ],
        }),
        encoding='utf-8',
    )

    parser_calls = []

    def parse_func(name, config):
        parser_calls.append((name, config))
        return {'struct_type': 'dummy_t', 'struct_var': 'dummy', 'struct_init': {}}

    monkeypatch.setattr(
        gen_mod,
        'load_parsers',
        lambda *args, **kwargs: {'i2s': ('v1', parse_func, lambda: [])},
    )
    gen = _generator(bmgr_root, tmp_path, monkeypatch)

    try:
        with pytest.raises(ValueError, match='esp32s31.*selected SoC capability catalog'):
            gen.process_peripherals(str(periph_yaml))
    finally:
        soc_capability_query.clear_soc_capabilities()

    assert parser_calls == []


def test_supported_peripheral_reaches_parser_dispatch(tmp_path, bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils import soc_capability_query
    import gen_bmgr_config_codes as gen_mod

    catalog_dir = _write_catalog(tmp_path, {'peripheral.i2s_master_pdm-in': True})
    soc_capability_query.configure_soc_capabilities(catalog_dir, '5.5.0', 'esp32c3')

    periph_yaml = tmp_path / 'board_peripherals.yaml'
    periph_yaml.write_text(
        yaml.safe_dump({
            'peripherals': [
                {
                    'name': 'i2s_mic',
                    'type': 'i2s',
                    'format': 'pdm-in',
                    'config': {},
                },
            ],
        }),
        encoding='utf-8',
    )

    parser_calls = []

    def parse_func(name, config):
        parser_calls.append((name, config))
        return {'struct_type': 'dummy_t', 'struct_var': 'dummy', 'struct_init': {}}

    monkeypatch.setattr(
        gen_mod,
        'load_parsers',
        lambda *args, **kwargs: {'i2s': ('v1', parse_func, lambda: [])},
    )
    gen = _generator(bmgr_root, tmp_path, monkeypatch)

    try:
        gen.process_peripherals(str(periph_yaml))
    finally:
        soc_capability_query.clear_soc_capabilities()

    assert len(parser_calls) == 1


def test_hardware_limit_failure_happens_before_parser_dispatch(tmp_path, bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils import soc_capability_query
    import gen_bmgr_config_codes as gen_mod

    catalog_dir = _write_catalog(
        tmp_path,
        {'peripheral.adc_continuous': True},
        hardware_limit_defs={
            'adc.pattern_length_max': {
                'appliesTo': [
                    {
                        'kind': 'peripheral',
                        'type': 'adc',
                        'role': ['continuous'],
                        'path': ['patterns'],
                        'check': 'arrayLength',
                    },
                ],
            },
        },
        hardware_limits={'adc.pattern_length_max': 2},
    )
    soc_capability_query.configure_soc_capabilities(catalog_dir, '5.5.0', 'esp32c3')

    periph_yaml = tmp_path / 'board_peripherals.yaml'
    periph_yaml.write_text(
        yaml.safe_dump({
            'peripherals': [
                {
                    'name': 'adc_in',
                    'type': 'adc',
                    'role': 'continuous',
                    'config': {
                        'patterns': [
                            {'unit': 'ADC_UNIT_1', 'channel': 0},
                            {'unit': 'ADC_UNIT_1', 'channel': 1},
                            {'unit': 'ADC_UNIT_1', 'channel': 2},
                        ],
                    },
                },
            ],
        }),
        encoding='utf-8',
    )

    parser_calls = []

    def parse_func(name, config):
        parser_calls.append((name, config))
        return {'struct_type': 'dummy_t', 'struct_var': 'dummy', 'struct_init': {}}

    monkeypatch.setattr(
        gen_mod,
        'load_parsers',
        lambda *args, **kwargs: {'adc': ('v1', parse_func, lambda: [])},
    )
    gen = _generator(bmgr_root, tmp_path, monkeypatch)

    try:
        with pytest.raises(ValueError, match='adc.pattern_length_max'):
            gen.process_peripherals(str(periph_yaml))
    finally:
        soc_capability_query.clear_soc_capabilities()

    assert parser_calls == []


@pytest.mark.parametrize(
    ('name', 'config', 'applies_path', 'expected_path_fragment'),
    [
        ('oneshot_channel', {'channel_id': 10}, ['channel_id'], 'channel_id'),
        ('continuous_channel_list', {'channel_list': [0, 9, 10]}, ['channel_list'], 'channel_list/2'),
        (
            'continuous_patterns',
            {'patterns': [{'channel': 0}, {'channel': 10}]},
            ['patterns', 'channel'],
            'patterns/1/channel',
        ),
    ],
)
def test_adc_channel_limit_failures_happen_before_parser_dispatch(
    tmp_path,
    bmgr_root,
    monkeypatch,
    name,
    config,
    applies_path,
    expected_path_fragment,
):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils import soc_capability_query
    import gen_bmgr_config_codes as gen_mod

    role = 'oneshot' if name == 'oneshot_channel' else 'continuous'
    catalog_dir = _write_catalog(
        tmp_path,
        {
            'peripheral.adc_oneshot': True,
            'peripheral.adc_continuous': True,
        },
        hardware_limit_defs={
            'adc.max_channel_count': {
                'appliesTo': [
                    {
                        'kind': 'peripheral',
                        'type': 'adc',
                        'role': [role],
                        'path': applies_path,
                        'check': 'value',
                        'compare': 'lt',
                    },
                ],
            },
        },
        hardware_limits={'adc.max_channel_count': 10},
    )
    soc_capability_query.configure_soc_capabilities(catalog_dir, '5.5.0', 'esp32c3')

    periph_yaml = tmp_path / 'board_peripherals.yaml'
    periph_yaml.write_text(
        yaml.safe_dump({
            'peripherals': [
                {
                    'name': 'adc_in',
                    'type': 'adc',
                    'role': role,
                    'config': config,
                },
            ],
        }),
        encoding='utf-8',
    )

    parser_calls = []

    def parse_func(name, config):
        parser_calls.append((name, config))
        return {'struct_type': 'dummy_t', 'struct_var': 'dummy', 'struct_init': {}}

    monkeypatch.setattr(
        gen_mod,
        'load_parsers',
        lambda *args, **kwargs: {'adc': ('v1', parse_func, lambda: [])},
    )
    gen = _generator(bmgr_root, tmp_path, monkeypatch)

    try:
        with pytest.raises(ValueError) as exc_info:
            gen.process_peripherals(str(periph_yaml))
    finally:
        soc_capability_query.clear_soc_capabilities()

    message = str(exc_info.value)
    assert 'adc.max_channel_count' in message
    assert expected_path_fragment in message
    assert parser_calls == []


def test_unsupported_device_fails_before_parser_dispatch(tmp_path, bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils import soc_capability_query
    import gen_bmgr_config_codes as gen_mod

    catalog_dir = _write_catalog(tmp_path, {'device.display_lcd_rgb': False})
    soc_capability_query.configure_soc_capabilities(catalog_dir, '5.5.0', 'esp32c3')

    device_yaml = tmp_path / 'board_devices.yaml'
    device_yaml.write_text(
        yaml.safe_dump({
            'devices': [
                {
                    'name': 'lcd',
                    'type': 'display_lcd',
                    'sub_type': 'rgb',
                    'config': {},
                },
            ],
        }),
        encoding='utf-8',
    )

    parser_calls = []

    def parse_func(name, config, peripherals):
        parser_calls.append((name, config, peripherals))
        return {'struct_type': 'dummy_t', 'struct_var': 'dummy', 'struct_init': {}}

    monkeypatch.setattr(
        gen_mod,
        'load_parsers',
        lambda *args, **kwargs: {'display_lcd': ('v1', parse_func, lambda: [])},
    )
    gen = _generator(bmgr_root, tmp_path, monkeypatch)

    try:
        with pytest.raises(ValueError, match='device.display_lcd_rgb'):
            gen.process_devices(str(device_yaml), {}, {}, None, {}, set())
    finally:
        soc_capability_query.clear_soc_capabilities()

    assert parser_calls == []


def test_ledc_channel_limit_fails_before_parser_dispatch(tmp_path, bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils import soc_capability_query
    import gen_bmgr_config_codes as gen_mod

    catalog_dir = _write_catalog(
        tmp_path,
        {},
        hardware_limit_defs={
            'ledc.channel_count': {
                'appliesTo': [
                    {
                        'kind': 'peripheral',
                        'type': 'ledc',
                        'path': ['channel'],
                        'check': 'value',
                        'compare': 'lt',
                    },
                ],
            },
        },
        hardware_limits={'ledc.channel_count': 6},
    )
    soc_capability_query.configure_soc_capabilities(catalog_dir, '5.5.0', 'esp32c3')

    periph_yaml = tmp_path / 'board_peripherals.yaml'
    periph_yaml.write_text(
        yaml.safe_dump({
            'peripherals': [
                {
                    'name': 'led',
                    'type': 'ledc',
                    'config': {
                        'channel': 6,
                        'gpio_num': 2,
                    },
                },
            ],
        }),
        encoding='utf-8',
    )

    parser_calls = []

    def parse_func(name, config):
        parser_calls.append((name, config))
        return {'struct_type': 'dummy_t', 'struct_var': 'dummy', 'struct_init': {}}

    monkeypatch.setattr(
        gen_mod,
        'load_parsers',
        lambda *args, **kwargs: {'ledc': ('v1', parse_func, lambda: [])},
    )
    gen = _generator(bmgr_root, tmp_path, monkeypatch)

    try:
        with pytest.raises(ValueError, match='ledc.channel_count'):
            gen.process_peripherals(str(periph_yaml))
    finally:
        soc_capability_query.clear_soc_capabilities()

    assert parser_calls == []


def test_dac_channel_limit_fails_before_parser_dispatch(tmp_path, bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils import soc_capability_query
    import gen_bmgr_config_codes as gen_mod

    catalog_dir = _write_catalog(
        tmp_path,
        {},
        hardware_limit_defs={
            'dac.channel_count': {
                'appliesTo': [
                    {
                        'kind': 'peripheral',
                        'type': 'dac',
                        'role': ['oneshot'],
                        'path': ['channel'],
                        'check': 'value',
                        'compare': 'lt',
                    },
                ],
            },
        },
        hardware_limits={'dac.channel_count': 2},
    )
    soc_capability_query.configure_soc_capabilities(catalog_dir, '5.5.0', 'esp32c3')

    periph_yaml = tmp_path / 'board_peripherals.yaml'
    periph_yaml.write_text(
        yaml.safe_dump({
            'peripherals': [
                {
                    'name': 'dac0',
                    'type': 'dac',
                    'role': 'oneshot',
                    'config': {
                        'channel': 2,
                    },
                },
            ],
        }),
        encoding='utf-8',
    )

    parser_calls = []

    def parse_func(name, config):
        parser_calls.append((name, config))
        return {'struct_type': 'dummy_t', 'struct_var': 'dummy', 'struct_init': {}}

    monkeypatch.setattr(
        gen_mod,
        'load_parsers',
        lambda *args, **kwargs: {'dac': ('v1', parse_func, lambda: [])},
    )
    gen = _generator(bmgr_root, tmp_path, monkeypatch)

    try:
        with pytest.raises(ValueError, match='dac.channel_count'):
            gen.process_peripherals(str(periph_yaml))
    finally:
        soc_capability_query.clear_soc_capabilities()

    assert parser_calls == []


def test_anacmpr_unit_limit_fails_before_parser_dispatch(tmp_path, bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils import soc_capability_query
    import gen_bmgr_config_codes as gen_mod

    catalog_dir = _write_catalog(
        tmp_path,
        {},
        hardware_limit_defs={
            'anacmpr.unit_count': {
                'appliesTo': [
                    {
                        'kind': 'peripheral',
                        'type': 'anacmpr',
                        'path': ['unit'],
                        'check': 'value',
                        'compare': 'lt',
                    },
                ],
            },
        },
        hardware_limits={'anacmpr.unit_count': 1},
    )
    soc_capability_query.configure_soc_capabilities(catalog_dir, '5.5.0', 'esp32c3')

    periph_yaml = tmp_path / 'board_peripherals.yaml'
    periph_yaml.write_text(
        yaml.safe_dump({
            'peripherals': [
                {
                    'name': 'cmpr',
                    'type': 'anacmpr',
                    'config': {
                        'unit': 1,
                    },
                },
            ],
        }),
        encoding='utf-8',
    )

    parser_calls = []

    def parse_func(name, config):
        parser_calls.append((name, config))
        return {'struct_type': 'dummy_t', 'struct_var': 'dummy', 'struct_init': {}}

    monkeypatch.setattr(
        gen_mod,
        'load_parsers',
        lambda *args, **kwargs: {'anacmpr': ('v1', parse_func, lambda: [])},
    )
    gen = _generator(bmgr_root, tmp_path, monkeypatch)

    try:
        with pytest.raises(ValueError, match='anacmpr.unit_count'):
            gen.process_peripherals(str(periph_yaml))
    finally:
        soc_capability_query.clear_soc_capabilities()

    assert parser_calls == []
