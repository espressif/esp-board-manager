# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

# Analog Comparator peripheral config parser
VERSION = 'v1.0.0'

from generators.utils.idf_enum import resolve_idf_macro

KNOWN_ANA_CMPR_REF_SRC = frozenset((
    'ANA_CMPR_REF_SRC_INTERNAL',
    'ANA_CMPR_REF_SRC_EXTERNAL',
))
KNOWN_ANA_CMPR_CROSS = frozenset((
    'ANA_CMPR_CROSS_POS',
    'ANA_CMPR_CROSS_NEG',
    'ANA_CMPR_CROSS_ANY',
    'ANA_CMPR_CROSS_DISABLE',
))
KNOWN_ANA_CMPR_CLK_SRC = frozenset((
    'ANA_CMPR_CLK_SRC_DEFAULT',
    'ANA_CMPR_CLK_SRC_XTAL',
    'ANA_CMPR_CLK_SRC_RC_FAST',
    'ANA_CMPR_CLK_SRC_PLL_F48M',
    'ANA_CMPR_CLK_SRC_PLL_F80M',
    'ANA_CMPR_CLK_SRC_REF_F80M',
))


def get_enum_value(value: str, full_enum: str, enum_type: str = None) -> str:
    specs = {
        'ana_cmpr_ref_src': (('ANA_CMPR_REF_SRC_',), KNOWN_ANA_CMPR_REF_SRC),
        'ana_cmpr_cross_type': (('ANA_CMPR_CROSS_',), KNOWN_ANA_CMPR_CROSS),
        'ana_cmpr_clk_src': (('ANA_CMPR_CLK_SRC_',), KNOWN_ANA_CMPR_CLK_SRC),
    }
    if not enum_type:
        return value if value else full_enum
    spec = specs.get(enum_type)
    if spec is None:
        print(f"⚠️  WARNING: Unknown enum type '{enum_type}' for validation")
        return value if value else full_enum
    prefixes, known = spec
    return resolve_idf_macro(value, full_enum, prefixes, known, enum_type)


def get_includes() -> list:
    """Return list of required include headers for Analog Comparator peripheral"""
    return [
        'periph_anacmpr.h'
    ]


def parse(name: str, config: dict) -> dict:
    """Parse Analog Comparator peripheral configuration.

    Args:
        name: Peripheral name (e.g. 'anacmpr-0')
        config: Configuration dictionary containing:
            - unit: Analog comparator unit number
            - ref_src: Reference source type
            - cross_type: Cross type for interrupt triggering
            - clk_src: Clock source
            - intr_priority: Interrupt priority
            - flags: Driver flags configuration

    Returns:
        Dictionary containing Analog Comparator configuration structure
    """
    try:
        # Get the actual config
        config = config.get('config', {})

        # Parse analog comparator configuration
        unit = config.get('unit', 0)

        ref_src = get_enum_value(config.get('ref_src'), 'ANA_CMPR_REF_SRC_INTERNAL', 'ana_cmpr_ref_src')
        cross_type = get_enum_value(config.get('cross_type'), 'ANA_CMPR_CROSS_ANY', 'ana_cmpr_cross_type')
        clk_src = get_enum_value(config.get('clk_src'), 'ANA_CMPR_CLK_SRC_DEFAULT', 'ana_cmpr_clk_src')
        intr_priority = config.get('intr_priority', 0)

        return {
            'struct_type': 'ana_cmpr_config_t',
            'struct_var': f'{name}_cfg',
            'struct_init': {
                'unit': unit,
                'ref_src': ref_src,
                'cross_type': cross_type,
                'clk_src': clk_src,
                'intr_priority': intr_priority,
            }
        }

    except ValueError as e:
        # Re-raise ValueError with more context
        raise ValueError(f"YAML validation error in Analog Comparator peripheral '{name}': {e}")
    except Exception as e:
        # Catch any other exceptions and provide context
        raise ValueError(f"Error parsing Analog Comparator peripheral '{name}': {e}")
