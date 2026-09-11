# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

# LEDC peripheral config parser
VERSION = 'v1.0.0'

PERIPH_LEDC_IO_LIST = [
    'gpio_num',
]

from generators.utils.idf_enum import resolve_idf_macro

KNOWN_LEDC_CHANNEL = frozenset(f'LEDC_CHANNEL_{i}' for i in range(8))
KNOWN_LEDC_TIMER = frozenset(('LEDC_TIMER_0', 'LEDC_TIMER_1', 'LEDC_TIMER_2', 'LEDC_TIMER_3'))
KNOWN_LEDC_DUTY_RESOLUTION = frozenset(f'LEDC_TIMER_{i}_BIT' for i in range(1, 21))
KNOWN_LEDC_SPEED_MODE = frozenset(('LEDC_LOW_SPEED_MODE', 'LEDC_HIGH_SPEED_MODE'))
KNOWN_LEDC_SLEEP_MODE = frozenset((
    'LEDC_SLEEP_MODE_NO_ALIVE_NO_PD',
    'LEDC_SLEEP_MODE_NO_ALIVE_ALLOW_PD',
    'LEDC_SLEEP_MODE_KEEP_ALIVE',
    'LEDC_SLEEP_MODE_INVALID',
))

def get_includes() -> list:
    """Return list of required include headers for LEDC peripheral"""
    return [
        'periph_ledc.h',
    ]

def parse(name: str, config: dict) -> dict:
    """Parse LEDC peripheral configuration from YAML to C structure

    Args:
        name: Peripheral name
        config: Configuration dictionary from YAML

    Returns:
        Dictionary containing LEDC configuration structure
    """
    try:
        # Get the config dictionary
        cfg = config.get('config', {})
        if not cfg:
            # If config is not found, try to use the entire config dict
            cfg = config

        # Get GPIO pin number with validation
        gpio = cfg.get('gpio_num', -1)
        if gpio < 0:
            raise ValueError(f'Invalid GPIO pin number: {gpio}. Must be >= 0.')

        # Get LEDC channel. The per-chip channel count limit
        # (SOC_LEDC_CHANNEL_NUM) is enforced by the SoC capability validator
        # before parser dispatch, so only handle type/shape and codegen here.
        channel = cfg.get('channel', 'LEDC_CHANNEL_0')

        if isinstance(channel, int):
            if channel < 0:
                raise ValueError(f'Invalid LEDC channel number: {channel}. Must be >= 0.')
            channel_enum = f'LEDC_CHANNEL_{channel}'
        elif isinstance(channel, str):
            channel_enum = channel
        else:
            raise ValueError(f'Invalid LEDC channel type: {type(channel)}. Must be int or str.')
        channel_enum = resolve_idf_macro(
            channel_enum,
            default='LEDC_CHANNEL_0',
            prefixes=('LEDC_CHANNEL_',),
            known=KNOWN_LEDC_CHANNEL,
            field_name='channel',
        )

        # Get LEDC timer_sel with validation
        timer_sel = cfg.get('timer_sel', 'LEDC_TIMER_0')

        # Handle both numeric and enum string values
        if isinstance(timer_sel, int):
            if timer_sel < 0:
                raise ValueError(f'Invalid LEDC timer number: {timer_sel}. Must be >= 0.')
            timer_enum = f'LEDC_TIMER_{timer_sel}'
        elif isinstance(timer_sel, str):
            timer_enum = timer_sel
        else:
            raise ValueError(f'Invalid LEDC timer type: {type(timer_sel)}. Must be int or str.')
        timer_enum = resolve_idf_macro(
            timer_enum,
            default='LEDC_TIMER_0',
            prefixes=('LEDC_TIMER_',),
            known=KNOWN_LEDC_TIMER,
            field_name='timer_sel',
            pattern=r'^LEDC_TIMER_[0-9]+$',
        )

        # Get frequency with validation
        freq_hz = cfg.get('freq_hz', 4000)
        if freq_hz <= 0:
            raise ValueError(f'Invalid frequency: {freq_hz}. Must be > 0.')

        # Get duty cycle with validation
        duty = cfg.get('duty', 0)
        if duty < 0:
            raise ValueError(f'Invalid duty cycle: {duty}. Must be >= 0.')

        # Get duty resolution with validation
        duty_resolution = cfg.get('duty_resolution', 'LEDC_TIMER_13_BIT')

        # Handle both numeric and enum string values
        if isinstance(duty_resolution, int):
            if duty_resolution < 1:
                raise ValueError(f'Invalid duty resolution number: {duty_resolution}. Must be >= 1.')
            duty_resolution_enum = f'LEDC_TIMER_{duty_resolution}_BIT'
        elif isinstance(duty_resolution, str):
            duty_resolution_enum = duty_resolution
        else:
            raise ValueError(f'Invalid duty resolution type: {type(duty_resolution)}. Must be int or str.')
        duty_resolution_enum = resolve_idf_macro(
            duty_resolution_enum,
            default='LEDC_TIMER_13_BIT',
            prefixes=('LEDC_TIMER_',),
            known=KNOWN_LEDC_DUTY_RESOLUTION,
            field_name='duty_resolution',
            pattern=r'^LEDC_TIMER_[0-9]+_BIT$',
        )

        speed_mode = resolve_idf_macro(
            cfg.get('speed_mode', 'LEDC_LOW_SPEED_MODE'),
            default='LEDC_LOW_SPEED_MODE',
            prefixes=('LEDC_',),
            known=KNOWN_LEDC_SPEED_MODE,
            field_name='speed_mode',
            pattern=r'^LEDC_[A-Z0-9_]*SPEED_MODE$',
        )

        # Get output inversion with validation
        output_invert = cfg.get('output_invert', False)
        if not isinstance(output_invert, bool):
            raise ValueError(f'Invalid output_invert value: {output_invert}. Must be boolean.')

        sleep_mode = resolve_idf_macro(
            cfg.get('sleep_mode', 'LEDC_SLEEP_MODE_NO_ALIVE_NO_PD'),
            default='LEDC_SLEEP_MODE_NO_ALIVE_NO_PD',
            prefixes=('LEDC_SLEEP_MODE_',),
            known=KNOWN_LEDC_SLEEP_MODE,
            field_name='sleep_mode',
        )

        # Create configuration structure
        result = {
            'struct_type': 'periph_ledc_config_t',
            'struct_var': f'{name}_cfg',
            'struct_init': {
                'handle': {
                    'channel': channel_enum,
                    'speed_mode': speed_mode
                },
                'gpio_num': gpio,
                'duty': duty,
                'freq_hz': freq_hz,
                'duty_resolution': duty_resolution_enum,
                'timer_sel': timer_enum,
                'sleep_mode': sleep_mode,
                'output_invert': output_invert
            }
        }

        # Use logger for debug output
        from generators.utils.logger import get_logger
        logger = get_logger('periph_ledc')
        logger.debug(f'LEDC {name} channel={channel_enum}, gpio={gpio}, freq={freq_hz}Hz, duty={duty}, duty_resolution={duty_resolution_enum}')
        return result

    except ValueError as e:
        # Re-raise ValueError with more context
        raise ValueError(f"YAML validation error in LEDC peripheral '{name}': {e}")
    except Exception as e:
        # Catch any other exceptions and provide context
        raise ValueError(f"Error parsing LEDC peripheral '{name}': {e}")
