# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

"""Validate GPIO control levels and their generated C initializers."""

import re

import pytest
import yaml


@pytest.fixture
def gpio_ctrl_parser(bmgr_root, monkeypatch):
    monkeypatch.syspath_prepend(str(bmgr_root))
    from devices.dev_gpio_ctrl import dev_gpio_ctrl

    return dev_gpio_ctrl


def _device(config):
    return {
        'type': 'gpio_ctrl',
        'config': config,
        'peripherals': [{'gpio_name': 'gpio_control'}],
    }


def test_gpio_ctrl_parser_preserves_legacy_defaults(gpio_ctrl_parser):
    result = gpio_ctrl_parser.parse('control', _device({}))

    assert result['struct_type'] == 'dev_gpio_ctrl_config_t'
    assert result['struct_init']['active_level'] == 1
    assert result['struct_init']['default_level'] == 0
    assert result['struct_init']['init_level'] == -1


@pytest.mark.parametrize(('active_level', 'default_level'), [(0, 0), (0, 1), (1, 0), (1, 1)])
@pytest.mark.parametrize('init_config', [{}, {'init_level': -1}, {'init_level': 0}, {'init_level': 1}])
def test_gpio_ctrl_parser_accepts_independent_levels(
    gpio_ctrl_parser, active_level, default_level, init_config,
):
    config = {'active_level': active_level, 'default_level': default_level, **init_config}

    result = gpio_ctrl_parser.parse('control', _device(config))['struct_init']

    assert result['active_level'] == active_level
    assert result['default_level'] == default_level
    assert result['init_level'] == init_config.get('init_level', -1)


@pytest.mark.parametrize('field', ['active_level', 'default_level', 'init_level'])
@pytest.mark.parametrize('value', [True, False, 0.0, 1.0, '0', '1', None])
def test_gpio_ctrl_parser_rejects_non_integer_levels(gpio_ctrl_parser, field, value):
    with pytest.raises(ValueError, match=rf'{field} must be an integer'):
        gpio_ctrl_parser.parse('control', _device({field: value}))


@pytest.mark.parametrize(
    ('field', 'value'),
    [
        ('active_level', -1),
        ('active_level', 2),
        ('default_level', -1),
        ('default_level', 2),
        ('init_level', -2),
        ('init_level', 2),
    ],
)
def test_gpio_ctrl_parser_rejects_levels_outside_range(gpio_ctrl_parser, field, value):
    with pytest.raises(ValueError, match=rf'{field} must be one of'):
        gpio_ctrl_parser.parse('control', _device({field: value}))


@pytest.fixture
def generated_gpio_ctrl_configs(bmgr_root, tmp_path, monkeypatch):
    monkeypatch.syspath_prepend(str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    board_dir = tmp_path / 'gpio_ctrl_board'
    board_dir.mkdir()
    (board_dir / 'board_info.yaml').write_text('board: gpio_ctrl_board\nchip: esp32s3\n', encoding='utf-8')
    peripherals = []
    devices = []
    expected = {}
    init_cases = [('omitted', None), ('legacy', -1), ('low', 0), ('high', 1)]
    for active_level in (0, 1):
        for suffix, init_level in init_cases:
            name = f'control_{active_level}_{suffix}'
            gpio_name = f'gpio_{name}'
            config = {'active_level': active_level, 'default_level': 1 - active_level}
            if init_level is not None:
                config['init_level'] = init_level
            devices.append({
                'name': name,
                'type': 'gpio_ctrl',
                'config': config,
                'peripherals': [{'gpio_name': gpio_name}],
            })
            peripherals.append({
                'name': gpio_name,
                'type': 'gpio',
                'config': {'pin': len(peripherals) + 2, 'mode': 'GPIO_MODE_OUTPUT'},
            })
            expected[name] = dict(config, init_level=-1 if init_level is None else init_level)

    (board_dir / 'board_peripherals.yaml').write_text(
        yaml.safe_dump({'peripherals': peripherals}), encoding='utf-8',
    )
    (board_dir / 'board_devices.yaml').write_text(
        yaml.safe_dump({'devices': devices}), encoding='utf-8',
    )
    generator = BoardConfigGenerator(bmgr_root)
    generator.project_root = str(tmp_path)
    peripherals_dict, periph_name_map, _ = generator.process_peripherals(
        str(board_dir / 'board_peripherals.yaml'),
    )
    generator.process_devices(str(board_dir / 'board_devices.yaml'), peripherals_dict, periph_name_map)
    generated_file = tmp_path / 'components' / 'gen_bmgr_codes' / 'gen_board_device_config.c'
    return generated_file.read_text(encoding='utf-8'), expected


def test_gpio_ctrl_generator_emits_explicit_init_level(generated_gpio_ctrl_configs):
    generated_c, expected = generated_gpio_ctrl_configs

    for name, levels in expected.items():
        match = re.search(
            rf'dev_gpio_ctrl_config_t esp_bmgr_{name}_cfg = \{{(.*?)^\}};',
            generated_c,
            re.DOTALL | re.MULTILINE,
        )
        assert match is not None, f'Missing generated configuration for {name}'
        for field, value in levels.items():
            assert f'.{field} = {value},' in match.group(1), f'{name}: missing explicit {field}={value}'


@pytest.mark.parametrize('package', ['esp_boards', 'esp_friends_boards', 'm5stack_boards'])
def test_existing_board_gpio_ctrl_keeps_active_fallback(bmgr_root, gpio_ctrl_parser, package):
    board_root = bmgr_root.parent / package
    if not board_root.is_dir():
        pytest.skip(f'Board package {package} is not available')
    checked = 0
    for path in sorted(board_root.rglob('board_devices.yaml')):
        devices = (yaml.safe_load(path.read_text(encoding='utf-8')) or {}).get('devices', [])
        for device in devices:
            if device.get('type') != 'gpio_ctrl' or 'init_level' in device.get('config', {}):
                continue
            result = gpio_ctrl_parser.parse(device['name'], device)['struct_init']
            context = f'{path}: {device["name"]}'
            assert result['init_level'] == -1, context
            assert result['active_level'] == device.get('config', {}).get('active_level', 1), context
            assert result['default_level'] == device.get('config', {}).get('default_level', 0), context
            checked += 1
    assert checked > 0, f'No existing GPIO control configuration checked in {package}'
