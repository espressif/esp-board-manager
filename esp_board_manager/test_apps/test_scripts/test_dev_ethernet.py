# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

"""Parser and generation coverage for the ethernet device (EMAC RMII/RGMII subtypes)."""

from pathlib import Path
import sys

import pytest
import yaml


@pytest.fixture
def s31_soc(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils.soc_capability_query import clear_soc_capabilities, configure_soc_capabilities

    configure_soc_capabilities(bmgr_root / 'private_inc' / 'soc_capability_catalog', '6.1.0', 'esp32s31')
    yield
    clear_soc_capabilities()


def _rgmii_config():
    return {
        'phy_config': {'phy_addr': -1, 'reset_gpio_num': 7},
        'smi_gpio': {'mdc_num': 5, 'mdio_num': 6},
        'clock_config': {
            'clock_rx_gpio': 14,
            'clock_tx_gpio': 13,
            'clock_phy_ref_gpio': -1,
        },
        'dataif_gpio': {
            'tx_ctl_num': 12,
            'txd0_num': 8,
            'txd1_num': 9,
            'txd2_num': 10,
            'txd3_num': 11,
            'rx_ctl_num': 15,
            'rxd0_num': 19,
            'rxd1_num': 18,
            'rxd2_num': 17,
            'rxd3_num': 16,
        },
    }


def _rmii_config():
    return {
        'phy_config': {'phy_addr': 1, 'reset_gpio_num': 5},
        'smi_gpio': {'mdc_num': 23, 'mdio_num': 18},
        'clock_config': {
            'clock_mode': 'EMAC_CLK_OUT',
            'clock_gpio': 0,
        },
    }


def _rmii_dataif_gpio():
    return {
        'tx_en_num': 21,
        'txd0_num': 19,
        'txd1_num': 22,
        'crs_dv_num': 25,
        'rxd0_num': 26,
        'rxd1_num': 27,
    }


def _compact_c(text):
    return ' '.join(text.split())


def test_ethernet_parser_populates_native_configuration_defaults(bmgr_root, s31_soc):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet

    config = _rmii_config()
    config['dataif_gpio'] = _rmii_dataif_gpio()
    init = dev_ethernet.parse('ethernet', {
        'sub_type': 'emac_rmii', 'config': config,
    })['struct_init']
    assert init['mac_config'] == {
        'sw_reset_timeout_ms': 100, 'rx_task_stack_size': 4096, 'rx_task_prio': 15, 'flags': 0,
    }
    assert init['phy_config'] == {
        'phy_addr': 1, 'reset_gpio_num': 5, 'reset_timeout_ms': 100,
        'autonego_timeout_ms': 4000, 'hw_reset_assert_time_us': 0, 'post_hw_reset_delay_ms': 0,
    }
    assert init['check_link_period_ms'] == 2000
    emac = init['sub_cfg']['emac_rmii']
    assert emac['dma_burst_len'] == 'ETH_DMA_BURST_LEN_16'
    assert emac['intr_priority'] == 0 and emac['mdc_freq_hz'] == 0
    assert emac['clock_config_out_in'] == {'rmii': {'clock_mode': 'EMAC_CLK_EXT_IN', 'clock_gpio': -1}}
    assert init['vendor_config'] is None
    assert init['vendor_config_size'] == 0


def test_ethernet_parser_exposes_all_native_creation_fields(bmgr_root, s31_soc):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet

    config = _rmii_config()
    mac_config = {
        'sw_reset_timeout_ms': 250, 'rx_task_stack_size': 6144, 'rx_task_prio': 17,
        'flags': ['ETH_MAC_FLAG_WORK_WITH_CACHE_DISABLE', 'ETH_MAC_FLAG_PIN_TO_CORE'],
    }
    phy_config = {
        'phy_addr': 3, 'reset_gpio_num': -1, 'reset_timeout_ms': 200,
        'autonego_timeout_ms': 5000, 'hw_reset_assert_time_us': 15000, 'post_hw_reset_delay_ms': -1,
    }
    config.update({
        'mac_config': mac_config, 'phy_config': phy_config,
        'dataif_gpio': _rmii_dataif_gpio(), 'dma_burst_len': 'ETH_DMA_BURST_LEN_8',
        'intr_priority': 3, 'mdc_freq_hz': 1000000, 'check_link_period_ms': 300,
        'clock_config_out_in': {'clock_mode': 'EMAC_CLK_EXT_IN', 'clock_gpio': 20},
    })
    init = dev_ethernet.parse('ethernet', {'sub_type': 'emac_rmii', 'config': config})['struct_init']
    assert init['mac_config'] == {**mac_config, 'flags': ' | '.join(mac_config['flags'])}
    assert init['phy_config'] == phy_config
    assert init['check_link_period_ms'] == 300
    emac = init['sub_cfg']['emac_rmii']
    assert emac['dma_burst_len'] == 'ETH_DMA_BURST_LEN_8'
    assert emac['intr_priority'] == 3 and emac['mdc_freq_hz'] == 1000000
    assert emac['clock_config_out_in'] == {'rmii': config['clock_config_out_in']}


@pytest.mark.parametrize(('extra', 'error'), [
    ({'mac_config': {'rx_task_stack_size': 0}}, 'rx_task_stack_size'),
    ({'mac_config': {'rx_task_prio': True}}, 'rx_task_prio'),
    ({'mac_config': {'rx_task_prio': 25}}, 'rx_task_prio'),
    ({'mac_config': {'sw_reset_timeout_ms': 9}}, 'sw_reset_timeout_ms'),
    ({'mac_config': {'flags': 4}}, 'flags'),
    ({'mac_config': {'flags': ['ETH_MAC_FLAG_UNKNOWN']}}, 'flags'),
    ({'mac_config': {'stack_size': 4096}}, 'stack_size'),
    ({'phy_config': {'phy_addr': 32}}, 'phy_addr'),
    ({'phy_config': {'hw_reset_assert_time_us': -1}}, 'hw_reset_assert_time_us'),
    ({'phy_config': {'post_hw_reset_delay_ms': -2}}, 'post_hw_reset_delay_ms'),
    ({'phy_config': {'reset_timeout_ms': 2**32}}, 'reset_timeout_ms'),
    ({'phy_config': {'reset_timeout_ms': 9}}, 'reset_timeout_ms'),
    ({'dma_burst_len': 'ETH_DMA_BURST_LEN_32'}, 'dma_burst_len'),
    ({'dma_burst_len': 16}, 'dma_burst_len'),
    ({'intr_priority': 4}, 'intr_priority'),
    ({'mdc_freq_hz': 2**31}, 'mdc_freq_hz'),
    ({'check_link_period_ms': 0}, 'check_link_period_ms'),
    ({'yt8531_rgmii_delay': {'rxc_dly_en': True, 'rx_delay_sel': 0, 'tx_delay_sel': 13, 'tx_delay_sel_fe': 13}}, 'yt8531_rgmii_delay'),
])
def test_ethernet_parser_validates_creation_fields(bmgr_root, s31_soc, extra, error):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet

    config = _rmii_config()
    config.update({'dataif_gpio': _rmii_dataif_gpio(), **extra})
    with pytest.raises(ValueError, match=error):
        dev_ethernet.parse('ethernet', {'sub_type': 'emac_rmii', 'config': config})


@pytest.mark.parametrize(('key', 'value'), [
    ('phy_addr', 2),
    ('phy_reset_gpio_num', 7),
    ('interface', 'EMAC_DATA_INTERFACE_RMII'),
    ('yt8531_rgmii_delay', {}),
    ('check_link_period_mss', 100),
])
def test_ethernet_parser_rejects_unsupported_config_fields(bmgr_root, key, value):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet

    config = {**_rmii_config(), key: value}
    with pytest.raises(ValueError, match=f'config contains unsupported fields: {key}'):
        dev_ethernet.parse('ethernet', {'sub_type': 'emac_rmii', 'config': config})


def test_ethernet_parser_defaults_omitted_phy_config(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet

    config = _rmii_config()
    config.pop('phy_config')
    init = dev_ethernet.parse('ethernet', {'sub_type': 'emac_rmii', 'config': config})['struct_init']
    assert init['phy_config']['phy_addr'] == -1
    assert init['phy_config']['reset_gpio_num'] == -1


@pytest.mark.parametrize(('target', 'out_in', 'error'), [
    ('esp32', {'clock_mode': 'EMAC_CLK_EXT_IN', 'clock_gpio': 20}, 'clock_config_out_in'),
    ('esp32p4', None, 'clock_config_out_in'),
    ('esp32p4', {'clock_mode': 'EMAC_CLK_EXT_IN', 'clock_gpio': -1}, 'clock_config_out_in'),
    ('esp32p4', {'clock_mode': 'EMAC_CLK_OUT', 'clock_gpio': 20}, 'clock_config_out_in'),
    ('esp32p4', {'clock_mode': 'EMAC_CLK_EXT_IN', 'clock_gpio': 50}, None),
])
def test_ethernet_parser_validates_rmii_clock_loopback(bmgr_root, target, out_in, error):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet
    from generators.utils.soc_capability_query import clear_soc_capabilities, configure_soc_capabilities

    configure_soc_capabilities(bmgr_root / 'private_inc/soc_capability_catalog', '6.1.0', target)
    config = _rmii_config()
    if target != 'esp32':
        config['dataif_gpio'] = _rmii_dataif_gpio()
    if out_in is not None:
        config['clock_config_out_in'] = out_in
    try:
        if error:
            with pytest.raises(ValueError, match=error):
                dev_ethernet.parse('ethernet', {'sub_type': 'emac_rmii', 'config': config})
        else:
            init = dev_ethernet.parse('ethernet', {'sub_type': 'emac_rmii', 'config': config})['struct_init']
            assert init['sub_cfg']['emac_rmii']['clock_config_out_in']['rmii'] == out_in
    finally:
        clear_soc_capabilities()


@pytest.mark.parametrize('delay', [
    {'rxc_dly_en': False, 'rx_delay_sel': 0, 'tx_delay_sel': 0, 'tx_delay_sel_fe': 0},
    {'rxc_dly_en': True, 'rx_delay_sel': 2, 'tx_delay_sel': 11, 'tx_delay_sel_fe': 15},
])
def test_ethernet_parser_exposes_yt8531_delay(bmgr_root, s31_soc, monkeypatch, delay):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet

    monkeypatch.setattr(dev_ethernet, 'get_idf_version', lambda: (6, 1, 0))
    result = dev_ethernet.parse('ethernet', {
        'sub_type': 'emac_rgmii', 'chip': 'yt8531',
        'config': {**_rgmii_config(), 'vendor_config': delay},
    })
    assert result['extra_configs'][0]['struct_init'] == delay
    assert result['extra_configs'][0]['struct_type'] == 'const yt8531_rgmii_clk_delay_config_t'
    assert result['struct_init']['vendor_config'] == '&esp_bmgr_ethernet_vendor_config'


@pytest.mark.parametrize(('device', 'expected'), [
    ({'type': 'ethernet', 'chip': 'yt8531'}, {'espressif/yt8531': '^0.2.0'}),
    ({'type': 'ethernet', 'chip': 'ip101'}, {}),
    ({'type': 'ethernet'}, {}),
    ({'type': 'custom', 'chip': 'yt8531'}, {}),
    ({'type': 'ethernet', 'chip': 'yt8531', 'gen_skip': True}, {}),
])
def test_ethernet_dependency_selection(bmgr_root, tmp_path, device, expected):
    sys.path.insert(0, str(bmgr_root))
    from generators.dependency_manager import DependencyManager

    dev_yaml = tmp_path / 'board_devices.yaml'
    dev_yaml.write_text(yaml.safe_dump({'devices': [device]}), encoding='utf-8')
    assert DependencyManager(bmgr_root).extract_device_dependencies(str(dev_yaml)) == expected


@pytest.mark.parametrize('explicit', [
    {'espressif/yt8531': '^0.3.0'},
    [{'espressif/yt8531': '^0.3.0'}],
    ['espressif/yt8531:^0.3.0'],
    {'espressif/yt8531': {'version': '^0.3.0', 'override_path': '../yt8531'}},
])
@pytest.mark.parametrize('explicit_first', [False, True])
def test_ethernet_explicit_dependency_overrides_default(bmgr_root, tmp_path, explicit, explicit_first):
    sys.path.insert(0, str(bmgr_root))
    from generators.dependency_manager import DependencyManager

    devices = [
        {'name': 'first', 'type': 'ethernet', 'chip': 'yt8531', 'dependencies': explicit},
        {'name': 'second', 'type': 'ethernet', 'chip': 'yt8531'},
    ]
    if not explicit_first:
        devices.reverse()
    dev_yaml = tmp_path / 'board_devices.yaml'
    dev_yaml.write_text(yaml.safe_dump({'devices': devices}), encoding='utf-8')
    expected = explicit['espressif/yt8531'] if isinstance(explicit, dict) else '^0.3.0'
    assert DependencyManager(bmgr_root).extract_device_dependencies(str(dev_yaml)) == {
        'espressif/yt8531': expected,
    }


@pytest.mark.parametrize(('source', 'base_chip', 'selected_chip'), [
    ('base', 'yt8531', 'yt8531'),
    ('amend', 'ip101', 'yt8531'),
    ('amend', 'yt8531', 'ip101'),
    ('revision', 'ip101', 'yt8531'),
])
def test_ethernet_chip_dependency_reaches_manifest(tmp_path, script_path, source, base_chip, selected_chip):
    import subprocess

    board = tmp_path / 'ethernet_board'
    board.mkdir()
    (board / 'board_info.yaml').write_text('board: ethernet_board\nchip: esp32\n', encoding='utf-8')
    (board / 'board_peripherals.yaml').write_text('peripherals: []\n', encoding='utf-8')
    (board / 'board_devices.yaml').write_text(yaml.safe_dump({'devices': [{
        'name': 'ethernet', 'type': 'ethernet', 'sub_type': 'emac_rmii',
        'chip': base_chip, 'config': _rmii_config(),
    }]}), encoding='utf-8')
    args = []
    if source != 'base':
        overlay_dir = board if source == 'revision' else tmp_path / 'amend'
        overlay_dir.mkdir(exist_ok=True)
        (overlay_dir / 'phy.yaml').write_text(yaml.safe_dump({'devices': [{
            'name': 'ethernet', 'chip': selected_chip,
        }]}), encoding='utf-8')
        manifest = ({'board_revisions': {'new_phy': ['phy.yaml']}} if source == 'revision'
                    else {'apply': ['phy.yaml']})
        (overlay_dir / 'board_amend.yaml').write_text(yaml.safe_dump(manifest), encoding='utf-8')
        if source == 'amend':
            args = ['-a', str(overlay_dir)]
    project = tmp_path / 'project'
    project.mkdir()
    result = subprocess.run(
        [sys.executable, str(script_path), '-b', str(board), '--project-dir', str(project), *args],
        cwd=project, capture_output=True, text=True, timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    manifest_path = project / 'components' / 'gen_bmgr_codes' / 'idf_component.yml'
    dependencies = yaml.safe_load(manifest_path.read_text(encoding='utf-8'))['dependencies']
    assert dependencies == ({'espressif/yt8531': '^0.2.0'} if selected_chip == 'yt8531' else {})


def _generate_device_c(bmgr_root, tmp_path, *, chip, devices):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator
    from generators.utils.soc_capability_query import (
        clear_soc_capabilities,
        configure_soc_capabilities,
    )

    case_dir = tmp_path / 'case'
    case_dir.mkdir(parents=True, exist_ok=True)
    periph_yaml = case_dir / 'board_peripherals.yaml'
    dev_yaml = case_dir / 'board_devices.yaml'
    periph_yaml.write_text(
        yaml.safe_dump({'peripherals': []}, sort_keys=False, allow_unicode=False),
        encoding='utf-8',
    )
    dev_yaml.write_text(
        yaml.safe_dump({'devices': devices}, sort_keys=False, allow_unicode=False),
        encoding='utf-8',
    )

    catalog_dir = Path(bmgr_root) / 'private_inc' / 'soc_capability_catalog'
    configure_soc_capabilities(catalog_dir, '6.1.0', chip)
    try:
        generator = BoardConfigGenerator(bmgr_root)
        generator.project_root = str(case_dir)
        peripherals_dict, periph_name_map, _ = generator.process_peripherals(str(periph_yaml))
        generator.process_devices(str(dev_yaml), peripherals_dict, periph_name_map)
        return (case_dir / 'components' / 'gen_bmgr_codes' / 'gen_board_device_config.c').read_text(
            encoding='utf-8'
        )
    finally:
        clear_soc_capabilities()


def test_ethernet_parser_emits_rgmii_union_arm(bmgr_root, monkeypatch, s31_soc):
    import sys
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet

    monkeypatch.setattr(dev_ethernet, 'get_idf_version', lambda: (6, 1, 0))
    result = dev_ethernet.parse(
        'ethernet',
        {
            'type': 'ethernet',
            'sub_type': 'emac_rgmii',
            'chip': 'yt8531',
            'config': _rgmii_config(),
        },
    )

    init = result['struct_init']
    assert result['struct_type'] == 'dev_ethernet_config_t'
    assert init['chip'] == 'yt8531'
    assert init['sub_type'] == 'emac_rgmii'
    assert init['phy_config']['phy_addr'] == -1
    assert init['phy_config']['reset_gpio_num'] == 7
    emac = init['sub_cfg']['emac_rgmii']
    assert 'interface' not in emac
    assert emac['smi_gpio'] == {'mdc_num': 5, 'mdio_num': 6}
    assert emac['clock_config'] == {
        'rgmii': {
            'clock_rx_gpio': 14,
            'clock_tx_gpio': 13,
            'clock_phy_ref_gpio': -1,
        },
    }
    assert 'rmii' not in emac['clock_config']
    assert set(emac['dataif_gpio'].keys()) == {'rgmii'}
    assert emac['dataif_gpio']['rgmii']['txd0_num'] == 8
    assert 'phy_post_init_func' not in init
    assert 'phy_post_init_func' not in emac


def test_ethernet_parser_rgmii_requires_idf_6_1(bmgr_root, monkeypatch, s31_soc):
    import sys
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet

    payload = {
        'type': 'ethernet',
        'sub_type': 'emac_rgmii',
        'chip': 'yt8531',
        'config': _rgmii_config(),
    }

    monkeypatch.setattr(dev_ethernet, 'get_idf_version', lambda: (6, 0, 1))
    with pytest.raises(ValueError, match='requires ESP-IDF v6.1'):
        dev_ethernet.parse('ethernet', payload)

    monkeypatch.setattr(dev_ethernet, 'get_idf_version', lambda: (6, 1, 0))
    result = dev_ethernet.parse('ethernet', payload)
    assert 'interface' not in result['struct_init']['sub_cfg']['emac_rgmii']
    assert 'rgmii' in result['struct_init']['sub_cfg']['emac_rgmii']['clock_config']


def test_ethernet_parser_emits_rmii_without_dataif(bmgr_root):
    import sys
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet

    result = dev_ethernet.parse(
        'ethernet',
        {
            'type': 'ethernet',
            'sub_type': 'emac_rmii',
            'chip': 'ip101',
            'config': _rmii_config(),
        },
    )

    emac = result['struct_init']['sub_cfg']['emac_rmii']
    assert 'interface' not in emac
    assert emac['clock_config'] == {
        'rmii': {
            'clock_mode': 'EMAC_CLK_OUT',
            'clock_gpio': 0,
        },
    }
    assert 'rgmii' not in emac['clock_config']
    assert 'dataif_gpio' not in emac


def test_ethernet_parser_optional_chip_defaults_empty(bmgr_root):
    import sys
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet

    result = dev_ethernet.parse(
        'ethernet',
        {'type': 'ethernet', 'sub_type': 'emac_rmii', 'config': _rmii_config()},
    )
    assert result['struct_init']['chip'] == ''


def test_ethernet_io_lists_follow_subtype(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet

    rmii = set(dev_ethernet.DEV_ETHERNET_IO_LIST['emac_rmii'])
    rgmii = set(dev_ethernet.DEV_ETHERNET_IO_LIST['emac_rgmii'])
    assert {'mdc_num', 'clock_gpio', 'tx_en_num', 'txd0_num'} <= rmii
    assert {'mdc_num', 'clock_rx_gpio', 'tx_ctl_num', 'txd3_num'} <= rgmii
    assert 'clock_rx_gpio' not in rmii
    assert 'clock_gpio' not in rgmii


def test_ethernet_schema_accepts_rmii_gpio_fields_and_warns_on_typos(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from generators.schema_validator import DeviceSchemaValidator

    validator = DeviceSchemaValidator(bmgr_root / 'devices')
    config = _rmii_config()
    config['dataif_gpio'] = _rmii_dataif_gpio()
    assert validator.validate_config('ethernet', 'emac_rmii', config, 'ethernet') == []

    config['dataif_gpio']['crs_dv_pin'] = config['dataif_gpio'].pop('crs_dv_num')
    issues = validator.validate_config('ethernet', 'emac_rmii', config, 'ethernet')
    assert [(issue.level, issue.key_path) for issue in issues] == [
        ('WARNING', 'dataif_gpio.crs_dv_pin'),
    ]


@pytest.mark.parametrize('target', ['esp32', 'esp32p4', 'esp32s31'])
def test_complete_rmii_template_matches_soc_gpio_rules(bmgr_root, target):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet
    from generators.utils.soc_capability_query import clear_soc_capabilities, configure_soc_capabilities

    entries = yaml.safe_load((bmgr_root / 'devices/dev_ethernet/dev_ethernet.yaml').read_text())
    entry = next(item for item in entries if item['sub_type'] == 'emac_rmii')
    configure_soc_capabilities(bmgr_root / 'private_inc/soc_capability_catalog', '6.1.0', target)
    try:
        if target == 'esp32':
            with pytest.raises(ValueError, match='dataif_gpio is not supported'):
                dev_ethernet.parse('ethernet', entry)
            # Follow the classic ESP32 instructions in the template.
            del entry['config']['dataif_gpio'], entry['config']['clock_config_out_in']
            entry['config']['clock_config'] = {'clock_mode': 'EMAC_CLK_EXT_IN', 'clock_gpio': 0}
            entry['config']['smi_gpio'] = {'mdc_num': 23, 'mdio_num': 18}
        elif target == 'esp32s31':
            entry['config']['dma_burst_len'] = 'ETH_DMA_BURST_LEN_16'
            entry['config']['clock_config_out_in']['clock_gpio'] = -1
            entry['config']['dataif_gpio'] = _rmii_dataif_gpio()
        result = dev_ethernet.parse('ethernet', entry)
        emac = result['struct_init']['sub_cfg']['emac_rmii']
        if target == 'esp32':
            assert 'dataif_gpio' not in emac
        else:
            assert emac['dataif_gpio']['rmii'] == entry['config']['dataif_gpio']
            assert emac['clock_config_out_in']['rmii'] == entry['config']['clock_config_out_in']
    finally:
        clear_soc_capabilities()


def test_ethernet_subtype_has_kconfig_symbol(bmgr_root):
    import sys
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    kconfig = BoardConfigGenerator(bmgr_root).generate_components_kconfig()
    assert 'config ESP_BOARD_DEV_ETHERNET_SUPPORT' in kconfig
    assert 'config ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RMII_SUPPORT' in kconfig
    assert 'config ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RGMII_SUPPORT' in kconfig


@pytest.mark.parametrize(
    ('payload', 'error'),
    [
        ({'type': 'ethernet', 'config': _rmii_config()}, 'missing required'),
        ({'type': 'ethernet', 'sub_type': 'emac', 'config': _rmii_config()}, 'not yet supported'),
        ({'type': 'ethernet', 'sub_type': 'spi', 'config': _rmii_config()}, 'not yet supported'),
        ({'type': 'ethernet', 'sub_type': 'emac_rmii', 'config': {**_rmii_config(), 'interface': 'EMAC_DATA_INTERFACE_MII'}}, 'interface'),
        ({'type': 'ethernet', 'sub_type': 'emac_rmii', 'config': {**_rmii_config(), 'clock_config': {'clock_rx_gpio': 14}}}, 'clock_config'),
        ({'type': 'ethernet', 'sub_type': 'emac_rgmii', 'config': {**_rgmii_config(), 'clock_config': {'clock_gpio': 0}}}, 'clock_config'),
        ({'type': 'ethernet', 'sub_type': 'emac_rmii', 'config': {**_rmii_config(), 'dataif_gpio': {'tx_en_num': 21}}}, 'dataif_gpio'),
        ({'type': 'ethernet', 'sub_type': 'emac_rmii', 'peripherals': [{'name': 'gpio_eth'}], 'config': _rmii_config()}, 'does not support peripheral'),
    ],
)
def test_ethernet_parser_rejects_invalid_configuration(bmgr_root, monkeypatch, payload, error, s31_soc):
    import sys
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet

    monkeypatch.setattr(dev_ethernet, 'get_idf_version', lambda: (6, 1, 0))
    with pytest.raises(ValueError, match=error):
        dev_ethernet.parse('ethernet', payload)


def test_ethernet_parser_allows_rmii_dataif_on_multi_io_mux(bmgr_root, tmp_path):
    import sys
    sys.path.insert(0, str(bmgr_root))
    from generators.utils.soc_capability_query import (
        clear_soc_capabilities,
        configure_soc_capabilities,
    )
    from devices.dev_ethernet import dev_ethernet

    catalog_dir = bmgr_root / 'private_inc' / 'soc_capability_catalog'
    configure_soc_capabilities(catalog_dir, '6.1.0', 'esp32p4')
    try:
        config = _rmii_config()
        config['clock_config']['clock_mode'] = 'EMAC_CLK_EXT_IN'
        config['dataif_gpio'] = {
            'tx_en_num': 49,
            'txd0_num': 34,
            'txd1_num': 35,
            'crs_dv_num': 28,
            'rxd0_num': 29,
            'rxd1_num': 30,
        }
        result = dev_ethernet.parse('ethernet', {
            'type': 'ethernet',
            'sub_type': 'emac_rmii',
            'chip': 'ip101',
            'config': config,
        })
        assert 'rmii' in result['struct_init']['sub_cfg']['emac_rmii']['dataif_gpio']
    finally:
        clear_soc_capabilities()


def test_ethernet_generation_emits_rgmii_union_arm(bmgr_root, tmp_path, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet
    from generators import parser_loader

    monkeypatch.setattr(dev_ethernet, 'get_idf_version', lambda: (6, 1, 0))
    orig_load = parser_loader._load_parser_module
    monkeypatch.setattr(
        parser_loader,
        '_load_parser_module',
        lambda module_name, script_path: (
            dev_ethernet if module_name == 'dev_ethernet' else orig_load(module_name, script_path)
        ),
    )
    generated = _generate_device_c(
        bmgr_root,
        tmp_path,
        chip='esp32s31',
        devices=[
            {
                'name': 'ethernet',
                'type': 'ethernet',
                'sub_type': 'emac_rgmii',
                'chip': 'yt8531',
                'config': _rgmii_config(),
            }
        ],
    )
    compact = _compact_c(generated)

    assert 'yt8531' in generated
    assert '.emac_rgmii =' in generated
    assert '.interface =' not in generated
    assert '.clock_config = { .rgmii = {' in compact
    assert '.dataif_gpio = { .rgmii = {' in compact
    assert 'phy_post_init_func' not in generated
    assert '.clock_config = { .rmii' not in compact


def test_ethernet_generation_rejects_classic_esp32_rmii_dataif(bmgr_root, tmp_path):
    config = _rmii_config()
    config['dataif_gpio'] = _rmii_dataif_gpio()

    with pytest.raises(ValueError, match='dataif_gpio'):
        _generate_device_c(
            bmgr_root,
            tmp_path,
            chip='esp32',
            devices=[
                {
                    'name': 'ethernet',
                    'type': 'ethernet',
                    'sub_type': 'emac_rmii',
                    'chip': 'ip101',
                    'config': config,
                }
            ],
        )


def test_ethernet_generation_emits_classic_esp32_rmii_without_dataif(bmgr_root, tmp_path):
    generated = _generate_device_c(
        bmgr_root,
        tmp_path,
        chip='esp32',
        devices=[
            {
                'name': 'ethernet',
                'type': 'ethernet',
                'sub_type': 'emac_rmii',
                'chip': 'ip101',
                'config': _rmii_config(),
            }
        ],
    )
    compact = _compact_c(generated)

    assert '.clock_config = { .rmii = {' in compact
    assert 'dataif_gpio' not in generated


@pytest.mark.parametrize('chip', ['esp32', 'esp32p4', 'esp32s3'])
def test_rgmii_rejected_on_soc_without_gigabit_emac(bmgr_root, tmp_path, monkeypatch, chip):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_ethernet import dev_ethernet
    monkeypatch.setattr(dev_ethernet, 'get_idf_version', lambda: (6, 1, 0))
    with pytest.raises(ValueError, match='ethernet|EMAC|emac'):
        _generate_device_c(bmgr_root, tmp_path, chip=chip, devices=[{
            'name': 'ethernet', 'type': 'ethernet', 'sub_type': 'emac_rgmii',
            'chip': 'yt8531', 'config': _rgmii_config(),
        }])


def test_ethernet_subtypes_are_discovered_from_c_sources(bmgr_root, tmp_path):
    import shutil

    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    device = bmgr_root / 'devices' / 'dev_ethernet'
    for source in device.glob('dev_ethernet_sub_*.c'):
        shutil.copyfile(source, tmp_path / source.name)
    generator = BoardConfigGenerator(bmgr_root)
    assert generator._scan_device_sub_types(tmp_path, 'dev_ethernet') == ['emac_rgmii', 'emac_rmii']


def test_ethernet_creator_uses_distinct_subtypes_and_soc_filters(bmgr_root, monkeypatch):
    sys.path.insert(0, str(bmgr_root))
    from create_new_board import BoardCreator
    creator = BoardCreator(bmgr_root)
    monkeypatch.setattr(creator, '_get_soc_boolean_caps', lambda target: {
        'SOC_EMAC_SUPPORTED': target in ('esp32', 'esp32p4', 'esp32s31'),
        'SOC_EMAC_SUPPORT_1000M': target == 'esp32s31',
    })
    ethernet = [x for x in creator.device_list if x.startswith('ethernet')]
    assert sorted(ethernet) == ['ethernet_emac_rgmii', 'ethernet_emac_rmii']
    assert creator.filter_by_chip_capability('esp32', ethernet) == ['ethernet_emac_rmii']
    assert creator.filter_by_chip_capability('esp32p4', ethernet) == ['ethernet_emac_rmii']
    assert creator.filter_by_chip_capability('esp32s3', ethernet) == []
    assert set(creator.filter_by_chip_capability('esp32s31', ethernet)) == set(ethernet)
    for key in ethernet:
        entry = yaml.safe_load(creator.device_config_cache[key])[0]
        assert entry['name'] == 'ethernet'
        assert entry['sub_type'] == key[len('ethernet_'):]
        assert 'interface' not in entry['config']


@pytest.mark.parametrize(('target', 'rmii', 'rgmii'), [
    ('esp32', 1, 0), ('esp32p4', 1, 0), ('esp32s31', 1, 1),
    ('esp32s31', 0, 1), ('esp32', 1, 1), ('esp32p4', 1, 1),
])
@pytest.mark.parametrize('with_yt8531', [False, True])
def test_ethernet_runtime_with_native_idf_headers(bmgr_root, tmp_path, target, rmii, rgmii, with_yt8531, host_board_linker_flags):
    import os
    import shutil
    import subprocess

    idf_value = os.environ.get('IDF_PATH')
    if not idf_value or not shutil.which('cc'):
        pytest.skip('Native IDF headers and a host C compiler are required')
    idf = Path(idf_value)
    sys.path.insert(0, str(bmgr_root))
    from generators.utils.idf_version import get_idf_version
    if with_yt8531 and get_idf_version() < (6, 1, 0):
        pytest.skip('The YT8531 component requires ESP-IDF v6.1 or later')
    soc = idf / 'components' / 'soc' / target / 'include'
    if not (soc / 'soc' / 'soc_caps.h').exists():
        pytest.skip(f'{target} is absent from this IDF checkout')
    fixture = bmgr_root / 'test_apps' / 'host_tests' / 'ethernet'
    (tmp_path / 'sdkconfig.h').write_text(
        f'#define CONFIG_IDF_TARGET_{target.upper()} 1\n'
        '#define CONFIG_ETH_USE_ESP32_EMAC 1\n'
        '#define CONFIG_ETH_RMII_CLK_INPUT 1\n'
        '#define CONFIG_ETH_RMII_CLK_IN_GPIO 0\n'
        f'#define BMGR_TEST_YT8531 {int(with_yt8531)}\n'
        f'#define CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RMII_SUPPORT {rmii}\n'
        f'#define CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RGMII_SUPPORT {rgmii}\n',
        encoding='utf-8',
    )
    includes = [tmp_path, fixture / 'include', bmgr_root / 'devices' / 'dev_ethernet',
                bmgr_root / 'include', bmgr_root / 'private_inc', soc]
    includes += [idf / 'components' / name / 'include' for name in (
        'esp_eth', 'esp_common', 'soc', 'hal', 'esp_hal_emac',
    )]
    if with_yt8531:
        includes.append(fixture / 'yt8531')
    device = bmgr_root / 'devices' / 'dev_ethernet'
    binary = tmp_path / 'ethernet_host'
    command = ['cc', '-std=gnu11', '-Wall', '-Wextra', '-Werror']
    command += [flag for path in includes for flag in ('-I', str(path))]
    command += [str(fixture / 'host_main.c'), str(device / 'dev_ethernet_emac_common.c'),
                str(device / 'dev_ethernet_sub_emac_rmii.c'),
                str(device / 'dev_ethernet_sub_emac_rgmii.c'),
                *([str(device / 'ethernet_extra' / 'yt8531.c')] if with_yt8531 else []),
                *host_board_linker_flags,
                '-o', str(binary)]
    build = subprocess.run(command, capture_output=True, text=True, timeout=30)
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=5)
    assert run.returncode == 0, run.stderr
