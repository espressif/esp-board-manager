"""
Regression tests for generated device dependency lifecycle behavior.
"""


def test_public_deinit_rejects_active_dependents(bmgr_root):
    """Public deinit must not consume dependency refs while dependents are alive."""
    source = (bmgr_root / 'src' / 'esp_board_device.c').read_text(encoding='utf-8')

    assert 'check_active_device_dependents(name)' in source
    assert 'if (!ignore_dep_check && handle->ref_count <= 1)' not in source


def test_esp32_s3_box_2_has_shared_gpio_expander_dependents(resolve_board_dir):
    """Keep the real shared-dependency scenario visible to this regression test."""
    board_devices = (resolve_board_dir('esp32_s3_box_2') / 'board_devices.yaml').read_text(encoding='utf-8')

    assert 'name: button_l' in board_devices
    assert 'name: button_m' in board_devices
    assert board_devices.count('depends_on: gpio_expander') >= 2


def test_c_files_include_standard_headers_for_used_libc_calls(bmgr_root):
    # Board setup_device.c files moved out with the board components and are
    # covered by their own repositories; only the in-repo device wrappers
    # remain this repository's responsibility.
    files_and_headers = {
        bmgr_root / 'devices' / 'dev_display_lcd' / 'dev_display_lcd_sub_i80.c': ('#include <stdlib.h>',),
        bmgr_root / 'devices' / 'dev_button' / 'dev_button_sub_custom.c': ('#include <stdlib.h>',),
    }

    for abs_path, headers in files_and_headers.items():
        content = abs_path.read_text(encoding='utf-8')
        for header in headers:
            assert header in content
