# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

"""YT8531 vendor settings expressed with the official driver configuration type."""


def parse_yt8531(name, vendor_config, context):
    """Validate the RGMII clock-delay fields consumed by the YT8531 driver."""
    if context.get('sub_type') != 'emac_rgmii':
        raise ValueError(f'Ethernet device {name!r} YT8531 vendor_config requires emac_rgmii')
    fields = {'rxc_dly_en', 'rx_delay_sel', 'tx_delay_sel', 'tx_delay_sel_fe'}
    if not isinstance(vendor_config, dict) or set(vendor_config) != fields:
        raise ValueError(f'Ethernet device {name!r} YT8531 vendor_config requires exactly {sorted(fields)}')
    if not isinstance(vendor_config['rxc_dly_en'], bool):
        raise ValueError('Ethernet device vendor_config rxc_dly_en must be a boolean')
    for key in ('rx_delay_sel', 'tx_delay_sel', 'tx_delay_sel_fe'):
        value = vendor_config[key]
        if isinstance(value, bool) or not isinstance(value, int) or not 0 <= value <= 15:
            raise ValueError(f'Ethernet device vendor_config {key} must be an integer between 0 and 15')
    return {
        'struct_type': 'yt8531_rgmii_clk_delay_config_t',
        'struct_init': dict(vendor_config),
        'includes': ['esp_eth_phy_yt8531.h'],
    }


def get_phy_parsers():
    """Publish this module's PHY configuration parser and runtime adapter."""
    return {
        'yt8531': {
            'parse': parse_yt8531,
            'sources': ['yt8531.c'],
        },
    }
