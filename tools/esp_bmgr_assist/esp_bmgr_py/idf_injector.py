from __future__ import annotations

"""Early bootstrap for ESP Board Manager inside the ESP-IDF ``idf.py`` startup flow.

This module is imported from ``esp_bmgr_py.pth`` before ``idf.py`` has finished building its
runtime state, so most helpers here work with only ``sys.argv``, environment variables and files
on disk.

The main steps are:
1. detect whether the current Python process is really serving an ``idf.py`` command
2. locate an existing ``esp_board_manager`` component for the current project
3. when the user explicitly runs ``bmgr`` / ``gen-bmgr-config``, resolve missing dependencies
4. make sure the discovered ``idf_ext.py`` can be found through ``IDF_EXTRA_ACTIONS_PATH``
"""

import errno
import hashlib
import os
import re
import subprocess
import sys
import tempfile
from contextlib import contextmanager
from importlib.abc import MetaPathFinder
from pathlib import Path
from typing import Any, Dict, Iterable, List, Mapping, NamedTuple, Optional, Tuple

try:
    from filelock import FileLock, Timeout as FileLockTimeout
except ImportError:  # pragma: no cover - runtime falls back to unlocked bootstrap
    FileLock = None
    FileLockTimeout = None

_DEBUG = os.environ.get('ESP_BMGR_DEBUG', '0') == '1'
_MAIN_EXECUTED = False
_IMPORT_NOTICE_EMITTED = False
_FILELOCK_NOTICE_EMITTED = False
_LOCATOR_LOG_KEYS: set = set()
ASSIST_TAG = '[ESP_BMGR_ASSIST]'

BOOTSTRAP_SKIP_COMPONENTS = {'gen_bmgr_codes'}
SKIP_BOOTSTRAP_ENV = 'ESP_BMGR_SKIP_BOOTSTRAP'
REEXEC_AFTER_BOOTSTRAP_ENV = 'ESP_BMGR_ASSIST_REEXECED'
LOCK_FILE_NAME = '.esp_bmgr_py.lock'
LOCK_FALLBACK_DIR = 'esp_bmgr_py_locks'
LOCK_TIMEOUT_ENV = 'ESP_BMGR_LOCK_TIMEOUT'
DEFAULT_LOCK_TIMEOUT_SEC = 60.0
DEFAULT_DEPENDENCY = {'version': '*', 'require': 'public'}
IDF_VERSION_CMAKE_RE = re.compile(r'^\s*set\s*\(\s*IDF_VERSION_([A-Z]{5})\s+(\d+)')
_YAML_MODULE = None


class _LazyYamlModule:
    """Keep the .pth bootstrap import stdlib-only until YAML parsing is actually needed."""

    def __getattr__(self, name: str) -> Any:
        return getattr(_require_pyyaml(), name)


yaml = _LazyYamlModule()

from esp_bmgr_py.bmgr_request import BMGR_ACTION_NAMES, resolve_bmgr_command, resolve_bmgr_request
from esp_bmgr_py.bmgr_discovery import (
    collect_local_components as _collect_local_components_impl,
    find_bmgr_from_lock as _find_bmgr_from_lock_impl,
    find_board_manager_component as _find_board_manager_component_impl,
    find_local_bmgr as _find_local_bmgr_impl,
    find_managed_bmgr as _find_managed_bmgr_impl,
    is_bmgr_component as _is_bmgr_component_impl,
    load_project_manifests as _load_project_manifests_impl,
    resolve_declared_local_bmgr as _resolve_declared_local_bmgr_impl,
)
from esp_bmgr_py.bmgr_manifest import (
    CANONICAL_BMGR_KEY,
    MANIFEST_NAMES,
    find_bmgr_dependency as _find_bmgr_dependency,
    find_component_manifest as _find_component_manifest,
    find_project_manifest as _find_manifest_file,
    load_dependencies as _load_dependencies,
    load_manifest_document as _load_manifest_document,
    read_manifest_text as _read_manifest_text,
)
from esp_bmgr_py._notice import emit_notice
from esp_bmgr_py.bootstrap_target import resolve_bootstrap_target
from esp_bmgr_py.update_check import maybe_warn_about_update


class MissingBoardManagerDependencyError(RuntimeError):
    """Raised when the project dependency graph does not include esp_board_manager."""


class CliContext(NamedTuple):
    is_idf_command: bool
    project_path: Path
    is_gen_bmgr_config: bool
    argv: Tuple[str, ...]
    bmgr_command: Optional[str] = None


def _require_pyyaml():
    global _YAML_MODULE
    if _YAML_MODULE is None:
        try:
            import yaml as yaml_module
        except ImportError as exc:
            raise RuntimeError(
                'PyYAML is required when esp-bmgr-assist needs to inspect manifests or dependencies.lock. '
                'Install esp-bmgr-assist with dependencies or add `pyyaml` to this Python environment.'
            ) from exc
        _YAML_MODULE = yaml_module
    return _YAML_MODULE


def _debug(message: str) -> None:
    if _DEBUG and not _is_shell_completion_invocation():
        print(f'{ASSIST_TAG} {message}')


def _debug_locator_once(kind: str, path: Path, message: str) -> None:
    if not _DEBUG:
        return
    try:
        key = f'{kind}:{_canonicalize_path(path)}'
    except OSError:
        key = f'{kind}:{path}'
    if key in _LOCATOR_LOG_KEYS:
        return
    _LOCATOR_LOG_KEYS.add(key)
    _debug(message)


def _emit_import_notice_once() -> None:
    """Print once per process when idf.py runs bootstrap (via ``import idf_py_actions``).

    ``esp_bmgr_py.pth`` loads this module for every Python in the venv; component downloads spawn
    many short-lived interpreters. If this ran at module import time, debug mode would print once
    per subprocess.
    """
    global _IMPORT_NOTICE_EMITTED
    if not _DEBUG or _IMPORT_NOTICE_EMITTED or _is_shell_completion_invocation():
        return
    _IMPORT_NOTICE_EMITTED = True
    print(
        f'{ASSIST_TAG} idf_injector loaded: .pth hook installed, '
        f'package {_package_actions_dir()}',
        file=sys.stderr,
        flush=True,
    )


def _emit_missing_filelock_notice_once() -> None:
    global _FILELOCK_NOTICE_EMITTED
    if not _DEBUG or _FILELOCK_NOTICE_EMITTED or _is_shell_completion_invocation():
        return
    _FILELOCK_NOTICE_EMITTED = True
    print(
        f'{ASSIST_TAG} filelock is not installed; dependency bootstrap will run without '
        'inter-process locking.',
        file=sys.stderr,
        flush=True,
    )


def _is_idf_command(argv0: str) -> bool:
    name = Path(argv0).name.lower()
    return name.endswith('idf.py') or name.endswith('idf.py.exe')


def _is_shell_completion_invocation() -> bool:
    return bool(os.environ.get('_IDF.PY_COMPLETE'))


def _parse_cli_context(argv: Optional[Iterable[str]] = None, cwd: Optional[Path] = None) -> CliContext:
    argv_list = tuple(argv or sys.argv)
    current_dir = Path(cwd or os.getcwd())
    if not argv_list or not _is_idf_command(argv_list[0]):
        return CliContext(False, current_dir.resolve(), False, argv_list)

    project_arg = None
    args = list(argv_list[1:])
    idx = 0
    while idx < len(args):
        token = args[idx]
        # ``idf.py`` has not parsed global options yet, so we resolve the project directory by
        # scanning the raw argument list ourselves.
        if token in ('-C', '--project-dir'):
            if idx + 1 < len(args):
                project_arg = args[idx + 1]
            idx += 2
            continue
        if token.startswith('-C='):
            project_arg = token[3:]
        elif token.startswith('--project-dir='):
            project_arg = token.split('=', 1)[1]
        idx += 1

    project_path = Path(project_arg).expanduser() if project_arg else current_dir
    try:
        resolved_project_path = project_path.resolve()
    except OSError:
        resolved_project_path = project_path.absolute()

    request = resolve_bmgr_request(argv_list)
    return CliContext(
        True,
        resolved_project_path,
        request.is_legacy_command,
        argv_list,
        request.command,
    )


def _is_version_query(argv: Iterable[str]) -> bool:
    args = list(argv)[1:]
    return '--version' in args or args == ['version']


def _resolve_bmgr_command_from_argv(argv: Iterable[str]) -> Optional[str]:
    return resolve_bmgr_command(argv)


def _resolve_context_bmgr_command(context: CliContext) -> Optional[str]:
    return context.bmgr_command or _resolve_bmgr_command_from_argv(context.argv)


def _format_dep_for_debug(dep: Any) -> str:
    """Render a dependency spec compactly for debug logs, keeping key order."""
    if isinstance(dep, Mapping):
        rendered = ', '.join(f'{key}={value!r}' for key, value in dep.items())
        return '{' + rendered + '}'
    return repr(dep)


def _scan_direct_bmgr_declaration(
    project_path: Path,
) -> Optional[Tuple[Path, str, Any]]:
    """Scan local component manifests for a direct esp_board_manager declaration.

    Returns ``(manifest_path, key, dependency_spec)`` for the first manifest that declares
    ``esp_board_manager`` (or ``espressif/esp_board_manager``) under ``dependencies:``. The
    mere presence of the key is sufficient to treat it as the user's declared intent --
    ``matches:`` / ``rules:`` gating is intentionally ignored, because running a
    bmgr/gen-bmgr-config command already implies the user wants the component available.
    Returns ``None`` when no local manifest declares it.
    """
    for _name, component_dir in _collect_local_components(project_path):
        manifest_path = _find_component_manifest(component_dir)
        if manifest_path is None:
            continue
        try:
            data, _newline, _has_bom, _backend = _load_manifest_document(manifest_path)
        except FileNotFoundError:
            continue
        # Intentionally do not swallow YAML syntax / permission errors here: we want the
        # real cause to surface so the user can fix their manifest.
        dependencies = data.get('dependencies')
        if not isinstance(dependencies, dict):
            continue
        result = _find_bmgr_dependency(dependencies)
        if result is not None:
            key, dep = result
            _debug(
                f'Direct esp_board_manager declaration found in {manifest_path} '
                f'(key={key!r})'
            )
            return manifest_path, key, dep
    return None


def _format_direct_invalid_override_message(
    manifest_path: Path,
    override_path_value: str,
    override_resolved: Optional[Path],
) -> str:
    resolved_repr = str(override_resolved) if override_resolved else override_path_value
    return (
        f'{ASSIST_TAG} The declared esp_board_manager dependency in {manifest_path} '
        f'uses override_path, but the location does not contain a valid board manager '
        f'component.\n'
        f'  override_path: {override_path_value!r}\n'
        f'  resolved to:   {resolved_repr}\n'
        f'\n'
        f'Please fix override_path to point at a directory that contains '
        f'idf_ext.py / idf_component.yml / gen_bmgr_config_codes.py, or remove '
        f'override_path to let the component manager download esp_board_manager '
        f'from the registry.\n'
    )


def _format_direct_invalid_local_path_message(
    manifest_path: Path,
    field_name: str,
    field_value: str,
    resolved_path: Path,
) -> str:
    return (
        f'{ASSIST_TAG} The declared esp_board_manager dependency in {manifest_path} '
        f'uses {field_name}, but the location does not contain a valid board manager '
        f'component.\n'
        f'  {field_name}: {field_value!r}\n'
        f'  resolved to: {resolved_path}\n'
        f'\n'
        f'Please fix {field_name} to point at a directory that contains '
        f'idf_ext.py / idf_component.yml / gen_bmgr_config_codes.py, or remove '
        f'{field_name} to let the component manager download esp_board_manager '
        f'from the registry.\n'
    )


def _format_direct_download_failure_message(
    manifest_path: Path,
    key: str,
    dependency: Any,
    cause: BaseException,
) -> str:
    return (
        f'{ASSIST_TAG} Failed to download the declared esp_board_manager dependency.\n'
        f'\n'
        f'  Declared in: {manifest_path}\n'
        f'  Declared as: {key}: {dependency!r}\n'
        f'\n'
        f'{ASSIST_TAG} Underlying cause:\n'
        f'  {type(cause).__name__}: {cause}\n'
    )


def _format_direct_post_download_missing_message(
    manifest_path: Path,
    dependency: Any,
) -> str:
    return (
        f'{ASSIST_TAG} Downloaded esp_board_manager but could not locate it on disk '
        f'afterwards. This usually indicates a broken managed_components/ state.\n'
        f'  Declared in: {manifest_path}\n'
        f'  Declared as: {dependency!r}\n'
    )


def _format_discovery_failure_message(
    cause: BaseException, project_path: Path
) -> str:
    # Re-resolve target info at format time so the hint reflects exactly what the
    # bootstrap handed to component manager (`resolve_bootstrap_target` is idempotent
    # and cheap; the temp env has already been torn down by the time we format).
    target_info = resolve_bootstrap_target(project_path)
    return (
        f'{ASSIST_TAG} Failed to resolve project dependency graph while looking for '
        f'esp_board_manager.\n'
        f'\n'
        f'{ASSIST_TAG} Underlying cause:\n'
        f'  {type(cause).__name__}: {cause}\n'
        f'\n'
        f'{ASSIST_TAG} Hints:\n'
        f'  - esp-bmgr-assist triggers the project-wide dependency resolution with '
        f'IDF_TARGET={target_info.target} (source: {target_info.source}). '
        f'Any component in your project that is incompatible with this target can '
        f'make the resolution fail here even though the issue is not in '
        f'esp_board_manager itself.\n'
        f'  - To work around this, either:\n'
        f'      * Run `idf.py set-target <target>` first so sdkconfig is populated '
        f'with your real target; the next run downloads components with that '
        f'target instead of the assist fallback.\n'
        f'      * Or declare `espressif/esp_board_manager` directly in '
        f'main/idf_component.yml so bmgr is fetched through a minimal manifest '
        f'and the full project graph is not touched.\n'
    )


def _format_project_not_depending_message() -> str:
    return (
        f'{ASSIST_TAG} Project does not depend on esp_board_manager '
        f'(directly or transitively).\n'
        f'Add `espressif/esp_board_manager` to main/idf_component.yml, or include a '
        f'package that pulls it in.\n'
    )


def _direct_download_bmgr(
    project_path: Path,
    manifest_path: Path,
    key: str,
    dependency: Any,
) -> Optional[Path]:
    """Download esp_board_manager using a minimal temporary manifest (Direct Path).

    Local ``override_path`` / ``path`` declarations are resolved directly against the
    declaring manifest and never rewritten into a temporary downloadable manifest.
    """
    local_decl = _resolve_declared_local_bmgr(manifest_path, dependency)
    if local_decl is not None:
        field_name, local_candidate = local_decl
        field_value = dependency.get(field_name) if isinstance(dependency, Mapping) else None
        if _is_bmgr_component(local_candidate):
            _debug(
                f'Direct Path: {field_name}={field_value!r} resolved to local board manager '
                f'{local_candidate}; no download needed'
            )
            return local_candidate

        if field_name == 'override_path':
            _debug(
                f'Direct Path: override_path={field_value!r} resolved to '
                f'{local_candidate} did not pass Layer 0 validation; refusing registry fallback'
            )
            raise MissingBoardManagerDependencyError(
                _format_direct_invalid_override_message(
                    manifest_path, str(field_value), local_candidate
                )
            )

        _debug(
            f'Direct Path: path={field_value!r} resolved to '
            f'{local_candidate} did not pass Layer 0 validation; refusing registry fallback'
        )
        raise MissingBoardManagerDependencyError(
            _format_direct_invalid_local_path_message(
                manifest_path, field_name, str(field_value), local_candidate
            )
        )

    try:
        _download_bmgr_component(project_path, dependency)
    except Exception as exc:
        _debug(
            f'Direct Path download raised {type(exc).__name__}: {exc}; '
            'wrapping as MissingBoardManagerDependencyError'
        )
        raise MissingBoardManagerDependencyError(
            _format_direct_download_failure_message(manifest_path, key, dependency, exc)
        ) from exc
    return None


def _discover_or_bootstrap_bmgr(context: CliContext) -> Optional[Path]:
    discovered = find_board_manager_component(context.project_path)
    if discovered is not None:
        _debug(f'Layer 0 hit: reusing existing board manager at {discovered}; no download needed')
        return discovered

    bmgr_cmd = _resolve_context_bmgr_command(context)
    if bmgr_cmd not in BMGR_ACTION_NAMES:
        _debug(
            f'Passive mode: current command {bmgr_cmd!r} is not in '
            f'{sorted(BMGR_ACTION_NAMES)}; skipping auto-download of esp_board_manager'
        )
        return None

    _debug(
        f'Active mode: command {bmgr_cmd!r} requires esp_board_manager; '
        'entering project lock for bootstrap'
    )
    with _project_lock(context.project_path):
        discovered = find_board_manager_component(context.project_path)
        if discovered is not None:
            _debug(f'Layer 0 hit after lock: reusing board manager at {discovered}')
            return discovered

        direct_decl = _scan_direct_bmgr_declaration(context.project_path)
        if direct_decl is not None:
            manifest_path, key, dependency = direct_decl
            _debug(
                f'Taking Direct Path: {key} declared in {manifest_path} -> '
                f'{_format_dep_for_debug(dependency)}'
            )
            direct_candidate = _direct_download_bmgr(
                context.project_path, manifest_path, key, dependency
            )
            if direct_candidate is not None:
                _debug(
                    f'Direct Path complete: board manager resolved directly to {direct_candidate}'
                )
                return direct_candidate
            discovered = find_board_manager_component(context.project_path)
            if discovered is None:
                _debug(
                    'Direct Path download returned without error but '
                    'find_board_manager_component still resolves to None'
                )
                raise MissingBoardManagerDependencyError(
                    _format_direct_post_download_missing_message(manifest_path, dependency)
                )
            _debug(f'Direct Path complete: board manager now at {discovered}')
            return discovered

        _debug(
            'No direct esp_board_manager declaration in local manifests; '
            'taking Discovery Path (full project dependency graph resolution)'
        )
        try:
            _bootstrap_project_dependencies(context.project_path)
        except Exception as exc:
            _debug(
                f'Discovery Path failed: {type(exc).__name__}: {exc}; '
                'wrapping as MissingBoardManagerDependencyError'
            )
            raise MissingBoardManagerDependencyError(
                _format_discovery_failure_message(exc, context.project_path)
            ) from exc

        discovered = find_board_manager_component(context.project_path)
        if discovered is None:
            _debug(
                'Discovery Path resolution completed but esp_board_manager was not '
                'produced in the graph; project does not depend on it'
            )
            raise MissingBoardManagerDependencyError(
                _format_project_not_depending_message()
            )
        _debug(
            f'Discovery Path complete: board manager now at {discovered} '
            '(pulled in as transitive dependency)'
        )
        return discovered


def _parse_idf_version_from_cmake(idf_path: Path) -> Optional[str]:
    version_path = idf_path / 'tools' / 'cmake' / 'version.cmake'
    if not version_path.is_file():
        return None

    version_parts: Dict[str, str] = {}
    try:
        with open(version_path, encoding='utf-8') as f:
            for line in f:
                match = IDF_VERSION_CMAKE_RE.match(line)
                if match:
                    version_parts[match.group(1)] = match.group(2)
    except OSError:
        return None

    try:
        return f"{version_parts['MAJOR']}.{version_parts['MINOR']}.{version_parts['PATCH']}"
    except KeyError:
        return None


def _resolve_idf_version_for_bootstrap() -> Optional[str]:
    for env_key in ('CI_TESTING_IDF_VERSION', 'IDF_VERSION'):
        value = os.environ.get(env_key)
        if value:
            _debug(f'Bootstrap IDF_VERSION resolved to {value} (source: env:{env_key})')
            return value

    idf_path_env = os.environ.get('IDF_PATH')
    if idf_path_env:
        parsed = _parse_idf_version_from_cmake(Path(idf_path_env))
        if parsed:
            _debug(
                f'Bootstrap IDF_VERSION resolved to {parsed} '
                f'(source: version.cmake via IDF_PATH={idf_path_env})'
            )
            return parsed

    argv0 = Path(sys.argv[0]) if sys.argv else None
    if argv0 and argv0.name.lower().startswith('idf.py'):
        idf_root = argv0.resolve().parent.parent
        parsed = _parse_idf_version_from_cmake(idf_root)
        if parsed:
            _debug(
                f'Bootstrap IDF_VERSION resolved to {parsed} '
                f'(source: version.cmake via argv[0]={argv0})'
            )
            return parsed

    if argv0 and argv0.exists():
        env = os.environ.copy()
        env[SKIP_BOOTSTRAP_ENV] = '1'
        try:
            output = subprocess.check_output([sys.executable, str(argv0), '--version'], env=env)
        except Exception as exc:
            _debug(f'Failed to resolve IDF version via idf.py --version fallback: {exc}')
            return None
        if not isinstance(output, str):
            output = output.decode('utf-8', 'ignore')
        match = re.search(r'v(\d+\.\d+(?:\.\d+)?)', output)
        if match:
            _debug(
                f'Bootstrap IDF_VERSION resolved to {match.group(1)} '
                '(source: idf.py --version subprocess)'
            )
            return match.group(1)

    _debug('Bootstrap IDF_VERSION could not be resolved; leaving IDF_VERSION unset')
    return None


def _is_project_path(project_path: Path) -> bool:
    cmake_file = project_path / 'CMakeLists.txt'
    main_dir = project_path / 'main'
    if not cmake_file.exists() or not main_dir.is_dir():
        return False
    try:
        content = cmake_file.read_text(encoding='utf-8', errors='ignore')
    except OSError:
        return False
    return 'project.cmake' in content


def _canonicalize_path(path: Path) -> str:
    try:
        resolved = path.resolve(strict=False)
    except TypeError:  # pragma: no cover - for very old Python runtimes
        resolved = path.resolve()
    normalized = os.path.normpath(str(resolved))
    return os.path.normcase(normalized)


def _atomic_write_utf8(manifest_path: Path, text: str, has_bom: bool) -> None:
    encoded = text.encode('utf-8-sig' if has_bom else 'utf-8')
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=str(manifest_path.parent), delete=False) as temp_file:
        temp_file.write(encoded)
        temp_name = temp_file.name
    os.replace(temp_name, str(manifest_path))


def _raw_line_declares_bmgr_dependency(line: str) -> bool:
    stripped = line.strip()
    if not stripped or stripped.startswith('#'):
        return False
    return bool(re.match(r'(?:espressif/esp_board_manager|esp_board_manager)\s*:', stripped))


def _raw_manifest_declares_bmgr_dependency(raw: str) -> bool:
    return any(_raw_line_declares_bmgr_dependency(line) for line in raw.splitlines())


def _infer_dependencies_value_indent(raw: str) -> str:
    """Indentation of keys nested under a ``dependencies:`` mapping line (default two spaces)."""
    lines = raw.splitlines()
    for i, line in enumerate(lines):
        if re.match(r'^[ \t]*dependencies:\s*(?:#.*)?$', line):
            for j in range(i + 1, len(lines)):
                follow = lines[j]
                if not follow.strip() or follow.lstrip().startswith('#'):
                    continue
                m = re.match(r'^(\s+)\S', follow)
                if m:
                    return m.group(1)
            break
    return '  '


def _replace_empty_dependencies_scalar(raw: str, newline: str) -> Optional[str]:
    """Replace ``dependencies: {}``, ``dependencies: null`` or ``dependencies: ~`` with a multi-line mapping.

    This is a single-line edit so we do not end up with duplicate top-level ``dependencies`` keys.
    """
    patterns = (
        re.compile(r'(?m)^([ \t]*dependencies:\s*)\{\s*\}\s*$'),
        re.compile(r'(?m)^([ \t]*dependencies:\s*)null\s*$', re.IGNORECASE),
        re.compile(r'(?m)^([ \t]*dependencies:\s*)~\s*$'),
    )

    def repl(m: Any) -> str:
        base = '  '
        inner = '    '
        return (
            f'{m.group(1)}{newline}{base}{CANONICAL_BMGR_KEY}:{newline}'
            f'{inner}version: \"*\"{newline}{inner}require: public'
        )

    for pat in patterns:
        new_raw, n = pat.subn(repl, raw, count=1)
        if n:
            return new_raw
    return None


def _bmgr_dependency_child_block(entry_indent: str, newline: str) -> str:
    vers_req_indent = entry_indent + '  '
    return (
        f'{entry_indent}{CANONICAL_BMGR_KEY}:{newline}'
        f'{vers_req_indent}version: \"*\"{newline}{vers_req_indent}require: public'
    )


def _bmgr_dependency_full_block(newline: str) -> str:
    return (
        f'dependencies:{newline}'
        f'  {CANONICAL_BMGR_KEY}:{newline}'
        f'    version: \"*\"{newline}'
        f'    require: public'
    )


def _is_bmgr_component(path: Path) -> bool:
    return _is_bmgr_component_impl(path)


def _find_local_bmgr(project_path: Path) -> Optional[Path]:
    return _find_local_bmgr_impl(project_path, debug_once=_debug_locator_once)


def _resolve_declared_local_bmgr(
    manifest_path: Path, dependency: Any
) -> Optional[Tuple[str, Path]]:
    return _resolve_declared_local_bmgr_impl(manifest_path, dependency)


def _find_managed_bmgr(project_path: Path) -> Optional[Path]:
    return _find_managed_bmgr_impl(project_path, debug_once=_debug_locator_once)


def _collect_local_components(project_path: Path) -> List[Tuple[str, Path]]:
    return _collect_local_components_impl(project_path, skip_components=BOOTSTRAP_SKIP_COMPONENTS)


def _load_project_manifests(project_path: Path):
    return _load_project_manifests_impl(project_path, skip_components=BOOTSTRAP_SKIP_COMPONENTS)


@contextmanager
def _temporary_bootstrap_env(
    project_path: Path,
    *,
    include_idf_version: bool = False,
    include_skip_guard: bool = False,
):
    previous_target = os.environ.get('IDF_TARGET')
    previous_idf_version = os.environ.get('IDF_VERSION')
    previous_skip = os.environ.get(SKIP_BOOTSTRAP_ENV)
    target_result = resolve_bootstrap_target(project_path)

    try:
        # ``download_project_dependencies`` usually runs after CMake has decided the target. During
        # assist bootstrap we may not have reached that step yet, so supply the best available
        # target only for the lifetime of this context.
        if not previous_target:
            os.environ['IDF_TARGET'] = target_result.target
            _debug(
                f'Bootstrap IDF_TARGET resolved to {target_result.target} '
                f'(source: {target_result.source})'
            )
        elif _DEBUG:
            _debug(f'Bootstrap reusing existing IDF_TARGET={previous_target}')

        if include_idf_version:
            resolved_idf_version = _resolve_idf_version_for_bootstrap()
            if resolved_idf_version:
                os.environ['IDF_VERSION'] = resolved_idf_version

        if include_skip_guard:
            os.environ[SKIP_BOOTSTRAP_ENV] = '1'

        yield target_result
    finally:
        if not previous_target:
            os.environ.pop('IDF_TARGET', None)
        else:
            os.environ['IDF_TARGET'] = previous_target
        if previous_idf_version is None:
            os.environ.pop('IDF_VERSION', None)
        else:
            os.environ['IDF_VERSION'] = previous_idf_version
        if previous_skip is None:
            os.environ.pop(SKIP_BOOTSTRAP_ENV, None)
        else:
            os.environ[SKIP_BOOTSTRAP_ENV] = previous_skip


@contextmanager
def _safe_temp_dir(prefix: str):
    """Cross-platform temp-directory context with best-effort cleanup.

    ``tempfile.TemporaryDirectory`` raises on cleanup if any file inside is still held
    open -- a flake we occasionally see on Windows when antivirus tools briefly lock
    freshly-closed files. Cleanup happens after ``download_project_dependencies`` is
    already done, so a cleanup failure has no functional impact beyond leaving a few
    KB in the system temp area. We degrade to ``rmtree(ignore_errors=True)`` instead of
    letting such a flake poison the whole command. We implement this ourselves because
    ``TemporaryDirectory(ignore_cleanup_errors=True)`` is Python 3.10+ only and we
    still support 3.8.
    """
    import shutil

    temp_dir = tempfile.mkdtemp(prefix=prefix)
    try:
        yield temp_dir
    finally:
        try:
            shutil.rmtree(temp_dir, ignore_errors=False)
        except OSError as exc:
            _debug(
                f'Temp dir cleanup fell back to ignore_errors due to {type(exc).__name__}: {exc}'
            )
            shutil.rmtree(temp_dir, ignore_errors=True)


def _bootstrap_project_dependencies(project_path: Path) -> None:
    try:
        from idf_component_manager.dependencies import download_project_dependencies
        from idf_component_tools.utils import ProjectRequirements
    except ImportError as exc:
        raise RuntimeError(
            'idf-component-manager is required to resolve project dependencies for bmgr/gen-bmgr-config'
        ) from exc

    manifests = _load_project_manifests(project_path)
    if not manifests:
        _debug(f'No manifest-bearing local components found in {project_path}; skipping dependency bootstrap')
        # Reaching this code already implies the user explicitly invoked bmgr/
        # gen-bmgr-config (Discovery Path is gated by BMGR_ACTION_NAMES upstream),
        # so an unconditional notice here is safe.
        emit_notice(
            f'No idf_component.yml found under {project_path}/main or '
            f'{project_path}/components/*. Cannot resolve dependencies for bmgr.',
            dedup_key=f'no-manifest:{project_path}',
        )
        return

    managed_path = project_path / 'managed_components'
    lock_path = project_path / 'dependencies.lock'
    managed_path.mkdir(parents=True, exist_ok=True)

    _debug(
        'Resolving project dependencies from manifests: '
        + ', '.join(f'{manifest.real_name}:{manifest.path}' for manifest in manifests)
    )

    with _temporary_bootstrap_env(
        project_path,
        include_idf_version=True,
        include_skip_guard=True,
    ):
        download_project_dependencies(
            ProjectRequirements(manifests),
            str(lock_path),
            str(managed_path),
        )


def _find_bmgr_from_lock(project_path: Path) -> Optional[Path]:
    return _find_bmgr_from_lock_impl(project_path, debug=_debug, debug_once=_debug_locator_once)


def find_board_manager_component(project_path: Path) -> Optional[Path]:
    """Resolve the first usable esp_board_manager component for a project."""
    return _find_board_manager_component_impl(project_path, debug=_debug, debug_once=_debug_locator_once)


def _select_actions_delimiter(_existing_value: str) -> str:
    """Return the delimiter used when joining paths into ``IDF_EXTRA_ACTIONS_PATH``.

    ESP-IDF ``tools/idf.py`` only splits this variable on ``';'`` (all platforms).
    Do not use ``os.pathsep`` (e.g. ``':'`` on POSIX): IDF will treat a multi-path
    value as a single directory name. We always join with ``';'`` to stay consistent.
    """
    return ';'


def _split_actions_paths(existing_value: str) -> list:
    """Split ``IDF_EXTRA_ACTIONS_PATH`` the same way as ESP-IDF ``idf.py`` (``';'`` only)."""
    if not existing_value:
        return []
    return [part.strip() for part in existing_value.split(';') if part.strip()]


def _merge_actions_path(existing_value: str, actions_path: str) -> str:
    delimiter = _select_actions_delimiter(existing_value)
    merged_paths = []
    seen = set()
    for raw_path in _split_actions_paths(existing_value) + [actions_path]:
        canonical = _canonicalize_path(Path(raw_path))
        if canonical in seen:
            continue
        seen.add(canonical)
        merged_paths.append(str(Path(raw_path)))
    if not merged_paths:
        return ''
    if len(merged_paths) == 1:
        return merged_paths[0]
    return delimiter.join(merged_paths)


def _prepend_actions_path(existing_value: str, actions_path: str) -> str:
    """Put ``actions_path`` first so later paths in IDF_EXTRA_ACTIONS_PATH win in merge_action_lists."""
    delimiter = _select_actions_delimiter(existing_value)
    merged_paths = []
    seen = set()
    for raw_path in [actions_path] + _split_actions_paths(existing_value):
        canonical = _canonicalize_path(Path(raw_path))
        if canonical in seen:
            continue
        seen.add(canonical)
        merged_paths.append(str(Path(raw_path)))
    if not merged_paths:
        return ''
    if len(merged_paths) == 1:
        return merged_paths[0]
    return delimiter.join(merged_paths)


def _package_actions_dir() -> Path:
    return Path(__file__).resolve().parent


def _ensure_placeholder_ext_path_first() -> None:
    pkg_dir = str(_package_actions_dir())
    current_value = os.environ.get('IDF_EXTRA_ACTIONS_PATH', '')
    merged_value = _prepend_actions_path(current_value, pkg_dir)
    if merged_value != current_value:
        os.environ['IDF_EXTRA_ACTIONS_PATH'] = merged_value
        _debug(f'Prepended package idf.py extensions dir to IDF_EXTRA_ACTIONS_PATH: {merged_value}')


def _set_idf_extra_actions_path(actions_path: str) -> None:
    current_value = os.environ.get('IDF_EXTRA_ACTIONS_PATH', '')
    merged_value = _merge_actions_path(current_value, actions_path)
    if merged_value != current_value:
        os.environ['IDF_EXTRA_ACTIONS_PATH'] = merged_value
        _debug(f'Set IDF_EXTRA_ACTIONS_PATH: {merged_value}')


def _fallback_lock_file(project_path: Path) -> Path:
    digest = hashlib.sha256(str(project_path).encode('utf-8')).hexdigest()[:16]
    lock_dir = Path(tempfile.gettempdir()) / LOCK_FALLBACK_DIR
    lock_dir.mkdir(parents=True, exist_ok=True)
    return lock_dir / f'{digest}.lock'


def _project_lock_file(project_path: Path) -> Path:
    return project_path / LOCK_FILE_NAME


def _get_lock_file(project_path: Path) -> Path:
    try:
        return _fallback_lock_file(project_path)
    except OSError:
        if os.access(project_path, os.W_OK):
            return _project_lock_file(project_path)
        raise


def _is_lock_fs_error(exc: OSError) -> bool:
    return exc.errno in (errno.EROFS, errno.EACCES, errno.EPERM)


def _is_lock_timeout_error(exc: Exception) -> bool:
    if FileLockTimeout is not None and isinstance(exc, FileLockTimeout):
        return True
    return exc.__class__.__name__ == 'Timeout'


def _resolve_lock_timeout(default: float = DEFAULT_LOCK_TIMEOUT_SEC) -> float:
    raw_value = os.environ.get(LOCK_TIMEOUT_ENV, '').strip()
    if not raw_value:
        return default
    try:
        timeout = float(raw_value)
    except ValueError:
        emit_notice(
            f'Ignoring invalid {LOCK_TIMEOUT_ENV}={raw_value!r}; using {default:g}s.',
            dedup_key=f'invalid-lock-timeout:{raw_value}',
        )
        return default
    if timeout <= 0:
        emit_notice(
            f'Ignoring non-positive {LOCK_TIMEOUT_ENV}={raw_value!r}; using {default:g}s.',
            dedup_key=f'non-positive-lock-timeout:{raw_value}',
        )
        return default
    return timeout


def _format_lock_diagnostics(project_path: Path, lock_path: Path, timeout: float) -> str:
    return (
        f'project: {project_path}\n'
        f'lock: {lock_path}\n'
        f'timeout: {timeout:g}s\n'
        'If this waits unexpectedly, check the process holding the lock:\n'
        f'  lsof {lock_path}\n'
        f'  fuser -v {lock_path}'
    )


def _emit_waiting_for_lock_notice(project_path: Path, lock_path: Path, timeout: float) -> None:
    emit_notice(
        'Waiting for board manager bootstrap lock.\n'
        + _format_lock_diagnostics(project_path, lock_path, timeout),
        dedup_key=f'waiting-lock:{lock_path}:{timeout:g}',
    )


@contextmanager
def _project_lock(project_path: Path, timeout: Optional[float] = None):
    if FileLock is None:
        _emit_missing_filelock_notice_once()
        yield
        return

    if timeout is None:
        timeout = _resolve_lock_timeout()

    try:
        primary_path = _get_lock_file(project_path)
    except OSError as exc:
        if not _is_lock_fs_error(exc):
            raise
        _debug(
            f'Lock file path is unavailable for {project_path}; proceeding without '
            f'inter-process locking ({exc})'
        )
        yield
        return

    primary_lock = FileLock(str(primary_path))
    try:
        # A lock is only needed for the active bootstrap path. It prevents two ``idf.py bmgr``
        # processes from downloading or rewriting dependency state for the same project together.
        _emit_waiting_for_lock_notice(project_path, primary_path, timeout)
        with primary_lock.acquire(timeout=timeout):
            yield
    except OSError as exc:
        if _is_lock_timeout_error(exc):
            raise RuntimeError(
                f'Timed out waiting for board manager bootstrap lock.\n'
                + _format_lock_diagnostics(project_path, primary_path, timeout)
                + '\nIf no process owns the lock, remove the lock file and retry.'
            ) from exc
        if not _is_lock_fs_error(exc):
            raise
        fallback_path = _project_lock_file(project_path)
        if primary_path != fallback_path and os.access(project_path, os.W_OK):
            fallback_lock = FileLock(str(fallback_path))
            _emit_waiting_for_lock_notice(project_path, fallback_path, timeout)
            with fallback_lock.acquire(timeout=timeout):
                yield
            return
        _debug(
            f'Lock file is not writable for {project_path}; proceeding without '
            f'inter-process locking ({exc})'
        )
        yield


def _ensure_bmgr_dependency(manifest_path: Path) -> Dict[str, Any]:
    raw_stored: Optional[str] = None
    if manifest_path.exists():
        raw_stored, _, _ = _read_manifest_text(manifest_path)

    data, newline, has_bom, _backend = _load_manifest_document(manifest_path)
    dependencies = data.get('dependencies')

    if isinstance(dependencies, dict):
        existing = _find_bmgr_dependency(dependencies)
        if existing:
            return dependencies
    elif dependencies is not None:
        raise RuntimeError(f'Manifest {manifest_path} has a non-mapping dependencies section')

    raw_for_scan = raw_stored or ''
    if _raw_manifest_declares_bmgr_dependency(raw_for_scan):
        return _load_dependencies(manifest_path)

    merged_return: Dict[str, Any]
    if isinstance(dependencies, dict):
        merged_return = dict(dependencies)
        merged_return[CANONICAL_BMGR_KEY] = dict(DEFAULT_DEPENDENCY)
    else:
        merged_return = {CANONICAL_BMGR_KEY: dict(DEFAULT_DEPENDENCY)}

    # Prefer a targeted text edit when the manifest already has a ``dependencies`` key. This keeps
    # comments, ordering and the user's formatting more intact than dumping the whole YAML again.
    replaced = _replace_empty_dependencies_scalar(raw_for_scan, newline)
    if replaced is not None:
        _atomic_write_utf8(manifest_path, replaced, has_bom)
        _debug(f'Expanded empty dependencies mapping with {CANONICAL_BMGR_KEY} in {manifest_path}')
        return merged_return

    has_dependencies_mapping = isinstance(dependencies, dict)
    raw_out = raw_for_scan

    if has_dependencies_mapping:
        entry_indent_p = _infer_dependencies_value_indent(raw_out)
        appendage = _bmgr_dependency_child_block(entry_indent_p, newline)
    else:
        appendage = _bmgr_dependency_full_block(newline)

    if raw_out and not raw_out.endswith(('\n', '\r\n')):
        raw_out += newline
    raw_out += appendage
    if not raw_out.endswith(newline):
        raw_out += newline

    _atomic_write_utf8(manifest_path, raw_out, has_bom)
    _debug(f'Appended {CANONICAL_BMGR_KEY} dependency at EOF in {manifest_path}')
    return merged_return


def _get_or_create_manifest(project_path: Path) -> Path:
    manifest_path = _find_manifest_file(project_path)
    if manifest_path:
        return manifest_path
    main_dir = project_path / 'main'
    if not main_dir.is_dir():
        raise RuntimeError(f'Project {project_path} does not contain a main directory')
    return main_dir / MANIFEST_NAMES[0]


def _to_builtin_value(value: Any) -> Any:
    if value is None or isinstance(value, (bool, int, float)):
        return value
    if isinstance(value, Path):
        return str(value)
    if isinstance(value, str):
        return str(value)
    if isinstance(value, Mapping):
        return {str(_to_builtin_value(key)): _to_builtin_value(item) for key, item in value.items()}
    if isinstance(value, (list, tuple, set)):
        return [_to_builtin_value(item) for item in value]
    return str(value)


def _sanitize_download_dependency(dependency: Any) -> Any:
    if dependency is None:
        return dict(DEFAULT_DEPENDENCY)
    if isinstance(dependency, str):
        return str(dependency)
    if not isinstance(dependency, Mapping):
        return dict(DEFAULT_DEPENDENCY)

    sanitized = {}
    for key in (
        'version',
        'git',
        'path',
        'override_path',
        'registry_url',
        'service_url',
        'pre_release',
        'require',
        'public',
    ):
        value = dependency.get(key)
        if value is not None:
            sanitized[key] = _to_builtin_value(value)

    sanitized.setdefault('version', '*')
    if 'require' not in sanitized and 'public' not in sanitized:
        sanitized['require'] = 'public'
    return sanitized


def _load_download_dependency(manifest_path: Path) -> Any:
    dependencies = _load_dependencies(manifest_path)
    result = _find_bmgr_dependency(dependencies)
    if not result:
        return dict(DEFAULT_DEPENDENCY)
    _, dependency = result
    return _sanitize_download_dependency(dependency)


def _download_bmgr_component(project_path: Path, dependency: Any) -> None:
    """Fetch esp_board_manager using a throwaway manifest that declares only bmgr.

    ``_sanitize_download_dependency`` restricts the payload to registry-relevant keys
    (``version`` / ``require`` / ``registry_url`` / ``service_url`` / ``pre_release``) so
    ``matches:`` / ``rules:`` conditions are implicitly dropped -- the Direct Path
    treats the user's declaration as unconditional intent. The temporary manifest and
    its lock file live inside a ``TemporaryDirectory`` so we never mutate the project's
    own ``dependencies.lock``; only the actual component lands under the project's
    ``managed_components/``.
    """
    try:
        from idf_component_manager.dependencies import download_project_dependencies
        from idf_component_tools.manager import ManifestManager
        from idf_component_tools.utils import ProjectRequirements
    except ImportError as exc:
        raise RuntimeError(
            'idf-component-manager is required to auto-download esp_board_manager for bmgr/gen-bmgr-config'
        ) from exc

    managed_path = project_path / 'managed_components'
    managed_path.mkdir(parents=True, exist_ok=True)
    _debug(f'Direct Path: download target dir {managed_path}')

    with _temporary_bootstrap_env(project_path):
        with _safe_temp_dir('esp_bmgr_direct_') as temp_dir:
            temp_root = Path(temp_dir)
            temp_manifest = temp_root / MANIFEST_NAMES[0]
            temp_lock = temp_root / 'dependencies.lock'
            sanitized = _sanitize_download_dependency(dependency)
            _debug(
                f'Direct Path: sanitized dep={_format_dep_for_debug(sanitized)}; '
                f'temp manifest={temp_manifest}; temp lock={temp_lock}'
            )
            manifest_payload = {
                'dependencies': {
                    CANONICAL_BMGR_KEY: sanitized,
                }
            }
            temp_manifest.write_text(
                yaml.safe_dump(_to_builtin_value(manifest_payload), sort_keys=False, default_flow_style=False),
                encoding='utf-8',
            )

            manifest = ManifestManager(str(temp_root), 'temp').load()
            download_project_dependencies(
                ProjectRequirements([manifest]),
                str(temp_lock),
                str(managed_path),
            )
            _debug(
                'Direct Path: component manager finished resolving minimal manifest; '
                f'temp dir will be cleaned up, project dependencies.lock untouched'
            )


def _warn_preset_extra_paths_invalid(preset: str) -> None:
    for raw in _split_actions_paths(preset):
        entry = Path(raw).expanduser()
        if not entry.is_dir():
            print(
                f'{ASSIST_TAG} WARNING: IDF_EXTRA_ACTIONS_PATH entry is not an existing directory: {raw!r}',
                file=sys.stderr,
            )


def _maybe_merge_discovered_bmgr(candidate: Path, preset_non_empty: bool) -> None:
    if not preset_non_empty:
        _set_idf_extra_actions_path(str(candidate))
        return
    discovered = _canonicalize_path(candidate)
    for raw in _split_actions_paths(os.environ.get('IDF_EXTRA_ACTIONS_PATH', '')):
        if _canonicalize_path(Path(raw)) == discovered:
            return
    print(
        f'{ASSIST_TAG} WARNING: Discovered board manager '
        f'{candidate} is not among IDF_EXTRA_ACTIONS_PATH entries '
        '(preset may point at a different clone).',
        file=sys.stderr,
    )


def _actions_path_contains_candidate(candidate: Path) -> bool:
    discovered = _canonicalize_path(candidate)
    for raw in _split_actions_paths(os.environ.get('IDF_EXTRA_ACTIONS_PATH', '')):
        if _canonicalize_path(Path(raw)) == discovered:
            return True
    return False


def _should_reexec_after_bootstrap(
    context: CliContext,
    had_bmgr_before: bool,
    discovered: Optional[Path],
) -> bool:
    if had_bmgr_before or discovered is None:
        return False
    if os.environ.get(REEXEC_AFTER_BOOTSTRAP_ENV) == '1':
        return False
    if _resolve_context_bmgr_command(context) not in BMGR_ACTION_NAMES:
        return False
    return _actions_path_contains_candidate(discovered)


def _reexec_after_bootstrap(context: CliContext) -> None:
    env = os.environ.copy()
    env[REEXEC_AFTER_BOOTSTRAP_ENV] = '1'
    _debug(
        'Re-executing idf.py after bootstrap so bmgr runs with the real board manager extension'
    )
    if sys.platform == 'win32':
        # Windows CRT exec can crash and does not wait for the replacement process.
        # Use CreateProcess via subprocess, preserving stdio and the final exit status.
        sys.stdout.flush()
        sys.stderr.flush()
        result = subprocess.run([sys.executable, *context.argv], env=env, check=False)
        # Python 3.11 converts SystemExit through a signed Windows C long.
        status = result.returncode
        if status >= 0x80000000:
            status -= 0x100000000
        raise SystemExit(status)
    os.execvpe(sys.executable, [sys.executable, *context.argv], env)


def _bootstrap_for_context(context: CliContext) -> None:
    if not context.is_idf_command:
        return

    if not _is_project_path(context.project_path):
        _debug(
            f'Skipping bootstrap: {context.project_path} is not an ESP-IDF project '
            f'(missing CMakeLists.txt with project.cmake, or missing main/)'
        )
        # Only escalate to stderr when the user is explicitly invoking bmgr.
        # For incidental commands (`idf.py --version`, `idf.py help`, etc.)
        # this branch is the normal "nothing to do" exit and a notice would be
        # pure noise.
        bmgr_cmd = _resolve_context_bmgr_command(context)
        if bmgr_cmd in BMGR_ACTION_NAMES and not _is_bmgr_component(context.project_path):
            # Skip the notice when cwd is itself a valid bmgr component directory:
            # that's the bmgr-developer workflow (e.g. running `bmgr -l` from
            # inside esp_board_manager/), and bmgr usually still works because
            # idf.py can pick up idf_ext.py from cwd directly.
            emit_notice(
                f'{context.project_path} is not an ESP-IDF project root '
                f'(missing CMakeLists.txt with project.cmake, or missing main/). '
                f'Skipped bmgr bootstrap.',
                dedup_key=f'not-project:{context.project_path}',
            )
        return

    # Snapshot before prepend: if the user already exported ``IDF_EXTRA_ACTIONS_PATH``, keep that
    # choice and only add the assist package path that exposes the placeholder extension.
    preset_initial = os.environ.get('IDF_EXTRA_ACTIONS_PATH', '').strip()
    if preset_initial:
        _warn_preset_extra_paths_invalid(preset_initial)

    _ensure_placeholder_ext_path_first()

    if _DEBUG:
        if preset_initial:
            _debug(
                f'bootstrap (preset IDF_EXTRA_ACTIONS_PATH): argv={list(context.argv)} '
                f'project={context.project_path}'
            )
        else:
            _debug(f'Module loaded, argv: {list(context.argv)}')
            _debug(f'Project path: {context.project_path}')

    # Explain merge policy before locator logs (e.g. "Found board manager...") for readability.
    if preset_initial and _DEBUG:
        _debug(
            'IDF_EXTRA_ACTIONS_PATH was set before idf.py started; '
            'discovered board manager paths will not be appended to it.'
        )

    had_bmgr_before = find_board_manager_component(context.project_path) is not None
    discovered = _discover_or_bootstrap_bmgr(context)
    if discovered:
        # Passive path: for ordinary commands we only expose a component that already exists.
        _maybe_merge_discovered_bmgr(discovered, bool(preset_initial))
        if _should_reexec_after_bootstrap(context, had_bmgr_before, discovered):
            _reexec_after_bootstrap(context)
        return


def _print_cause_traceback(cause: BaseException) -> None:
    """Emit the full traceback of a chained cause to stderr.

    Called after we print the user-facing message so that the underlying exception
    (SolverError, network failures, YAML parse errors, etc.) is never hidden. We always
    print it; suppressing it forced users to re-run with ``ESP_BMGR_DEBUG=1`` just to see
    what actually failed, which is exactly the situation we want to avoid.
    """
    import traceback

    print(f'{ASSIST_TAG} Underlying cause traceback:', file=sys.stderr, flush=True)
    traceback.print_exception(
        type(cause), cause, cause.__traceback__, file=sys.stderr
    )


def _main() -> None:
    global _MAIN_EXECUTED
    if _MAIN_EXECUTED:
        return
    _MAIN_EXECUTED = True

    context = _parse_cli_context()
    if _is_shell_completion_invocation():
        if context.is_idf_command:
            _ensure_placeholder_ext_path_first()
        return
    if os.environ.get(SKIP_BOOTSTRAP_ENV) == '1':
        return
    if _is_version_query(context.argv):
        return
    _emit_import_notice_once()
    try:
        _bootstrap_for_context(context)
        maybe_warn_about_update(_resolve_context_bmgr_command(context))
    except MissingBoardManagerDependencyError as exc:
        print(str(exc), file=sys.stderr, flush=True)
        if exc.__cause__ is not None:
            _print_cause_traceback(exc.__cause__)
        raise SystemExit(1)
    except Exception:
        import traceback

        print(
            f'{ASSIST_TAG} Unexpected error during bootstrap:',
            file=sys.stderr,
            flush=True,
        )
        traceback.print_exc()
        raise SystemExit(1)


class IdfPyActionsHook(MetaPathFinder):
    """Import hook to execute _main() when idf_py_actions is imported."""

    def find_spec(self, name, path, target=None):
        if name == 'idf_py_actions':
            _main()
            return None
        return None


if not any(isinstance(entry, IdfPyActionsHook) for entry in sys.meta_path):
    sys.meta_path.insert(0, IdfPyActionsHook())
