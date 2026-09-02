# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path


def test_lcd_test_does_not_cache_deinitialized_ledc_handle():
    source = (
        Path(__file__).parents[1] / 'main' / 'test_dev_lcd_init.c'
    ).read_text(encoding='utf-8')

    backlight_start = source.index('static esp_err_t lcd_backlight_set')
    backlight_end = source.index('static bool lcd_frame_format_is_rgb565', backlight_start)
    backlight = source[backlight_start:backlight_end]

    assert 'if (ledc_handle == NULL)' not in backlight
    assert backlight.count('esp_board_manager_get_device_handle') == 1


def test_ledc_device_deinit_logs_before_releasing_handle():
    source = (
        Path(__file__).parents[2] / 'devices' / 'dev_ledc_ctrl' / 'dev_ledc_ctrl.c'
    ).read_text(encoding='utf-8')

    deinit_start = source.index('int dev_ledc_ctrl_deinit')
    deinit = source[deinit_start:]
    release = deinit.index('esp_board_periph_unref_handle')
    log = deinit.index('LEDC control device channel', release)
    assert 'ledc_channel_t channel = ledc_handle->channel;' in deinit[:release]
    assert 'channel);' in deinit[log:]
