# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

"""Parse the CI coverage boards against the bundled SoC catalog."""

from pathlib import Path
import json
import sys

import pytest
import yaml


COVERAGE_ROOT = Path(__file__).resolve().parents[1] / 'components' / 'ci_coverage_boards'
BOARDS = (
    'esp32_coverage',
    'esp32s3_coverage',
    'esp32c3_coverage',
    'esp32p4_coverage',
    'esp32s31_coverage',
)

# Catalog keys that coverage boards still omit (pin-heavy or unimplemented).
SKIP_DEVICE_CAPS = frozenset((
    'device.camera_dvp',
    'device.camera_spi',
    'device.display_lcd_i80',
    'device.display_lcd_parlio',
    'device.display_lcd_rgb',
    'device.display_lcd_rgb_3wire_spi',
    'device.lcd_touch_spi',
))


def _board_dir(name: str) -> Path:
    path = COVERAGE_ROOT / name
    if not (path / 'board_info.yaml').is_file():
        pytest.fail(f'missing coverage board {path}')
    return path


def _load_generator():
    import importlib.util

    script = COVERAGE_ROOT / 'generate_coverage_boards.py'
    spec = importlib.util.spec_from_file_location('generate_coverage_boards', script)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


def _catalog_caps(chip: str) -> dict:
    return _load_generator()._load_chip(chip)['capabilities']


def _parse_board(bmgr_root, tmp_path: Path, board: str, idf_version: str = '6.1.0'):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator
    from generators.utils.soc_capability_query import (
        clear_soc_capabilities,
        configure_soc_capabilities,
    )

    board_dir = _board_dir(board)
    info = yaml.safe_load((board_dir / 'board_info.yaml').read_text(encoding='utf-8'))
    chip = info['chip']
    case_dir = tmp_path / board
    case_dir.mkdir()

    catalog_dir = Path(bmgr_root) / 'private_inc' / 'soc_capability_catalog'
    configure_soc_capabilities(catalog_dir, idf_version, chip)
    try:
        generator = BoardConfigGenerator(bmgr_root)
        generator.project_root = str(case_dir)
        peripherals_dict, periph_name_map, periph_types = generator.process_peripherals(
            str(board_dir / 'board_peripherals.yaml')
        )
        device_types, _device_subtypes = generator.process_devices(
            str(board_dir / 'board_devices.yaml'),
            peripherals_dict,
            periph_name_map,
            board_path=str(board_dir),
        )
        return {
            'chip': chip,
            'periph_types': set(periph_types),
            'device_types': set(device_types or []),
            'peripherals': yaml.safe_load(
                (board_dir / 'board_peripherals.yaml').read_text(encoding='utf-8')
            )['peripherals'],
        }
    finally:
        clear_soc_capabilities()


IDF_PARSE_VERSIONS = ('5.4.0', '5.5.0', '6.0.0', '6.1.0', '6.2.0')


@pytest.mark.parametrize('board', BOARDS)
@pytest.mark.parametrize('idf_version', IDF_PARSE_VERSIONS)
def test_coverage_board_parses(bmgr_root, tmp_path, board, idf_version):
    info = yaml.safe_load((_board_dir(board) / 'board_info.yaml').read_text(encoding='utf-8'))
    if info['chip'] == 'esp32s31' and idf_version.startswith(('5.', '6.0')):
        pytest.skip('ESP32-S31 is only in IDF v6.1 and later catalogs')
    parsed = _parse_board(bmgr_root, tmp_path, board, idf_version)
    assert 'adc' in parsed['periph_types']
    assert 'i2s' in parsed['periph_types']
    assert 'uart' in parsed['periph_types']
    oneshot = next(
        p for p in parsed['peripherals'] if p.get('role') == 'oneshot' and p['type'] == 'adc'
    )
    assert 'clk_src' not in oneshot.get('config', {})


@pytest.mark.parametrize('board', BOARDS)
def test_coverage_board_includes_supported_peripherals(bmgr_root, tmp_path, board):
    parsed = _parse_board(bmgr_root, tmp_path, board)
    caps = _catalog_caps(parsed['chip'])
    present_types = {p['type'] for p in parsed['peripherals']}

    for key, enabled in caps.items():
        if not enabled or not key.startswith('peripheral.'):
            continue
        name = key.split('.', 1)[1]
        if name == 'adc_oneshot':
            assert any(p['type'] == 'adc' and p.get('role') == 'oneshot' for p in parsed['peripherals'])
        elif name == 'adc_continuous':
            assert any(p['type'] == 'adc' and p.get('role') == 'continuous' for p in parsed['peripherals'])
        elif name.startswith('i2s_master_'):
            fmt = name[len('i2s_master_'):]
            if fmt in ('pdm-in', 'pdm-out', 'tdm'):
                continue
            if fmt == 'std':
                has_std = any(
                    p['type'] == 'i2s' and str(p.get('format', '')).startswith('std')
                    for p in parsed['peripherals']
                )
                assert has_std or parsed['chip'] == 'esp32c3'
        else:
            assert name in present_types, f'{board} missing peripheral {name}'


def test_coverage_boards_skip_heavy_device_types_on_purpose(bmgr_root):
    caps = json.loads(
        (Path(bmgr_root) / 'private_inc' / 'soc_capability_catalog' / 'idf_6_1.json').read_text(
            encoding='utf-8'
        )
    )['chips']['esp32s3']['capabilities']
    for key in SKIP_DEVICE_CAPS:
        assert key in caps


@pytest.mark.parametrize('board', BOARDS)
def test_coverage_board_includes_fake_factory_devices(bmgr_root, tmp_path, board):
    parsed = _parse_board(bmgr_root, tmp_path, board)
    devices = yaml.safe_load(
        (_board_dir(board) / 'board_devices.yaml').read_text(encoding='utf-8')
    )['devices']
    pairs = {(d['type'], d.get('sub_type')) for d in devices}
    types = {d['type'] for d in devices}
    assert 'lcd_touch' in types
    assert 'gpio_expander' in types
    assert 'custom' in types
    assert ('lcd_touch', 'i2c') in pairs
    assert ('fs_fat', 'spi') in pairs
    assert ('littlefs', 'spi') in pairs
    assert ('button', 'adc_single') in pairs
    if parsed['chip'] == 'esp32p4':
        assert 'display_lcd' not in types
        assert ('camera', 'csi') in pairs
    else:
        assert 'display_lcd' in types
        assert ('display_lcd', 'spi') in pairs
    if parsed['chip'] not in ('esp32', 'esp32c3'):
        assert ('led_strip', 'spi') in pairs


def test_committed_coverage_yaml_matches_generator(tmp_path, monkeypatch):
    mod = _load_generator()
    monkeypatch.setattr(mod, 'HERE', tmp_path)
    mod.main()
    files = (
        'board_info.yaml',
        'board_peripherals.yaml',
        'board_devices.yaml',
        'sdkconfig.defaults.board',
        'setup_device.c',
    )
    for board, _chip in mod.BOARDS:
        for name in files:
            committed = (COVERAGE_ROOT / board / name).read_text(encoding='utf-8')
            generated = (tmp_path / board / name).read_text(encoding='utf-8')
            assert committed == generated, f'{board}/{name} is out of date; rerun generate_coverage_boards.py'
