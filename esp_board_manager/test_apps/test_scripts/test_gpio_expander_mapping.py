# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

import os
import sys
import shutil
import subprocess

import pytest


def _parse(bmgr_root, mapping=None, max_pins=10):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_gpio_expander import dev_gpio_expander

    config = {'max_pins': max_pins}
    if mapping is not None:
        config['gpio_mapping'] = mapping
    return dev_gpio_expander.parse('gpio_expander', {
        'type': 'gpio_expander',
        'chip': 'esp32_a1',
        'config': config,
        'peripherals': [{'i2c_name': 'i2c_master', 'i2c_addr': 0x58}],
    })['struct_init']


def test_mapping_is_disabled_for_existing_boards(bmgr_root):
    result = _parse(bmgr_root)
    assert result['gpio_mapping_enabled'] is False
    assert result['gpio_start_io_num'] == -1


def test_mapping_default_start_uses_native_gpio_count(bmgr_root):
    result = _parse(bmgr_root, {'enabled': True})
    assert result['gpio_mapping_enabled'] is True
    assert result['gpio_start_io_num'] == -1


def test_mapping_preserves_explicit_start_above_mask_width(bmgr_root):
    result = _parse(bmgr_root, {'enabled': True, 'start_io_num': 64})
    assert result['gpio_mapping_enabled'] is True
    assert result['gpio_start_io_num'] == 64
    assert result['i2c_addr'] == [0x58]


@pytest.mark.parametrize('mapping', [
    True, [], 'enabled', {'enabled': 'true'}, {'enabled': 1},
    {'enabled': True, 'start_io_num': True},
    {'enabled': True, 'start_io_num': -1},
    {'enabled': True, 'start_io_num': '64'},
    {'enabled': True, 'start_io_num': 2147483640},
    {'enabled': True, 'start': 64},
])
def test_mapping_rejects_invalid_configuration(bmgr_root, mapping):
    with pytest.raises(ValueError, match='gpio_mapping'):
        _parse(bmgr_root, mapping)


@pytest.mark.parametrize('max_pins', [0, 33])
def test_expander_pin_count_fits_driver_mask(bmgr_root, max_pins):
    with pytest.raises(ValueError, match='max_pins'):
        _parse(bmgr_root, max_pins=max_pins)


@pytest.mark.parametrize('wrapper_enabled', [0, 1])
def test_mapping_lifecycle_with_bmgr_wrapper(bmgr_root, tmp_path, wrapper_enabled):
    compiler = shutil.which('cc')
    if compiler is None:
        pytest.skip('Host C compiler is not installed')
    common = r'''
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
typedef int esp_err_t;
typedef void *i2c_master_bus_handle_t;
typedef int esp_io_expander_output_mode_t;
typedef int esp_io_expander_pullupdown_t;
typedef struct { struct { uint32_t io_count; } config; } esp_io_expander_t;
typedef esp_io_expander_t *esp_io_expander_handle_t;
#define ESP_OK 0
#define ESP_ERR_NOT_FOUND 1
#define GPIO_NUM_MAX 62
#define IO_EXPANDER_OUTPUT 1
#define IO_EXPANDER_INPUT 0
#define IO_EXPANDER_PULL_UP 1
#define IO_EXPANDER_PULL_DOWN 2
#define ESP_LOGE(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGD(...) ((void)0)
#define ESP_GOTO_ON_ERROR(expr, label, tag, ...) do { ret = (expr); if (ret != ESP_OK) goto label; } while (0)
esp_err_t i2c_master_probe(i2c_master_bus_handle_t, uint16_t, int);
esp_err_t esp_io_expander_set_dir(esp_io_expander_handle_t, uint32_t, int);
esp_err_t esp_io_expander_set_level(esp_io_expander_handle_t, uint32_t, uint8_t);
esp_err_t esp_io_expander_set_output_mode(esp_io_expander_handle_t, uint32_t, int);
esp_err_t esp_io_expander_set_pullupdown(esp_io_expander_handle_t, uint32_t, int);
esp_err_t esp_io_expander_del(esp_io_expander_handle_t);
esp_err_t esp_io_expander_gpio_wrapper_append_handler(esp_io_expander_handle_t, uint32_t);
esp_err_t esp_io_expander_gpio_wrapper_remove_handler(esp_io_expander_handle_t);
static inline const char *esp_err_to_name(esp_err_t err) { (void)err; return "stub"; }
'''
    (tmp_path / 'common.h').write_text(common, encoding='utf-8')
    for header in ('esp_log.h', 'esp_check.h', 'esp_err.h',
                   'driver/i2c_master.h', 'driver/gpio.h', 'sdkconfig.h',
                   'esp_io_expander.h', 'esp_io_expander_gpio_wrapper.h'):
        path = tmp_path / header
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('#include "common.h"\n', encoding='utf-8')
    device_dir = bmgr_root / 'devices/dev_gpio_expander'
    binary = tmp_path / 'mapping_test'
    compiled = subprocess.run([
        compiler, '-std=c11', '-Wall', '-Werror', '-Wno-unused-variable',
        '-fsanitize=undefined', '-include', str(tmp_path / 'common.h'),
        f'-DCONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER={wrapper_enabled}',
        '-I', str(tmp_path), '-I', str(device_dir),
        '-I', str(bmgr_root / 'include'), '-I', str(bmgr_root / 'private_inc'),
        str(device_dir / 'dev_gpio_expander.c'),
        str(bmgr_root / 'test_apps/host_tests/gpio_expander/host_main.c'),
        '-o', str(binary),
    ], capture_output=True, text=True)
    assert compiled.returncode == 0, compiled.stderr
    executed = subprocess.run([str(binary)], capture_output=True, text=True)
    assert executed.returncode == 0, executed.stderr
    failures = ['failed-direction-cleanup', 'failed-factory-cleanup']
    if wrapper_enabled:
        failures.append('failed-mapping-cleanup')
    for failure in failures:
        executed = subprocess.run([str(binary), failure], capture_output=True, text=True, timeout=10)
        assert executed.returncode == 0, failure + ': ' + executed.stderr


@pytest.mark.parametrize('start', [None, 62, 65])
def test_mapped_gpio_peripheral_metadata_accepts_pa_pin(bmgr_root, tmp_path, start):
    _write_mapped_gpio_metadata(bmgr_root, tmp_path, start=start, pin=start or 62)


@pytest.mark.parametrize('kwargs', [
    {'enabled': False}, {'pin': 61, 'start': 60},
    {'pin': 72}, {'peripheral_type': 'i2c'},
])
def test_mapped_gpio_metadata_does_not_relax_native_validation(bmgr_root, tmp_path, kwargs):
    with pytest.raises(ValueError):
        _write_mapped_gpio_metadata(bmgr_root, tmp_path, **kwargs)


def _write_mapped_gpio_metadata(bmgr_root, tmp_path, start=62, enabled=True,
                               pin=62, peripheral_type='gpio'):
    sys.path.insert(0, str(bmgr_root))
    from devices.dev_gpio_expander import dev_gpio_expander
    from peripherals.periph_gpio import periph_gpio
    from generators.board_metadata_generator import BoardMetadataGenerator
    from generators.utils.soc_capability_query import configure_soc_capabilities, clear_soc_capabilities

    configure_soc_capabilities(bmgr_root / 'private_inc/soc_capability_catalog', '6.2.0', 'esp32s31')
    try:
        mapping = {'enabled': enabled}
        if start is not None:
            mapping['start_io_num'] = start
        raw = {'type': 'gpio_expander', 'config': {'max_pins': 10, 'gpio_mapping': mapping}}
        return BoardMetadataGenerator().write_metadata_file(
            str(tmp_path / 'metadata.yaml'), 'mapped_pa', 'esp32s31',
            device_artifacts=[{'name': 'expander', 'type': 'gpio_expander', 'raw': raw,
                               'result': dev_gpio_expander.parse('expander', raw),
                               'parse_func': dev_gpio_expander.parse}],
            peripheral_artifacts=[{'name': 'pa', 'type': peripheral_type, 'raw': {},
                                   'result': periph_gpio.parse('pa', {'config': {'pin': pin}}),
                                   'parse_func': periph_gpio.parse}],
        )
    finally:
        clear_soc_capabilities()


@pytest.mark.parametrize('pin', [64, 65, 32767])
def test_gpio_peripheral_warns_without_generating_invalid_bitmask(bmgr_root, caplog, pin):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_gpio import periph_gpio
    result = periph_gpio.parse('pa', {'config': {'pin': pin}})
    assert result['struct_init']['gpio_num'] == pin
    assert result['struct_init']['gpio_config']['pin_bit_mask'] == 0
    assert str(pin) in caplog.text
    assert '64-bit' in caplog.text
    assert periph_gpio.extract_metadata('pa', {}, result, {}) == {'io': {'pin': pin}}


@pytest.mark.parametrize('pin', [0, 2, 62, 63])
def test_native_gpio_peripheral_keeps_bitmask(bmgr_root, pin):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_gpio import periph_gpio
    result = periph_gpio.parse('native', {'config': {'pin': pin}})
    assert result['struct_init']['gpio_config']['pin_bit_mask'] == f'BIT64({pin})'
    assert periph_gpio.extract_metadata('native', {}, result, {}) == {'io': {'pin': pin}}


@pytest.mark.parametrize('pin', [-1, True, '65', 65.0, None, 2147483648])
def test_gpio_peripheral_rejects_invalid_pin(bmgr_root, pin):
    sys.path.insert(0, str(bmgr_root))
    from peripherals.periph_gpio import periph_gpio
    with pytest.raises(ValueError, match='Invalid GPIO pin number'):
        periph_gpio.parse('pa', {'config': {'pin': pin}})


@pytest.mark.parametrize('native_count', [49, 62])
def test_gpio_peripheral_runtime_skips_virtual_configuration(bmgr_root, tmp_path, native_count):
    compiler = shutil.which('cc')
    if compiler is None:
        pytest.skip('Host C compiler is not installed')
    sys.path.insert(0, str(bmgr_root))
    from generators.config_generator import ConfigGenerator
    from peripherals.periph_gpio import periph_gpio

    result = periph_gpio.parse('pa', {'config': {'pin': 65, 'mode': 'GPIO_MODE_OUTPUT'}})
    initializer = ConfigGenerator(bmgr_root).dict_to_c_initializer(result['struct_init'])
    (tmp_path / 'generated_gpio.h').write_text(
        'static const periph_gpio_config_t generated_gpio = {\n' + '\n'.join(initializer) + '\n};\n',
        encoding='utf-8',
    )
    (tmp_path / 'esp_err.h').write_text(
        '#pragma once\ntypedef int esp_err_t;\n#define ESP_OK 0\n', encoding='utf-8',
    )
    (tmp_path / 'driver').mkdir()
    (tmp_path / 'driver/gpio.h').write_text(r'''
#pragma once
#include <stdint.h>
typedef int gpio_num_t;
typedef int esp_err_t;
#define ESP_OK 0
#define BIT64(n) (UINT64_C(1) << (n))
#define GPIO_MODE_INPUT 1
#define GPIO_MODE_OUTPUT 2
#define GPIO_MODE_INPUT_OUTPUT 3
#define GPIO_MODE_OUTPUT_OD 6
#define GPIO_MODE_INPUT_OUTPUT_OD 7
#define GPIO_PULLUP_DISABLE 0
#define GPIO_PULLDOWN_DISABLE 0
#define GPIO_INTR_DISABLE 0
typedef struct {
    uint64_t pin_bit_mask;
    int mode, pull_up_en, pull_down_en, intr_type;
} gpio_config_t;
esp_err_t gpio_config(const gpio_config_t *);
esp_err_t gpio_set_level(gpio_num_t, uint32_t);
esp_err_t gpio_reset_pin(gpio_num_t);
''', encoding='utf-8')
    (tmp_path / 'esp_log.h').write_text(r'''
#pragma once
#include <stdio.h>
#define ESP_LOGE(tag, ...) ((void)(tag))
#define ESP_LOGW(tag, ...) ((void)(tag))
#define ESP_LOGI(tag, ...) ((void)(tag))
''', encoding='utf-8')
    peripheral_dir = bmgr_root / 'peripherals/periph_gpio'
    binary = tmp_path / 'gpio_test'
    compiled = subprocess.run([
        compiler, '-std=c11', '-Wall', '-Werror', '-fsanitize=undefined',
        f'-DGPIO_NUM_MAX={native_count}', '-I', str(tmp_path), '-I', str(peripheral_dir),
        '-I', str(bmgr_root / 'include'), '-I', str(bmgr_root / 'private_inc'),
        str(peripheral_dir / 'periph_gpio.c'),
        str(bmgr_root / 'test_apps/host_tests/gpio/host_main.c'), '-o', str(binary),
    ], capture_output=True, text=True)
    assert compiled.returncode == 0, compiled.stderr
    executed = subprocess.run([str(binary)], capture_output=True, text=True,
                              env={**os.environ, 'UBSAN_OPTIONS': 'halt_on_error=1'})
    assert executed.returncode == 0, executed.stderr
