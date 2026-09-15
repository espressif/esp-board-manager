#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

"""Generate synthetic CI coverage boards from the bundled SoC catalogs."""

from __future__ import annotations

import json
from pathlib import Path

import yaml

HERE = Path(__file__).resolve().parent
SETUP_DEVICE_SRC = Path(__file__).resolve().parent / 'setup_device.c'
BMGR_ROOT = HERE.parents[2]
CATALOG_DIR = BMGR_ROOT / 'private_inc' / 'soc_capability_catalog'
COMMON_SDKCONFIG = 'CONFIG_LITTLEFS_SDMMC_SUPPORT=y\n'

BOARDS = (
    ('esp32_coverage', 'esp32'),
    ('esp32s3_coverage', 'esp32s3'),
    ('esp32c3_coverage', 'esp32c3'),
    ('esp32p4_coverage', 'esp32p4'),
    ('esp32s31_coverage', 'esp32s31'),
)

# Product board esp32_p4_function_ev_board already compiles DSI LCD + LVGL.
# Coverage P4 omits LCD so IDF v5.4.4 test_apps do not overflow internal RAM.
SKIP_DEVICE_CAPS_BY_CHIP = {
    'esp32p4': frozenset((
        'device.display_lcd_spi',
        'device.display_lcd_dsi',
    )),
}

# GPIOs commonly tied to flash / strapping that we still keep out of the pool.
FLASH_SKIP = {
    'esp32': {6, 7, 8, 9, 10, 11},
    'esp32c3': {12, 13, 14, 15, 16, 17},
}

SDKCONFIG = {
    'esp32': '\n'.join((
        'CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y',
        'CONFIG_SPIRAM=y',
        'CONFIG_SPIRAM_SPEED_80M=y',
        'CONFIG_CODEC_ES8311_SUPPORT=y',
        '',
    )),
    'esp32s3': '\n'.join((
        'CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y',
        'CONFIG_SPIRAM=y',
        'CONFIG_SPIRAM_MODE_OCT=y',
        'CONFIG_CODEC_ES8311_SUPPORT=y',
        '',
    )),
    'esp32c3': '\n'.join((
        '# CONFIG_SPIRAM is not set',
        'CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y',
        'CONFIG_ESPTOOLPY_FLASHSIZE="8MB"',
        'CONFIG_PARTITION_TABLE_CUSTOM=y',
        'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions_test_app.csv"',
        'CONFIG_PARTITION_TABLE_FILENAME="partitions_test_app.csv"',
        'CONFIG_CODEC_ES8311_SUPPORT=n',
        'CONFIG_CODEC_DUMMY_SUPPORT=y',
        'CONFIG_CODEC_DATA_ADC_SUPPORT=y',
        '',
    )),
    'esp32p4': '\n'.join((
        'CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y',
        'CONFIG_SPIRAM=y',
        'CONFIG_SPIRAM_MODE_HEX=y',
        'CONFIG_CODEC_ES8311_SUPPORT=y',
        '',
    )),
    'esp32s31': '\n'.join((
        'CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y',
        'CONFIG_SPIRAM=y',
        'CONFIG_SPIRAM_MODE_OCT=y',
        'CONFIG_CODEC_ES8311_SUPPORT=y',
        '',
    )),
}


class PinPool:
    def __init__(self, pins: list[int]):
        self._pins = list(pins)

    def take(self) -> int:
        if not self._pins:
            raise RuntimeError('coverage board ran out of GPIOs')
        return self._pins.pop(0)

    @property
    def remaining(self) -> int:
        return len(self._pins)


def _load_chip(chip: str) -> dict:
    """Intersect every catalog profile that contains ``chip``.

    Coverage YAML is compiled on IDF v5.4 through latest. ``idf.py bmgr``
    validates against the running IDF's catalog, so a 6.1-only capability
    would fail on v5.4.4.
    """
    index = json.loads((CATALOG_DIR / 'index.json').read_text(encoding='utf-8'))
    profiles = []
    for entry in index['profiles']:
        data = json.loads((CATALOG_DIR / entry['path']).read_text(encoding='utf-8'))
        chip_data = data.get('chips', {}).get(chip)
        if chip_data is not None:
            profiles.append(chip_data)
    if not profiles:
        raise KeyError(f'no SoC catalog profile contains chip {chip}')

    cap_keys = set().union(*(p['capabilities'].keys() for p in profiles))
    capabilities = {
        key: all(bool(p['capabilities'].get(key)) for p in profiles)
        for key in cap_keys
    }

    limit_keys = set().union(*(p.get('hardwareLimits', {}).keys() for p in profiles))
    hardware_limits = {}
    for key in limit_keys:
        values = [p.get('hardwareLimits', {}).get(key) for p in profiles]
        values = [int(v) for v in values if v is not None]
        if values:
            hardware_limits[key] = min(values)

    gpio_out = [pin for pin in profiles[0]['gpio']['validOutput']
                if all(pin in p['gpio']['validOutput'] for p in profiles)]
    gpio_in = [pin for pin in profiles[0]['gpio']['validInput']
               if all(pin in p['gpio']['validInput'] for p in profiles)]

    merged = dict(profiles[0])
    merged['capabilities'] = capabilities
    merged['hardwareLimits'] = hardware_limits
    merged['gpio'] = {
        **profiles[0].get('gpio', {}),
        'validOutput': gpio_out,
        'validInput': gpio_in,
    }
    return merged


def _cap(chip_data: dict, key: str) -> bool:
    return bool(chip_data['capabilities'].get(key))


def _want_device(chip: str, chip_data: dict, key: str) -> bool:
    if key in SKIP_DEVICE_CAPS_BY_CHIP.get(chip, frozenset()):
        return False
    return _cap(chip_data, key)


def _i2s_instances(chip_data: dict) -> int:
    return int(chip_data['hardwareLimits']['i2s.instance_count'])


def _spi_instances(chip_data: dict) -> int:
    return int(chip_data['hardwareLimits']['spi.instance_count'])


def _gpio_pool(chip: str, chip_data: dict) -> PinPool:
    skip = FLASH_SKIP.get(chip, set())
    pins = [p for p in chip_data['gpio']['validOutput'] if p not in skip]
    pins.reverse()
    return PinPool(pins)


def _gpio(name: str, pin: int, mode: str = 'GPIO_MODE_OUTPUT', pull_up: bool = False) -> dict:
    return {
        'name': name,
        'type': 'gpio',
        'role': 'io',
        'config': {
            'pin': pin,
            'mode': mode,
            'pull_up': pull_up,
            'pull_down': False,
            'intr_type': 'GPIO_INTR_DISABLE',
            'default_level': 0,
        },
    }


def _std_i2s(name: str, fmt: str, port: int, pins: dict) -> dict:
    return {
        'name': name,
        'type': 'i2s',
        'role': 'master',
        'format': fmt,
        'config': {
            'port': port,
            'sample_rate_hz': 48000,
            'clk_src': 'I2S_CLK_SRC_DEFAULT',
            'data_bit_width': 16,
            'pins': pins,
        },
    }


def _build_peripherals(chip: str, chip_data: dict, pins: PinPool) -> list[dict]:
    periphs: list[dict] = []
    i2s_used = 0
    i2s_budget = _i2s_instances(chip_data)

    periphs.append(_gpio('gpio_monitor', pins.take(), 'GPIO_MODE_INPUT', True))
    periphs.append(_gpio('gpio_pa_control', pins.take()))
    periphs.append(_gpio('gpio_button', pins.take(), 'GPIO_MODE_INPUT', True))
    periphs.append(_gpio('gpio_power', pins.take()))

    periphs.append({
        'name': 'i2c_master',
        'type': 'i2c',
        'role': 'master',
        'config': {
            'port': 'I2C_NUM_0',
            'pins': {'sda': pins.take(), 'scl': pins.take()},
            'enable_internal_pullup': True,
        },
    })
    if _cap(chip_data, 'i2c.lp_supported'):
        periphs.append({
            'name': 'i2c_lp',
            'type': 'i2c',
            'role': 'master',
            'config': {
                'port': 'LP_I2C_NUM_0',
                'pins': {'sda': pins.take(), 'scl': pins.take()},
                'enable_internal_pullup': True,
            },
        })

    use_pdm_as_primary = chip == 'esp32c3' and _cap(chip_data, 'peripheral.i2s_master_pdm-out')
    if use_pdm_as_primary:
        periphs.append({
            'name': 'i2s_audio_out',
            'type': 'i2s',
            'role': 'master',
            'format': 'pdm-out',
            'config': {
                'port': 0,
                'sample_rate_hz': 16000,
                'clk_src': 'I2S_CLK_SRC_DEFAULT',
                'data_bit_width': 16,
                'pins': {'clk': -1, 'dout': pins.take()},
            },
        })
        i2s_used += 1
    elif _cap(chip_data, 'peripheral.i2s_master_std'):
        bclk, ws, dout, din = pins.take(), pins.take(), pins.take(), pins.take()
        periphs.append(_std_i2s(
            'i2s_audio_out',
            'std-out',
            0,
            {'mclk': -1, 'bclk': bclk, 'ws': ws, 'dout': dout, 'din': -1},
        ))
        periphs.append(_std_i2s(
            'i2s_audio_in',
            'std-in',
            0,
            {'mclk': -1, 'bclk': bclk, 'ws': ws, 'dout': -1, 'din': din},
        ))
        i2s_used += 1

    if (
        i2s_used < i2s_budget
        and _cap(chip_data, 'peripheral.i2s_master_tdm')
        and not use_pdm_as_primary
    ):
        periphs.append({
            'name': 'i2s_tdm_out',
            'type': 'i2s',
            'role': 'master',
            'format': 'tdm-out',
            'config': {
                'port': i2s_used,
                'sample_rate_hz': 48000,
                'clk_src': 'I2S_CLK_SRC_DEFAULT',
                'data_bit_width': 16,
                'slot_mask': 'I2S_TDM_SLOT0 | I2S_TDM_SLOT1',
                'pins': {
                    'mclk': -1,
                    'bclk': pins.take(),
                    'ws': pins.take(),
                    'dout': pins.take(),
                    'din': -1,
                },
            },
        })
        i2s_used += 1

    if (
        i2s_used < i2s_budget
        and i2s_budget >= 3
        and _cap(chip_data, 'peripheral.i2s_master_pdm-in')
    ):
        periphs.append({
            'name': 'i2s_pdm_in',
            'type': 'i2s',
            'role': 'master',
            'format': 'pdm-in',
            'config': {
                'port': i2s_used,
                'sample_rate_hz': 16000,
                'clk_src': 'I2S_CLK_SRC_DEFAULT',
                'pins': {'clk': pins.take(), 'din': pins.take()},
            },
        })
        i2s_used += 1
    elif (
        i2s_used < i2s_budget
        and _cap(chip_data, 'peripheral.i2s_master_pdm-out')
        and not use_pdm_as_primary
    ):
        periphs.append({
            'name': 'i2s_pdm_out',
            'type': 'i2s',
            'role': 'master',
            'format': 'pdm-out',
            'config': {
                'port': i2s_used,
                'sample_rate_hz': 16000,
                'clk_src': 'I2S_CLK_SRC_DEFAULT',
                'pins': {'clk': -1, 'dout': pins.take()},
            },
        })

    periphs.append({
        'name': 'spi_master',
        'type': 'spi',
        'config': {
            'spi_bus_config': {
                'spi_port': 'SPI2_HOST',
                'mosi_io_num': pins.take(),
                'miso_io_num': pins.take(),
                'sclk_io_num': pins.take(),
            },
        },
    })
    periphs.append({
        'name': 'uart_1',
        'type': 'uart',
        'config': {
            'uart_num': 1,
            'tx_io_num': pins.take(),
            'rx_io_num': pins.take(),
        },
    })
    periphs.append({
        'name': 'adc_oneshot',
        'type': 'adc',
        'role': 'oneshot',
        'config': {
            'unit_id': 'ADC_UNIT_1',
            'channel_id': 0,
            'atten': 'ADC_ATTEN_DB_12',
            'bit_width': 'ADC_BITWIDTH_DEFAULT',
        },
    })
    periphs.append({
        'name': 'adc_continuous',
        'type': 'adc',
        'role': 'continuous',
        'config': {
            'unit_id': 'ADC_UNIT_1',
            'channel_list': [0],
            'atten': 'ADC_ATTEN_DB_12',
            'bit_width': 'SOC_ADC_DIGI_MAX_BITWIDTH',
            'sample_freq_hz': 20000,
            'max_store_buf_size': 1024,
            'conv_frame_size': 256,
        },
    })
    periphs.append({
        'name': 'ledc_backlight',
        'type': 'ledc',
        'config': {
            'gpio_num': pins.take(),
            'channel': 'LEDC_CHANNEL_0',
            'timer_sel': 'LEDC_TIMER_0',
            'speed_mode': 'LEDC_LOW_SPEED_MODE',
        },
    })
    periphs.append({
        'name': 'rmt_tx',
        'type': 'rmt',
        'role': 'tx',
        'config': {
            'gpio_num': pins.take(),
            'clk_src': 'RMT_CLK_SRC_DEFAULT',
            'resolution_hz': 10000000,
            'mem_block_symbols': 64,
            'trans_queue_depth': 4,
        },
    })
    if _cap(chip_data, 'peripheral.sdm'):
        periphs.append({
            'name': 'sdm',
            'type': 'sdm',
            'config': {
                'gpio_num': pins.take(),
                'clk_src': 'SDM_CLK_SRC_DEFAULT',
                'sample_rate_hz': 1000000,
            },
        })
    if _cap(chip_data, 'peripheral.dac'):
        periphs.append({
            'name': 'dac_channel_0',
            'type': 'dac',
            'role': 'oneshot',
            'config': {'channel': 0},
        })
    if _cap(chip_data, 'peripheral.mcpwm'):
        periphs.append({
            'name': 'mcpwm_group_0',
            'type': 'mcpwm',
            'config': {
                'timer_config': {'group_id': 0},
                'operator_config': {'group_id': 0},
                'comparator_configs': [{'comparator': 0}],
                'generator_config': {'gpio_num': pins.take()},
            },
        })
    if _cap(chip_data, 'peripheral.pcnt'):
        periphs.append({
            'name': 'pcnt_unit',
            'type': 'pcnt',
            'config': {
                'unit_config': {'low_limit': -100, 'high_limit': 100},
                'channel_count': 1,
                'channel_list': [{
                    'channel': 0,
                    'channel_config': {
                        'edge_gpio_num': -1,
                        'level_gpio_num': -1,
                    },
                    'pos_act': 'PCNT_CHANNEL_EDGE_ACTION_INCREASE',
                    'neg_act': 'PCNT_CHANNEL_EDGE_ACTION_HOLD',
                }],
            },
        })
    if _cap(chip_data, 'peripheral.anacmpr'):
        periphs.append({
            'name': 'anacmpr_unit_0',
            'type': 'anacmpr',
            'config': {
                'unit': 0,
                'ref_src': 'ANA_CMPR_REF_SRC_INTERNAL',
                'clk_src': 'ANA_CMPR_CLK_SRC_DEFAULT',
            },
        })
    if _cap(chip_data, 'peripheral.ldo'):
        periphs.append({
            'name': 'ldo_mipi',
            'type': 'ldo',
            'config': {'chan_id': 3, 'voltage_mv': 2500},
        })
    if _cap(chip_data, 'peripheral.dsi'):
        periphs.append({
            'name': 'dsi_display',
            'type': 'dsi',
            'config': {
                'bus_id': 0,
                'data_lanes': 2,
                'phy_clk_src': 0,
                'lane_bit_rate_mbps': 1000,
            },
        })
    return periphs


def _build_devices(chip: str, chip_data: dict, pins: PinPool) -> list[dict]:
    devices: list[dict] = [
        {
            'name': 'lcd_power',
            'type': 'gpio_ctrl',
            'version': '1.0.0',
            'config': {'active_level': 1, 'default_level': 0},
            'peripherals': [{'gpio_name': 'gpio_power'}],
        },
        {
            'name': 'gpio_button_0',
            'type': 'button',
            'sub_type': 'gpio',
            'config': {'active_level': 0},
            'peripherals': [{'gpio_name': 'gpio_button'}],
        },
        {
            'name': 'adc_button_0',
            'type': 'button',
            'sub_type': 'adc_single',
            'config': {
                'button_index': 0,
                'min_voltage': 0,
                'max_voltage': 500,
            },
            'peripherals': [{'adc_name': 'adc_oneshot'}],
        },
        {
            'name': 'adc_button_group',
            'type': 'button',
            'sub_type': 'adc_multi',
            'config': {
                'button_num': 2,
                'voltage_range': [380, 820],
                'max_voltage': 3000,
            },
            'peripherals': [{'adc_name': 'adc_oneshot'}],
        },
        {
            'name': 'lcd_brightness',
            'type': 'ledc_ctrl',
            'version': '1.0.0',
            'config': {'default_percent': 50},
            'peripherals': [{'ledc_name': 'ledc_backlight'}],
        },
        {
            'name': 'audio_power',
            'type': 'power_ctrl',
            'sub_type': 'gpio',
            'peripherals': [{'gpio_name': 'gpio_pa_control', 'active_level': 1}],
        },
        {
            'name': 'fs_spiffs',
            'type': 'fs_spiffs',
            'version': '1.0.0',
            'config': {
                'base_path': '/spiffs',
                'partition_label': 'storage',
                'max_files': 5,
                'format_if_mount_failed': False,
            },
        },
        {
            'name': 'littlefs',
            'type': 'littlefs',
            'sub_type': 'flash',
            'version': 'default',
            'config': {
                'vfs_config': {
                    'base_path': '/littlefs',
                    'partition_label': 'littlefs',
                    'format_if_mount_failed': False,
                },
            },
        },
        {
            'name': 'coverage_custom',
            'type': 'custom',
            'version': '1.0.0',
            'config': {'placeholder': 1},
        },
    ]

    if _want_device(chip, chip_data, 'device.display_lcd_spi'):
        devices.append({
            'name': 'display_lcd_spi',
            'type': 'display_lcd',
            'chip': 'fake_lcd',
            'sub_type': 'spi',
            'version': '1.0.0',
            'config': {
                'x_max': 320,
                'y_max': 240,
                'frame_format': 'RGB565_BE',
                'io_spi_config': {
                    'cs_gpio_num': -1,
                    'dc_gpio_num': -1,
                    'pclk_hz': 40000000,
                    'trans_queue_depth': 1,
                },
                'lcd_panel_config': {
                    'reset_gpio_num': -1,
                    'bits_per_pixel': 16,
                },
            },
            'peripherals': [{'spi_name': 'spi_master'}],
        })

    if _want_device(chip, chip_data, 'device.display_lcd_dsi'):
        devices.append({
            'name': 'display_lcd_dsi',
            'type': 'display_lcd',
            'chip': 'fake_lcd',
            'sub_type': 'dsi',
            'version': '1.0.0',
            'config': {
                'reset_gpio_num': -1,
                'bits_per_pixel': 16,
                'frame_format': 'RGB565_LE',
                'dbi_config': {
                    'virtual_channel': 0,
                    'lcd_cmd_bits': 8,
                    'lcd_param_bits': 8,
                },
                'dpi_config': {
                    'virtual_channel': 0,
                    'dpi_clk_src': 'MIPI_DSI_DPI_CLK_SRC_DEFAULT',
                    'dpi_clock_freq_mhz': 48,
                    'pixel_format': 'LCD_COLOR_PIXEL_FORMAT_RGB565',
                    'in_color_format': 'LCD_COLOR_FMT_RGB565',
                    'out_color_format': 'LCD_COLOR_FMT_RGB565',
                    'num_fbs': 1,
                    'video_timing': {
                        'h_size': 800,
                        'v_size': 480,
                        'hsync_back_porch': 120,
                        'hsync_pulse_width': 10,
                        'hsync_front_porch': 120,
                        'vsync_back_porch': 20,
                        'vsync_pulse_width': 1,
                        'vsync_front_porch': 20,
                    },
                },
            },
            'peripherals': [
                {'ldo_name': 'ldo_mipi'},
                {'dsi_name': 'dsi_display'},
            ],
        })

    if _cap(chip_data, 'device.lcd_touch_i2c'):
        devices.append({
            'name': 'lcd_touch',
            'chip': 'fake_touch',
            'type': 'lcd_touch',
            'sub_type': 'i2c',
            'version': '1.0.0',
            'config': {
                'io_i2c_config': {
                    'control_phase_bytes': 1,
                    'dc_bit_offset': 0,
                    'lcd_cmd_bits': 8,
                    'lcd_param_bits': 0,
                    'scl_speed_hz': 100000,
                    'flags': {'disable_control_phase': True},
                },
                'touch_config': {
                    'x_max': 320,
                    'y_max': 240,
                    'rst_gpio_num': -1,
                    'int_gpio_num': -1,
                },
            },
            'peripherals': [{'i2c_name': 'i2c_master', 'i2c_addr': [0xba]}],
        })

    if _cap(chip_data, 'device.gpio_expander'):
        devices.append({
            'name': 'gpio_expander',
            'chip': 'fake_expander',
            'type': 'gpio_expander',
            'config': {
                'max_pins': 8,
                'output_io_mask': [1, 2],
                'output_io_level_mask': [1, 0],
            },
            'peripherals': [{'i2c_name': 'i2c_master', 'i2c_addr': [0x70]}],
        })

    if _cap(chip_data, 'device.fs_fat_spi'):
        devices.append({
            'name': 'fs_fat_spi',
            'type': 'fs_fat',
            'version': 'default',
            'sub_type': 'spi',
            'config': {
                'mount_point': '/sdcard',
                'vfs_config': {
                    'format_if_mount_failed': False,
                    'max_files': 5,
                    'allocation_unit_size': 16384,
                },
                'sub_config': {'cs_gpio_num': -1},
            },
            'peripherals': [{'spi_name': 'spi_master'}],
        })

    if _cap(chip_data, 'device.fs_fat_sdmmc'):
        devices.append({
            'name': 'fs_fat_sdmmc',
            'type': 'fs_fat',
            'version': '1.0.0',
            'sub_type': 'sdmmc',
            'config': {
                'mount_point': '/sdmmc',
                'vfs_config': {
                    'format_if_mount_failed': False,
                    'max_files': 5,
                    'allocation_unit_size': 16384,
                },
                'sub_config': {
                    'slot': 'SDMMC_HOST_SLOT_1',
                    'frequency': 'SDMMC_FREQ_DEFAULT',
                    'bus_width': 1,
                    'pins': {
                        'clk': -1, 'cmd': -1, 'd0': -1, 'd1': -1, 'd2': -1, 'd3': -1,
                        'd4': -1, 'd5': -1, 'd6': -1, 'd7': -1, 'cd': -1, 'wp': -1,
                    },
                    'ldo_chan_id': -1,
                },
            },
        })

    if _cap(chip_data, 'device.littlefs_spi'):
        devices.append({
            'name': 'littlefs_spi',
            'type': 'littlefs',
            'sub_type': 'spi',
            'version': 'default',
            'config': {
                'vfs_config': {
                    'base_path': '/littlefs_spi',
                    'format_if_mount_failed': False,
                },
                'sub_config': {
                    'cs_gpio_num': -1,
                    'frequency': 'SDMMC_FREQ_DEFAULT',
                },
            },
            'peripherals': [{'spi_name': 'spi_master'}],
        })

    if _cap(chip_data, 'device.littlefs_sdmmc'):
        devices.append({
            'name': 'littlefs_sdmmc',
            'type': 'littlefs',
            'sub_type': 'sdmmc',
            'version': 'default',
            'config': {
                'vfs_config': {
                    'base_path': '/littlefs_sdmmc',
                    'format_if_mount_failed': False,
                },
                'sub_config': {
                    'slot': 'SDMMC_HOST_SLOT_1',
                    'frequency': 'SDMMC_FREQ_DEFAULT',
                    'bus_width': 1,
                    'pins': {
                        'clk': -1, 'cmd': -1, 'd0': -1, 'd1': -1, 'd2': -1, 'd3': -1,
                        'd4': -1, 'd5': -1, 'd6': -1, 'd7': -1, 'cd': -1, 'wp': -1,
                    },
                    'ldo_chan_id': -1,
                },
            },
        })

    if _cap(chip_data, 'device.camera_csi'):
        devices.append({
            'name': 'camera',
            'type': 'camera',
            'sub_type': 'csi',
            'config': {
                'csi_config': {
                    'reset_io': -1,
                    'pwdn_io': -1,
                    'dont_init_ldo': True,
                    'xclk_config': {
                        'xclk_pin': -1,
                        'xclk_freq_hz': 20000000,
                    },
                },
            },
            'peripherals': [
                {'i2c_name': 'i2c_master', 'frequency': 100000},
                {'ldo_name': 'ldo_mipi'},
            ],
        })

    if _cap(chip_data, 'device.led_strip_rmt'):
        devices.append({
            'name': 'led_strip',
            'type': 'led_strip',
            'chip': 'ws2812',
            'sub_type': 'rmt',
            'version': '1.0.0',
            'config': {
                'strip_gpio_num': pins.take(),
                'max_leds': 1,
                'led_model': 'LED_MODEL_WS2812',
                'color_component_format': 'LED_STRIP_COLOR_COMPONENT_FMT_GRB',
                'rmt': {'clk_src': 'RMT_CLK_SRC_DEFAULT'},
            },
        })

    # SPI2 is already used by spi_master; only add the SPI LED-strip backend
    # when the SoC has a spare GPSPI host (SPI3) and leftover GPIOs after knob.
    knob_reserve = 0 if chip == 'esp32c3' else 2
    if (
        _cap(chip_data, 'device.led_strip_spi')
        and _spi_instances(chip_data) >= 3
        and pins.remaining > knob_reserve
    ):
        devices.append({
            'name': 'led_strip_spi',
            'type': 'led_strip',
            'chip': 'ws2812',
            'sub_type': 'spi',
            'version': '1.0.0',
            'config': {
                'strip_gpio_num': pins.take(),
                'max_leds': 1,
                'led_model': 'LED_MODEL_WS2812',
                'color_component_format': 'LED_STRIP_COLOR_COMPONENT_FMT_GRB',
                'spi': {
                    'clk_src': 'SPI_CLK_SRC_DEFAULT',
                    'spi_bus': 'SPI3_HOST',
                    'with_dma': True,
                },
            },
        })

    if chip == 'esp32c3':
        devices.append({
            'name': 'audio_dac',
            'chip': 'internal',
            'type': 'audio_codec',
            'config': {'dac_enabled': True},
            'peripherals': [
                {'i2s_name': 'i2s_audio_out'},
                {'pa_name': 'gpio_pa_control', 'gain': 6, 'active_level': 1},
            ],
        })
        devices.append({
            'name': 'audio_adc',
            'chip': 'internal',
            'type': 'audio_codec',
            'config': {'adc_enabled': True},
            'peripherals': [{'adc_name': 'adc_continuous'}],
        })
    else:
        devices.append({
            'name': 'audio_dac',
            'chip': 'es8311',
            'type': 'audio_codec',
            'config': {
                'adc_enabled': False,
                'dac_enabled': True,
                'sys_cfg': {'no_mclk': True},
            },
            'peripherals': [
                {'pa_name': 'gpio_pa_control', 'gain': 6, 'active_level': 1},
                {'i2s_name': 'i2s_audio_out'},
                {'i2c_name': 'i2c_master', 'address': 0x30, 'frequency': 100000},
            ],
        })

    if chip != 'esp32c3':
        devices.append({
            'name': 'knob',
            'type': 'knob',
            'sub_type': 'gpio',
            'config': {
                'gpio_encoder_a': pins.take(),
                'gpio_encoder_b': pins.take(),
                'default_direction': 0,
            },
        })
    return devices


def _dump(path: Path, payload: dict) -> None:
    path.write_text(
        yaml.safe_dump(payload, sort_keys=False, allow_unicode=False),
        encoding='utf-8',
    )


def generate_board(board: str, chip: str) -> None:
    chip_data = _load_chip(chip)
    pins = _gpio_pool(chip, chip_data)
    board_dir = HERE / board
    board_dir.mkdir(parents=True, exist_ok=True)
    (board_dir / 'board_info.yaml').write_text(
        yaml.safe_dump(
            {
                'board': board,
                'chip': chip,
                'version': '1.0.0',
                'description': f'CI compile-coverage board for {chip}',
                'manufacturer': 'ESPRESSIF',
            },
            sort_keys=False,
        ),
        encoding='utf-8',
    )
    _dump(board_dir / 'board_peripherals.yaml', {'peripherals': _build_peripherals(chip, chip_data, pins)})
    _dump(board_dir / 'board_devices.yaml', {'devices': _build_devices(chip, chip_data, pins)})
    (board_dir / 'sdkconfig.defaults.board').write_text(SDKCONFIG[chip] + COMMON_SDKCONFIG, encoding='utf-8')
    (board_dir / 'setup_device.c').write_text(SETUP_DEVICE_SRC.read_text(encoding='utf-8'), encoding='utf-8')


def main() -> None:
    for board, chip in BOARDS:
        generate_board(board, chip)
        print(f'wrote {board}')


if __name__ == '__main__':
    main()
