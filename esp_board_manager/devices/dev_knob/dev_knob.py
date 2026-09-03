# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

"""Knob device configuration parser."""

VERSION = 'v1.0.0'

_GPIO_SUB_TYPE = 'gpio'

DEV_KNOB_IO_LIST = {
    _GPIO_SUB_TYPE: ['gpio_encoder_a', 'gpio_encoder_b'],
}


def get_includes() -> list:
    """Return required include headers for the knob device."""
    return ['dev_knob.h']


def _get_gpio_number(config: dict, key: str) -> int:
    """Read and validate one encoder GPIO number."""
    if key not in config:
        raise ValueError(f'Knob device requires {key}')
    value = config[key]
    if isinstance(value, bool) or not isinstance(value, int):
        raise ValueError(f'Knob device {key} must be an integer')
    gpio_num = value
    if gpio_num < 0:
        raise ValueError(f'Knob device {key} must be non-negative')
    if gpio_num > 255:
        raise ValueError(f'Knob device {key} must be between 0 and 255')
    return gpio_num


def parse(name: str, full_config: dict, peripherals_dict=None) -> dict:
    """Parse a GPIO quadrature knob configuration into a C initializer."""
    del peripherals_dict

    peripherals = full_config.get('peripherals', [])
    if peripherals is None:
        peripherals = []
    if not isinstance(peripherals, list):
        raise ValueError('Knob device peripherals must be a list')
    if peripherals:
        raise ValueError('Knob device does not support peripheral bindings')

    sub_type = full_config.get('sub_type')
    if sub_type != _GPIO_SUB_TYPE:
        raise ValueError("Knob device must use sub_type 'gpio'")

    config = full_config.get('config', {})
    if not isinstance(config, dict):
        raise ValueError('Knob device config must be a mapping')

    gpio_encoder_a = _get_gpio_number(config, 'gpio_encoder_a')
    gpio_encoder_b = _get_gpio_number(config, 'gpio_encoder_b')
    if gpio_encoder_a == gpio_encoder_b:
        raise ValueError('Knob device gpio_encoder_a and gpio_encoder_b must be different')

    default_direction = config.get('default_direction', 0)
    if isinstance(default_direction, bool) or not isinstance(default_direction, int):
        raise ValueError('Knob device default_direction must be an integer')
    if default_direction not in (0, 1):
        raise ValueError('Knob device default_direction must be 0 or 1')

    use_rtc = config.get('use_rtc', False)
    if not isinstance(use_rtc, bool):
        raise ValueError('Knob device use_rtc must be a boolean')

    enable_power_save = config.get('enable_power_save', False)
    if not isinstance(enable_power_save, bool):
        raise ValueError('Knob device enable_power_save must be a boolean')

    return {
        'struct_type': 'dev_knob_config_t',
        'struct_var': f"{name.replace('-', '_')}_cfg",
        'struct_init': {
            'name': name,
            'sub_type': sub_type,
            'use_rtc': use_rtc,
            'knob_config': {
                'default_direction': default_direction,
                'gpio_encoder_a': gpio_encoder_a,
                'gpio_encoder_b': gpio_encoder_b,
                'enable_power_save': enable_power_save,
            },
        },
    }
