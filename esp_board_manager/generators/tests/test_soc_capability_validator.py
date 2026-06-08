# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from generators.utils.soc_capabilities import SocCapabilityCatalog
from generators.utils import soc_capability_validator
from generators.utils.soc_capability_validator import SocValidationField, SocValidationInstance, validate_soc_capabilities


def _catalog(display_lcd_i80_supported=True, i2s_std_supported=True, i2s_pdm_supported=True) -> SocCapabilityCatalog:
    return SocCapabilityCatalog.from_dict({
        'schemaVersion': 3,
        'profile': {'id': '5.5'},
        'capabilityDefs': {
            'device.display_lcd_i80': {'kind': 'device', 'type': 'display_lcd', 'sub_type': ['i80']},
            'peripheral.i2c': {'kind': 'peripheral', 'type': 'i2c'},
            'peripheral.adc_continuous': {'kind': 'peripheral', 'type': 'adc', 'role': ['continuous']},
            'peripheral.i2s_master_std': {'kind': 'peripheral', 'type': 'i2s', 'format': ['std-out']},
            'peripheral.i2s_master_pdm-in': {'kind': 'peripheral', 'type': 'i2s', 'format': ['pdm-in']},
        },
        'hardwareLimitDefs': {
            'i2c.instance_count': {
                'appliesTo': [{'kind': 'peripheral', 'type': 'i2c', 'check': 'instanceCount'}],
            },
            'lcd.i80_bus_width': {
                'appliesTo': [
                    {
                        'kind': 'device',
                        'type': 'display_lcd',
                        'sub_type': ['i80'],
                        'path': ['bus_width'],
                        'check': 'value',
                    },
                ],
            },
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
            'adc.unit_max': {
                'appliesTo': [
                    {
                        'kind': 'peripheral',
                        'type': 'adc',
                        'role': ['continuous'],
                        'path': ['unit'],
                        'check': 'value',
                    },
                ],
            },
            'adc.pattern_unit_max': {
                'appliesTo': [
                    {
                        'kind': 'peripheral',
                        'type': 'adc',
                        'role': ['continuous'],
                        'path': ['patterns', 'unit'],
                        'check': 'value',
                    },
                ],
            },
            'adc.max_channel_count': {
                'appliesTo': [
                    {
                        'kind': 'peripheral',
                        'type': 'adc',
                        'role': ['oneshot'],
                        'path': ['channel_id'],
                        'check': 'value',
                        'compare': 'lt',
                    },
                    {
                        'kind': 'peripheral',
                        'type': 'adc',
                        'role': ['continuous'],
                        'path': ['channel_list'],
                        'check': 'value',
                        'compare': 'lt',
                    },
                    {
                        'kind': 'peripheral',
                        'type': 'adc',
                        'role': ['continuous'],
                        'path': ['patterns', 'channel'],
                        'check': 'value',
                        'compare': 'lt',
                    },
                ],
            },
            'adc.unit_count': {
                'appliesTo': [
                    {
                        'kind': 'peripheral',
                        'type': 'adc',
                        'path': ['unit_id'],
                        'check': 'value',
                    },
                    {
                        'kind': 'peripheral',
                        'type': 'adc',
                        'path': ['patterns', 'unit'],
                        'check': 'value',
                    },
                ],
            },
            'ledc.channel_count': {
                'appliesTo': [
                    {
                        'kind': 'device',
                        'type': 'ledc_ctrl',
                        'path': ['channel'],
                        'check': 'value',
                    },
                ],
            },
            'gpio.number_max': {
                'appliesTo': [
                    {
                        'kind': 'peripheral',
                        'type': 'gpio_probe',
                        'path': ['gpio_num'],
                        'check': 'value',
                    },
                ],
            },
            'nested.channel_count': {
                'appliesTo': [
                    {
                        'kind': 'peripheral',
                        'type': 'nested_probe',
                        'path': ['outer', 'channels', 'id'],
                        'check': 'value',
                    },
                ],
            },
        },
        'chips': {
            'esp32s3': {
                'capabilities': {
                    'device.display_lcd_i80': display_lcd_i80_supported,
                    'peripheral.i2c': True,
                    'peripheral.adc_continuous': True,
                    'peripheral.i2s_master_std': i2s_std_supported,
                    'peripheral.i2s_master_pdm-in': i2s_pdm_supported,
                },
                'gpio': {'validInput': [0, 1, 2], 'validOutput': [0, 1]},
                'hardwareLimits': {
                    'i2c.instance_count': 1,
                    'lcd.i80_bus_width': 8,
                    'adc.pattern_length_max': 2,
                    'adc.unit_max': 1,
                    'adc.pattern_unit_max': 1,
                    'adc.max_channel_count': 10,
                    'adc.unit_count': 1,
                    'ledc.channel_count': 0,
                    'gpio.number_max': 9,
                    'nested.channel_count': 1,
                },
            },
        },
    })


def inst(instance_id, kind, typ, selectors=None, fields=None):
    return SocValidationInstance(
        instance_id=instance_id,
        kind=kind,
        type=typ,
        selectors=selectors or {},
        fields=fields or [],
    )


def field(path, value):
    return SocValidationField(path=path, value=value)


def test_rejects_unsupported_capability() -> None:
    issues = validate_soc_capabilities(
        catalog=_catalog(display_lcd_i80_supported=False),
        chip='esp32s3',
        instances=[inst('lcd0', 'device', 'display_lcd', {'sub_type': 'i80'})],
    )

    assert [i.code for i in issues] == ['SOC_CAPABILITY_UNSUPPORTED']
    assert issues[0].path == ['devices', 'lcd0']


def test_rejects_any_unsupported_capability_match_for_list_selectors() -> None:
    issues = validate_soc_capabilities(
        catalog=_catalog(i2s_std_supported=False, i2s_pdm_supported=True),
        chip='esp32s3',
        instances=[
            inst('i2s0', 'peripheral', 'i2s', {'format': ['std-out', 'pdm-in']}),
        ],
    )

    assert [i.code for i in issues] == ['SOC_CAPABILITY_UNSUPPORTED']
    assert issues[0].capability == 'peripheral.i2s_master_std'


def test_rejects_hardware_limit_value_array_length_and_instance_count() -> None:
    issues = validate_soc_capabilities(
        catalog=_catalog(),
        chip='esp32s3',
        instances=[
            inst('i2c0', 'peripheral', 'i2c'),
            inst('i2c1', 'peripheral', 'i2c'),
            inst('lcd0', 'device', 'display_lcd', {'sub_type': 'i80'}, [field(['bus_width'], '16')]),
            inst('adc0', 'peripheral', 'adc', {'role': 'continuous'}, [field(['patterns'], '[1, 2, 3]')]),
        ],
    )

    assert [i.code for i in issues] == [
        'SOC_NUMBER_LIMIT_EXCEEDED',
        'SOC_NUMBER_LIMIT_EXCEEDED',
        'SOC_NUMBER_LIMIT_EXCEEDED',
    ]
    assert {i.limit_key for i in issues} == {
        'i2c.instance_count',
        'lcd.i80_bus_width',
        'adc.pattern_length_max',
    }


def test_instance_count_counts_unnamed_instances_independently() -> None:
    issues = validate_soc_capabilities(
        catalog=_catalog(),
        chip='esp32s3',
        instances=[
            inst('', 'peripheral', 'i2c'),
            inst('', 'peripheral', 'i2c'),
        ],
    )

    assert [i.limit_key for i in issues] == ['i2c.instance_count']
    assert issues[0].actual == 2


def test_rejects_raw_python_list_array_length() -> None:
    issues = validate_soc_capabilities(
        catalog=_catalog(),
        chip='esp32s3',
        instances=[
            inst(
                'adc0',
                'peripheral',
                'adc',
                {'role': 'continuous'},
                [field(['patterns'], [{'unit': 'ADC_UNIT_1'}, {'unit': 'ADC_UNIT_1'}, {'unit': 'ADC_UNIT_1'}])],
            ),
        ],
    )

    assert [i.limit_key for i in issues] == ['adc.pattern_length_max']
    assert issues[0].actual == 3


def test_rejects_unit_value_from_macro_and_number_values() -> None:
    issues = validate_soc_capabilities(
        catalog=_catalog(),
        chip='esp32s3',
        instances=[
            inst('adc0', 'peripheral', 'adc', {'role': 'continuous'}, [field(['unit'], 'ADC_UNIT_2')]),
            inst('adc1', 'peripheral', 'adc', {'role': 'continuous'}, [field(['unit'], 2)]),
        ],
    )

    assert [i.limit_key for i in issues] == ['adc.unit_max', 'adc.unit_max']
    assert [i.actual for i in issues] == [2, 2]

def test_rejects_unit_value_from_list_of_mappings() -> None:
    issues = validate_soc_capabilities(
        catalog=_catalog(),
        chip='esp32s3',
        instances=[
            inst(
                'adc0',
                'peripheral',
                'adc',
                {'role': 'continuous'},
                [field(['patterns'], [{'unit': 'ADC_UNIT_1'}, {'unit': 'ADC_UNIT_2'}])],
            ),
        ],
    )

    old_issue = next(i for i in issues if i.limit_key == 'adc.pattern_unit_max')
    assert old_issue.actual == 2


def test_compare_lt_rejects_equal_channel_id_and_allows_below_limit() -> None:
    issues = validate_soc_capabilities(
        catalog=_catalog(),
        chip='esp32s3',
        instances=[
            inst('adc_bad', 'peripheral', 'adc', {'role': 'oneshot'}, [field(['channel_id'], 10)]),
            inst('adc_ok', 'peripheral', 'adc', {'role': 'oneshot'}, [field(['channel_id'], 9)]),
        ],
    )

    assert [i.limit_key for i in issues] == ['adc.max_channel_count']
    assert issues[0].actual == 10
    assert issues[0].path == ['peripherals', 'adc_bad', 'channel_id']


def test_compare_relations() -> None:
    compare_cases = [
        ('le', 10, []),
        ('le', 11, ['SOC_NUMBER_LIMIT_EXCEEDED']),
        ('lt', 9, []),
        ('lt', 10, ['SOC_NUMBER_LIMIT_EXCEEDED']),
        ('ge', 10, []),
        ('ge', 9, ['SOC_NUMBER_LIMIT_EXCEEDED']),
        ('gt', 11, []),
        ('gt', 10, ['SOC_NUMBER_LIMIT_EXCEEDED']),
        ('eq', 10, []),
        ('eq', 9, ['SOC_NUMBER_LIMIT_EXCEEDED']),
        ('ne', 9, []),
        ('ne', 10, ['SOC_NUMBER_LIMIT_EXCEEDED']),
    ]

    for compare, value, expected_codes in compare_cases:
        catalog = SocCapabilityCatalog.from_dict({
            'schemaVersion': 3,
            'profile': {'id': 'test'},
            'capabilityDefs': {},
            'hardwareLimitDefs': {
                'gpio.number_max': {
                    'appliesTo': [
                        {
                            'kind': 'peripheral',
                            'type': 'gpio_probe',
                            'path': ['gpio_num'],
                            'check': 'value',
                            'compare': compare,
                        },
                    ],
                },
            },
            'chips': {
                'esp32s3': {
                    'capabilities': {},
                    'gpio': {'validInput': [], 'validOutput': []},
                    'hardwareLimits': {'gpio.number_max': 10},
                },
            },
        })

        issues = validate_soc_capabilities(
            catalog=catalog,
            chip='esp32s3',
            instances=[inst('gpio0', 'peripheral', 'gpio_probe', fields=[field(['gpio_num'], value)])],
        )

        assert [i.code for i in issues] == expected_codes


def test_value_check_expands_scalar_list_and_reports_index_path() -> None:
    issues = validate_soc_capabilities(
        catalog=_catalog(),
        chip='esp32s3',
        instances=[
            inst('adc0', 'peripheral', 'adc', {'role': 'continuous'}, [field(['channel_list'], [0, 9, 10])]),
        ],
    )

    assert [i.limit_key for i in issues] == ['adc.max_channel_count']
    assert issues[0].actual == 10
    assert issues[0].path == ['peripherals', 'adc0', 'channel_list', 2]


def test_value_check_expands_object_list_and_reports_nested_index_path() -> None:
    issues = validate_soc_capabilities(
        catalog=_catalog(),
        chip='esp32s3',
        instances=[
            inst(
                'adc0',
                'peripheral',
                'adc',
                {'role': 'continuous'},
                [field(['patterns'], [{'channel': 0}, {'channel': 10}])],
            ),
        ],
    )

    assert [i.limit_key for i in issues] == ['adc.max_channel_count']
    assert issues[0].actual == 10
    assert issues[0].path == ['peripherals', 'adc0', 'patterns', 1, 'channel']


def test_value_check_resolves_nested_mapping_object_list_path() -> None:
    issues = validate_soc_capabilities(
        catalog=_catalog(),
        chip='esp32s3',
        instances=[
            inst(
                'nested0',
                'peripheral',
                'nested_probe',
                fields=[
                    field(['outer'], {'channels': [{'id': 0}, {'id': 2}]}),
                    field(['outer', 'channels'], [{'id': 0}, {'id': 2}]),
                ],
            ),
        ],
    )

    assert [i.limit_key for i in issues] == ['nested.channel_count']
    assert issues[0].actual == 2
    assert issues[0].path == ['peripherals', 'nested0', 'outer', 'channels', 1, 'id']


def test_value_check_parses_adc_unit_enum_at_scalar_and_object_list_paths() -> None:
    issues = validate_soc_capabilities(
        catalog=_catalog(),
        chip='esp32s3',
        instances=[
            inst('adc0', 'peripheral', 'adc', {'role': 'oneshot'}, [field(['unit_id'], 'ADC_UNIT_2')]),
            inst(
                'adc1',
                'peripheral',
                'adc',
                {'role': 'continuous'},
                [field(['patterns'], [{'unit': 'ADC_UNIT_1'}, {'unit': 'ADC_UNIT_2'}])],
            ),
        ],
    )

    new_issues = [i for i in issues if i.limit_key == 'adc.unit_count']
    assert [i.actual for i in new_issues] == [2, 2]
    assert new_issues[0].path == ['peripherals', 'adc0', 'unit_id']
    assert new_issues[1].path == ['peripherals', 'adc1', 'patterns', 1, 'unit']


def test_value_check_parses_trailing_number_enums_and_skips_bool_ints() -> None:
    issues = validate_soc_capabilities(
        catalog=_catalog(),
        chip='esp32s3',
        instances=[
            inst('ledc0', 'device', 'ledc_ctrl', fields=[field(['channel'], 'LEDC_CHANNEL_0')]),
            inst('gpio0', 'peripheral', 'gpio_probe', fields=[field(['gpio_num'], 'GPIO_NUM_10')]),
            inst('gpio_bool', 'peripheral', 'gpio_probe', fields=[field(['gpio_num'], True)]),
        ],
    )

    assert [i.limit_key for i in issues] == ['gpio.number_max']
    assert issues[0].actual == 10


def test_builds_instance_from_dict_like_yaml_item() -> None:
    builder = getattr(soc_capability_validator, 'build_soc_validation_instance')
    item = {
        'name': 'adc0',
        'type': 'adc',
        'role': ['continuous', 'oneshot'],
        'format': 'raw',
        'config': {
            'channel_count': 3,
            'nested': {'x': 1},
            'patterns': [{'unit': 'ADC_UNIT_1'}],
        },
    }

    instance = builder('peripheral', item)

    assert instance == inst(
        'adc0',
        'peripheral',
        'adc',
        {'role': ['continuous', 'oneshot'], 'format': 'raw'},
        [
            field(['channel_count'], 3),
            field(['nested'], {'x': 1}),
            field(['nested', 'x'], 1),
            field(['patterns'], [{'unit': 'ADC_UNIT_1'}]),
        ],
    )


def test_builds_instance_from_object_like_yaml_item_with_explicit_id() -> None:
    builder = getattr(soc_capability_validator, 'build_soc_validation_instance')

    class Item:
        name = 'ignored'
        type = 'display_lcd'
        sub_type = 'i80'
        config = {'bus_width': '16'}

    instance = builder('device', Item(), instance_id='lcd0')

    assert instance == inst(
        'lcd0',
        'device',
        'display_lcd',
        {'sub_type': 'i80'},
        [field(['bus_width'], '16')],
    )
