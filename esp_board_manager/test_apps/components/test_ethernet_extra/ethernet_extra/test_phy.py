# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

"""Custom PHY parser discovered from the compile fixture's board directory."""


def parse_test_phy(name, vendor_config, context):
    """Use an explicit driver-owned type for the generated vendor payload."""
    if set(vendor_config) != {'settle_time_ms'}:
        raise ValueError(f'{name}: vendor_config requires settle_time_ms')
    value = vendor_config['settle_time_ms']
    if type(value) is not int or not 1 <= value <= 0xFFFFFFFF:
        raise ValueError(f'{name}: settle_time_ms must be a positive uint32_t')
    return {
        'struct_type': 'bmgr_test_eth_phy_config_t',
        'struct_init': {'settle_time_ms': value},
        'includes': ['bmgr_test_eth_phy.h'],
    }


def get_phy_parsers():
    """Publish the custom PHY configuration parser and selected C adapter."""
    return {
        'bmgr_test_phy': {
            'parse': parse_test_phy,
            'sources': ['test_phy.c'],
        },
    }
