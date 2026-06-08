# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

"""
Tests for PCNT peripheral parser SoC capability validation.
"""

import sys

import pytest


class FakeSoc:
    def __init__(self, limits=None):
        self._limits = limits or {}

    def limit(self, key, default=None):
        return self._limits.get(key, default)


def _load_pcnt(bmgr_root):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_pcnt import periph_pcnt as mod

    return mod


def _channel(edge_gpio_num=4, level_gpio_num=5):
    return {
        'channel_config': {
            'edge_gpio_num': edge_gpio_num,
            'level_gpio_num': level_gpio_num,
        },
    }


def _pcnt_config(channels=None, watch_points=None):
    channels = channels if channels is not None else [_channel()]
    watch_points = watch_points if watch_points is not None else []
    return {
        'config': {
            'channel_count': len(channels),
            'channel_list': channels,
            'watch_point_count': len(watch_points),
            'watch_point_list': watch_points,
        },
    }


def test_pcnt_channel_count_accepts_more_channels_without_soc_limit(bmgr_root):
    mod = _load_pcnt(bmgr_root)

    result = mod.parse('pcnt_unit', _pcnt_config(channels=[_channel(4, 5), _channel(6, 7), _channel(8, 9)]))
    assert result['struct_init']['channel_count'] == 3


def test_pcnt_watch_points_use_local_limit_and_keep_fixed_points_offset(bmgr_root, monkeypatch):
    mod = _load_pcnt(bmgr_root)
    monkeypatch.setattr(
        mod,
        'current_soc',
        lambda: FakeSoc({'pcnt.watch_points_per_unit': 2}),
        raising=False,
    )

    allowed_by_offset = [-100, 0, 100, 200, 300]
    result = mod.parse('pcnt_unit', _pcnt_config(watch_points=allowed_by_offset))
    assert result['struct_init']['watch_point_count'] == 5

    with pytest.raises(ValueError, match='Invalid watch point configuration'):
        mod.parse('pcnt_unit', _pcnt_config(watch_points=allowed_by_offset + [400]))
