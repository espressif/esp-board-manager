"""
Regression tests for the ``test_amend_features`` fixture board and its
``-a/--amend`` flag (manifest-driven board augmentation).

The fixture board lives at ``test_apps/components/test_amend_features/`` so it
ships with the test app rather than polluting the global ``boards/`` listing.
Tests below feed bmgr's customer-board scanner (``-c``) the surrounding
``test_apps/components/`` directory so the fixture (and the sibling
``test_apps/components/test_amend_features_extra/`` used to verify ``../``
references in the manifest) become discoverable. See
``docs/zh_CN/create-board/amend.rst`` for the full design.
"""

from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path


# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------

def _prepare_project(project_dir: Path) -> None:
    if project_dir.exists():
        shutil.rmtree(project_dir)
    project_dir.mkdir(parents=True)
    (project_dir / 'CMakeLists.txt').write_text(
        'cmake_minimum_required(VERSION 3.16)\nproject(bmgr_amend_fixture)\n',
        encoding='utf-8',
    )


def _copy_bmgr_to_writable_tmp(bmgr_root: Path, tmp_path: Path) -> Path:
    writable_bmgr = tmp_path / 'esp_board_manager_copy'
    ignore = shutil.ignore_patterns(
        '.git',
        '__pycache__',
        '.pytest_cache',
        'build',
        'gen_codes',
        'managed_components',
    )
    shutil.copytree(bmgr_root, writable_bmgr, ignore=ignore)
    return writable_bmgr


def _run_fixture_board(bmgr_root: Path, tmp_path: Path, amend_rel: str | None,
                       *, expect_success: bool = True) -> tuple[Path, subprocess.CompletedProcess]:
    """Generate against ``test_amend_features`` and return ``(gen_dir, result)``.

    ``amend_rel`` is a path relative to the writable copy of the board manager
    root (e.g. ``test_apps/components/test_amend_features/amend_basic``). It
    is resolved to an absolute path before being passed to ``-a`` so the test
    does not depend on the subprocess cwd.

    ``-c <test_apps/components>`` is added unconditionally so bmgr discovers
    the fixture board under ``test_apps/components/test_amend_features/``.
    """
    writable_bmgr = _copy_bmgr_to_writable_tmp(bmgr_root, tmp_path)
    label = (amend_rel or 'base').replace('/', '_').replace('..', 'dotdot')
    project_dir = tmp_path / ('project_' + label)
    _prepare_project(project_dir)

    customer_path = str((writable_bmgr / 'test_apps' / 'components').resolve())
    args = ['-b', 'test_amend_features',
            '-c', customer_path,
            '--project-dir', str(project_dir)]
    if amend_rel:
        amend_abs = str((writable_bmgr / amend_rel).resolve())
        args[4:4] = ['-a', amend_abs]

    env = os.environ.copy()
    env['PYTHONDONTWRITEBYTECODE'] = '1'
    result = subprocess.run(
        ['python3', str(writable_bmgr / 'gen_bmgr_config_codes.py')] + args,
        cwd=project_dir,
        capture_output=True,
        text=True,
        env=env,
    )
    if expect_success:
        assert result.returncode == 0, result.stdout + '\n' + result.stderr
    return project_dir / 'components' / 'gen_bmgr_codes', result


# ---------------------------------------------------------------------------
# happy-path tests
# ---------------------------------------------------------------------------

def test_amend_gen_skip_drops_base_device(bmgr_root, tmp_path):
    """Setting ``gen_skip: true`` on a base device via an amend fragment must
    drop the device from the generated outputs. This pins the release-note
    guarantee "Configuring gen_skip ... in the amend file ... will skip the
    parsing and generation of those devices/peripherals".

    Base fixture contains both ``status_led_power`` (kept) and ``boot_button``
    (skipped by amend). The generated device config must mention the kept one
    and must NOT mention the skipped one.
    """
    gen_dir, result = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_gen_skip',
    )

    device_content = (gen_dir / 'gen_board_device_config.c').read_text(encoding='utf-8')
    assert 'status_led_power' in device_content, (
        f'status_led_power should still be generated, got:\n{device_content}'
    )
    assert 'boot_button' not in device_content, (
        f'boot_button must be skipped via gen_skip, but appears in:\n{device_content}'
    )

    # The generator's INFO log should mention that boot_button was skipped so
    # users have an observable trail for the gen_skip decision.
    combined = result.stdout + result.stderr
    assert 'boot_button' in combined and 'gen_skip' in combined.lower(), (
        f'Expected an INFO log mentioning boot_button and gen_skip; got:\n{combined}'
    )


def test_amend_basic_yaml_merge(bmgr_root, tmp_path):
    """Single-YAML amend tweaks an existing peripheral's pin/default_level."""
    gen_dir, _ = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_basic',
    )

    periph_content = (gen_dir / 'gen_board_periph_config.c').read_text(encoding='utf-8')
    assert '.pin_bit_mask = BIT64(4)' in periph_content, \
        f'expected gpio_status_led.pin=4 after amend, got:\n{periph_content}'


def test_amend_full_yaml_merge_and_append(bmgr_root, tmp_path):
    """amend_full performs base-field tweaks and adds new peripherals/devices."""
    gen_dir, _ = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_full',
    )

    periph_content = (gen_dir / 'gen_board_periph_config.c').read_text(encoding='utf-8')
    device_content = (gen_dir / 'gen_board_device_config.c').read_text(encoding='utf-8')

    # base_led_tweak.yaml moves gpio_status_led to pin 6
    assert '.pin_bit_mask = BIT64(6)' in periph_content
    # new_led.yaml adds gpio_full_new_led at pin 5 + a new power_ctrl device
    assert '.pin_bit_mask = BIT64(5)' in periph_content
    assert 'full_new_led_power' in device_content
    # pack/pack_extra.yaml adds gpio_pack_extra at pin 7 + pack_extra_power device
    assert '.pin_bit_mask = BIT64(7)' in periph_content
    assert 'pack_extra_power' in device_content
    # ../../test_amend_features_extra/common_extra.yaml adds gpio_common_extra at pin 8
    assert '.pin_bit_mask = BIT64(8)' in periph_content


def test_amend_only_explicit_files_are_applied(bmgr_root, tmp_path):
    """Manifest-only rule: any file not in ``apply:`` must be ignored, no
    matter its name or location (no special-case auto-discovery)."""
    gen_dir, _ = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_full',
    )

    periph_content = (gen_dir / 'gen_board_periph_config.c').read_text(encoding='utf-8')
    cmake_content = (gen_dir / 'CMakeLists.txt').read_text(encoding='utf-8')

    # pack_unused.yaml lives next to pack_extra.yaml inside pack/ but is NOT
    # listed in board_amend.yaml, so its gpio_unused_should_be_ignored must not
    # appear anywhere in the generated peripherals.
    assert 'gpio_unused_should_be_ignored' not in periph_content
    # Similarly pack_unused.c must not be compiled (no SRCS entry).
    assert 'pack_unused.c' not in cmake_content
    # But pack_setup.c (explicitly listed) must appear in SRCS.
    assert 'pack/pack_setup.c' in cmake_content


def test_amend_sdkconfig_priority(bmgr_root, tmp_path):
    """Priority follows apply: list order — later entries override earlier
    same-CONFIG_* lines via the BMGR_CONFIG_OVERRIDE marker.

    In ``amend_full`` the apply order ends with
    ``pack/sdkconfig.defaults.board`` then the amend-root
    ``sdkconfig.defaults.board``, so the root version wins.
    """
    gen_dir, _ = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_full',
    )

    defaults_content = (gen_dir / 'board_manager.defaults').read_text(encoding='utf-8')

    # Final value comes from the last-applied (amend-root) sdkconfig section.
    assert 'CONFIG_BMGR_TEST_AMEND_BASE_VALUE=99' in defaults_content
    # Both earlier values must remain commented for traceability.
    assert '# BMGR_CONFIG_OVERRIDE by Amend sdkconfig defaults (' in defaults_content
    assert ': CONFIG_BMGR_TEST_AMEND_BASE_VALUE=1' in defaults_content
    assert (': CONFIG_BMGR_TEST_AMEND_BASE_VALUE=42' in defaults_content
            or 'CONFIG_BMGR_TEST_AMEND_BASE_VALUE=42' in defaults_content)
    # Section headers must appear in apply: order (pack/... before sdkconfig.defaults.board).
    base_idx = defaults_content.index('Board sdkconfig defaults')
    pack_idx = defaults_content.index('apply[5] = pack/sdkconfig.defaults.board')
    top_idx = defaults_content.index('apply[9] = sdkconfig.defaults.board')
    assert base_idx < pack_idx < top_idx
    # Base "# CONFIG_..._UNSET is not set" was overridden by amend-root layer.
    assert 'CONFIG_BMGR_TEST_AMEND_BASE_UNSET=y' in defaults_content


def test_amend_kconfig_append_order(bmgr_root, tmp_path):
    """Kconfig.projbuild is plain text append: base -> pack -> amend top."""
    gen_dir, _ = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_full',
    )

    kconfig_content = (gen_dir / 'Kconfig.projbuild').read_text(encoding='utf-8')
    assert kconfig_content.index('BMGR_TEST_AMEND_BOARD_KCONFIG') \
        < kconfig_content.index('BMGR_TEST_AMEND_PACK_KCONFIG') \
        < kconfig_content.index('BMGR_TEST_AMEND_TOP_KCONFIG')


def test_amend_srcs_precise(bmgr_root, tmp_path):
    """target_sources contains exactly the manifest-listed C files; INCLUDE_DIRS are deduped.

    Critically, the generated SRC_DIRS scan must remain intact (covers '.' and the
    base board directory). Specifying SRCS inside idf_component_register would
    silently disable SRC_DIRS, so amend sources are added via target_sources()
    after the register call.
    """
    gen_dir, _ = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_full',
    )

    cmake_content = (gen_dir / 'CMakeLists.txt').read_text(encoding='utf-8')

    # SRC_DIRS must still scan '.' (for generated gen_board_*.c) and the base board dir.
    assert 'SRC_DIRS "."' in cmake_content
    assert 'test_amend_features"' in cmake_content
    # amend sources are appended via target_sources, NOT via SRCS inside idf_component_register.
    assert 'target_sources(${COMPONENT_LIB} PRIVATE' in cmake_content
    # Inside idf_component_register, there must be no bare ``SRCS`` keyword line.
    register_block = cmake_content.split('idf_component_register(', 1)[1].split(')', 1)[0]
    assert '\n    SRCS' not in register_block, \
        f'idf_component_register must NOT contain SRCS, got:\n{register_block}'
    # The three explicitly listed source files must appear with their apply-index comments.
    # apply: order in amend_full/board_amend.yaml:
    #   [0] new_led.yaml
    #   [1] base_led_tweak.yaml
    #   [2] strong_setup.c
    #   [3] pack/pack_extra.yaml
    #   [4] pack/pack_setup.c
    #   [5] pack/sdkconfig.defaults.board
    #   [6] pack/Kconfig.projbuild
    #   [7] ../../test_amend_features_extra/common_extra.yaml
    #   [8] ../../test_amend_features_extra/common_setup.c
    #   [9] sdkconfig.defaults.board
    #  [10] Kconfig.projbuild
    assert '# apply[2]: strong_setup.c' in cmake_content
    assert '# apply[4]: pack/pack_setup.c' in cmake_content
    assert '# apply[8]: ../../test_amend_features_extra/common_setup.c' in cmake_content
    # No SRCS entry for unused source.
    assert 'pack_unused.c' not in cmake_content
    # INCLUDE_DIRS contains amend_full, pack, and the extra dir.
    assert 'test_amend_features/amend_full"' in cmake_content
    assert 'amend_full/pack"' in cmake_content
    assert 'test_amend_features_extra"' in cmake_content


def test_amend_sub_type_swap_no_stale_warnings(bmgr_root, tmp_path, resolve_board_dir):
    """Switching a device's sub_type (here display_lcd rgb_3wire_spi -> rgb)
    must not produce 'Unknown config key' WARNINGs for fields that legitimately
    belong to the previous sub_type's schema. Such fields are demoted to debug
    logs so amend users see clean stdout on default INFO log level.
    """
    writable_bmgr = _copy_bmgr_to_writable_tmp(bmgr_root, tmp_path)
    project_dir = tmp_path / 'project_sub3_no_warn'
    _prepare_project(project_dir)
    board_dir = resolve_board_dir('esp32_s3_lcd_ev_board')
    boards_src = str(board_dir.parent)
    amend_abs = str((board_dir / 'sub_board_800_480_lcd').resolve())

    env = os.environ.copy()
    env['PYTHONDONTWRITEBYTECODE'] = '1'
    result = subprocess.run(
        ['python3', str(writable_bmgr / 'gen_bmgr_config_codes.py'),
         '-b', 'esp32_s3_lcd_ev_board',
         '-a', amend_abs,
         '-c', boards_src,
         '--project-dir', str(project_dir)],
        cwd=project_dir, capture_output=True, text=True, env=env,
    )
    assert result.returncode == 0, result.stdout + '\n' + result.stderr

    combined = result.stdout + result.stderr
    # The fields that used to noise up this amend's stdout under strict validation.
    for stale_key in (
        'io_3wire_spi_config',
        'lcd_panel_config',
    ):
        # Use a per-line check so we tolerate the strings appearing inside e.g.
        # a metadata snapshot that does not start with 'Unknown config key'.
        offending = [line for line in combined.splitlines()
                     if 'Unknown config key' in line and stale_key in line]
        assert not offending, (
            f"Expected no 'Unknown config key' warning for {stale_key!r}; got:\n"
            + '\n'.join(offending)
        )


def test_amend_external_relative_path_resolved(bmgr_root, tmp_path):
    """``../../test_amend_features_extra/...`` references resolve against the manifest dir."""
    gen_dir, result = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_full',
    )

    # The out-of-tree YAML must have been merged.
    periph_content = (gen_dir / 'gen_board_periph_config.c').read_text(encoding='utf-8')
    assert 'gpio_common_extra' in periph_content

    # Out-of-tree absolute paths get a portability info log (not fatal).
    assert 'is not portable across machines' in result.stdout or \
        'is not portable across machines' in result.stderr


# ---------------------------------------------------------------------------
# error-path tests
# ---------------------------------------------------------------------------

def test_amend_error_missing_manifest(bmgr_root, tmp_path):
    """Pointing -a at a directory without board_amend.yaml must abort."""
    gen_dir, result = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_bad_no_manifest',
        expect_success=False,
    )
    assert result.returncode != 0
    assert 'board_amend.yaml' in result.stdout + result.stderr


def test_amend_error_unknown_suffix(bmgr_root, tmp_path):
    """Manifest listing a .txt file must abort with the supported-suffix list."""
    gen_dir, result = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_bad_unknown_suffix',
        expect_success=False,
    )
    assert result.returncode != 0
    combined = result.stdout + result.stderr
    assert 'unsupported suffix' in combined
    assert '.txt' in combined


def test_amend_error_yaml_no_root_keys(bmgr_root, tmp_path):
    """YAML fragment without devices/peripherals must abort."""
    gen_dir, result = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_bad_no_root_keys',
        expect_success=False,
    )
    assert result.returncode != 0
    combined = result.stdout + result.stderr
    assert "must define top-level 'devices' or 'peripherals'" in combined


def test_amend_error_directory_item(bmgr_root, tmp_path):
    """Listing a directory in apply: must abort with a clear migration hint.
    Directory items were removed when auto-discovery was dropped: every file
    that should participate has to be listed explicitly.
    """
    gen_dir, result = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_bad_dir_item',
        expect_success=False,
    )
    assert result.returncode != 0
    combined = result.stdout + result.stderr
    assert 'is a directory' in combined
    assert 'Directory items are not supported' in combined
    assert 'list each file explicitly' in combined


def test_amend_unreferenced_top_files_are_ignored(bmgr_root, tmp_path):
    """A sdkconfig.defaults.board placed at the amend root but missing from
    apply: must NOT be applied. The generator emits an info log to flag the
    stray file so authors don't silently lose their changes.
    """
    gen_dir, result = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_unreferenced_sdkconfig',
    )

    defaults_content = (gen_dir / 'board_manager.defaults').read_text(encoding='utf-8')
    # The tripwire CONFIG_* from the unreferenced file must NOT be in defaults.
    assert 'CONFIG_BMGR_TEST_AMEND_UNREFERENCED_TRIPWIRE' not in defaults_content
    # And the info log must surface it so users can debug.
    combined = result.stdout + result.stderr
    assert 'present at amend root but not listed in apply' in combined


def test_amend_error_item_not_found(bmgr_root, tmp_path):
    """Manifest referencing a missing path must abort."""
    gen_dir, result = _run_fixture_board(
        bmgr_root, tmp_path,
        'test_apps/components/test_amend_features/amend_bad_item_not_found',
        expect_success=False,
    )
    assert result.returncode != 0
    assert 'Amend item not found' in result.stdout + result.stderr


def test_amend_keeps_old_artifacts_on_error(bmgr_root, tmp_path):
    """Bad amend must not wipe a previously generated gen_bmgr_codes/."""
    writable_bmgr = _copy_bmgr_to_writable_tmp(bmgr_root, tmp_path)
    project_dir = tmp_path / 'project_preserve'
    _prepare_project(project_dir)

    env = os.environ.copy()
    env['PYTHONDONTWRITEBYTECODE'] = '1'
    customer_path = str((writable_bmgr / 'test_apps' / 'components').resolve())

    # First a successful generation to seed gen_bmgr_codes/.
    ok = subprocess.run(
        ['python3', str(writable_bmgr / 'gen_bmgr_config_codes.py'),
         '-b', 'test_amend_features',
         '-c', customer_path,
         '--project-dir', str(project_dir)],
        cwd=project_dir, capture_output=True, text=True, env=env,
    )
    assert ok.returncode == 0, ok.stdout + ok.stderr
    seed = (project_dir / 'components' / 'gen_bmgr_codes' / 'gen_board_info.c').read_text(encoding='utf-8')

    # Then a doomed amend run.
    amend_abs = str((writable_bmgr / 'test_apps' / 'components' / 'test_amend_features' / 'amend_bad_item_not_found').resolve())
    bad = subprocess.run(
        ['python3', str(writable_bmgr / 'gen_bmgr_config_codes.py'),
         '-b', 'test_amend_features',
         '-c', customer_path,
         '-a', amend_abs,
         '--project-dir', str(project_dir)],
        cwd=project_dir, capture_output=True, text=True, env=env,
    )
    assert bad.returncode != 0

    # Existing artifact untouched.
    preserved = (project_dir / 'components' / 'gen_bmgr_codes' / 'gen_board_info.c').read_text(encoding='utf-8')
    assert preserved == seed
