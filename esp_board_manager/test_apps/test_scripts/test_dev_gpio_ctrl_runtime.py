# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

"""Regression checks for the gpio-control runtime initialization contract."""

import re


def test_gpio_ctrl_init_applies_active_level(bmgr_root):
    source = (bmgr_root / 'devices' / 'dev_gpio_ctrl' / 'dev_gpio_ctrl.c').read_text(
        encoding='utf-8'
    )
    match = re.search(
        r'gpio_set_level\(gpio_handle->gpio_num,\s*config->([a-z_]+)\)',
        source,
    )

    assert match is not None
    assert match.group(1) == 'active_level'
