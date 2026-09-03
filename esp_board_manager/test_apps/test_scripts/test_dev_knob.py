"""Parser coverage for the knob device."""

import sys

import pytest


def test_knob_gpio_parser_emits_native_config(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_knob import dev_knob

    result = dev_knob.parse(
        'knob',
        {
            'type': 'knob',
            'sub_type': 'gpio',
            'config': {
                'gpio_encoder_a': 41,
                'gpio_encoder_b': 40,
                'default_direction': 1,
                'enable_power_save': True,
            },
        },
    )

    assert result['struct_type'] == 'dev_knob_config_t'
    assert result['struct_init']['sub_type'] == 'gpio'
    assert result['struct_init']['use_rtc'] is False
    assert result['struct_init']['knob_config'] == {
        'default_direction': 1,
        'gpio_encoder_a': 41,
        'gpio_encoder_b': 40,
        'enable_power_save': True,
    }


def test_knob_gpio_parser_selects_rtc_backend(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_knob import dev_knob

    result = dev_knob.parse(
        'sleep_knob',
        {
            'type': 'knob',
            'sub_type': 'gpio',
            'config': {
                'gpio_encoder_a': 3,
                'gpio_encoder_b': 4,
                'use_rtc': True,
            },
        },
    )

    assert result['struct_init']['sub_type'] == 'gpio'
    assert result['struct_init']['use_rtc'] is True


def test_knob_gpio_subtype_has_a_kconfig_symbol(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from gen_bmgr_config_codes import BoardConfigGenerator

    kconfig = BoardConfigGenerator(bmgr_root).generate_components_kconfig()

    assert 'config ESP_BOARD_DEV_KNOB_SUB_GPIO_SUPPORT' in kconfig


@pytest.mark.parametrize(
    ('field', 'value'),
    [
        ('gpio_encoder_a', 1.5),
        ('gpio_encoder_a', '1'),
        ('gpio_encoder_a', True),
        ('gpio_encoder_b', 2.5),
        ('default_direction', 0.5),
        ('default_direction', '0'),
        ('default_direction', True),
    ],
)
def test_knob_parser_rejects_non_integer_numeric_fields(bmgr_root, field, value):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_knob import dev_knob

    config = {
        'gpio_encoder_a': 41,
        'gpio_encoder_b': 40,
        'default_direction': 0,
    }
    config[field] = value

    with pytest.raises(ValueError, match='must be an integer'):
        dev_knob.parse(
            'knob',
            {
                'type': 'knob',
                'sub_type': 'gpio',
                'config': config,
            },
        )


def test_knob_parser_rejects_peripheral_bindings(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_knob import dev_knob

    with pytest.raises(ValueError, match='does not support peripheral bindings'):
        dev_knob.parse(
            'knob',
            {
                'type': 'knob',
                'sub_type': 'gpio',
                'peripherals': [{'gpio_name': 'gpio_encoder_a'}],
                'config': {
                    'gpio_encoder_a': 41,
                    'gpio_encoder_b': 40,
                },
            },
        )


@pytest.mark.parametrize(
    ('sub_type', 'config', 'error'),
    [
        ('rtc', {'gpio_encoder_a': 3, 'gpio_encoder_b': 4}, "must use sub_type 'gpio'"),
        ('gpio', {'gpio_encoder_a': 3}, 'requires gpio_encoder_b'),
        ('gpio', {'gpio_encoder_a': 3, 'gpio_encoder_b': 3}, 'must be different'),
        ('gpio', {'gpio_encoder_a': -1, 'gpio_encoder_b': 4}, 'must be non-negative'),
        ('gpio', {'gpio_encoder_a': 3, 'gpio_encoder_b': 256}, 'must be between 0 and 255'),
    ],
)
def test_knob_parser_rejects_invalid_configuration(bmgr_root, sub_type, config, error):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_knob import dev_knob

    with pytest.raises(ValueError, match=error):
        dev_knob.parse(
            'knob',
            {
                'type': 'knob',
                'sub_type': sub_type,
                'config': config,
            },
        )
