# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

"""PHY configuration extensions and their Ethernet parser integration."""

import sys

import pytest


@pytest.fixture
def ethernet(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet
    from generators.utils.soc_capability_query import clear_soc_capabilities

    clear_soc_capabilities()
    yield dev_ethernet
    clear_soc_capabilities()


@pytest.fixture
def loader(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from generators import phy_parser_loader
    return phy_parser_loader


def _device(chip='generic', **extra):
    return {
        'chip': chip,
        'sub_type': 'emac_rmii',
        'config': {
            'smi_gpio': {'mdc_num': 23, 'mdio_num': 18},
            'clock_config': {'clock_mode': 'EMAC_CLK_EXT_IN', 'clock_gpio': 0},
            **extra,
        },
    }


def _write_extension(path, chip='custom_phy', marker=1):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        'def parse_custom(name, vendor_config, context):\n'
        '    return {"struct_type": "custom_phy_config_t",\n'
        f'            "struct_init": {{"marker": {marker}, "gain": vendor_config.get("gain", 0)}},\n'
        '            "includes": ["custom_phy.h"]}\n'
        'def get_phy_parsers():\n'
        f'    return {{{chip!r}: {{"parse": parse_custom}}}}\n',
        encoding='utf-8',
    )
    return path


def test_generic_phy_without_vendor_configuration_has_no_vendor_object(ethernet):
    result = ethernet.parse('ethernet', _device())
    assert result['struct_init']['vendor_config'] is None
    assert result['struct_init']['vendor_config_size'] == 0
    assert not result.get('extra_configs')


@pytest.mark.parametrize('chip', ['', 'generic', 'unknown_phy', 'yt8531'])
def test_selected_phy_records_runtime_source_group(ethernet, loader, chip):
    result = ethernet.parse('ethernet', _device(chip))
    sources = sorted(loader.current_phy_parser_registry().get(chip, {}).get('sources', []))
    assert result.get('runtime_source_groups', {}) == ({f'ethernet_phy:{chip}': sources} if chip else {})


def test_unknown_phy_with_vendor_configuration_fails(ethernet):
    with pytest.raises(ValueError, match='vendor_config.*parser|parser.*vendor_config'):
        ethernet.parse('ethernet', _device('unknown_phy', vendor_config={}))


def test_builtin_registry_contains_yt8531(loader):
    descriptor = loader.current_phy_parser_registry()['yt8531']
    assert callable(descriptor['parse'])
    assert set(descriptor) == {'parse', 'sources'}
    assert descriptor['sources'] == [str(loader._BUILTIN_DIR / 'yt8531.c')]


def test_builtin_dependency_is_independent_of_phy_parser_overrides(ethernet, loader, tmp_path):
    script = tmp_path / 'ethernet_extra' / 'custom_yt8531.py'
    script.parent.mkdir()
    script.write_text(
        'def get_phy_parsers():\n'
        '    return {"yt8531": {"parse": lambda *args: {}}}\n', encoding='utf-8',
    )
    with loader.phy_parser_context(board_dir=tmp_path):
        assert ethernet.get_dependencies({'chip': 'yt8531'}) == {'espressif/yt8531': '^0.2.0'}


def test_extension_sources_are_resolved_relative_to_script(ethernet, loader, tmp_path):
    script = tmp_path / 'ethernet_extra' / 'source.py'
    source = tmp_path / 'phy' / 'custom.c'
    script.parent.mkdir()
    source.parent.mkdir()
    source.write_text('/* custom runtime */\n', encoding='utf-8')
    script.write_text(
        'def get_phy_parsers():\n'
        '    return {"custom_phy": {"sources": ["../phy/custom.c"]}}\n', encoding='utf-8',
    )
    with loader.phy_parser_context(board_dir=tmp_path) as registry:
        assert registry['custom_phy']['sources'] == [str(source)]
        result = ethernet.parse('ethernet', _device('custom_phy'))
    assert result['runtime_sources'] == [str(source)]


@pytest.mark.parametrize('sources', ['"runtime.c"', '[3]', '[""]', '["missing.c"]'])
def test_extension_sources_are_validated(loader, tmp_path, sources):
    script = tmp_path / 'ethernet_extra' / 'invalid_sources.py'
    script.parent.mkdir()
    script.write_text(
        f'def get_phy_parsers():\n    return {{"invalid": {{"sources": {sources}}}}}\n', encoding='utf-8',
    )
    with pytest.raises(ValueError, match='sources|source'):
        with loader.phy_parser_context(board_dir=tmp_path):
            pass


@pytest.mark.parametrize(('suffix', 'supported'), [
    ('.c', True), ('.cc', True), ('.cpp', True), ('.cxx', True),
    ('.s', True), ('.S', True), ('.h', False), ('.hpp', False), ('.py', False),
])
def test_runtime_sources_require_compilable_suffix(loader, tmp_path, suffix, supported):
    script = tmp_path / 'ethernet_extra' / 'custom.py'
    source = tmp_path / f'runtime{suffix}'
    script.parent.mkdir()
    source.write_text('/* runtime */\n', encoding='utf-8')
    script.write_text(
        'def get_phy_parsers():\n'
        f'    return {{"custom_phy": {{"sources": ["../runtime{suffix}"]}}}}\n', encoding='utf-8',
    )
    if supported:
        with loader.phy_parser_context(board_dir=tmp_path) as registry:
            assert registry['custom_phy']['sources'] == [str(source)]
    else:
        with pytest.raises(ValueError, match='source'):
            with loader.phy_parser_context(board_dir=tmp_path):
                pass


def test_one_extension_file_can_register_multiple_chips(loader, tmp_path):
    script = tmp_path / 'board' / 'ethernet_extra' / 'several.py'
    _write_extension(script)
    script.write_text(script.read_text(encoding='utf-8').replace(
        "return {'custom_phy':", "return {'second_phy': {'parse': parse_custom}, 'custom_phy':"
    ), encoding='utf-8')
    with loader.phy_parser_context(board_dir=script.parent.parent) as registry:
        assert 'custom_phy' in registry and 'second_phy' in registry
        assert registry['second_phy']['sources'] == []


def test_extension_scope_priority_and_explicit_order(loader, tmp_path):
    builtin_parse = loader.current_phy_parser_registry()['yt8531']['parse']
    board = tmp_path / 'board'
    _write_extension(board / 'ethernet_extra' / 'same.py', 'yt8531', 1)
    first = _write_extension(tmp_path / 'amend' / 'first.py', 'yt8531', 2)
    last = _write_extension(tmp_path / 'amend' / 'last.py', 'yt8531', 3)
    with loader.phy_parser_context(board_dir=board) as registry:
        assert registry['yt8531']['parse']('test', {}, {})['struct_init']['marker'] == 1
    with loader.phy_parser_context(board_dir=board, python_files=[first, last]) as registry:
        assert registry['yt8531']['parse']('test', {}, {})['struct_init']['marker'] == 3
    assert loader.current_phy_parser_registry()['yt8531']['parse'] is builtin_parse


def test_duplicate_chip_in_one_scope_reports_both_files(loader, tmp_path):
    first = _write_extension(tmp_path / 'ethernet_extra' / 'first.py')
    second = _write_extension(tmp_path / 'ethernet_extra' / 'second.py')
    with pytest.raises(ValueError) as error:
        with loader.phy_parser_context(board_dir=tmp_path):
            pass
    assert str(first) in str(error.value) and str(second) in str(error.value)


def test_nested_context_restores_registry_after_failure(loader, tmp_path):
    outer = tmp_path / 'outer'
    inner = tmp_path / 'inner'
    _write_extension(outer / 'ethernet_extra' / 'custom.py', marker=1)
    _write_extension(inner / 'ethernet_extra' / 'custom.py', marker=2)
    with loader.phy_parser_context(board_dir=outer) as registry:
        original_parse = registry['custom_phy']['parse']
        with pytest.raises(RuntimeError, match='test failure'):
            with loader.phy_parser_context(board_dir=inner):
                raise RuntimeError('test failure')
        assert loader.current_phy_parser_registry()['custom_phy']['parse'] is original_parse
    assert 'custom_phy' not in loader.current_phy_parser_registry()


def test_module_cache_is_by_absolute_path(loader, tmp_path):
    first = tmp_path / 'first'
    second = tmp_path / 'second'
    _write_extension(first / 'ethernet_extra' / 'same.py', marker=1)
    _write_extension(second / 'ethernet_extra' / 'same.py', marker=2)
    with loader.phy_parser_context(board_dir=first) as registry:
        first_parse = registry['custom_phy']['parse']
    with loader.phy_parser_context(board_dir=first) as registry:
        assert registry['custom_phy']['parse'] is first_parse
    with loader.phy_parser_context(board_dir=second) as registry:
        assert registry['custom_phy']['parse'] is not first_parse
        assert registry['custom_phy']['parse']('test', {}, {})['struct_init']['marker'] == 2


def test_module_cache_refreshes_changed_source(loader, tmp_path):
    script = _write_extension(tmp_path / 'ethernet_extra' / 'custom.py', marker=1)
    with loader.phy_parser_context(board_dir=tmp_path) as registry:
        previous = registry['custom_phy']['parse']
    _write_extension(script, marker=2)
    with loader.phy_parser_context(board_dir=tmp_path) as registry:
        assert registry['custom_phy']['parse'] is not previous
        assert registry['custom_phy']['parse']('test', {}, {})['struct_init']['marker'] == 2


@pytest.mark.parametrize('descriptor', [
    'None',
    '{"parse": 7}',
    '{"parse": lambda *args: {}, "dependencies": {"vendor/phy": "^1.0.0"}}',
    '{"parse": lambda *args: {}, "extra": True}',
])
def test_extension_descriptor_is_validated(loader, tmp_path, descriptor):
    script = tmp_path / 'ethernet_extra' / 'invalid.py'
    script.parent.mkdir()
    script.write_text(f'def get_phy_parsers():\n    return {{"invalid": {descriptor}}}\n', encoding='utf-8')
    with pytest.raises(ValueError, match='invalid|descriptor|dependenc|parse'):
        with loader.phy_parser_context(board_dir=tmp_path):
            pass


def test_extension_import_failure_is_not_swallowed(loader, tmp_path):
    script = tmp_path / 'ethernet_extra' / 'broken.py'
    script.parent.mkdir()
    script.write_text('raise RuntimeError("extension import failed")\n', encoding='utf-8')
    with pytest.raises(RuntimeError, match='extension import failed'):
        with loader.phy_parser_context(board_dir=tmp_path):
            pass


def test_custom_phy_produces_typed_vendor_object(ethernet, loader, tmp_path):
    _write_extension(tmp_path / 'ethernet_extra' / 'custom.py')
    with loader.phy_parser_context(board_dir=tmp_path):
        result = ethernet.parse('lan-port', _device('custom_phy', vendor_config={'gain': 6}))
        assert ethernet.get_dependencies({'chip': 'custom_phy'}) == {}
    assert result['includes'] == ['custom_phy.h']
    assert result['extra_configs'] == [{
        'struct_type': 'const custom_phy_config_t',
        'struct_var': 'esp_bmgr_lan_port_vendor_config',
        'struct_init': {'marker': 1, 'gain': 6},
    }]
    assert result['struct_init']['vendor_config'] == '&esp_bmgr_lan_port_vendor_config'
    assert result['struct_init']['vendor_config_size'] == 'sizeof(esp_bmgr_lan_port_vendor_config)'


def test_vendor_parser_receives_device_context(ethernet, loader, tmp_path, monkeypatch):
    script = tmp_path / 'ethernet_extra' / 'context.py'
    script.parent.mkdir()
    script.write_text(
        'def parse_context(name, vendor_config, context):\n'
        '    assert name == "ethernet"\n'
        '    assert context == {"chip": "context_phy", "sub_type": "emac_rmii", "idf_version": (6, 1, 0), "target": ""}\n'
        '    return {"struct_type": "context_config_t", "struct_init": {}, "includes": []}\n'
        'def get_phy_parsers():\n'
        '    return {"context_phy": {"parse": parse_context}}\n', encoding='utf-8',
    )
    monkeypatch.setattr(ethernet, 'get_idf_version', lambda: (6, 1, 0))
    with loader.phy_parser_context(board_dir=tmp_path):
        ethernet.parse('ethernet', _device('context_phy', vendor_config={}))


@pytest.mark.parametrize('result', [
    'None',
    '{"struct_type": 1, "struct_init": {}, "includes": []}',
    '{"struct_type": "cfg_t", "struct_init": [], "includes": []}',
    '{"struct_type": "cfg_t", "struct_init": {}, "includes": "phy.h"}',
])
def test_vendor_parser_output_is_validated(ethernet, loader, tmp_path, result):
    script = tmp_path / 'ethernet_extra' / 'invalid_output.py'
    script.parent.mkdir()
    script.write_text(
        f'def parse_output(*args):\n    return {result}\n'
        'def get_phy_parsers():\n    return {"invalid": {"parse": parse_output}}\n', encoding='utf-8',
    )
    with loader.phy_parser_context(board_dir=tmp_path):
        with pytest.raises(ValueError, match='vendor|PHY|parser|includes|struct'):
            ethernet.parse('ethernet', _device('invalid', vendor_config={}))


def _yt8531_configuration():
    return {'rxc_dly_en': True, 'rx_delay_sel': 0, 'tx_delay_sel': 13, 'tx_delay_sel_fe': 13}


def test_yt8531_uses_native_vendor_type(loader):
    callback = loader.current_phy_parser_registry()['yt8531']['parse']
    config = _yt8531_configuration()
    result = callback('ethernet', config, {'sub_type': 'emac_rgmii'})
    assert result == {
        'struct_type': 'yt8531_rgmii_clk_delay_config_t',
        'struct_init': config,
        'includes': ['esp_eth_phy_yt8531.h'],
    }


@pytest.mark.parametrize(('key', 'value'), [
    ('rxc_dly_en', 1), ('rx_delay_sel', -1), ('rx_delay_sel', 16),
    ('tx_delay_sel', True), ('tx_delay_sel_fe', 16),
])
def test_yt8531_validates_native_vendor_fields(loader, key, value):
    config = {**_yt8531_configuration(), key: value}
    with pytest.raises(ValueError, match=key):
        loader.current_phy_parser_registry()['yt8531']['parse']('ethernet', config, {'sub_type': 'emac_rgmii'})


def test_yt8531_vendor_config_requires_rgmii(ethernet):
    with pytest.raises(ValueError, match='RGMII|emac_rgmii'):
        ethernet.parse('ethernet', _device('yt8531', vendor_config=_yt8531_configuration()))


def test_yt8531_rejects_unsupported_config_field(ethernet):
    with pytest.raises(ValueError, match='config contains unsupported fields: yt8531_rgmii_delay'):
        ethernet.parse('ethernet', _device('yt8531', yt8531_rgmii_delay=_yt8531_configuration()))
