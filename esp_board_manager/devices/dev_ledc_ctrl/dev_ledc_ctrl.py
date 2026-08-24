# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

# LEDC control device config parser
VERSION = 'v1.0.0'

import sys
import logging

logger = logging.getLogger(__name__)

def get_includes():
    """Get required header includes for LEDC control device"""
    return ['dev_ledc_ctrl.h']

def parse(name, config, peripherals_dict=None):
    """
    Parse LEDC control device configuration from YAML to C structure

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

    # LEDC control configuration with defaults
    default_percent = device_config.get('default_percent', 100)  # Default 100% brightness

    # Extract LEDC peripheral name from peripherals list
    explicit = []
    legacy = []
    for periph in peripherals_list:
        if isinstance(periph, dict) and ('ledc_name' in periph or periph.get('_binding_role') == 'ledc'):
            explicit.append(periph.get('ledc_name', periph.get('name')))
        else:
            periph_name = periph.get('name') if isinstance(periph, dict) else str(periph)
            if periph_name.startswith('ledc'):
                legacy.append(periph_name)
    if len(explicit) > 1 or len(legacy) > 1:
        raise ValueError(f'LEDC device {name} references multiple LEDC peripherals')
    if explicit and legacy:
        raise ValueError(f'LEDC device {name} cannot mix ledc_name with legacy LEDC binding')
    if explicit:
        ledc_name = explicit[0]
        if peripherals_dict is not None:
            if ledc_name not in peripherals_dict or getattr(peripherals_dict[ledc_name], 'type', None) != 'ledc':
                raise ValueError(f'LEDC device {name} ledc peripheral must have type ledc')
    elif legacy:
        ledc_name = legacy[0]
        logger.warning('LEDC device %s uses legacy LEDC peripheral inference; migrate to ledc_name.', name)
        if peripherals_dict is not None and ledc_name not in peripherals_dict:
            raise ValueError(f"LEDC device {name} references undefined peripheral '{ledc_name}'")
    else:
        raise ValueError(f'LEDC device {name} must have at least one LEDC peripheral defined')

    # Build structure initialization
    struct_init = {
        'name': f'"{name}"',
        'type': str(config.get('type', 'ledc_ctrl')),
        'ledc_name': ledc_name,
        'default_percent': default_percent
    }

    return {
        'struct_type': 'dev_ledc_ctrl_config_t',
        'struct_init': struct_init
    }
