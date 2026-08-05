# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

# GPIO control device config parser
VERSION = 'v1.0.0'

import sys
import logging

logger = logging.getLogger(__name__)

def get_includes():
    """Get required header includes for GPIO control device"""
    return ['dev_gpio_ctrl.h']

def parse(name, config, peripherals_dict=None):
    """
    Parse GPIO control device configuration from YAML to C structure

    Args:
        name (str): Device name
        config (dict): Device configuration dictionary
        peripherals_dict (dict): Dictionary of available peripherals for validation

    Returns:
        dict: Parsed configuration with 'struct_type' and 'struct_init' keys
    """
    # Extract configuration parameters
    device_config = config.get('config', {})
    peripherals_list = config.get('peripherals', [])

    # GPIO control configuration
    active_level = device_config.get('active_level', 1)
    default_level = device_config.get('default_level', 0)

    # Extract GPIO peripheral name from peripherals list
    explicit = []
    legacy = []
    for periph in peripherals_list:
        if isinstance(periph, dict) and ('gpio_name' in periph or periph.get('_binding_role') == 'gpio'):
            explicit.append(periph.get('gpio_name', periph.get('name')))
        else:
            periph_name = periph.get('name') if isinstance(periph, dict) else str(periph)
            if periph_name.startswith('gpio'):
                legacy.append(periph_name)
    if len(explicit) > 1 or len(legacy) > 1:
        raise ValueError(f'GPIO device {name} references multiple GPIO peripherals')
    if explicit and legacy:
        raise ValueError(f'GPIO device {name} cannot mix gpio_name with legacy GPIO binding')
    if explicit:
        gpio_name = explicit[0]
        if peripherals_dict is not None:
            if gpio_name not in peripherals_dict or getattr(peripherals_dict[gpio_name], 'type', None) != 'gpio':
                raise ValueError(f'GPIO device {name} gpio peripheral must have type gpio')
    elif legacy:
        gpio_name = legacy[0]
        logger.warning('GPIO device %s uses legacy GPIO peripheral inference; migrate to gpio_name.', name)
        if peripherals_dict is not None and gpio_name not in peripherals_dict:
            raise ValueError(f"GPIO device {name} references undefined peripheral '{gpio_name}'")
    else:
        raise ValueError(f'GPIO device {name} must have at least one GPIO peripheral defined')

    # Build structure initialization
    struct_init = {
        'name': f'"{name}"',
        'type': str(config.get('type', 'gpio_ctrl')),
        'gpio_name': gpio_name,
        'active_level': active_level,
        'default_level': default_level
    }

    return {
        'struct_type': 'dev_gpio_ctrl_config_t',
        'struct_init': struct_init
    }
