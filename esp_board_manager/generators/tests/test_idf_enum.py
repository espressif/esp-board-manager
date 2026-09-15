# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from generators.utils.idf_enum import resolve_idf_macro
from peripherals.periph_anacmpr.periph_anacmpr import KNOWN_ANA_CMPR_CLK_SRC, KNOWN_ANA_CMPR_CROSS
from peripherals.periph_i2s.periph_i2s import KNOWN_I2S_CLK_SRC
from peripherals.periph_ledc.periph_ledc import KNOWN_LEDC_DUTY_RESOLUTION, KNOWN_LEDC_TIMER
from peripherals.periph_mcpwm.idf5.periph_mcpwm import KNOWN_MCPWM_TIMER_COUNT_MODE
from peripherals.periph_spi.periph_spi import KNOWN_SPI_HOST

_HELPER_KNOWN_CLK = ('I2S_CLK_SRC_DEFAULT', 'I2S_CLK_SRC_APLL')


def test_known_macro_passes_silently(capsys):
    value = resolve_idf_macro(
        'I2S_CLK_SRC_DEFAULT',
        default='I2S_CLK_SRC_DEFAULT',
        prefixes=('I2S_CLK_SRC_',),
        known=_HELPER_KNOWN_CLK,
        field_name='clk_src',
    )
    assert value == 'I2S_CLK_SRC_DEFAULT'
    assert 'WARNING' not in capsys.readouterr().out


def test_unknown_prefixed_macro_passes_with_hint(capsys):
    value = resolve_idf_macro(
        'I2S_CLK_SRC_FUTURE_CHIP',
        default='I2S_CLK_SRC_DEFAULT',
        prefixes=('I2S_CLK_SRC_',),
        known=_HELPER_KNOWN_CLK,
        field_name='clk_src',
    )
    assert value == 'I2S_CLK_SRC_FUTURE_CHIP'
    captured = capsys.readouterr().out
    assert 'WARNING' in captured
    assert 'I2S_CLK_SRC_FUTURE_CHIP' in captured
    assert 'passing it through' in captured


def test_unprefixed_macro_is_rejected():
    with pytest.raises(ValueError, match="Invalid clk_src value 'apll'"):
        resolve_idf_macro(
            'apll',
            default='I2S_CLK_SRC_DEFAULT',
            prefixes=('I2S_CLK_SRC_',),
            known=_HELPER_KNOWN_CLK,
            field_name='clk_src',
        )


def test_empty_value_uses_default():
    value = resolve_idf_macro(
        '',
        default='I2S_CLK_SRC_DEFAULT',
        prefixes=('I2S_CLK_SRC_',),
        known=_HELPER_KNOWN_CLK,
        field_name='clk_src',
    )
    assert value == 'I2S_CLK_SRC_DEFAULT'


def test_extra_allowed_integer_zero():
    value = resolve_idf_macro(
        0,
        default='MIPI_DSI_PHY_CLK_SRC_DEFAULT',
        prefixes=('MIPI_DSI_PHY_CLK_SRC_',),
        known=('MIPI_DSI_PHY_CLK_SRC_DEFAULT',),
        field_name='phy_clk_src',
        extra_allowed=(0,),
    )
    assert value == 0


def test_pattern_rejects_wrong_shape():
    with pytest.raises(ValueError, match='timer_sel'):
        resolve_idf_macro(
            'LEDC_TIMER_13_BIT',
            default='LEDC_TIMER_0',
            prefixes=('LEDC_TIMER_',),
            known=('LEDC_TIMER_0',),
            field_name='timer_sel',
            pattern=r'^LEDC_TIMER_[0-9]+$',
        )


def test_i2s_known_list_includes_apll_and_xtal():
    assert 'I2S_CLK_SRC_APLL' in KNOWN_I2S_CLK_SRC
    assert 'I2S_CLK_SRC_XTAL' in KNOWN_I2S_CLK_SRC
    assert 'I2S_CLK_SRC_PLL_160M' in KNOWN_I2S_CLK_SRC
    assert 'I2S_CLK_SRC_APB' not in KNOWN_I2S_CLK_SRC
    assert 'I2S_CLK_SRC_PLL_F160M' not in KNOWN_I2S_CLK_SRC


def test_parser_known_lists_cover_cross_version_clk_names():
    assert 'ANA_CMPR_CLK_SRC_PLL_F80M' in KNOWN_ANA_CMPR_CLK_SRC
    assert 'ANA_CMPR_CROSS_DISABLE' in KNOWN_ANA_CMPR_CROSS
    assert 'LEDC_TIMER_0' in KNOWN_LEDC_TIMER
    assert 'LEDC_TIMER_13_BIT' in KNOWN_LEDC_DUTY_RESOLUTION
    assert 'LEDC_TIMER_13_BIT' not in KNOWN_LEDC_TIMER
    assert 'SPI3_HOST' in KNOWN_SPI_HOST
    assert 'MCPWM_TIMER_COUNT_MODE_UP_DOWN' in KNOWN_MCPWM_TIMER_COUNT_MODE
