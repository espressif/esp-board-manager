# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

"""Check optional panel vendor pointers from YAML parsing through C generation."""

import sys

import pytest


@pytest.mark.parametrize('sub_type', ['spi', 'parlio', 'i80', 'rgb_3wire_spi'])
@pytest.mark.parametrize('panel_config, expected_c', [
    pytest.param({}, 'NULL', id='omitted'),
    pytest.param({'vendor_config': None}, 'NULL', id='yaml-null'),
    pytest.param({'vendor_config': ''}, 'NULL', id='empty'),
    pytest.param({'vendor_config': 'auto'}, 'NULL', id='auto'),
    pytest.param({'vendor_config': 'NULL'}, 'NULL', id='c-null'),
    pytest.param({'vendor_config': '&esp_bmgr_panel_vendor_config'},
                 '&esp_bmgr_panel_vendor_config', id='custom-pointer'),
])
def test_lcd_vendor_config_generates_pointer(bmgr_root, monkeypatch, sub_type,
                                           panel_config, expected_c):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_display_lcd import dev_display_lcd as mod
    from generators.config_generator import ConfigGenerator

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (6, 1, 0))
    panel_key = 'panel_config' if sub_type == 'i80' else 'lcd_panel_config'
    config = {
        'name': 'display',
        'type': 'display_lcd',
        'sub_type': sub_type,
        'config': {panel_key: panel_config},
        'peripherals': [{'spi_name': 'spi_display'}] if sub_type == 'spi' else [],
    }
    result = mod.parse('display', config)
    panel = result['struct_init']['sub_cfg'][sub_type]['panel_config']
    if not panel_config or panel_config.get('vendor_config') in (None, '', 'auto', 'NULL'):
        assert panel['vendor_config'] is None
    else:
        assert panel['vendor_config'] == panel_config['vendor_config']
    generated = '\n'.join(ConfigGenerator(bmgr_root).dict_to_c_initializer(result['struct_init']))
    assert f'.vendor_config = {expected_c},' in generated
    assert '.vendor_config = ""' not in generated
    assert '.vendor_config = "auto"' not in generated


@pytest.mark.parametrize('sub_type', ['dsi', 'rgb'])
def test_native_lcd_config_does_not_generate_vendor_config(bmgr_root, monkeypatch, sub_type):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_display_lcd import dev_display_lcd as mod
    from generators.config_generator import ConfigGenerator

    monkeypatch.setattr(mod, 'get_idf_version', lambda: (6, 1, 0))
    config = {
        'name': 'display',
        'type': 'display_lcd',
        'sub_type': sub_type,
        'config': {},
        'peripherals': [{'dsi_name': 'dsi_display'}, {'ldo_name': 'ldo_display'}],
    }
    result = mod.parse('display', config)
    generated = '\n'.join(ConfigGenerator(bmgr_root).dict_to_c_initializer(result['struct_init']))
    assert '.vendor_config' not in generated
