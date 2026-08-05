# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

# Audio codec device config parser
VERSION = 'v2.0.0'

import logging
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from generators.adc_channel_mapper import adc_channels_to_gpios, adc_patterns_to_gpios

logger = logging.getLogger(__name__)

_AUDIO_CODEC_PERIPHERAL_ONLY_CONFIG_GROUPS = ('pa_cfg', 'reset_cfg')
_AUDIO_CODEC_LEGACY_INIT_FIELDS = frozenset({
    'mclk_enabled',
    'adc_channel_labels',
    'adc_max_channel',
    'adc_channel_mask',
    'adc_init_gain',
    'dac_max_channel',
    'dac_channel_mask',
    'dac_init_gain',
    'aec',
    'eq',
    'alc',
})


def _validate_audio_codec_config_layout(device_name: str, device_config: dict) -> None:
    for config_group in _AUDIO_CODEC_PERIPHERAL_ONLY_CONFIG_GROUPS:
        if config_group in device_config:
            raise ValueError(
                f'Audio codec device {device_name} config.{config_group} is configured through peripherals; '
                f'declare the corresponding GPIO under peripherals instead.'
            )

    adc_cfg = device_config.get('adc_cfg', {})
    if isinstance(adc_cfg, dict) and 'adc_channel_labels' in device_config and 'label' in adc_cfg:
        raise ValueError(
            f'Audio codec device {device_name} cannot specify both config.adc_channel_labels and '
            'config.adc_cfg.label.'
        )

def get_includes() -> list:
    """Return list of required include headers for audio codec device"""
    return [
        'dev_audio_codec.h'
    ]

def _parse_bool(value, default: bool) -> bool:
    if value is None:
        return default
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, float)):
        return value != 0
    if isinstance(value, str):
        text = value.strip().lower()
        if text in ('1', 'true', 'yes', 'on'):
            return True
        if text in ('0', 'false', 'no', 'off'):
            return False
    return default

def _validate_peripheral_reference(device_name: str, periph_name: str, peripherals_dict) -> None:
    if peripherals_dict is not None and periph_name not in peripherals_dict:
        raise ValueError(f"Audio codec device {device_name} references undefined peripheral '{periph_name}'")

def _get_peripheral_base_config(peripherals_dict, periph_name: str) -> dict:
    if not peripherals_dict or periph_name not in peripherals_dict:
        return {}
    return peripherals_dict[periph_name].config

def _is_input_i2s_peripheral(peripherals_dict, periph_name: str) -> bool:
    if not peripherals_dict or periph_name not in peripherals_dict:
        return False

    periph = peripherals_dict[periph_name]
    if getattr(periph, 'type', None) != 'i2s':
        return False

    format_str = getattr(periph, 'format', None)
    if not format_str:
        return True
    return 'in' in str(format_str).lower().split('-')

def _parse_codec_gpio_peripheral(device_name: str, periph_name: str, peripherals_dict, purpose: str) -> int:
    _validate_peripheral_reference(device_name, periph_name, peripherals_dict)
    peripheral_config = _get_peripheral_base_config(peripherals_dict, periph_name)
    if peripherals_dict is not None:
        peripheral = peripherals_dict[periph_name]
        if getattr(peripheral, 'type', None) != 'gpio':
            raise ValueError(
                f'Audio codec device {device_name} {purpose} GPIO peripheral must have type gpio'
            )
    if 'pin' not in peripheral_config:
        raise ValueError(
            f'Audio codec device {device_name} {purpose} GPIO peripheral must define config.pin'
        )
    return int(peripheral_config['pin'])


def _parse_codec_peripherals(device_name: str, peripherals: list, peripherals_dict=None):
    """Parse non-ADC peripherals and collect ADC reuse peripheral name (if any)."""
    # These fields are struct members in dev_audio_codec_config_t (not pointers),
    # so keep zero/default struct initializers instead of None/NULL.
    pa_cfg = {
        'name': None,
        'port': -1,
        'active_level': 0,
        'gain': 0.0,
    }
    reset_cfg = {
        'name': None,
        'port': -1,
        'active_level': 0,
    }
    i2c_cfg = {
        'name': None,
        'port': 0,
        'address': 0,
        'frequency': 0,
    }
    i2s_cfg = {
        'name': None,
        'port': 0,
        'clk_src': 0,
        'tx_aux_out_io': -1,
        'tx_aux_out_line': 0,
        'tx_aux_out_invert': False,
    }
    adc_periph_name = ''
    adc_periph_count = 0
    explicit = {'pa': [], 'reset': [], 'i2c': [], 'i2s': [], 'adc': []}
    legacy = []

    for periph in peripherals:
        if not isinstance(periph, dict):
            periph = {'name': periph}
        else:
            role_keys = [key for key in periph if key.endswith('_name') and key != 'name']
            if len(role_keys) > 1:
                raise ValueError(
                    f'Audio codec device {device_name} peripheral cannot contain more than one role-specific name field'
                )
            if role_keys:
                role_key = role_keys[0]
                if 'name' in periph:
                    raise ValueError(
                        f"Audio codec device {device_name} peripheral cannot contain both '{role_key}' and 'name'"
                    )
                periph = dict(periph)
                periph['name'] = periph.pop(role_key)
                periph['_binding_role'] = role_key[:-5]
        binding_role = periph.get('_binding_role')
        if binding_role in explicit:
            explicit[binding_role].append(periph)
        elif binding_role is None:
            legacy.append(periph)
        else:
            raise ValueError(
                f'Audio codec device {device_name} does not support peripheral binding role: {binding_role}'
            )

    for role, refs in explicit.items():
        if len(refs) > 1:
            role_label = 'PA GPIO' if role == 'pa' else 'reset GPIO' if role == 'reset' else role
            raise ValueError(
                f'Audio codec device {device_name} references multiple {role_label} peripherals, only one is supported'
            )
        if not refs:
            continue
        periph = refs[0]
        periph_name = periph['name']
        expected_type = 'gpio' if role in ('pa', 'reset') else role
        _validate_peripheral_reference(device_name, periph_name, peripherals_dict)
        if peripherals_dict is not None and getattr(peripherals_dict[periph_name], 'type', None) != expected_type:
            raise ValueError(
                f'Audio codec device {device_name} {role} peripheral must have type {expected_type}'
            )
        if role == 'pa':
            if 'pa_active_level' in periph:
                raise ValueError(
                    f'Audio codec device {device_name} cannot combine pa_name with pa_active_level; '
                    'use active_level with pa_name instead.'
                )
            pa_cfg = {
                'name': periph_name,
                'port': _parse_codec_gpio_peripheral(device_name, periph_name, peripherals_dict, 'PA'),
                'active_level': int(periph.get('active_level', 0)),
                'gain': float(periph.get('gain', 0.0)),
            }
        elif role == 'reset':
            if 'reset_active_level' in periph:
                raise ValueError(
                    f'Audio codec device {device_name} cannot combine reset_name with reset_active_level; '
                    'use active_level with reset_name instead.'
                )
            reset_cfg = {
                'name': periph_name,
                'port': _parse_codec_gpio_peripheral(device_name, periph_name, peripherals_dict, 'reset'),
                'active_level': int(periph.get('active_level', 0)),
            }
        elif role == 'i2c':
            peripheral_config = _get_peripheral_base_config(peripherals_dict, periph_name)
            i2c_cfg = {
                'name': periph_name,
                'port': peripheral_config.get('port', 0),
                'address': periph.get('address', 0x30),
                'frequency': int(periph.get('frequency', 100000)),
            }
        elif role == 'i2s':
            peripheral_config = _get_peripheral_base_config(peripherals_dict, periph_name)
            i2s_cfg = {
                'name': periph_name,
                'port': peripheral_config.get('port', 0),
                'clk_src': periph.get('clk_src', 0),
                'tx_aux_out_io': int(periph.get('tx_aux_out_io', -1)),
                'tx_aux_out_line': int(periph.get('tx_aux_out_line', 0)),
                'tx_aux_out_invert': _parse_bool(periph.get('tx_aux_out_invert', False), False),
            }
        elif role == 'adc':
            adc_periph_name = periph_name
            adc_periph_count += 1

    for periph in legacy:
        periph_name = periph.get('name', '')
        has_pa_active_level = 'pa_active_level' in periph
        has_reset_active_level = 'reset_active_level' in periph
        has_legacy_active_level = 'active_level' in periph

        legacy_role = None
        if has_reset_active_level:
            legacy_role = 'reset'
        elif has_pa_active_level or periph_name.startswith('gpio'):
            legacy_role = 'pa'
        elif periph_name.startswith('i2c'):
            legacy_role = 'i2c'
        elif periph_name.startswith('i2s'):
            legacy_role = 'i2s'
        elif periph_name.startswith('adc'):
            legacy_role = 'adc'
        if legacy_role and explicit[legacy_role]:
            raise ValueError(
                f'Audio codec device {device_name} cannot mix {legacy_role}_name with legacy '
                f'peripheral binding for the same role'
            )

        if periph.get('role') == 'reset':
            raise ValueError(
                f'Audio codec device {device_name} does not support role: reset; '
                'use reset_active_level on the GPIO peripheral instead.'
            )
        if has_pa_active_level and has_reset_active_level:
            raise ValueError(
                f'Audio codec device {device_name} peripheral {periph_name} cannot specify both '
                'pa_active_level and reset_active_level.'
            )
        if has_legacy_active_level and has_pa_active_level:
            raise ValueError(
                f'Audio codec device {device_name} peripheral {periph_name} cannot specify both '
                'active_level and pa_active_level.'
            )
        if has_legacy_active_level and has_reset_active_level:
            raise ValueError(
                f'Audio codec device {device_name} peripheral {periph_name} cannot specify both '
                'active_level and reset_active_level.'
            )

        if has_reset_active_level:
            logger.warning(
                'Audio codec device %s uses legacy reset binding on peripheral %s; migrate to reset_name.',
                device_name, periph_name,
            )
            if reset_cfg['name'] is not None:
                raise ValueError(
                    f'Audio codec device {device_name} references multiple reset GPIO peripherals, only one is supported'
                )
            reset_cfg = {
                'name': periph_name,
                'port': _parse_codec_gpio_peripheral(
                    device_name, periph_name, peripherals_dict, 'reset'),
                'active_level': int(periph['reset_active_level']),
            }
            continue

        if has_pa_active_level:
            logger.warning(
                'Audio codec device %s uses legacy PA binding on peripheral %s; migrate to pa_name.',
                device_name, periph_name,
            )
            if pa_cfg['name'] is not None:
                raise ValueError(
                    f'Audio codec device {device_name} references multiple PA GPIO peripherals, only one is supported'
                )
            pa_cfg = {
                'name': periph_name,
                'port': _parse_codec_gpio_peripheral(
                    device_name, periph_name, peripherals_dict, 'PA'),
                'active_level': int(periph['pa_active_level']),
                'gain': float(periph.get('gain', 0.0)),
            }
            continue

        if periph_name.startswith('gpio'):
            if pa_cfg['name'] is not None:
                raise ValueError(
                    f'Audio codec device {device_name} references multiple PA GPIO peripherals, only one is supported'
                )
            logger.warning(
                'Audio codec device %s peripheral %s uses legacy PA inference; '
                'add pa_active_level explicitly.',
                device_name,
                periph_name,
            )
            pa_cfg = {
                'name': periph_name,
                'port': _parse_codec_gpio_peripheral(
                    device_name, periph_name, peripherals_dict, 'PA'),
                'active_level': int(periph.get('active_level', 0)),
                'gain': float(periph.get('gain', 0.0)),
            }
            continue

        if periph_name.startswith('i2c'):
            logger.warning(
                'Audio codec device %s uses legacy I2C peripheral inference; migrate to i2c_name.',
                device_name,
            )
            _validate_peripheral_reference(device_name, periph_name, peripherals_dict)
            peripheral_config = _get_peripheral_base_config(peripherals_dict, periph_name)
            i2c_cfg = {
                'name': periph_name,
                'port': peripheral_config.get('port', 0),
                'address': periph.get('address', 0x30),
                'frequency': int(periph.get('frequency', 100000)),
            }
            continue

        if periph_name.startswith('i2s'):
            logger.warning(
                'Audio codec device %s uses legacy I2S peripheral inference; migrate to i2s_name.',
                device_name,
            )
            _validate_peripheral_reference(device_name, periph_name, peripherals_dict)
            peripheral_config = _get_peripheral_base_config(peripherals_dict, periph_name)
            i2s_cfg = {
                'name': periph_name,
                'port': peripheral_config.get('port', 0),
                'clk_src': periph.get('clk_src', 0),
                'tx_aux_out_io': int(periph.get('tx_aux_out_io', -1)),
                'tx_aux_out_line': int(periph.get('tx_aux_out_line', 0)),
                'tx_aux_out_invert': _parse_bool(periph.get('tx_aux_out_invert', False), False),
            }
            continue

        if periph_name.startswith('adc'):
            logger.warning(
                'Audio codec device %s uses legacy ADC peripheral inference; migrate to adc_name.',
                device_name,
            )
            _validate_peripheral_reference(device_name, periph_name, peripherals_dict)
            adc_periph_count += 1
            if adc_periph_count == 1:
                adc_periph_name = periph_name

    if adc_periph_count > 1:
        raise ValueError(
            f'Audio codec device {device_name} references multiple adc peripherals, only one is supported currently'
        )

    return pa_cfg, i2c_cfg, i2s_cfg, adc_periph_name, reset_cfg


def _parse_adc_local_cfg(device_name: str, device_config: dict) -> dict:
    """Parse ADC local cfg (ADC mic local-create path) and normalize to one structure."""
    adc_local_cfg = device_config.get('adc_local_cfg', {})
    if adc_local_cfg is None:
        adc_local_cfg = {}
    if not isinstance(adc_local_cfg, dict):
        raise ValueError(f'Audio codec device {device_name} config.adc_local_cfg must be a map')

    sample_rate_hz = int(adc_local_cfg.get('sample_rate_hz', 16000))
    max_store_buf_size = int(adc_local_cfg.get('max_store_buf_size', 1024))
    conv_frame_size = int(adc_local_cfg.get('conv_frame_size', 256))
    unit_id = adc_local_cfg.get('unit_id', 'ADC_UNIT_1')
    atten = adc_local_cfg.get('atten', 'ADC_ATTEN_DB_0')
    bit_width = adc_local_cfg.get('bit_width', 'SOC_ADC_DIGI_MAX_BITWIDTH')
    adc_format = adc_local_cfg.get('format', 'ADC_DIGI_OUTPUT_FORMAT_TYPE2')

    channel_list = adc_local_cfg.get('channel_list', [])
    if channel_list is None:
        channel_list = []
    if not isinstance(channel_list, list):
        channel_list = [channel_list]
    adc_channel_list = [int(ch) for ch in channel_list]

    adc_patterns = adc_local_cfg.get('patterns', [])
    if adc_patterns is None:
        adc_patterns = []
    if not isinstance(adc_patterns, list):
        raise ValueError(f'Audio codec device {device_name} config.adc_local_cfg.patterns must be a list')
    if adc_channel_list and adc_patterns:
        raise ValueError(f'Audio codec device {device_name} adc_local_cfg cannot mix channel_list and patterns')

    has_adc_local = len(adc_channel_list) > 0 or len(adc_patterns) > 0
    adc_cfg_mode = 'BOARD_CODEC_ADC_CFG_MODE_SINGLE_UNIT'
    adc_pattern_num = 0
    adc_cfg_union = {
        'single_unit': {
            'unit_id': unit_id,
            'atten': atten,
            'bit_width': bit_width,
            'channel_id': [],
        }
    }

    if has_adc_local:
        if adc_patterns:
            adc_cfg_mode = 'BOARD_CODEC_ADC_CFG_MODE_PATTERN'
            parsed_patterns = []
            for item in adc_patterns:
                if not isinstance(item, dict):
                    raise ValueError(f'Audio codec device {device_name} adc_local_cfg.patterns item must be map')
                parsed_patterns.append({
                    'unit': item.get('unit', 'ADC_UNIT_1'),
                    'channel': int(item.get('channel', -1)),
                    'atten': item.get('atten', 'ADC_ATTEN_DB_0'),
                    'bit_width': item.get('bit_width', 'SOC_ADC_DIGI_MAX_BITWIDTH'),
                })
            adc_pattern_num = len(parsed_patterns)
            adc_cfg_union = {'patterns': parsed_patterns}

            if 'conv_mode' in adc_local_cfg:
                adc_conv_mode = adc_local_cfg.get('conv_mode')
            else:
                unit_set = {p['unit'] for p in parsed_patterns}
                if unit_set == {'ADC_UNIT_1'}:
                    adc_conv_mode = 'ADC_CONV_SINGLE_UNIT_1'
                elif unit_set == {'ADC_UNIT_2'}:
                    adc_conv_mode = 'ADC_CONV_SINGLE_UNIT_2'
                else:
                    adc_conv_mode = 'ADC_CONV_BOTH_UNIT'
        else:
            adc_pattern_num = len(adc_channel_list)
            adc_cfg_union = {
                'single_unit': {
                    'unit_id': unit_id,
                    'atten': atten,
                    'bit_width': bit_width,
                    'channel_id': adc_channel_list,
                }
            }
            adc_conv_mode = adc_local_cfg.get('conv_mode')
            if adc_conv_mode is None:
                adc_conv_mode = 'ADC_CONV_SINGLE_UNIT_1' if unit_id == 'ADC_UNIT_1' else 'ADC_CONV_SINGLE_UNIT_2'
    else:
        adc_conv_mode = adc_local_cfg.get('conv_mode', 'ADC_CONV_SINGLE_UNIT_1')

    return {
        'has_adc_local': has_adc_local,
        'sample_rate_hz': sample_rate_hz,
        'max_store_buf_size': max_store_buf_size,
        'conv_frame_size': conv_frame_size,
        'conv_mode': adc_conv_mode,
        'format': adc_format,
        'pattern_num': adc_pattern_num,
        'cfg_mode': adc_cfg_mode,
        'cfg': adc_cfg_union,
    }

def _validate_internal_adc_config(device_name: str, chip_name: str, device_config: dict,
                                  adc_periph_name: str, has_adc_local: bool,
                                  i2s_periph_name: str, peripherals_dict=None) -> None:
    if bool(device_config.get('adc_enabled', False)) and str(chip_name).lower() == 'internal':
        has_i2s_input_path = bool(i2s_periph_name) and _is_input_i2s_peripheral(peripherals_dict, i2s_periph_name)
        if adc_periph_name == '' and not has_adc_local and not has_i2s_input_path:
            raise ValueError(
                f'Audio codec device {device_name} (chip=internal, adc_enabled=true) requires either '
                'adc_* peripheral, input i2s peripheral, or config.adc_local_cfg'
            )

def _validate_direction_flags(device_name: str, adc_enabled: bool, dac_enabled: bool) -> None:
    """Validate mutually-exclusive ADC/DAC enablement for one logical audio_codec device."""
    if adc_enabled and dac_enabled:
        raise ValueError(
            f'Audio codec device {device_name} cannot enable both adc_enabled and dac_enabled. '
            'Please split full-duplex use cases into separate logical devices.'
        )

def _build_disabled_adc_cfg() -> dict:
    """Return zeroed ADC config for devices that do not use ADC at all."""
    return {
        'periph_name': None,
        'sample_rate_hz': 0,
        'max_store_buf_size': 0,
        'conv_frame_size': 0,
        'conv_mode': 0,
        'format': 0,
        'pattern_num': 0,
        'cfg_mode': 0,
        'cfg': {
            'single_unit': {
                'unit_id': 0,
                'atten': 0,
                'bit_width': 0,
                'channel_id': [],
            }
        },
    }


def _parse_codec_label(value, device_name: str) -> str:
    if value is None:
        return ''
    if isinstance(value, list):
        if not all(isinstance(item, str) for item in value):
            raise ValueError(f'Audio codec device {device_name} adc_cfg.label must contain strings')
        return ','.join(value)
    if isinstance(value, str):
        return value
    raise ValueError(f'Audio codec device {device_name} adc_cfg.label must be a string or list')


def _parse_codec_cfg_map(device_name: str, device_config: dict, key: str) -> dict:
    value = device_config.get(key, {})
    if value is None:
        return {}
    if not isinstance(value, dict):
        raise ValueError(f'Audio codec device {device_name} config.{key} must be a map')
    return value


def _warn_deprecated_codec_configs(device_name: str, device_config: dict) -> None:
    if 'mclk_enabled' in device_config:
        logger.warning(
            'Audio codec device %s uses deprecated config.mclk_enabled; migrate it to config.sys_cfg.no_mclk '
            '(the value is inverted during legacy parsing).',
            device_name,
        )
    if 'adc_channel_labels' in device_config:
        logger.warning(
            'Audio codec device %s uses deprecated config.adc_channel_labels; it is copied without reordering '
            'to config.adc_cfg.label. Codec_dev 2.0 interprets labels from LSB to MSB; the legacy field '
            'was documented from MSB to LSB. Review and migrate to config.adc_cfg.label.',
            device_name,
        )

    deferred_fields = (
        'adc_max_channel',
        'adc_channel_mask',
        'dac_max_channel',
        'dac_channel_mask',
        'adc_init_gain',
        'dac_init_gain',
        'aec',
        'eq',
        'alc',
    )
    configured_fields = [field for field in deferred_fields if field in device_config]
    if configured_fields:
        logger.warning(
            'Audio codec device %s uses deprecated config.%s; these are not codec initialization settings '
            'in esp_codec_dev 2.0 and are ignored by dev_audio_codec.',
            device_name,
            ', config.'.join(configured_fields),
        )


def _parse_codec_init_configs(device_name: str, device_config: dict, pa_peripheral_cfg: dict,
                              reset_peripheral_cfg: dict) -> tuple:
    sys_input = _parse_codec_cfg_map(device_name, device_config, 'sys_cfg')
    adc_input = _parse_codec_cfg_map(device_name, device_config, 'adc_cfg')
    dac_input = _parse_codec_cfg_map(device_name, device_config, 'dac_cfg')

    legacy_mclk_enabled = _parse_bool(device_config.get('mclk_enabled', False), False)
    codec_sys_cfg = {
        'is_master': _parse_bool(sys_input.get('is_master', False), False),
        'no_mclk': _parse_bool(sys_input.get('no_mclk', not legacy_mclk_enabled), not legacy_mclk_enabled),
    }
    legacy_labels = device_config.get('adc_channel_labels', [])
    codec_adc_cfg = {
        'digital_mic': _parse_bool(adc_input.get('digital_mic', False), False),
        'label': _parse_codec_label(adc_input.get('label', legacy_labels), device_name),
    }
    codec_dac_cfg = {
        'ref_enable': _parse_bool(dac_input.get('ref_enable', False), False),
        'ref_dac_ch': int(dac_input.get('ref_dac_ch', 0)),
        'real_adc_data_ch': int(dac_input.get('real_adc_data_ch', 0)),
    }

    pa_peripheral = {'name': pa_peripheral_cfg['name'], 'port': pa_peripheral_cfg['port']}
    pa_active_low = pa_peripheral_cfg['active_level'] != 1
    pa_gain = pa_peripheral_cfg['gain']
    codec_pa_cfg = {
        'pa_pin': pa_peripheral['port'],
        'pa_active_low': pa_active_low,
        'hw_gain': {'pa_gain': pa_gain},
    }

    reset_peripheral = {'name': reset_peripheral_cfg['name'], 'port': reset_peripheral_cfg['port']}
    codec_reset_cfg = {
        'reset_pin': reset_peripheral['port'],
        'reset_active_low': reset_peripheral_cfg['active_level'] != 1,
    }
    return (pa_peripheral, reset_peripheral, codec_sys_cfg, codec_adc_cfg,
            codec_dac_cfg, codec_pa_cfg, codec_reset_cfg)


def parse(name: str, config: dict, peripherals_dict=None) -> dict:
    # Parse the device name - use name directly for C naming
    c_name = name.replace('-', '_')  # Convert hyphens to underscores for C naming

    # Get the device config and peripherals
    device_config = config.get('config', {})
    peripherals = config.get('peripherals', [])

    # Get chip name from device level
    chip_name = config.get('chip', 'unknown')

    _validate_audio_codec_config_layout(name, device_config)
    _warn_deprecated_codec_configs(name, device_config)

    pa_cfg, i2c_cfg, i2s_cfg, adc_periph_name, reset_cfg = _parse_codec_peripherals(
        name, peripherals, peripherals_dict)
    adc_enabled = bool(device_config.get('adc_enabled', False))
    dac_enabled = bool(device_config.get('dac_enabled', False))
    _validate_direction_flags(name, adc_enabled, dac_enabled)

    if adc_periph_name != '':
        has_adc_local = False
        adc_data_cfg = {
            'periph_name': adc_periph_name,
        }
    else:
        adc_local_cfg = _parse_adc_local_cfg(name, device_config)
        has_adc_local = adc_local_cfg['has_adc_local']
        if has_adc_local:
            adc_data_cfg = {
                'sample_rate_hz': adc_local_cfg['sample_rate_hz'],
                'max_store_buf_size': adc_local_cfg['max_store_buf_size'],
                'conv_frame_size': adc_local_cfg['conv_frame_size'],
                'conv_mode': adc_local_cfg['conv_mode'],
                'format': adc_local_cfg['format'],
                'pattern_num': adc_local_cfg['pattern_num'],
                'cfg_mode': adc_local_cfg['cfg_mode'],
                'cfg': adc_local_cfg['cfg'],
            }
        else:
            adc_data_cfg = _build_disabled_adc_cfg()
    data_if_type = 1 if (adc_periph_name != '' or has_adc_local) else 0
    _validate_internal_adc_config(
        name,
        chip_name,
        device_config,
        adc_periph_name,
        has_adc_local,
        i2s_cfg.get('name'),
        peripherals_dict,
    )

    (pa_peripheral, reset_peripheral, codec_sys_cfg, codec_adc_cfg,
     codec_dac_cfg, codec_pa_cfg, codec_reset_cfg) = _parse_codec_init_configs(
         name, device_config, pa_cfg, reset_cfg)

    # Parse metadata if provided
    metadata = device_config.get('metadata', None)
    metadata_size = len(metadata) if metadata else 0

    result = {
        'struct_type': 'dev_audio_codec_config_t',
        'struct_var': f'{c_name}_cfg',  # Use the correct C name format
        'struct_init': {
            'name': c_name,  # Use the correct C name
            'chip': chip_name,  # Add chip field
            'type': str(config.get('type', 'audio_codec')),
            'data_if_type': data_if_type,
            'adc_enabled': adc_enabled,
            'dac_enabled': dac_enabled,
            'pa_peripheral': pa_peripheral,
            'reset_peripheral': reset_peripheral,
            'i2c_cfg': i2c_cfg,
            'i2s_cfg': i2s_cfg,
            'adc_data_cfg': adc_data_cfg,
            'codec_sys_cfg': codec_sys_cfg,
            'codec_adc_cfg': codec_adc_cfg,
            'codec_dac_cfg': codec_dac_cfg,
            'codec_pa_cfg': codec_pa_cfg,
            'codec_reset_cfg': codec_reset_cfg,
            'metadata': metadata,
            'metadata_size': metadata_size,
        }
    }
    return result


def extract_metadata(name: str, raw_config: dict, parse_result: dict, context: dict) -> dict:
    chip_name = context.get('chip')
    adc_cfg = parse_result.get('struct_init', {}).get('adc_data_cfg', {})

    if adc_cfg.get('periph_name'):
        return {'io': {}}

    cfg_mode = adc_cfg.get('cfg_mode')
    cfg = adc_cfg.get('cfg', {})

    if cfg_mode == 'BOARD_CODEC_ADC_CFG_MODE_PATTERN':
        patterns = cfg.get('patterns', [])
        if not patterns:
            return {'io': {}}
        try:
            mapped_channels = adc_patterns_to_gpios(chip_name, patterns)
        except FileNotFoundError as e:
            logger.warning(
                "Skipping ADC metadata extraction for device '%s' on chip '%s': %s",
                name,
                chip_name,
                e,
            )
            return {'io': {}}
        return {
            'io': {
                'channel': mapped_channels,
            }
        }

    single_unit_cfg = cfg.get('single_unit', {})
    channels = single_unit_cfg.get('channel_id', [])
    if not channels:
        return {'io': {}}

    try:
        mapped_channels = adc_channels_to_gpios(
            chip_name,
            single_unit_cfg.get('unit_id', 'ADC_UNIT_1'),
            channels,
        )
    except FileNotFoundError as e:
        logger.warning(
            "Skipping ADC metadata extraction for device '%s' on chip '%s': %s",
            name,
            chip_name,
            e,
        )
        return {'io': {}}

    return {
        'io': {
            'channel_id': mapped_channels,
        }
    }
