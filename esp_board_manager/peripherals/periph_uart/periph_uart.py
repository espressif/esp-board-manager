# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

# UART device config parser
VERSION = 'v1.0.0'

from generators.utils.idf_enum import resolve_idf_macro

KNOWN_UART_DATA_BITS = frozenset((
    'UART_DATA_5_BITS',
    'UART_DATA_6_BITS',
    'UART_DATA_7_BITS',
    'UART_DATA_8_BITS',
    'UART_DATA_BITS_MAX',
))
KNOWN_UART_PARITY = frozenset(('UART_PARITY_DISABLE', 'UART_PARITY_EVEN', 'UART_PARITY_ODD'))
KNOWN_UART_STOP_BITS = frozenset((
    'UART_STOP_BITS_1',
    'UART_STOP_BITS_1_5',
    'UART_STOP_BITS_2',
    'UART_STOP_BITS_MAX',
))
KNOWN_UART_FLOW_CTRL = frozenset((
    'UART_HW_FLOWCTRL_DISABLE',
    'UART_HW_FLOWCTRL_RTS',
    'UART_HW_FLOWCTRL_CTS',
    'UART_HW_FLOWCTRL_CTS_RTS',
    'UART_HW_FLOWCTRL_MAX',
))
KNOWN_UART_MODE = frozenset((
    'UART_MODE_UART',
    'UART_MODE_RS485_HALF_DUPLEX',
    'UART_MODE_IRDA',
    'UART_MODE_RS485_COLLISION_DETECT',
    'UART_MODE_RS485_APP_CTRL',
))
KNOWN_UART_SCLK = frozenset((
    'UART_SCLK_DEFAULT',
    'UART_SCLK_APB',
    'UART_SCLK_REF_TICK',
    'UART_SCLK_RTC',
    'UART_SCLK_XTAL',
    'UART_SCLK_PLL_F40M',
    'UART_SCLK_PLL_F48M',
    'UART_SCLK_PLL_F80M',
    'UART_SCLK_REF_F80M',
))
KNOWN_LP_UART_SCLK = frozenset((
    'LP_UART_SCLK_DEFAULT',
    'LP_UART_SCLK_RC_FAST',
    'LP_UART_SCLK_XTAL',
    'LP_UART_SCLK_XTAL_D2',
    'LP_UART_SCLK_LP_FAST',
    'LP_UART_SCLK_LP_PLL',
))

PERIPH_UART_IO_LIST = [
    'tx_io_num',
    'rx_io_num',
    'rts_io_num',
    'cts_io_num',
]

_UART_ENUMS = {
    'data_bits': (('UART_DATA_',), KNOWN_UART_DATA_BITS),
    'parity': (('UART_PARITY_',), KNOWN_UART_PARITY),
    'stop_bits': (('UART_STOP_BITS_',), KNOWN_UART_STOP_BITS),
    'flow_ctrl': (('UART_HW_FLOWCTRL_',), KNOWN_UART_FLOW_CTRL),
    'uart_mode': (('UART_MODE_',), KNOWN_UART_MODE),
    'source_clk': (('UART_SCLK_',), KNOWN_UART_SCLK),
    'lp_source_clk': (('LP_UART_SCLK_',), KNOWN_LP_UART_SCLK),
}


def get_enum_value(value: str, default_value: str, enum_type: str = None) -> str:
    """Resolve a YAML UART enumerator, passing through unknown prefixed macros."""
    if not enum_type:
        return value if value else default_value
    spec = _UART_ENUMS.get(enum_type)
    if spec is None:
        print(f"⚠️  WARNING: Unknown enum type '{enum_type}' for UART")
        return value if value else default_value
    prefixes, known = spec
    return resolve_idf_macro(value, default_value, prefixes, known, enum_type)

def get_includes() -> list:
    """Return list of required include headers for UART"""
    return [
        'periph_uart.h'
    ]

def parse_uart_config(uart_config_dict: dict) -> dict:
    """Parse UART configuration structure from YAML"""

    uart_config = {
        'baud_rate': uart_config_dict.get('baud_rate', 115200),
        'data_bits': get_enum_value(uart_config_dict.get('data_bits'), 'UART_DATA_8_BITS', 'data_bits'),
        'parity': get_enum_value(uart_config_dict.get('parity'), 'UART_PARITY_DISABLE', 'parity'),
        'stop_bits': get_enum_value(uart_config_dict.get('stop_bits'), 'UART_STOP_BITS_1', 'stop_bits'),
        'flow_ctrl': get_enum_value(uart_config_dict.get('flow_ctrl'), 'UART_HW_FLOWCTRL_DISABLE', 'flow_ctrl'),  # Default disabled
        'rx_flow_ctrl_thresh': uart_config_dict.get('rx_flow_ctrl_thresh', 0),
    }

    # Handle union for source clock - ensure only one is specified
    source_clk = uart_config_dict.get('source_clk')
    lp_source_clk = uart_config_dict.get('lp_source_clk')

    if source_clk is not None and lp_source_clk is not None:
        raise ValueError("YAML validation error in UART device: Cannot specify both 'source_clk' and 'lp_source_clk' in the union")

    if source_clk is not None:
        uart_config['source_clk'] = get_enum_value(source_clk, 'UART_SCLK_DEFAULT', 'source_clk')
    elif lp_source_clk is not None:
        uart_config['lp_source_clk'] = get_enum_value(lp_source_clk, 'LP_UART_SCLK_DEFAULT', 'lp_source_clk')
    else:
        uart_config['source_clk'] = 'UART_SCLK_DEFAULT'

    # Handle nested flags structure with bitfields
    flags_dict = uart_config_dict.get('flags', {})
    flags_config = {
        'allow_pd': flags_dict.get('allow_pd', 0),  # Default to 0 (not allowed)
        'backup_before_sleep': flags_dict.get('backup_before_sleep', 0)  # Default to 0
    }
    uart_config['flags'] = flags_config

    return uart_config

def parse(name: str, full_config: dict, peripherals_dict=None) -> dict:
    """Parse UART device configuration from YAML"""
    # Get the actual config from the full_config
    config = full_config.get('config', {})

    # Get UART number
    uart_num = config.get('uart_num')

    # Parse UART configuration
    uart_config_raw = config.get('uart_config', {})
    uart_config = parse_uart_config(uart_config_raw)

    # Build the main configuration structure
    struct_init = {
        'uart_num': uart_num,
        'rx_buffer_size': config.get('rx_buffer_size', 256),
        'tx_buffer_size': config.get('tx_buffer_size', 256),
        'use_queue': config.get('use_queue', False),
        'queue_size': config.get('queue_size', 10),  # Default queue size
        'intr_alloc_flags': config.get('intr_alloc_flags', 0),
        'uart_config': uart_config,
        'tx_io_num': config.get('tx_io_num', -1),
        'rx_io_num': config.get('rx_io_num', -1),
        'rts_io_num': config.get('rts_io_num', -1),
        'cts_io_num': config.get('cts_io_num', -1),
        'uart_mode': get_enum_value(config.get('uart_mode'), 'UART_MODE_UART', 'uart_mode')
    }

    # Build the result
    result = {
        'struct_type': 'periph_uart_config_t',
        'struct_var': f'{name}_cfg',
        'struct_init': struct_init
    }

    return result
