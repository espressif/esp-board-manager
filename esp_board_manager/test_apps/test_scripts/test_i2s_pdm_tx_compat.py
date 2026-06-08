"""
Compatibility behavior tests for the I2S peripheral parser.
"""

import sys

import pytest


class FakeSoc:
    def __init__(self, limits=None, supports=None, invalid=None):
        self._limits = limits or {}
        self._supports = supports or {}
        self._invalid = set(invalid or [])
        self.calls = []

    def supports(self, key, default=False):
        self.calls.append(('supports', key, default))
        return self._supports.get(key, default)

    def limit(self, key, default=None):
        self.calls.append(('limit', key, default))
        return self._limits.get(key, default)

    def valid_gpio(self, pins, direction='any', allow_nc=True, default=True):
        self.calls.append(('valid_gpio', tuple(_pin_list(pins)), direction, allow_nc, default))
        for pin in _pin_list(pins):
            if allow_nc and pin == -1:
                continue
            if (pin, direction) in self._invalid or (pin, 'any') in self._invalid:
                return False
        return default


def _pin_list(pins):
    if isinstance(pins, (list, tuple, set)):
        return list(pins)
    return [pins]


def test_std_mode_omits_bclk_div_on_idf_5_4(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils import idf_version as idf_version_mod
    from peripherals.periph_i2s import periph_i2s as mod

    monkeypatch.setattr(idf_version_mod, '_idf_version', (5, 4, 0))

    result = mod.parse(
        'i2s_audio_out',
        {
            'type': 'i2s',
            'role': 'master',
            'format': 'std-out',
            'config': {
                'sample_rate_hz': 16000,
                'bclk_div': 12,
                'pins': {
                    'bclk': 4,
                    'ws': 5,
                    'dout': 6,
                },
            },
        },
    )

    clk_cfg = result['struct_init']['i2s_cfg']['std']['clk_cfg']
    assert 'bclk_div' not in clk_cfg

def test_std_mode_keeps_bclk_div_on_idf_5_5_and_newer(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils import idf_version as idf_version_mod
    from peripherals.periph_i2s import periph_i2s as mod

    monkeypatch.setattr(idf_version_mod, '_idf_version', (5, 5, 0))

    result = mod.parse(
        'i2s_audio_out',
        {
            'type': 'i2s',
            'role': 'master',
            'format': 'std-out',
            'config': {
                'sample_rate_hz': 16000,
                'bclk_div': 12,
                'pins': {
                    'bclk': 4,
                    'ws': 5,
                    'dout': 6,
                },
            },
        },
    )

    clk_cfg = result['struct_init']['i2s_cfg']['std']['clk_cfg']
    assert clk_cfg['bclk_div'] == 12

def test_unknown_chip_omits_pdm_tx_dout2_in_generated_gpio_cfg(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_i2s import periph_i2s as mod

    monkeypatch.setattr(mod, 'get_effective_chip_name', lambda: None)

    result = mod.parse(
        'i2s_audio_out',
        {
            'type': 'i2s',
            'role': 'master',
            'format': 'pdm-out',
            'config': {
                'sample_rate_hz': 16000,
                'pins': {
                    'clk': 4,
                    'dout': 5,
                    'dout2': 6,
                },
            },
        },
    )

    gpio_cfg = result['struct_init']['i2s_cfg']['pdm_tx']['gpio_cfg']
    assert 'dout2' not in gpio_cfg

def test_catalog_line_limit_keeps_pdm_tx_dout2_in_generated_gpio_cfg(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_i2s import periph_i2s as mod

    soc = FakeSoc(limits={'i2s.pdm_max_tx_lines': 2})
    monkeypatch.setattr(mod, 'current_soc', lambda: soc, raising=False)
    monkeypatch.setattr(mod, 'get_effective_chip_name', lambda: 'esp32s3')

    result = mod.parse(
        'i2s_audio_out',
        {
            'type': 'i2s',
            'role': 'master',
            'format': 'pdm-out',
            'config': {
                'sample_rate_hz': 16000,
                'pins': {
                    'clk': 4,
                    'dout': 5,
                    'dout2': 6,
                },
            },
        },
    )

    gpio_cfg = result['struct_init']['i2s_cfg']['pdm_tx']['gpio_cfg']
    assert gpio_cfg['dout2'] == 6
    assert ('limit', 'i2s.pdm_max_tx_lines', 1) in soc.calls

def test_tdm_default_slot_mask_tracks_slot_mode_enum(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_i2s import periph_i2s as mod

    stereo = mod.parse(
        'i2s_audio_out',
        {
            'type': 'i2s',
            'role': 'master',
            'format': 'tdm-out',
            'config': {
                'slot_mode': 'I2S_SLOT_MODE_STEREO',
            },
        },
    )
    mono = mod.parse(
        'i2s_audio_out',
        {
            'type': 'i2s',
            'role': 'master',
            'format': 'tdm-out',
            'config': {
                'slot_mode': 'I2S_SLOT_MODE_MONO',
            },
        },
    )

    assert stereo['struct_init']['i2s_cfg']['tdm']['slot_cfg']['slot_mask'] == 'I2S_TDM_SLOT0 | I2S_TDM_SLOT1'
    assert mono['struct_init']['i2s_cfg']['tdm']['slot_cfg']['slot_mask'] == 'I2S_TDM_SLOT0'

def _pdm_rx_slot_cfg(mod):
    result = mod.parse(
        'i2s_audio_in',
        {
            'type': 'i2s',
            'role': 'master',
            'format': 'pdm-in',
            'config': {
                'sample_rate_hz': 16000,
                'pins': {
                    'clk': 4,
                    'din': 5,
                },
            },
        },
    )
    return result['struct_init']['i2s_cfg']['pdm_rx']['slot_cfg']

def test_pdm_rx_omits_hp_filter_fields_when_soc_cap_is_missing(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_i2s import periph_i2s as mod

    soc = FakeSoc(limits={'i2s.hw_version': 2})
    monkeypatch.setattr(mod, 'current_soc', lambda: soc, raising=False)
    monkeypatch.setattr(mod, 'get_effective_chip_name', lambda: 'esp32s3')

    slot_cfg = _pdm_rx_slot_cfg(mod)
    assert 'hp_en' not in slot_cfg
    assert 'hp_cut_off_freq_hz' not in slot_cfg
    assert 'amplify_num' not in slot_cfg
    assert ('supports', 'i2s.supports_pdm_rx_hp_filter', False) in soc.calls

def test_pdm_rx_keeps_hp_filter_fields_for_p4_catalog_cap(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_i2s import periph_i2s as mod

    soc = FakeSoc(
        limits={'i2s.hw_version': 2},
        supports={'i2s.supports_pdm_rx_hp_filter': True},
    )
    monkeypatch.setattr(mod, 'current_soc', lambda: soc, raising=False)
    monkeypatch.setattr(mod, 'get_effective_chip_name', lambda: 'esp32p4')

    slot_cfg = _pdm_rx_slot_cfg(mod)
    assert slot_cfg['hp_en'] is True
    assert slot_cfg['hp_cut_off_freq_hz'] == 35.5
    assert slot_cfg['amplify_num'] == 1

def test_pdm_rx_catalog_false_overrides_chip_name_fallback(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_i2s import periph_i2s as mod

    soc = FakeSoc(
        limits={'i2s.hw_version': 2},
        supports={'i2s.supports_pdm_rx_hp_filter': False},
    )
    monkeypatch.setattr(mod, 'current_soc', lambda: soc, raising=False)
    monkeypatch.setattr(mod, 'get_effective_chip_name', lambda: 'esp32s31')

    slot_cfg = _pdm_rx_slot_cfg(mod)
    assert 'hp_en' not in slot_cfg
    assert 'hp_cut_off_freq_hz' not in slot_cfg
    assert 'amplify_num' not in slot_cfg

def test_hw_version_uses_soc_catalog_limit(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_i2s import periph_i2s as mod

    soc = FakeSoc(limits={'i2s.hw_version': 1})
    monkeypatch.setattr(mod, 'current_soc', lambda: soc, raising=False)
    monkeypatch.setattr(mod, 'get_effective_chip_name', lambda: 'esp32c5')

    result = mod.parse(
        'i2s_audio_out',
        {
            'type': 'i2s',
            'role': 'master',
            'format': 'std-out',
            'config': {
                'pins': {
                    'bclk': 4,
                    'ws': 5,
                    'dout': 6,
                },
            },
        },
    )

    slot_cfg = result['struct_init']['i2s_cfg']['std']['slot_cfg']
    assert 'msb_right' in slot_cfg
    assert ('limit', 'i2s.hw_version', None) in soc.calls

def test_pdm_rx_dins_uses_soc_catalog_line_limit(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_i2s import periph_i2s as mod

    soc = FakeSoc(limits={'i2s.pdm_max_rx_lines': 2})
    monkeypatch.setattr(mod, 'current_soc', lambda: soc, raising=False)
    monkeypatch.setattr(mod, 'get_effective_chip_name', lambda: 'esp32c5')

    result = mod.parse(
        'i2s_audio_in',
        {
            'type': 'i2s',
            'role': 'master',
            'format': 'pdm-in',
            'config': {
                'pins': {
                    'clk': 4,
                    'din0': 5,
                    'din1': 6,
                },
            },
        },
    )

    gpio_cfg = result['struct_init']['i2s_cfg']['pdm_rx']['gpio_cfg']
    assert gpio_cfg['dins'] == [5, 6]
    assert ('limit', 'i2s.pdm_max_rx_lines', 1) in soc.calls

    with pytest.raises(ValueError, match='din2.*2 PDM RX data line'):
        mod.parse(
            'i2s_audio_in',
            {
                'type': 'i2s',
                'role': 'master',
                'format': 'pdm-in',
                'config': {
                    'pins': {
                        'clk': 4,
                        'din0': 5,
                        'din1': 6,
                        'din2': 7,
                    },
                },
            },
        )

def test_pdm_rx_hp_filter_uses_soc_catalog_supports(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_i2s import periph_i2s as mod

    soc = FakeSoc(supports={'i2s.supports_pdm_rx_hp_filter': True})
    monkeypatch.setattr(mod, 'current_soc', lambda: soc, raising=False)
    monkeypatch.setattr(mod, 'get_effective_chip_name', lambda: 'esp32s3')

    slot_cfg = _pdm_rx_slot_cfg(mod)
    assert slot_cfg['hp_en'] is True
    assert ('supports', 'i2s.supports_pdm_rx_hp_filter', False) in soc.calls

def test_i2s_gpios_use_soc_gpio_directions_and_allow_nc(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_i2s import periph_i2s as mod

    soc = FakeSoc(invalid={(99, 'input'), (98, 'output')})
    monkeypatch.setattr(mod, 'current_soc', lambda: soc, raising=False)

    mod.parse(
        'i2s_audio_out',
        {
            'type': 'i2s',
            'role': 'master',
            'format': 'std-out',
            'config': {
                'pins': {
                    'mclk': -1,
                    'bclk': 4,
                    'ws': 5,
                    'dout': 6,
                    'din': -1,
                },
            },
        },
    )
    assert ('valid_gpio', (6,), 'output', True, True) in soc.calls
    assert ('valid_gpio', (-1,), 'input', True, True) in soc.calls

    with pytest.raises(ValueError, match='Invalid I2S GPIO configuration'):
        mod.parse(
            'i2s_audio_in',
            {
                'type': 'i2s',
                'role': 'master',
                'format': 'std-in',
                'config': {
                    'pins': {
                        'bclk': 4,
                        'ws': 5,
                        'din': 99,
                    },
                },
            },
        )

    with pytest.raises(ValueError, match='Invalid I2S GPIO configuration'):
        mod.parse(
            'i2s_audio_out',
            {
                'type': 'i2s',
                'role': 'master',
                'format': 'pdm-out',
                'config': {
                    'pins': {
                        'clk': 4,
                        'dout': 98,
                    },
                },
            },
        )
