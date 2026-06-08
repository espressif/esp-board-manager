"""
Tests for the auto-amend feature and ``-c`` multi-path support.

Auto-amend discovers, for the currently selected board, sibling overlay
directories whose name matches the board name and that contain a
``board_amend.yaml`` manifest *without* being a full board directory. Such
overlays are applied automatically (no explicit ``-a`` needed), with explicit
``-a`` layered last as the highest-priority override.

The overlay fixtures intentionally live under ``test_apps/board_overlays`` and
``test_apps/board_overlays_b`` (NOT under ``test_apps/components``) so the
existing ``test_amend_features_board.py`` cases — which scan ``-c
test_apps/components`` — never auto-discover them. The base board itself
(``test_apps/components/test_amend_features``) is supplied via a separate ``-c``
entry, exercising the new ';'-separated multi-path behaviour.
"""

from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path
from typing import List, Optional


# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------

def _prepare_project(project_dir: Path) -> None:
    if project_dir.exists():
        shutil.rmtree(project_dir)
    project_dir.mkdir(parents=True)
    (project_dir / 'CMakeLists.txt').write_text(
        'cmake_minimum_required(VERSION 3.16)\nproject(bmgr_auto_amend_fixture)\n',
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


def _run_auto_amend(
    bmgr_root: Path,
    tmp_path: Path,
    *,
    customer_rels: List[str],
    amend_rel: Optional[str] = None,
    env_extra: Optional[dict] = None,
    label: str = 'run',
    expect_success: bool = True,
) -> tuple[Path, subprocess.CompletedProcess]:
    """Generate against ``test_amend_features`` with the given ``-c`` roots.

    ``customer_rels`` are paths relative to the writable bmgr copy; they are
    resolved to absolute paths and joined with ';' for a single ``-c`` value.
    ``amend_rel`` (optional) is the explicit ``-a`` directory (also relative to
    the writable copy).
    """
    writable_bmgr = _copy_bmgr_to_writable_tmp(bmgr_root, tmp_path)
    project_dir = tmp_path / ('project_' + label)
    _prepare_project(project_dir)

    customer_abs = ';'.join(str((writable_bmgr / rel).resolve()) for rel in customer_rels)
    args = ['-b', 'test_amend_features',
            '-c', customer_abs,
            '--project-dir', str(project_dir)]
    if amend_rel:
        amend_abs = str((writable_bmgr / amend_rel).resolve())
        args += ['-a', amend_abs]

    env = os.environ.copy()
    env['PYTHONDONTWRITEBYTECODE'] = '1'
    if env_extra:
        env.update(env_extra)

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


_COMPONENTS = 'test_apps/components'
_OVERLAYS = 'test_apps/board_overlays'
_OVERLAYS_B = 'test_apps/board_overlays_b'
_EXPLICIT = 'test_apps/board_overlays_explicit'
_OVERLAYS_BAD = 'test_apps/board_overlays_bad'


# ---------------------------------------------------------------------------
# tests
# ---------------------------------------------------------------------------

def test_auto_amend_applies_without_explicit_a(bmgr_root, tmp_path):
    """An overlay discovered via ``-c`` is applied even without ``-a``."""
    gen_dir, result = _run_auto_amend(
        bmgr_root, tmp_path,
        customer_rels=[_COMPONENTS, _OVERLAYS],
        label='applies',
    )

    device_content = (gen_dir / 'gen_board_device_config.c').read_text(encoding='utf-8')
    periph_content = (gen_dir / 'gen_board_periph_config.c').read_text(encoding='utf-8')

    assert 'auto_tweak_power' in device_content, (
        f'auto-amend device should be generated, got:\n{device_content}'
    )
    assert '.pin_bit_mask = BIT64(11)' in periph_content, (
        f'auto-amend peripheral gpio_auto_tweak_led (pin 11) should appear, got:\n{periph_content}'
    )

    combined = result.stdout + result.stderr
    assert 'Amend sources for board' in combined, (
        f'expected a consolidated amend-sources summary, got:\n{combined}'
    )
    assert '[auto]' in combined, (
        f'expected the overlay to be tagged [auto] in the summary, got:\n{combined}'
    )


def test_auto_amend_explicit_a_overrides_auto(bmgr_root, tmp_path):
    """An explicit ``-a`` overlay is layered last and overrides auto-amend."""
    gen_dir, _ = _run_auto_amend(
        bmgr_root, tmp_path,
        customer_rels=[_COMPONENTS, _OVERLAYS],
        amend_rel=_EXPLICIT,
        label='explicit_override',
    )

    periph_content = (gen_dir / 'gen_board_periph_config.c').read_text(encoding='utf-8')

    # Explicit -a moves gpio_auto_tweak_led from the auto value (pin 11) to pin 21.
    assert '.pin_bit_mask = BIT64(21)' in periph_content, (
        f'explicit -a should override gpio_auto_tweak_led to pin 21, got:\n{periph_content}'
    )
    assert '.pin_bit_mask = BIT64(11)' not in periph_content, (
        f'auto value (pin 11) should have been overridden, got:\n{periph_content}'
    )

    # The explicit overlay also wins for sdkconfig: its CONFIG value (21) must
    # override the auto overlay value (7).
    defaults_content = (gen_dir / 'board_manager.defaults').read_text(encoding='utf-8')
    assert 'CONFIG_BMGR_TEST_AUTO_AMEND_VALUE=21' in defaults_content, (
        f'explicit -a sdkconfig (=21) should override the auto value, got:\n{defaults_content}'
    )


def test_auto_amend_disabled_by_env(bmgr_root, tmp_path):
    """Setting ESP_BOARD_MANAGER_DISABLE_AUTO_AMEND disables auto discovery."""
    gen_dir, result = _run_auto_amend(
        bmgr_root, tmp_path,
        customer_rels=[_COMPONENTS, _OVERLAYS],
        env_extra={'ESP_BOARD_MANAGER_DISABLE_AUTO_AMEND': '1'},
        label='disabled',
    )

    device_content = (gen_dir / 'gen_board_device_config.c').read_text(encoding='utf-8')
    periph_content = (gen_dir / 'gen_board_periph_config.c').read_text(encoding='utf-8')

    assert 'auto_tweak_power' not in device_content, (
        f'auto-amend must be disabled, but auto_tweak_power appears in:\n{device_content}'
    )
    assert '.pin_bit_mask = BIT64(11)' not in periph_content, (
        f'auto-amend must be disabled, but gpio_auto_tweak_led (pin 11) appears in:\n{periph_content}'
    )

    # The overlay's sdkconfig / Kconfig / C fragments must all be skipped too.
    defaults_content = (gen_dir / 'board_manager.defaults').read_text(encoding='utf-8')
    assert 'CONFIG_BMGR_TEST_AUTO_AMEND_VALUE' not in defaults_content, (
        f'auto-amend sdkconfig must be skipped when disabled, got:\n{defaults_content}'
    )
    kconfig_content = (gen_dir / 'Kconfig.projbuild').read_text(encoding='utf-8')
    assert 'BMGR_TEST_AUTO_AMEND_KCONFIG' not in kconfig_content, (
        f'auto-amend Kconfig must be skipped when disabled, got:\n{kconfig_content}'
    )
    cmake_content = (gen_dir / 'CMakeLists.txt').read_text(encoding='utf-8')
    assert 'auto_setup.c' not in cmake_content, (
        f'auto-amend C source must be skipped when disabled, got:\n{cmake_content}'
    )

    combined = result.stdout + result.stderr
    assert '[auto]' not in combined, (
        f'no [auto] amend source expected when disabled, got:\n{combined}'
    )


def test_auto_amend_multi_customer_path_stack(bmgr_root, tmp_path):
    """Two ``-c`` overlay roots stack; the later path wins for shared fields."""
    gen_dir, _ = _run_auto_amend(
        bmgr_root, tmp_path,
        customer_rels=[_COMPONENTS, _OVERLAYS, _OVERLAYS_B],
        label='stack',
    )

    periph_content = (gen_dir / 'gen_board_periph_config.c').read_text(encoding='utf-8')

    # board_overlays sets gpio_auto_stack_led pin 12; board_overlays_b overrides to 13.
    assert '.pin_bit_mask = BIT64(13)' in periph_content, (
        f'later -c overlay (pin 13) should win for gpio_auto_stack_led, got:\n{periph_content}'
    )
    assert '.pin_bit_mask = BIT64(12)' not in periph_content, (
        f'earlier -c overlay value (pin 12) should have been overridden, got:\n{periph_content}'
    )

    # sdkconfig also stacks: board_overlays sets =7, board_overlays_b overrides to =8.
    defaults_content = (gen_dir / 'board_manager.defaults').read_text(encoding='utf-8')
    assert 'CONFIG_BMGR_TEST_AUTO_AMEND_VALUE=8' in defaults_content, (
        f'later -c overlay sdkconfig (=8) should win, got:\n{defaults_content}'
    )


def test_auto_amend_appends_sdkconfig_and_overrides_base(bmgr_root, tmp_path):
    """Auto-amend ``sdkconfig.defaults.board`` is appended and can override a
    base-board value via the BMGR_CONFIG_OVERRIDE marker."""
    gen_dir, _ = _run_auto_amend(
        bmgr_root, tmp_path,
        customer_rels=[_COMPONENTS, _OVERLAYS],
        label='sdkconfig',
    )

    defaults_content = (gen_dir / 'board_manager.defaults').read_text(encoding='utf-8')

    # New symbol contributed only by the auto-amend overlay.
    assert 'CONFIG_BMGR_TEST_AUTO_AMEND_VALUE=7' in defaults_content, (
        f'auto-amend should append CONFIG_BMGR_TEST_AUTO_AMEND_VALUE=7, got:\n{defaults_content}'
    )
    # Base board value (=1) overridden by the auto-amend overlay (=55).
    assert 'CONFIG_BMGR_TEST_AMEND_BASE_VALUE=55' in defaults_content, (
        f'auto-amend should override base value to 55, got:\n{defaults_content}'
    )
    # The earlier base value must be commented with an override marker for traceability.
    assert '# BMGR_CONFIG_OVERRIDE by Amend sdkconfig defaults (' in defaults_content, (
        f'expected a BMGR_CONFIG_OVERRIDE marker, got:\n{defaults_content}'
    )
    assert ': CONFIG_BMGR_TEST_AMEND_BASE_VALUE=1' in defaults_content, (
        f'overridden base value (=1) should remain commented, got:\n{defaults_content}'
    )


def test_auto_amend_appends_kconfig(bmgr_root, tmp_path):
    """Auto-amend ``Kconfig.projbuild`` fragment is appended to the generated Kconfig."""
    gen_dir, _ = _run_auto_amend(
        bmgr_root, tmp_path,
        customer_rels=[_COMPONENTS, _OVERLAYS],
        label='kconfig',
    )

    kconfig_content = (gen_dir / 'Kconfig.projbuild').read_text(encoding='utf-8')
    assert 'BMGR_TEST_AUTO_AMEND_KCONFIG' in kconfig_content, (
        f'auto-amend Kconfig symbol should be appended, got:\n{kconfig_content}'
    )


def test_auto_amend_appends_c_source(bmgr_root, tmp_path):
    """Auto-amend ``.c`` fragment is added to the generated component's target_sources."""
    gen_dir, _ = _run_auto_amend(
        bmgr_root, tmp_path,
        customer_rels=[_COMPONENTS, _OVERLAYS],
        label='csource',
    )

    cmake_content = (gen_dir / 'CMakeLists.txt').read_text(encoding='utf-8')
    assert 'auto_setup.c' in cmake_content, (
        f'auto-amend C source should be compiled, got:\n{cmake_content}'
    )
    assert 'target_sources(${COMPONENT_LIB} PRIVATE' in cmake_content, (
        f'amend sources must be added via target_sources(), got:\n{cmake_content}'
    )


def test_auto_amend_full_board_with_manifest_is_not_applied(bmgr_root, tmp_path):
    """A directory that is a *complete board* (board_info/devices/peripherals)
    must be treated as a board even if it contains ``board_amend.yaml``; its
    manifest must NOT be auto-applied to itself.

    ``test_apps/components/test_amend_features`` is a full board and ships its own
    tripwire ``board_amend.yaml`` (adds ``gpio_fullboard_tripwire`` at pin 63).
    Running with only ``-c components`` (no overlay root) must not apply it.
    """
    gen_dir, result = _run_auto_amend(
        bmgr_root, tmp_path,
        customer_rels=[_COMPONENTS],
        label='fullboard',
    )

    periph_content = (gen_dir / 'gen_board_periph_config.c').read_text(encoding='utf-8')
    assert 'gpio_fullboard_tripwire' not in periph_content, (
        f'full board manifest must not be auto-applied to itself, got:\n{periph_content}'
    )
    assert '.pin_bit_mask = BIT64(63)' not in periph_content, (
        f'tripwire pin 63 must not appear, got:\n{periph_content}'
    )

    combined = result.stdout + result.stderr
    assert '[auto]' not in combined, (
        f'the full board must not be reported as an auto-amend source, got:\n{combined}'
    )


def test_auto_amend_dedup_same_dir_from_multiple_roots(bmgr_root, tmp_path):
    """The same overlay reachable from multiple scan roots is applied once.

    Passing the same ``-c`` path twice makes ``scan_auto_amend_directories``
    encounter the same resolved directory twice; the ``seen`` set must collapse
    it to a single source (one ``[auto]`` line in the summary).
    """
    gen_dir, result = _run_auto_amend(
        bmgr_root, tmp_path,
        customer_rels=[_COMPONENTS, _OVERLAYS, _OVERLAYS],
        label='dedup',
    )

    device_content = (gen_dir / 'gen_board_device_config.c').read_text(encoding='utf-8')
    assert 'auto_tweak_power' in device_content, (
        f'overlay should still apply once, got:\n{device_content}'
    )

    combined = result.stdout + result.stderr
    assert combined.count('[auto]') == 1, (
        f'duplicate overlay must be deduplicated to a single auto source, got:\n{combined}'
    )


def test_auto_amend_bad_manifest_aborts_and_preserves_artifacts(bmgr_root, tmp_path):
    """A broken auto-amend manifest must fail fast without wiping prior output.

    Mirrors the explicit ``-a`` preservation guarantee: ``resolve_amend_plan``
    runs before the generated directory is cleared, so a bad auto overlay aborts
    the run and leaves the previously generated ``gen_bmgr_codes/`` intact.
    """
    writable_bmgr = _copy_bmgr_to_writable_tmp(bmgr_root, tmp_path)
    project_dir = tmp_path / 'project_auto_bad'
    _prepare_project(project_dir)

    env = os.environ.copy()
    env['PYTHONDONTWRITEBYTECODE'] = '1'
    components = str((writable_bmgr / _COMPONENTS).resolve())
    bad_root = str((writable_bmgr / _OVERLAYS_BAD).resolve())
    gen_info = project_dir / 'components' / 'gen_bmgr_codes' / 'gen_board_info.c'

    # Seed a good generation (no overlay) first.
    ok = subprocess.run(
        ['python3', str(writable_bmgr / 'gen_bmgr_config_codes.py'),
         '-b', 'test_amend_features',
         '-c', components,
         '--project-dir', str(project_dir)],
        cwd=project_dir, capture_output=True, text=True, env=env,
    )
    assert ok.returncode == 0, ok.stdout + '\n' + ok.stderr
    seed = gen_info.read_text(encoding='utf-8')

    # Now a run whose auto-amend overlay has a broken manifest.
    bad = subprocess.run(
        ['python3', str(writable_bmgr / 'gen_bmgr_config_codes.py'),
         '-b', 'test_amend_features',
         '-c', f'{components};{bad_root}',
         '--project-dir', str(project_dir)],
        cwd=project_dir, capture_output=True, text=True, env=env,
    )
    assert bad.returncode != 0, bad.stdout + '\n' + bad.stderr
    combined = bad.stdout + bad.stderr
    assert 'Auto-amend error' in combined, (
        f'expected an Auto-amend error message, got:\n{combined}'
    )

    # Previously generated artifact must be untouched.
    preserved = gen_info.read_text(encoding='utf-8')
    assert preserved == seed, 'bad auto-amend run must not modify prior artifacts'
