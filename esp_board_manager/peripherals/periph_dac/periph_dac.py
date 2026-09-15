# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

# DAC peripheral config parser
VERSION = 'v1.0.0'

from generators.utils.idf_enum import resolve_idf_macro

KNOWN_DAC_CHANNEL = frozenset(('DAC_CHAN_0', 'DAC_CHAN_1', 'DAC_CHAN_MAX'))
KNOWN_DAC_CHANNEL_MASK = frozenset((
    'DAC_CHANNEL_MASK_CH0',
    'DAC_CHANNEL_MASK_CH1',
    'DAC_CHANNEL_MASK_ALL',
))
KNOWN_DAC_CHANNEL_MODE = frozenset(('DAC_CHANNEL_MODE_SIMUL', 'DAC_CHANNEL_MODE_ALTER'))
KNOWN_DAC_COSINE_ATTEN = frozenset((
    'DAC_COSINE_ATTEN_DEFAULT',
    'DAC_COSINE_ATTEN_DB_0',
    'DAC_COSINE_ATTEN_DB_6',
    'DAC_COSINE_ATTEN_DB_12',
    'DAC_COSINE_ATTEN_DB_18',
))
KNOWN_DAC_COSINE_PHASE = frozenset(('DAC_COSINE_PHASE_0', 'DAC_COSINE_PHASE_180'))
KNOWN_DAC_DIGI_CLK_SRC = frozenset((
    'DAC_DIGI_CLK_SRC_DEFAULT',
    'DAC_DIGI_CLK_SRC_APB',
    'DAC_DIGI_CLK_SRC_APLL',
    'DAC_DIGI_CLK_SRC_PLLD2',
    'DAC_DIGI_CLK_SRC_PLL_160M',
))
KNOWN_DAC_COSINE_CLK_SRC = frozenset((
    'DAC_COSINE_CLK_SRC_DEFAULT',
    'DAC_COSINE_CLK_SRC_RTC_FAST',
))

# Map string roles to enum values
DAC_ROLE_MAP = {
    'oneshot': 'ESP_BOARD_PERIPH_ROLE_ONESHOT',
    'continuous': 'ESP_BOARD_PERIPH_ROLE_CONTINUOUS',
    'cosine': 'ESP_BOARD_PERIPH_ROLE_COSINE'
}

_DAC_ENUMS = {
    'dac_channel': (('DAC_CHAN_',), KNOWN_DAC_CHANNEL),
    'dac_channel_mask': (('DAC_CHANNEL_MASK_',), KNOWN_DAC_CHANNEL_MASK),
    'dac_continuous_chan_mode': (('DAC_CHANNEL_MODE_',), KNOWN_DAC_CHANNEL_MODE),
    'dac_cosine_atten': (('DAC_COSINE_ATTEN_',), KNOWN_DAC_COSINE_ATTEN),
    'dac_cosine_phase': (('DAC_COSINE_PHASE_',), KNOWN_DAC_COSINE_PHASE),
    'dac_digi_clk_src': (('DAC_DIGI_CLK_SRC_',), KNOWN_DAC_DIGI_CLK_SRC),
    'dac_cosine_clk_src': (('DAC_COSINE_CLK_SRC_',), KNOWN_DAC_COSINE_CLK_SRC),
}


def get_enum_value(value: str, full_enum: str, enum_type: str = None) -> str:
    """Resolve a YAML DAC enumerator, passing through unknown prefixed macros."""
    if not enum_type:
        return value if value else full_enum
    spec = _DAC_ENUMS.get(enum_type)
    if spec is None:
        print(f"⚠️  WARNING: Unknown enum type '{enum_type}' for validation")
        return value if value else full_enum
    prefixes, known = spec
    return resolve_idf_macro(value, full_enum, prefixes, known, enum_type)


def _parse_dac_channel(channel) -> str:
    if isinstance(channel, int):
        if channel < 0:
            raise ValueError(f'Invalid DAC channel number: {channel}. Must be >= 0.')
        channel = f'DAC_CHAN_{channel}'
    return get_enum_value(channel, 'DAC_CHAN_0', 'dac_channel')

def get_includes() -> list:
    """Return list of required include headers for DAC peripheral"""
    return [
        'periph_dac.h'
    ]

def parse(name: str, config: dict) -> dict:
    """Parse DAC peripheral configuration from YAML.

    Args:
        name: Peripheral name (e.g. 'dac-0')
        config: Configuration dictionary containing:
            - role: DAC role type ('oneshot', 'continuous', or 'cosine')
            - config: Configuration specific to the role

    Returns:
        Dictionary containing DAC configuration structure
    """
    try:
        # Get role type
        role = config.get('role')
        if role is None:
            raise ValueError("Missing 'role' in DAC configuration")

        if role not in ['oneshot', 'continuous', 'cosine']:
            raise ValueError(f"Invalid DAC role: {role}. Must be 'oneshot', 'continuous', or 'cosine'")

        # Convert string role to enum value
        enum_role = DAC_ROLE_MAP.get(role)
        if enum_role is None:
            raise ValueError(f"Invalid DAC role '{role}' for DAC device '{name}'")

        # Get the actual config
        config = config.get('config', {})
        if role == 'oneshot':
            # Parse oneshot configuration
            channel_enum = _parse_dac_channel(config.get('channel', 0))
            return {
                'struct_type': 'periph_dac_config_t',
                'struct_var': f'{name}_cfg',
                'struct_init': {
                    'role': enum_role,
                    'oneshot_cfg': {
                        'chan_id': channel_enum
                    }
                }
            }

        elif role == 'continuous':
            # Parse continuous configuration
            chan_mask = get_enum_value(config.get('chan_mask'), 'DAC_CHANNEL_MASK_CH0', 'dac_channel_mask')
            desc_num = config.get('desc_num', 8)
            buf_size = config.get('buf_size', 2048)
            freq_hz = config.get('freq_hz', 1000000)
            offset = config.get('offset', 0)
            clk_src = get_enum_value(config.get('clk_src'), 'DAC_DIGI_CLK_SRC_DEFAULT', 'dac_digi_clk_src')
            chan_mode = get_enum_value(config.get('chan_mode'), 'DAC_CHANNEL_MODE_SIMUL', 'dac_continuous_chan_mode')

            return {
                'struct_type': 'periph_dac_config_t',
                'struct_var': f'{name}_cfg',
                'struct_init': {
                    'role': enum_role,
                    'continuous_cfg': {
                        'chan_mask': chan_mask,
                        'desc_num': desc_num,
                        'buf_size': buf_size,
                        'freq_hz': freq_hz,
                        'offset': offset,
                        'clk_src': clk_src,
                        'chan_mode': chan_mode
                    }
                }
            }

        else:  # cosine mode
            # Parse cosine configuration
            channel_enum = _parse_dac_channel(config.get('channel', 0))
            freq_hz = config.get('freq_hz', 1000)
            clk_src = get_enum_value(config.get('clk_src'), 'DAC_COSINE_CLK_SRC_DEFAULT', 'dac_cosine_clk_src')

            atten = get_enum_value(config.get('atten'), 'DAC_COSINE_ATTEN_DEFAULT', 'dac_cosine_atten')
            phase = get_enum_value(config.get('phase'), 'DAC_COSINE_PHASE_0', 'dac_cosine_phase')
            offset = config.get('offset', 0)
            force_set_freq = config.get('force_set_freq', False)

            return {
                'struct_type': 'periph_dac_config_t',
                'struct_var': f'{name}_cfg',
                'struct_init': {
                    'role': enum_role,
                    'cosine_cfg': {
                        'chan_id': channel_enum,
                        'freq_hz': freq_hz,
                        'clk_src': clk_src,
                        'atten': atten,
                        'phase': phase,
                        'offset': offset,
                        'flags': {
                            'force_set_freq': force_set_freq
                        }
                    }
                }
            }

    except ValueError as e:
        # Re-raise ValueError with more context
        raise ValueError(f"YAML validation error in DAC peripheral '{name}': {e}")
    except Exception as e:
        # Catch any other exceptions and provide context
        raise ValueError(f"Error parsing DAC peripheral '{name}': {e}")
