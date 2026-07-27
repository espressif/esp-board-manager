# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

"""Focused tests for custom power control device support."""

import sys

import pytest


def test_power_ctrl_custom_parser_outputs_dependent_peripherals(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_power_ctrl import dev_power_ctrl as mod

    result = mod.parse(
        'board_power_ctrl',
        {
            'type': 'power_ctrl',
            'sub_type': 'custom',
            'peripherals': [
                {'name': 'i2c_pmic'},
                {'name': 'gpio_lcd_power'},
            ],
            'config': {
                'i2c_addr': 0x34,
                'startup_delay_ms': 10,
            },
        },
        peripherals_dict={
            'i2c_pmic': object(),
            'gpio_lcd_power': object(),
        },
    )

    custom_cfg = result['struct_init']['sub_cfg']['custom']

    assert result['struct_type'] == 'dev_power_ctrl_config_t'
    assert result['struct_init']['sub_type'] == 'custom'
    assert custom_cfg['periph_count'] == 2
    assert custom_cfg['periph_names'] == 'esp_bmgr_board_power_ctrl_periph_names'
    assert custom_cfg['user_cfg'] == '&esp_bmgr_board_power_ctrl_custom_cfg'
    assert custom_cfg['user_cfg_size'] == 'sizeof(esp_bmgr_board_power_ctrl_custom_cfg)'
    assert result['extra_configs'][0]['struct_init'] == ['i2c_pmic', 'gpio_lcd_power']
    assert 'struct_definition' in result


def test_power_ctrl_custom_parser_allows_no_dependent_peripherals(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_power_ctrl import dev_power_ctrl as mod

    result = mod.parse(
        'board_power_ctrl',
        {
            'type': 'power_ctrl',
            'sub_type': 'custom',
        },
        peripherals_dict={},
    )

    custom_cfg = result['struct_init']['sub_cfg']['custom']

    assert custom_cfg['periph_names'] is None
    assert custom_cfg['periph_count'] == 0
    assert custom_cfg['user_cfg'] is None
    assert custom_cfg['user_cfg_size'] == 0
    assert 'extra_configs' not in result


def test_power_ctrl_custom_parser_rejects_unknown_peripheral(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_power_ctrl import dev_power_ctrl as mod

    with pytest.raises(ValueError, match="references unknown peripheral 'missing_pmic'"):
        mod.parse(
            'board_power_ctrl',
            {
                'type': 'power_ctrl',
                'sub_type': 'custom',
                'peripherals': [{'name': 'missing_pmic'}],
            },
            peripherals_dict={},
        )


def test_power_ctrl_custom_parser_rejects_peripheral_without_name(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_power_ctrl import dev_power_ctrl as mod

    with pytest.raises(ValueError, match='peripheral entry without name'):
        mod.parse(
            'board_power_ctrl',
            {
                'type': 'power_ctrl',
                'sub_type': 'custom',
                'peripherals': [{}],
            },
            peripherals_dict={},
        )


@pytest.mark.parametrize('name', ('gpio_power_ctrl', 'custom_power_ctrl'))
def test_power_ctrl_custom_parser_rejects_reserved_dispatch_names(bmgr_root, name):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_power_ctrl import dev_power_ctrl as mod

    with pytest.raises(ValueError, match='collides with the internal power_ctrl dispatch key'):
        mod.parse(
            name,
            {
                'type': 'power_ctrl',
                'sub_type': 'custom',
            },
            peripherals_dict={},
        )


def test_power_ctrl_custom_allows_board_specific_config_fields(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from generators.schema_validator import DeviceSchemaValidator

    validator = DeviceSchemaValidator(bmgr_root / 'devices')

    assert validator.validate_config(
        device_type='power_ctrl',
        sub_type='custom',
        config={'pmic_rail': 'dldo1', 'startup_delay_ms': 10},
        device_name='board_power_ctrl',
    ) == []


def test_custom_power_ctrl_board_defaults_enable_custom_subtype(bmgr_root, tmp_path):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    board_dir = tmp_path / 'custom_power_ctrl_board'
    board_dir.mkdir()
    (board_dir / 'board_info.yaml').write_text('board: custom_power_ctrl_board\nchip: esp32s3\n', encoding='utf-8')
    (board_dir / 'board_peripherals.yaml').write_text(
        """
peripherals:
  - name: i2c_pmic
    type: i2c
    role: master
    config:
      port: 0
      clk_speed: 400000
      sda_io_num: 8
      scl_io_num: 9
  - name: gpio_lcd_power
    type: gpio
    config:
      pin: 10
      mode: GPIO_MODE_OUTPUT
""",
        encoding='utf-8',
    )
    (board_dir / 'board_devices.yaml').write_text(
        """
devices:
  - name: board_power_ctrl
    type: power_ctrl
    sub_type: custom
    peripherals:
      - name: i2c_pmic
      - name: gpio_lcd_power
    config:
      i2c_addr: 0x34
      startup_delay_ms: 10
""",
        encoding='utf-8',
    )

    generator = BoardConfigGenerator(bmgr_root)
    generator.project_root = str(tmp_path)
    peripherals_dict, periph_name_map, peripheral_types = generator.process_peripherals(
        str(board_dir / 'board_peripherals.yaml')
    )
    device_types, device_subtypes = generator.process_devices(
        str(board_dir / 'board_devices.yaml'),
        peripherals_dict,
        periph_name_map,
    )
    output_file = tmp_path / 'components' / 'gen_bmgr_codes' / 'board_manager.defaults'

    generator.sdkconfig_manager.generate_board_manager_defaults(
        board_path=str(board_dir),
        project_path=str(tmp_path),
        board_name='custom_power_ctrl_board',
        chip_name='esp32s3',
        output_file=str(output_file),
        device_types=device_types,
        peripheral_types=peripheral_types,
        device_subtypes=device_subtypes,
    )

    defaults = output_file.read_text(encoding='utf-8')
    generated_c = (tmp_path / 'components' / 'gen_bmgr_codes' / 'gen_board_device_config.c').read_text(encoding='utf-8')
    generated_custom_h = (tmp_path / 'components' / 'gen_bmgr_codes' / 'gen_board_device_custom.h').read_text(encoding='utf-8')

    assert 'CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUPPORT=y' in defaults
    assert 'CONFIG_ESP_BOARD_DEV_POWER_CTRL_SUB_CUSTOM_SUPPORT=y' in defaults
    assert 'static const char * esp_bmgr_board_power_ctrl_periph_names[] = {' in generated_c
    assert '"i2c_pmic", "gpio_lcd_power"' in generated_c
    assert 'dev_custom_board_power_ctrl_custom_config_t' in generated_c
    assert '.periph_names = esp_bmgr_board_power_ctrl_periph_names,' in generated_c
    assert '.periph_count = 2,' in generated_c
    assert '.user_cfg = &esp_bmgr_board_power_ctrl_custom_cfg,' in generated_c
    assert '.user_cfg_size = sizeof(esp_bmgr_board_power_ctrl_custom_cfg),' in generated_c
    assert '#include "dev_custom.h"' not in generated_custom_h
