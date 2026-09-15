# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

# GPIO peripheral config parser
VERSION = 'v1.0.0'

import re

from generators.utils.idf_enum import resolve_idf_macro

KNOWN_GPIO_MODE = frozenset((
    'GPIO_MODE_DISABLE',
    'GPIO_MODE_INPUT',
    'GPIO_MODE_OUTPUT',
    'GPIO_MODE_OUTPUT_OD',
    'GPIO_MODE_INPUT_OUTPUT',
    'GPIO_MODE_INPUT_OUTPUT_OD',
))
KNOWN_GPIO_INTR = frozenset((
    'GPIO_INTR_DISABLE',
    'GPIO_INTR_POSEDGE',
    'GPIO_INTR_NEGEDGE',
    'GPIO_INTR_ANYEDGE',
    'GPIO_INTR_LOW_LEVEL',
    'GPIO_INTR_HIGH_LEVEL',
))

PIN_BIT_MASK_RE = re.compile(r'BIT64\((\d+)\)')

def get_includes() -> list:
    """Return list of required include headers for GPIO peripheral"""
    return [
        'periph_gpio.h',
    ]

def parse(name: str, config: dict) -> dict:
    """Parse GPIO peripheral configuration from YAML to C structure

    Args:
        name: Peripheral name
        config: Configuration dictionary from YAML

    Returns:
        Dictionary containing GPIO configuration structure
    """
    try:
        # Get the config dictionary
        cfg = config.get('config', {})

        # Get GPIO pin number with validation
        pin = cfg.get('pin', -1)
        if pin < 0:
            raise ValueError(f'Invalid GPIO pin number: {pin}. Must be >= 0.')

        # Use BIT64(pin) format for pin_bit_mask
        pin_bit_mask = f'BIT64({pin})'

        # Get GPIO mode with validation
        mode = resolve_idf_macro(
            cfg.get('mode', 'GPIO_MODE_INPUT'),
            default='GPIO_MODE_INPUT',
            prefixes=('GPIO_MODE_',),
            known=KNOWN_GPIO_MODE,
            field_name='mode',
        )

        # Get pull-up/down configuration with validation
        pull_up = cfg.get('pull_up', False)
        if not isinstance(pull_up, bool):
            raise ValueError(f'Invalid pull_up value: {pull_up}. Must be boolean.')
        pull_up_en = 'GPIO_PULLUP_ENABLE' if pull_up else 'GPIO_PULLUP_DISABLE'

        pull_down = cfg.get('pull_down', False)
        if not isinstance(pull_down, bool):
            raise ValueError(f'Invalid pull_down value: {pull_down}. Must be boolean.')
        pull_down_en = 'GPIO_PULLDOWN_ENABLE' if pull_down else 'GPIO_PULLDOWN_DISABLE'

        # Get interrupt type with validation
        intr_type = resolve_idf_macro(
            cfg.get('intr_type', 'GPIO_INTR_DISABLE'),
            default='GPIO_INTR_DISABLE',
            prefixes=('GPIO_INTR_',),
            known=KNOWN_GPIO_INTR,
            field_name='intr_type',
        )

        # Get default level with validation
        default_level = cfg.get('default_level', 0)
        if default_level not in [0, 1]:
            raise ValueError(f'Invalid default_level: {default_level}. Must be 0 or 1.')

        # Use logger for debug output
        from generators.utils.logger import get_logger
        logger = get_logger('periph_gpio')
        logger.debug(f'GPIO {name} pin={pin}, mode={mode}, intr_type={intr_type}, default_level={default_level}')

        # Create configuration structure
        result = {
            'struct_type': 'periph_gpio_config_t',
            'struct_var': f'{name}_cfg',
            'struct_init': {
                'gpio_config': {
                    'pin_bit_mask': pin_bit_mask,
                    'mode': mode,
                    'pull_up_en': pull_up_en,
                    'pull_down_en': pull_down_en,
                    'intr_type': intr_type
                },
                'default_level': default_level
            }
        }

        return result

    except ValueError as e:
        # Re-raise ValueError with more context
        raise ValueError(f"YAML validation error in GPIO peripheral '{name}': {e}")
    except Exception as e:
        # Catch any other exceptions and provide context
        raise ValueError(f"Error parsing GPIO peripheral '{name}': {e}")


def extract_metadata(name: str, raw_config: dict, parse_result: dict, context: dict) -> dict:
    gpio_config = parse_result.get('struct_init', {}).get('gpio_config', {})
    pin_bit_mask = gpio_config.get('pin_bit_mask')
    if not isinstance(pin_bit_mask, str):
        return {'io': {}}

    match = PIN_BIT_MASK_RE.fullmatch(pin_bit_mask.strip())
    if not match:
        return {'io': {}}

    return {
        'io': {
            'pin': int(match.group(1)),
        }
    }
