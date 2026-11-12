# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

"""Compile and exercise the GPIO control initializer against host GPIO substitutes."""

import os
import shutil
import subprocess

import pytest


@pytest.fixture(scope='module')
def gpio_ctrl_binary(bmgr_root, tmp_path_factory):
    compiler = shutil.which('cc')
    if compiler is None:
        pytest.skip('Host C compiler is not installed')
    workdir = tmp_path_factory.mktemp('gpio_ctrl_runtime')
    (workdir / 'driver').mkdir()
    (workdir / 'driver/gpio.h').write_text(
        '#pragma once\n#include <stdint.h>\n#include "esp_err.h"\n'
        'typedef int gpio_num_t;\n'
        'typedef struct { uint64_t pin_bit_mask; int mode; } gpio_config_t;\n'
        'esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);\n',
        encoding='utf-8',
    )
    (workdir / 'esp_log.h').write_text(
        '#pragma once\n#include "host_idf_stubs.h"\n'
        '#undef ESP_LOGI\n'
        '#define ESP_LOGI(tag, fmt, ...) '
        'fprintf(stderr, "I (%s) " fmt "\\n", tag, ##__VA_ARGS__)\n',
        encoding='utf-8',
    )
    stubs = bmgr_root / 'test_apps/host_tests/runtime_override/stubs'
    device_dir = bmgr_root / 'devices/dev_gpio_ctrl'
    binary = workdir / 'gpio_ctrl_test'
    compiled = subprocess.run([
        compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined',
        '-I', str(workdir), '-I', str(stubs), '-I', str(device_dir),
        '-I', str(bmgr_root / 'peripherals/periph_gpio'),
        '-I', str(bmgr_root / 'include'), '-I', str(bmgr_root / 'private_inc'),
        str(device_dir / 'dev_gpio_ctrl.c'),
        str(bmgr_root / 'test_apps/host_tests/gpio_ctrl/host_main.c'),
        '-o', str(binary),
    ], capture_output=True, text=True, timeout=30)
    assert compiled.returncode == 0, compiled.stderr
    return binary


@pytest.mark.parametrize('case', ['levels', 'validation', 'lifecycle', 'rollback'])
def test_gpio_ctrl_initialization(gpio_ctrl_binary, case):
    executed = subprocess.run(
        [str(gpio_ctrl_binary), case], capture_output=True, text=True, timeout=10,
        env={**os.environ, 'UBSAN_OPTIONS': 'halt_on_error=1'},
    )
    assert executed.returncode == 0, executed.stdout + executed.stderr
