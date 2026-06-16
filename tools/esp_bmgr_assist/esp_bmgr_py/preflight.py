"""Assist-side preflight checks for ESP Board Manager integration."""

from __future__ import annotations

from dataclasses import dataclass
import os
from pathlib import Path
from typing import Any, Callable, List, Optional, Sequence

from esp_bmgr_py.artifact_check import list_missing_gen_files
from esp_bmgr_py.project_integration import project_uses_board_manager
from esp_bmgr_py.project_paths import (
    board_manager_defaults_path,
    gen_bmgr_codes_dir,
    resolve_project_dir,
    sdkconfig_path,
)
from esp_bmgr_py.target_check import find_target_mismatch
from esp_bmgr_py.task_policy import is_build_like


ASSIST_PREFLIGHT_ENV = 'ESP_BMGR_ASSIST_PREFLIGHT'
ASSIST_PREFLIGHT_LEVEL_ARG = 'bmgr_preflight_level'
SKIP_GEN_CHECK_ENV = 'ESP_BOARD_MANAGER_SKIP_GEN_CHECK'
SKIP_SDKCONFIG_CHECK_ENV = 'ESP_BOARD_MANAGER_SKIP_SDKCONFIG_CHECK'
ASSIST_TAG = 'ESP_BMGR_ASSIST'

PREFLIGHT_LEVEL_SILENT = 0
PREFLIGHT_LEVEL_WARNING = 1
PREFLIGHT_LEVEL_ERROR = 2
DEFAULT_PREFLIGHT_LEVEL = PREFLIGHT_LEVEL_ERROR

_PREFLIGHT_LEVEL_ALIASES = {
    '0': PREFLIGHT_LEVEL_SILENT,
    'false': PREFLIGHT_LEVEL_SILENT,
    'no': PREFLIGHT_LEVEL_SILENT,
    'off': PREFLIGHT_LEVEL_SILENT,
    'quiet': PREFLIGHT_LEVEL_SILENT,
    'silent': PREFLIGHT_LEVEL_SILENT,
    '1': PREFLIGHT_LEVEL_WARNING,
    'on': PREFLIGHT_LEVEL_WARNING,
    'true': PREFLIGHT_LEVEL_WARNING,
    'warn': PREFLIGHT_LEVEL_WARNING,
    'warning': PREFLIGHT_LEVEL_WARNING,
    'yes': PREFLIGHT_LEVEL_WARNING,
    '2': PREFLIGHT_LEVEL_ERROR,
    'abort': PREFLIGHT_LEVEL_ERROR,
    'block': PREFLIGHT_LEVEL_ERROR,
    'break': PREFLIGHT_LEVEL_ERROR,
    'error': PREFLIGHT_LEVEL_ERROR,
    'fail': PREFLIGHT_LEVEL_ERROR,
    'fatal': PREFLIGHT_LEVEL_ERROR,
    'stop': PREFLIGHT_LEVEL_ERROR,
}


class PreflightAbortError(RuntimeError):
    """Raised when preflight blocks the command outside ``idf.py``."""


@dataclass(frozen=True)
class PreflightCheck:
    skip_env: str
    message_factory: Callable[[Path, str], Optional[str]]


def _env_flag_true(var_name: str) -> bool:
    value = os.environ.get(var_name, '')
    return value.strip().lower() in ('1', 'true', 'yes', 'on')


def _warn(message: str) -> None:
    print(message, flush=True)


def _global_arg_value(global_args: Any, name: str) -> Any:
    if isinstance(global_args, dict):
        return global_args.get(name)
    return getattr(global_args, name, None)


def _parse_preflight_level(value: Any) -> Optional[int]:
    if value is None:
        return None
    return _PREFLIGHT_LEVEL_ALIASES.get(str(value).strip().lower())


def resolve_preflight_level(global_args: Any = None) -> int:
    cli_level = _parse_preflight_level(_global_arg_value(global_args, ASSIST_PREFLIGHT_LEVEL_ARG))
    if cli_level is not None:
        return cli_level

    env_level = _parse_preflight_level(os.environ.get(ASSIST_PREFLIGHT_ENV))
    if env_level is not None:
        return env_level

    return DEFAULT_PREFLIGHT_LEVEL


def _message_for_level(message: str, level: int) -> str:
    if level == PREFLIGHT_LEVEL_ERROR:
        return message.replace(
            '[{0}] Warning:'.format(ASSIST_TAG),
            '[{0}] Error:'.format(ASSIST_TAG),
            1,
        )
    return message


def _raise_preflight_error(message: str) -> None:
    try:
        from idf_py_actions.errors import FatalError
    except ImportError:
        raise PreflightAbortError(message)
    raise FatalError(message)


def _preflight_skip_hint(command: str = 'build') -> str:
    return (
        '[{tag}] Hint: temporarily skip assist preflight with '
        '`ESP_BMGR_ASSIST_PREFLIGHT=0 idf.py {command}`.'
    ).format(tag=ASSIST_TAG, command=command)


def _build_like_command_name(tasks: Sequence[object]) -> str:
    for task in tasks:
        name = getattr(task, 'name', '')
        if name:
            return name
    return 'build'


def _format_missing_artifacts_warning(project_dir: Path, missing, skip_hint: str) -> str:
    gen_dir = gen_bmgr_codes_dir(project_dir)
    bullet = '\n'.join('  - {0}'.format(name) for name in missing)
    return (
        '[{tag}] Warning: missing generated board artifacts in {gen_dir}.\n\n'
        '[{tag}] Hint: run "idf.py bmgr -b <board>" before build or flash for a correct board build '
        '(legacy: "idf.py gen-bmgr-config -b <board>").\n'
        '[{tag}] Hint: run "idf.py bmgr -l" to list available boards.\n'
        '{skip_hint}'
    ).format(tag=ASSIST_TAG, gen_dir=gen_dir, bullet=bullet, skip_hint=skip_hint)


def _generated_artifact_warning(project_dir: Path, skip_hint: str) -> Optional[str]:
    missing = list_missing_gen_files(project_dir)
    if not missing:
        return None
    return _format_missing_artifacts_warning(project_dir, missing, skip_hint)


def _target_mismatch_warning(project_dir: Path, skip_hint: str) -> Optional[str]:
    defaults_path = board_manager_defaults_path(project_dir)
    sdk_path = sdkconfig_path(project_dir)
    mismatch = find_target_mismatch(defaults_path, sdk_path)
    if not mismatch:
        return None
    return '[{0}] Warning: {1}\n{2}'.format(ASSIST_TAG, mismatch, skip_hint)


PREFLIGHT_CHECKS = (
    PreflightCheck(SKIP_GEN_CHECK_ENV, _generated_artifact_warning),
    PreflightCheck(SKIP_SDKCONFIG_CHECK_ENV, _target_mismatch_warning),
)


def _suppress_component_duplicate_warnings() -> None:
    # Keep generated-artifact warning ownership in assist, avoid duplicate warning from component callback.
    os.environ[SKIP_GEN_CHECK_ENV] = '1'


def collect_preflight_warnings(project_dir: Path, skip_hint: Optional[str] = None) -> List[str]:
    if skip_hint is None:
        skip_hint = _preflight_skip_hint()
    warnings = []
    for check in PREFLIGHT_CHECKS:
        if _env_flag_true(check.skip_env):
            continue
        message = check.message_factory(project_dir, skip_hint)
        if message:
            warnings.append(message)
    return warnings


def should_run_preflight(project_dir: Path, tasks: Sequence[object]) -> bool:
    # Only build-like commands need these checks. Configuration-only commands often run before the
    # generated artifacts exist, so warning there would be noisy and misleading.
    if not is_build_like(tasks):
        return False
    if not project_uses_board_manager(project_dir):
        return False
    return True


def run_preflight(project_path: str, global_args: Any, tasks: Sequence[object]) -> None:
    project_dir = resolve_project_dir(global_args, project_path)
    if project_dir is None:
        return
    if not should_run_preflight(project_dir, tasks):
        return

    level = resolve_preflight_level(global_args)
    if level == PREFLIGHT_LEVEL_SILENT:
        # Silence assist-side output, but still suppress the matching component-side warning so the
        # same missing-artifact issue is not reported twice.
        _suppress_component_duplicate_warnings()
        return

    skip_hint = _preflight_skip_hint(_build_like_command_name(tasks))
    warnings = collect_preflight_warnings(project_dir, skip_hint)
    _suppress_component_duplicate_warnings()
    if not warnings:
        return
    if level == PREFLIGHT_LEVEL_WARNING:
        for message in warnings:
            _warn(message)
        return

    rendered = '\n\n'.join(_message_for_level(message, level) for message in warnings)
    _raise_preflight_error(rendered)
