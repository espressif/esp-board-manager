# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

"""Ethernet device configuration parser (EMAC RMII/RGMII subtypes)."""

from generators.phy_parser_loader import current_phy_parser_registry
from generators.utils.idf_version import get_idf_version
from generators.utils.soc_capability_query import current_soc, current_soc_chip_name

VERSION = 'v1.0.0'
DYNAMIC_CONFIG_FIELDS = ('vendor_config',)
_EMAC_INTERFACES = {
    'emac_rmii': 'EMAC_DATA_INTERFACE_RMII',
    'emac_rgmii': 'EMAC_DATA_INTERFACE_RGMII',
}
IDF_VERSION_WITH_RGMII = (6, 1, 0)
_RMII_CLOCK_KEYS = frozenset(('clock_mode', 'clock_gpio'))
_RGMII_CLOCK_KEYS = frozenset(('clock_rx_gpio', 'clock_tx_gpio', 'clock_phy_ref_gpio'))
_RMII_DATAIF_KEYS = frozenset(('tx_en_num', 'txd0_num', 'txd1_num', 'crs_dv_num', 'rxd0_num', 'rxd1_num'))
_RGMII_DATAIF_KEYS = frozenset((
    'tx_ctl_num', 'txd0_num', 'txd1_num', 'txd2_num', 'txd3_num',
    'rx_ctl_num', 'rxd0_num', 'rxd1_num', 'rxd2_num', 'rxd3_num',
))
_RMII_CLOCK_MODES = ('EMAC_CLK_EXT_IN', 'EMAC_CLK_OUT')
_UINT32_MAX = 2**32 - 1
_INT32_MIN = -(2**31)
_INT32_MAX = 2**31 - 1
_MAC_FIELDS = {
    'sw_reset_timeout_ms': (100, 10, _UINT32_MAX),
    'rx_task_stack_size': (4096, 1, _UINT32_MAX),
    'rx_task_prio': (15, 0, 24),
}
_PHY_FIELDS = {
    'phy_addr': (-1, -1, 31),
    'reset_gpio_num': (-1, -1, 255),
    'reset_timeout_ms': (100, 10, _UINT32_MAX),
    'autonego_timeout_ms': (4000, 0, _UINT32_MAX),
    'hw_reset_assert_time_us': (0, 0, _INT32_MAX),
    'post_hw_reset_delay_ms': (0, -1, _INT32_MAX),
}
_MAC_FLAGS = ('ETH_MAC_FLAG_WORK_WITH_CACHE_DISABLE', 'ETH_MAC_FLAG_PIN_TO_CORE')
_DMA_BURST_LENGTHS = tuple(f'ETH_DMA_BURST_LEN_{length}' for length in (1, 2, 4, 8, 16, 32))
_CONFIG_FIELDS = frozenset((
    'mac_config', 'phy_config', 'smi_gpio', 'clock_config', 'clock_config_out_in',
    'dataif_gpio', 'dma_burst_len', 'intr_priority', 'mdc_freq_hz',
    'check_link_period_ms', 'vendor_config',
))
_DEFAULT_PHY_DEPENDENCIES = {
    'yt8531': {'espressif/yt8531': '^0.2.0'},
}

DEV_ETHERNET_IO_LIST = {
    'emac_rmii': [
        'phy_reset_gpio_num', 'mdc_num', 'mdio_num', 'clock_gpio', 'clock_out_in_gpio',
        'tx_en_num', 'txd0_num', 'txd1_num', 'crs_dv_num', 'rxd0_num', 'rxd1_num',
    ],
    'emac_rgmii': [
        'phy_reset_gpio_num', 'mdc_num', 'mdio_num',
        'clock_rx_gpio', 'clock_tx_gpio', 'clock_phy_ref_gpio',
        'tx_ctl_num', 'txd0_num', 'txd1_num', 'txd2_num', 'txd3_num',
        'rx_ctl_num', 'rxd0_num', 'rxd1_num', 'rxd2_num', 'rxd3_num',
    ],
}


def extract_metadata(name: str, raw_config: dict, parse_result: dict, context: dict) -> dict:
    """Expose PHY reset, SMI, data, and separate RMII clock GPIOs."""
    init = parse_result['struct_init']
    emac = init['sub_cfg'][init['sub_type']]
    interface = 'rmii' if init['sub_type'] == 'emac_rmii' else 'rgmii'
    io = {'phy_reset_gpio_num': init['phy_config']['reset_gpio_num']}
    io.update(emac['smi_gpio'])
    io.update(emac['clock_config'][interface])
    io.update(emac.get('dataif_gpio', {}).get(interface, {}))
    if 'clock_config_out_in' in emac:
        io['clock_out_in_gpio'] = emac['clock_config_out_in']['rmii']['clock_gpio']
    return {'io': {key: value for key, value in io.items() if isinstance(value, int) and value >= 0}}


def get_includes() -> list:
    """Return required include headers for the ethernet device."""
    return ['dev_ethernet.h']


def get_dependencies(full_config: dict) -> dict:
    """Return built-in PHY driver defaults; device YAML supplies custom dependencies."""
    return dict(_DEFAULT_PHY_DEPENDENCIES.get(full_config.get('chip'), {}))


def _require_int(config, key, allow_negative=False):
    """Read an integer GPIO/address field from a mapping."""
    if key not in config:
        raise ValueError(f"Ethernet device missing required '{key}'")
    value = config[key]
    if isinstance(value, bool) or not isinstance(value, int):
        raise ValueError(f"Ethernet device '{key}' must be an integer")
    if allow_negative:
        if value != -1 and (value < 0 or value > 255):
            raise ValueError(f"Ethernet device '{key}' must be -1 or between 0 and 255")
    elif value < 0 or value > 255:
        raise ValueError(f"Ethernet device '{key}' must be between 0 and 255")
    return value


def _require_mapping(value, what):
    """Require a YAML mapping and return it."""
    if not isinstance(value, dict):
        raise ValueError(f'Ethernet device {what} must be a mapping')
    return value


def _read_int(config, key, default, minimum, maximum):
    """Read a bounded native integer without accepting YAML booleans."""
    value = config.get(key, default)
    if isinstance(value, bool) or not isinstance(value, int) or not minimum <= value <= maximum:
        raise ValueError(f"Ethernet device '{key}' must be an integer between {minimum} and {maximum}")
    return value


def _reject_unknown_keys(config, allowed, what):
    unknown = set(config) - set(allowed)
    if unknown:
        raise ValueError(f'Ethernet device {what} contains unsupported fields: {", ".join(sorted(unknown))}')


def _parse_mac_config(config):
    mac = _require_mapping(config.get('mac_config', {}), 'mac_config')
    _reject_unknown_keys(mac, set(_MAC_FIELDS) | {'flags'}, 'mac_config')
    result = {key: _read_int(mac, key, *limits) for key, limits in _MAC_FIELDS.items()}
    flags = mac.get('flags', 0)
    if isinstance(flags, int) and not isinstance(flags, bool) and 0 <= flags <= 3:
        result['flags'] = flags
    elif isinstance(flags, list) and all(isinstance(flag, str) and flag in _MAC_FLAGS for flag in flags):
        result['flags'] = ' | '.join(dict.fromkeys(flags)) if flags else 0
    elif isinstance(flags, str) and flags in _MAC_FLAGS:
        result['flags'] = flags
    else:
        raise ValueError('Ethernet device mac_config flags must contain only supported ETH_MAC_FLAG values')
    return result


def _parse_phy_config(config):
    phy = _require_mapping(config.get('phy_config', {}), 'phy_config')
    _reject_unknown_keys(phy, _PHY_FIELDS, 'phy_config')
    return {key: _read_int(phy, key, *limits) for key, limits in _PHY_FIELDS.items()}


def _parse_emac_options(config):
    # ESP32-S31's AXI EMAC supports bursts up to 16 beats; ESP32/P4 default to 32.
    s31 = current_soc_chip_name() == 'esp32s31'
    dma_burst_len = config.get('dma_burst_len', 'ETH_DMA_BURST_LEN_16' if s31 else 'ETH_DMA_BURST_LEN_32')
    if dma_burst_len not in _DMA_BURST_LENGTHS or (s31 and dma_burst_len == 'ETH_DMA_BURST_LEN_32'):
        raise ValueError("Ethernet device 'dma_burst_len' is not supported on this SoC")
    return {
        'dma_burst_len': dma_burst_len,
        'intr_priority': _read_int(config, 'intr_priority', 0, _INT32_MIN, 3),
        'mdc_freq_hz': _read_int(config, 'mdc_freq_hz', 0, _INT32_MIN, _INT32_MAX),
    }


def _parse_clock_config_out_in(config, interface):
    explicit = 'clock_config_out_in' in config
    internal_loopback = current_soc().supports(
        'SOC_EMAC_RMII_CLK_OUT_INTERNAL_LOOPBACK',
        default=current_soc_chip_name() in ('', 'esp32'),
    )
    rmii_output = (interface == 'EMAC_DATA_INTERFACE_RMII' and
                   config['clock_config']['clock_mode'] == 'EMAC_CLK_OUT')
    if explicit and (internal_loopback or not rmii_output):
        raise ValueError('Ethernet device clock_config_out_in requires RMII clock output with external loopback support')
    if internal_loopback or interface != 'EMAC_DATA_INTERFACE_RMII':
        return None
    clock = _require_mapping(config.get('clock_config_out_in', {
        'clock_mode': 'EMAC_CLK_EXT_IN', 'clock_gpio': -1,
    }), 'clock_config_out_in')
    _require_exact_keys(clock, _RMII_CLOCK_KEYS, 'clock_config_out_in')
    gpio = _gpio_or_nc(clock, 'clock_gpio')
    if clock['clock_mode'] != 'EMAC_CLK_EXT_IN':
        raise ValueError('Ethernet device clock_config_out_in clock_mode must be EMAC_CLK_EXT_IN')
    if rmii_output and current_soc_chip_name() == 'esp32p4' and gpio < 0:
        raise ValueError('Ethernet device clock_config_out_in needs a return clock GPIO on ESP32-P4')
    return {'rmii': {'clock_mode': 'EMAC_CLK_EXT_IN', 'clock_gpio': gpio}}


def _parse_vendor_config(name, config, chip, sub_type):
    """Delegate chip-owned fields and emit a separate typed configuration object."""
    if 'vendor_config' not in config:
        return None
    vendor_config = _require_mapping(config['vendor_config'], 'vendor_config')
    descriptor = current_phy_parser_registry().get(chip, {})
    callback = descriptor.get('parse')
    if not callable(callback):
        raise ValueError(f'Ethernet device {name!r} vendor_config has no PHY parser for chip {chip!r}')
    context = {
        'chip': chip,
        'sub_type': sub_type,
        'idf_version': get_idf_version(),
        'target': current_soc_chip_name(),
    }
    result = callback(name, vendor_config, context)
    if not isinstance(result, dict):
        raise ValueError(f'Ethernet device {name!r} PHY parser must return a vendor configuration mapping')
    struct_type = result.get('struct_type')
    if not isinstance(struct_type, str) or not struct_type.strip():
        raise ValueError(f'Ethernet device {name!r} PHY parser needs a vendor struct_type')
    if not isinstance(result.get('struct_init'), dict):
        raise ValueError(f'Ethernet device {name!r} PHY parser vendor struct_init must be a mapping')
    includes = result.get('includes', [])
    if not isinstance(includes, list) or any(not isinstance(header, str) or not header for header in includes):
        raise ValueError(f'Ethernet device {name!r} PHY parser includes must be a list of header names')
    native_type = struct_type.strip()
    return {
        'struct_type': native_type if native_type.startswith('const ') else f'const {native_type}',
        'struct_var': f'esp_bmgr_{name.replace("-", "_")}_vendor_config',
        'struct_init': result['struct_init'],
        'includes': includes,
    }


def _gpio_or_nc(config, key):
    """Read a GPIO number that may be -1 (not connected)."""
    return _require_int(config, key, allow_negative=True)


def _has_configurable_dataif():
    """Match the native EMAC data-plane GPIO field availability."""
    return (current_soc().supports('SOC_EMAC_USE_MULTI_IO_MUX', default=False) or
            current_soc().supports('SOC_EMAC_MII_USE_GPIO_MATRIX', default=False))


def _require_exact_keys(mapping, expected, what):
    """Reject extra or missing keys against an exact expected set."""
    if frozenset(mapping.keys()) != expected:
        raise ValueError(f'Ethernet device {what} keys are invalid')


def _parse_clock_config(name, config, interface):
    """Map a flat YAML clock_config onto one C union arm."""
    clock_config = _require_mapping(config.get('clock_config'), 'clock_config')
    if interface == 'EMAC_DATA_INTERFACE_RMII':
        _require_exact_keys(clock_config, _RMII_CLOCK_KEYS, 'clock_config')
        clock_mode = clock_config.get('clock_mode')
        if clock_mode not in _RMII_CLOCK_MODES:
            raise ValueError(
                f"Ethernet device '{name}' clock_config clock_mode '{clock_mode}' is invalid"
            )
        return {
            'rmii': {
                'clock_mode': clock_mode,
                'clock_gpio': _require_int(clock_config, 'clock_gpio'),
            },
        }
    _require_exact_keys(clock_config, _RGMII_CLOCK_KEYS, 'clock_config')
    return {
        'rgmii': {
            'clock_rx_gpio': _require_int(clock_config, 'clock_rx_gpio'),
            'clock_tx_gpio': _require_int(clock_config, 'clock_tx_gpio'),
            'clock_phy_ref_gpio': _gpio_or_nc(clock_config, 'clock_phy_ref_gpio'),
        },
    }


def _parse_int_keys(mapping, keys):
    """Parse every expected GPIO key as a required 0–255 pin."""
    return {key: _require_int(mapping, key) for key in keys}


def _parse_dataif_gpio(name, config, interface):
    """Parse optional/required dataif_gpio into one C union arm, or None."""
    dataif_gpio = config.get('dataif_gpio')
    if interface == 'EMAC_DATA_INTERFACE_RGMII':
        dataif_gpio = _require_mapping(dataif_gpio, 'dataif_gpio')
        _require_exact_keys(dataif_gpio, _RGMII_DATAIF_KEYS, 'dataif_gpio')
        return {'rgmii': _parse_int_keys(dataif_gpio, _RGMII_DATAIF_KEYS)}

    if dataif_gpio is not None:
        if not _has_configurable_dataif():
            raise ValueError(
                f"Ethernet device '{name}' dataif_gpio is not supported without configurable EMAC data GPIOs"
            )
        dataif_gpio = _require_mapping(dataif_gpio, 'dataif_gpio')
        _require_exact_keys(dataif_gpio, _RMII_DATAIF_KEYS, 'dataif_gpio')
        return {'rmii': _parse_int_keys(dataif_gpio, _RMII_DATAIF_KEYS)}

    if _has_configurable_dataif():
        raise ValueError(
            f"Ethernet device '{name}' dataif_gpio is required when EMAC data GPIOs are configurable"
        )
    return None


def parse(name: str, full_config: dict, peripherals_dict=None) -> dict:
    """Parse an EMAC ethernet configuration into a C initializer."""
    del peripherals_dict

    peripherals = full_config.get('peripherals', [])
    if peripherals is None:
        peripherals = []
    if not isinstance(peripherals, list):
        raise ValueError(f"Ethernet device '{name}' peripherals must be a list")
    if peripherals:
        raise ValueError(f"Ethernet device '{name}' does not support peripheral bindings")

    sub_type = full_config.get('sub_type')
    if not sub_type:
        raise ValueError(f"Ethernet device '{name}' missing required 'sub_type'")
    if sub_type not in _EMAC_INTERFACES:
        raise ValueError(
            f"Ethernet device '{name}' sub_type '{sub_type}' is not yet supported"
        )

    if 'chip' in full_config:
        chip = full_config['chip']
        if not isinstance(chip, str):
            raise ValueError(f"Ethernet device '{name}' chip must be a string")
    else:
        chip = ''

    config = _require_mapping(full_config.get('config', {}), 'config')
    _reject_unknown_keys(config, _CONFIG_FIELDS, 'config')
    interface = _EMAC_INTERFACES[sub_type]
    if sub_type == 'emac_rgmii':
        if get_idf_version() < IDF_VERSION_WITH_RGMII:
            raise ValueError(f"Ethernet device '{name}' emac_rgmii requires ESP-IDF v6.1+")
        if not current_soc().supports('SOC_EMAC_SUPPORT_1000M', default=False):
            raise ValueError(f"Ethernet device '{name}' emac_rgmii requires SOC_EMAC_SUPPORT_1000M")

    smi_gpio_cfg = _require_mapping(config.get('smi_gpio'), 'smi_gpio')
    smi_gpio = {
        'mdc_num': _require_int(smi_gpio_cfg, 'mdc_num'),
        'mdio_num': _require_int(smi_gpio_cfg, 'mdio_num'),
    }

    emac_cfg = {
        'smi_gpio': smi_gpio,
        'clock_config': _parse_clock_config(name, config, interface),
        **_parse_emac_options(config),
    }
    dataif_gpio = _parse_dataif_gpio(name, config, interface)
    if dataif_gpio is not None:
        emac_cfg['dataif_gpio'] = dataif_gpio
    clock_config_out_in = _parse_clock_config_out_in(config, interface)
    if clock_config_out_in is not None:
        emac_cfg['clock_config_out_in'] = clock_config_out_in

    vendor = _parse_vendor_config(name, config, chip, sub_type)
    result = {
        'struct_type': 'dev_ethernet_config_t',
        'struct_var': f"{name.replace('-', '_')}_cfg",
        'struct_init': {
            'name': name,
            'chip': chip,
            'sub_type': sub_type,
            'mac_config': _parse_mac_config(config),
            'phy_config': _parse_phy_config(config),
            'check_link_period_ms': _read_int(config, 'check_link_period_ms', 2000, 1, _UINT32_MAX),
            'vendor_config': f'&{vendor["struct_var"]}' if vendor else None,
            'vendor_config_size': f'sizeof({vendor["struct_var"]})' if vendor else 0,
            'sub_cfg': {
                sub_type: emac_cfg,
            },
        },
    }
    if vendor is not None:
        result['includes'] = vendor.pop('includes')
        result['extra_configs'] = [vendor]
    descriptor = current_phy_parser_registry().get(chip, {})
    if descriptor.get('sources'):
        result['runtime_sources'] = list(descriptor['sources'])
    if chip:
        result['runtime_source_groups'] = {f'ethernet_phy:{chip}': sorted(descriptor.get('sources', []))}
    return result
