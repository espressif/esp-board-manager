# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

"""Cover custom PHY configuration, device YAML dependencies, and runtime sources."""

import sys

import pytest
import yaml


def test_amend_python_is_explicit_and_is_not_a_c_source(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from generators.amend import build_amend_plan

    (tmp_path / 'phy.py').write_text('def get_phy_parsers(): return {}\n')
    (tmp_path / 'ignored.py').write_text('raise RuntimeError("not listed")\n')
    (tmp_path / 'board_amend.yaml').write_text('apply: [phy.py]\n')
    plan = build_amend_plan(tmp_path)
    assert plan.python_files() == [tmp_path / 'phy.py']
    assert plan.source_files() == []


def test_phy_schema_delegates_vendor_fields_but_keeps_common_typo_checks(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from generators.schema_validator import DeviceSchemaValidator

    issues = DeviceSchemaValidator(bmgr_root / 'devices').validate_config(
        'ethernet', 'emac_rmii',
        {'vendor_config': {'custom_field': {'level': 7}}, 'check_link_period_mss': 1}, 'eth',
    )
    assert [issue.key_path for issue in issues] == ['check_link_period_mss']


def test_custom_phy_dependencies_use_device_yaml_without_loading_extensions(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from generators.dependency_manager import DependencyManager

    extension_dir = tmp_path / 'ethernet_extra'
    extension_dir.mkdir()
    (extension_dir / 'custom.py').write_text('raise RuntimeError("dependency extraction loaded PHY extension")\n')
    dependencies = {'example/custom_phy': {'path': '${BOARD_PATH}/custom_phy'}}
    dev_yaml = tmp_path / 'board_devices.yaml'
    dev_yaml.write_text(yaml.safe_dump({'devices': [{
        'type': 'ethernet', 'chip': 'custom_phy', 'dependencies': dependencies,
    }]}))
    assert DependencyManager(bmgr_root).extract_device_dependencies(str(dev_yaml)) == dependencies


def _generate_custom_phy_fixture(bmgr_root, tmp_path, scope):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator
    from generators.amend import build_amend_plan
    from generators.utils.soc_capability_query import clear_soc_capabilities, configure_soc_capabilities

    board = tmp_path / 'board'
    project = tmp_path / 'project'
    board.mkdir()
    project.mkdir()
    plugin_dir = tmp_path / 'amend' if scope == 'amend' else board / 'ethernet_extra'
    plugin_dir.mkdir(parents=True)
    plugin = plugin_dir / 'custom.py'
    adapter = plugin_dir / 'custom.c'
    adapter.write_text('/* custom PHY registration source */\n')
    plugin.write_text('''def parse(name, config, context):
    if set(config) != {'capacity'}:
        raise ValueError('unknown custom PHY field')
    assert context['chip'] == 'custom_phy'
    return {'struct_type': 'custom_phy_config_t',
            'struct_init': {'capacity': config['capacity']},
            'includes': ['custom_phy.h']}

def get_phy_parsers():
    return {'custom_phy': {'parse': parse,
            'sources': ['custom.c']}}
''')
    plan = None
    if scope == 'amend':
        (plugin_dir / 'board_amend.yaml').write_text('apply: [custom.py]\n')
        plan = build_amend_plan(plugin_dir)
    dev_yaml = board / 'board_devices.yaml'
    dev_yaml.write_text(yaml.safe_dump({'devices': [{
        'name': 'eth', 'type': 'ethernet', 'sub_type': 'emac_rmii', 'chip': 'custom_phy',
        'dependencies': {'example/custom_phy': '^1.0.0'},
        'config': {'smi_gpio': {'mdc_num': 23, 'mdio_num': 18},
                   'clock_config': {'clock_mode': 'EMAC_CLK_OUT', 'clock_gpio': 0},
                   'vendor_config': {'capacity': 7}},
    }]}))
    generator = BoardConfigGenerator(bmgr_root, project_dir=str(project))
    generator.amend_plan = plan
    configure_soc_capabilities(bmgr_root / 'private_inc' / 'soc_capability_catalog', '6.1.0', 'esp32')
    try:
        dependencies = generator.dependency_manager.extract_device_dependencies(
            str(dev_yaml), amend_plan=plan,
        )
        assert dependencies == {'example/custom_phy': '^1.0.0'}
        generator.process_devices(str(dev_yaml), {}, {}, str(board))
        assert generator.setup_gen_bmgr_codes_component(str(project), str(board), dependencies)
        generated = project / 'components' / 'gen_bmgr_codes'
        source = (generated / 'gen_board_device_config.c').read_text()
        assert '#include "custom_phy.h"' in source
        assert 'static const custom_phy_config_t esp_bmgr_eth_vendor_config' in source
        assert '.vendor_config = &esp_bmgr_eth_vendor_config' in source
        assert '.vendor_config_size = sizeof(esp_bmgr_eth_vendor_config)' in source
        assert '.capacity = 7' in source
        cmake = (generated / 'CMakeLists.txt').read_text()
        assert 'custom.c' in cmake and 'custom.py' not in cmake
        assert 'WHOLE_ARCHIVE TRUE' in cmake
        manifest = yaml.safe_load((generated / 'idf_component.yml').read_text())
        assert manifest['dependencies'] == dependencies
    finally:
        clear_soc_capabilities()
    return generator, board, project, plugin


@pytest.mark.parametrize('scope', ['board', 'amend'])
def test_custom_phy_generates_native_config_dependencies_and_runtime_source(bmgr_root, tmp_path, scope):
    _generate_custom_phy_fixture(bmgr_root, tmp_path, scope)


def test_phy_revision_sources_form_union_and_reject_conflicting_adapters(bmgr_root, tmp_path):
    generator, board, project, plugin = _generate_custom_phy_fixture(bmgr_root, tmp_path, 'board')
    from generators.utils.soc_capability_query import clear_soc_capabilities, configure_soc_capabilities

    original_plugin = plugin.read_text()
    (plugin.parent / 'revision.py').write_text(
        original_plugin.replace('custom_phy', 'revision_phy').replace('custom.c', 'revision.c'))
    (plugin.parent / 'revision.c').write_text('/* revision PHY */\n')
    dev_yaml = board / 'board_devices.yaml'
    data = yaml.safe_load(dev_yaml.read_text())
    data['devices'][0]['chip'] = 'revision_phy'
    dev_yaml.write_text(yaml.safe_dump(data))
    configure_soc_capabilities(bmgr_root / 'private_inc' / 'soc_capability_catalog', '6.1.0', 'esp32')
    try:
        generator.active_revision = 'other_chip'
        generator.process_devices(str(dev_yaml), {}, {}, str(board))
        assert generator.setup_gen_bmgr_codes_component(str(project), str(board), {})
        cmake = (project / 'components' / 'gen_bmgr_codes' / 'CMakeLists.txt').read_text()
        assert 'custom.c' in cmake and 'revision.c' in cmake

        # A revision cannot replace a global registration for an existing chip.
        plugin.write_text(original_plugin.replace('custom.c', 'replacement.c'))
        (plugin.parent / 'replacement.c').write_text('/* conflicting adapter */\n')
        data['devices'][0]['chip'] = 'custom_phy'
        dev_yaml.write_text(yaml.safe_dump(data))
        generator.active_revision = 'same_chip'
        with pytest.raises(ValueError, match='Conflicting runtime sources'):
            generator.process_devices(str(dev_yaml), {}, {}, str(board))
    finally:
        clear_soc_capabilities()
