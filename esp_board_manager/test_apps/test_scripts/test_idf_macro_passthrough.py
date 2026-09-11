# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

import sys

import pytest


def _add_bmgr_root(bmgr_root):
    sys.path.insert(0, str(bmgr_root))


def test_gpio_known_mode_is_silent(bmgr_root, capsys):
    _add_bmgr_root(bmgr_root)
    from peripherals.periph_gpio import periph_gpio as mod

    result = mod.parse('gpio_out', {'config': {'pin': 4, 'mode': 'GPIO_MODE_OUTPUT'}})
    assert result['struct_init']['gpio_config']['mode'] == 'GPIO_MODE_OUTPUT'
    assert 'WARNING' not in capsys.readouterr().out


def test_gpio_unknown_prefixed_mode_passes_with_hint(bmgr_root, capsys):
    _add_bmgr_root(bmgr_root)
    from peripherals.periph_gpio import periph_gpio as mod

    result = mod.parse('gpio_out', {'config': {'pin': 4, 'mode': 'GPIO_MODE_FUTURE'}})
    assert result['struct_init']['gpio_config']['mode'] == 'GPIO_MODE_FUTURE'
    captured = capsys.readouterr().out
    assert 'WARNING' in captured
    assert 'passing it through' in captured


def test_gpio_unprefixed_mode_is_rejected(bmgr_root):
    _add_bmgr_root(bmgr_root)
    from peripherals.periph_gpio import periph_gpio as mod

    with pytest.raises(ValueError, match='mode'):
        mod.parse('gpio_out', {'config': {'pin': 4, 'mode': 'output'}})


def test_ledc_timer_duty_patterns_are_distinct(bmgr_root, capsys):
    _add_bmgr_root(bmgr_root)
    from peripherals.periph_ledc import periph_ledc as mod

    result = mod.parse(
        'ledc_backlight',
        {'config': {'gpio_num': 5, 'timer_sel': 'LEDC_TIMER_0', 'duty_resolution': 'LEDC_TIMER_13_BIT'}},
    )
    assert result['struct_init']['timer_sel'] == 'LEDC_TIMER_0'
    assert result['struct_init']['duty_resolution'] == 'LEDC_TIMER_13_BIT'
    assert 'WARNING' not in capsys.readouterr().out

    with pytest.raises(ValueError, match='timer_sel'):
        mod.parse('ledc_backlight', {'config': {'gpio_num': 5, 'timer_sel': 'LEDC_TIMER_13_BIT'}})

    result = mod.parse('ledc_backlight', {'config': {'gpio_num': 5, 'timer_sel': 'LEDC_TIMER_4'}})
    assert result['struct_init']['timer_sel'] == 'LEDC_TIMER_4'
    assert 'passing it through' in capsys.readouterr().out


def test_spi_host_prefix_and_isr_affinity(bmgr_root, capsys):
    _add_bmgr_root(bmgr_root)
    from peripherals.periph_spi import periph_spi as mod

    result = mod.parse(
        'spi_master',
        {'config': {'spi_bus_config': {'spi_port': 'SPI2_HOST'}}},
    )
    assert result['struct_init']['spi_port'] == 'SPI2_HOST'
    assert 'WARNING' not in capsys.readouterr().out

    result = mod.parse(
        'spi_master',
        {'config': {'spi_bus_config': {'spi_port': 'SPI4_HOST'}}},
    )
    assert result['struct_init']['spi_port'] == 'SPI4_HOST'
    assert 'passing it through' in capsys.readouterr().out

    with pytest.raises(ValueError, match='spi_port'):
        mod.parse('spi_master', {'config': {'spi_bus_config': {'spi_port': 'SPI_HOST'}}})


def test_anacmpr_accepts_clk_src_beyond_default(bmgr_root, capsys):
    _add_bmgr_root(bmgr_root)
    from peripherals.periph_anacmpr import periph_anacmpr as mod

    result = mod.parse('anacmpr_unit_0', {'config': {'clk_src': 'ANA_CMPR_CLK_SRC_PLL_F80M'}})
    assert result['struct_init']['clk_src'] == 'ANA_CMPR_CLK_SRC_PLL_F80M'
    assert 'WARNING' not in capsys.readouterr().out


def test_dsi_phy_clk_src_zero_still_allowed(bmgr_root):
    _add_bmgr_root(bmgr_root)
    from peripherals.periph_dsi import periph_dsi as mod

    result = mod.parse(
        'dsi_panel',
        {'config': {'bus_id': 0, 'data_lanes': 2, 'phy_clk_src': 0, 'lane_bit_rate_mbps': 1000}},
    )
    assert result['struct_init']['phy_clk_src'] == 0


def test_sdm_rejects_unprefixed_clk_src(bmgr_root):
    _add_bmgr_root(bmgr_root)
    from peripherals.periph_sdm import periph_sdm as mod

    with pytest.raises(ValueError, match='clk_src'):
        mod.parse('sdm', {'config': {'gpio_num': 6, 'clk_src': 'apb'}})


def test_rmt_idf5_empty_clk_src_stays_empty(bmgr_root, capsys):
    _add_bmgr_root(bmgr_root)
    from peripherals.periph_rmt.idf5 import periph_rmt as mod

    result = mod.parse('rmt_tx', {'role': 'tx', 'config': {'gpio_num': 18}})
    assert result['struct_init']['channel_config']['tx']['clk_src'] == ''
    assert 'WARNING' not in capsys.readouterr().out

    with pytest.raises(ValueError, match='clk_src'):
        mod.parse('rmt_tx', {'role': 'tx', 'config': {'gpio_num': 18, 'clk_src': 'xtal'}})


def test_mcpwm_clk_src_and_count_mode(bmgr_root, capsys):
    _add_bmgr_root(bmgr_root)
    from peripherals.periph_mcpwm.idf6 import periph_mcpwm as mod

    result = mod.parse(
        'mcpwm_group_0',
        {
            'config': {
                'timer_config': {'clk_src': 'MCPWM_TIMER_CLK_SRC_XTAL'},
                'generator_config': {'gpio_num': 7},
            }
        },
    )
    assert result['struct_init']['timer_config']['clk_src'] == 'MCPWM_TIMER_CLK_SRC_XTAL'
    assert result['struct_init']['timer_config']['count_mode'] == 'MCPWM_TIMER_COUNT_MODE_UP'
    assert 'WARNING' not in capsys.readouterr().out


def test_led_strip_clk_src_passthrough(bmgr_root, capsys):
    _add_bmgr_root(bmgr_root)
    from devices.dev_led_strip import dev_led_strip as mod

    result = mod.parse(
        'led_strip',
        {'sub_type': 'rmt', 'config': {'strip_gpio_num': 37, 'max_leds': 1, 'rmt': {'clk_src': 'RMT_CLK_SRC_XTAL'}}},
    )
    assert result['struct_init']['sub_cfg']['rmt']['rmt_config']['clk_src'] == 'RMT_CLK_SRC_XTAL'
    assert 'WARNING' not in capsys.readouterr().out

    with pytest.raises(ValueError, match='clk_src'):
        mod.parse(
            'led_strip',
            {'sub_type': 'rmt', 'config': {'strip_gpio_num': 37, 'max_leds': 1, 'rmt': {'clk_src': 'xtal'}}},
        )


def test_lcd_i80_clk_src_zero_is_allowed(bmgr_root):
    _add_bmgr_root(bmgr_root)
    from devices.dev_display_lcd import dev_display_lcd as mod

    result = mod.parse_i80_sub_config({'config': {'bus_config': {'clk_src': 0}}})
    assert result['bus_config']['clk_src'] == 0
