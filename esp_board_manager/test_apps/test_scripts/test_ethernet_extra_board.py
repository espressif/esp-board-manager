# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

"""CLI generation coverage for the custom Ethernet board compiled by CI."""

import os
import shutil
import subprocess
import sys

import yaml


BOARD_NAME = 'test_ethernet_extra'


def test_ethernet_extra_board_generates_without_amend(bmgr_root, tmp_path):
    board = bmgr_root / 'test_apps' / 'components' / BOARD_NAME
    assert (board / 'board_info.yaml').is_file(), 'custom Ethernet compile fixture is missing'

    # The full CLI updates component Kconfig files; keep the checkout untouched.
    writable_bmgr = tmp_path / 'esp_board_manager'
    shutil.copytree(bmgr_root, writable_bmgr, ignore=shutil.ignore_patterns(
        'test_apps', '__pycache__', '.pytest_cache', 'build', 'managed_components', 'gen_bmgr_codes',
    ))
    project = tmp_path / 'project'
    project.mkdir()
    project_extra = project / 'ethernet_extra'
    project_extra.mkdir()
    (project_extra / 'ignored.py').write_text(
        "raise RuntimeError('project-root Ethernet extensions must not be loaded')\n", encoding='utf-8',
    )
    (project / 'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.16)\n', encoding='utf-8')
    copied_board = project / 'components' / BOARD_NAME
    shutil.copytree(board, copied_board)
    result = subprocess.run([
        sys.executable, str(writable_bmgr / 'gen_bmgr_config_codes.py'),
        '-b', BOARD_NAME, '--project-dir', str(project),
    ], cwd=project, capture_output=True, text=True, timeout=60,
        env={**os.environ, 'PYTHONDONTWRITEBYTECODE': '1'})
    assert result.returncode == 0, result.stdout + result.stderr

    generated = project / 'components' / 'gen_bmgr_codes'
    source = (generated / 'gen_board_device_config.c').read_text(encoding='utf-8')
    assert '#include "bmgr_test_eth_phy.h"' in source
    assert 'static const bmgr_test_eth_phy_config_t esp_bmgr_ethernet_vendor_config' in source
    assert '.settle_time_ms = 200' in source
    assert '.vendor_config = &esp_bmgr_ethernet_vendor_config' in source
    assert '.vendor_config_size = sizeof(esp_bmgr_ethernet_vendor_config)' in source
    cmake = (generated / 'CMakeLists.txt').read_text(encoding='utf-8')
    assert f'{BOARD_NAME}/ethernet_extra/test_phy.c' in cmake
    assert 'test_phy.py' not in cmake
    manifest = yaml.safe_load((generated / 'idf_component.yml').read_text(encoding='utf-8'))
    assert manifest['dependencies'] == {
        'bmgr_test_eth_phy': {'path': copied_board.as_posix() + '/bmgr_test_eth_phy'},
    }
    assert not (copied_board / 'board_amend.yaml').exists()


def test_ethernet_extra_board_is_in_generation_to_build_matrix(bmgr_root):
    ci = yaml.safe_load((bmgr_root.parent / '.gitlab/ci/build-test-apps.yml').read_text(encoding='utf-8'))
    job = ci['build_test_apps']
    groups = [group for group in job['parallel']['matrix'] if BOARD_NAME in group['BOARD']]
    assert len(groups) == 1, 'the fixture must be compiled by the normal board CI job'
    assert groups[0]['IDF_TARGET'] == ['esp32']
    assert groups[0]['IDF_IMAGE'] == ci['.idf_image_matrix']
    assert any('idf.py bmgr -b' in command for command in job['script'])
    assert any('idf-build-apps build' in command for command in job['script'])
