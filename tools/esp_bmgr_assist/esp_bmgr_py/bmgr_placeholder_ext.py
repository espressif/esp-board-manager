"""
idf.py extension stub for bmgr/gen-bmgr-config before esp_board_manager is available.

Loaded from the esp-bmgr-assist package directory (via IDF_EXTRA_ACTIONS_PATH). Options must stay
compatible with espressif/esp_board_manager idf_ext.py so flags like -l parse before the real
extension overwrites this action.
"""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path
from typing import Any, Dict, Optional

from esp_bmgr_py import preflight, project_integration

# Keep in sync with esp_board_manager idf_ext.py (flag names / types).
BMGR_PLACEHOLDER_OPTIONS: Any = [
    {'names': ['-l', '--list-boards'], 'help': 'List all available boards and exit', 'is_flag': True},
    {'names': ['-b', '--board'], 'help': 'Specify board name, index, or directory path', 'type': str},
    {'names': ['-c', '--customer-path'], 'help': 'Path to customer boards directory (use "NONE" to skip)', 'type': str},
    {
        'names': ['-a', '--amend'],
        'help': (
            'Path to an amend directory containing a board_amend.yaml manifest. '
            'The manifest declares an ordered list of fragments (YAML / C / directory) to '
            'apply on top of the selected board; later entries override earlier ones.'
        ),
        'type': str,
    },
    {'names': ['-x', '--clean'], 'help': 'Clean generated files', 'is_flag': True},
    {'names': ['-n', '--new-board'], 'help': 'Create a new board', 'type': str},
    {'names': ['--peripherals-only'], 'help': 'Only process peripherals', 'is_flag': True},
    {'names': ['--devices-only'], 'help': 'Only process devices', 'is_flag': True},
    {'names': ['--kconfig-only'], 'help': 'Only generate Kconfig menu', 'is_flag': True},
    {
        'names': ['--skip-sdkconfig-check'],
        'help': 'Skip CONFIG_IDF_TARGET and ESP_BOARD_* sdkconfig preflight warnings in idf_ext',
        'is_flag': True,
    },
    {
        'names': ['--log-level'],
        'help': 'Set the log level (DEBUG, INFO, WARNING, ERROR)',
        'type': str,
        'default': 'INFO',
    },
]
GEN_BMGR_CONFIG_PLACEHOLDER_OPTIONS = BMGR_PLACEHOLDER_OPTIONS
ASSIST_PREFLIGHT_GLOBAL_OPTIONS: Any = [
    {
        'names': ['--bmgr-preflight-level'],
        'help': 'Set assist preflight level: error/2 aborts (default), warning/1 logs only, silent/0 hides output',
        'type': str,
        'default': None,
    },
]


def _placeholder_bmgr_callback(target_name: str, ctx, args, **kwargs) -> None:
    from idf_py_actions.errors import FatalError

    raise FatalError(
        'esp_board_manager is missing or failed to load.\n'
        '- Run `idf.py bmgr` once without extra flags to finish downloading '
        '`managed_components/espressif__esp_board_manager`, or use the legacy '
        '`idf.py gen-bmgr-config`, or add a local component under `components/esp_board_manager`.\n'
        '- With ESP_BMGR_DEBUG=1, check bootstrap logs from esp-bmgr-assist.\n'
        '- If the component exists but you still see this message, verify '
        '`managed_components/espressif__esp_board_manager/idf_ext.py` imports cleanly.'
    )


def _fake_actions(callback) -> Dict[str, Any]:
    # Register a minimal action table so ``idf.py`` can parse bmgr options even before the real
    # board manager extension exists on disk.
    return {
        'global_action_callbacks': [callback],
        'global_options': list(ASSIST_PREFLIGHT_GLOBAL_OPTIONS),
        'actions': {
            'bmgr': {
                'callback': _placeholder_bmgr_callback,
                'options': BMGR_PLACEHOLDER_OPTIONS,
                'short_help': 'Run ESP Board Manager',
            },
            'gen-bmgr-config': {
                'callback': _placeholder_bmgr_callback,
                'options': BMGR_PLACEHOLDER_OPTIONS,
                'short_help': 'Generate ESP Board Manager configuration files',
            },
        },
    }


def _attach_assist_preflight(actions: Dict[str, Any], callback) -> Dict[str, Any]:
    result: Dict[str, Any] = dict(actions)
    callbacks = list(actions.get('global_action_callbacks', []))
    # Run assist preflight first so obvious problems can stop the command before the real action
    # spends time entering a build flow.
    callbacks.insert(0, callback)
    result['global_action_callbacks'] = callbacks
    global_options = list(actions.get('global_options', []))
    global_options.extend(ASSIST_PREFLIGHT_GLOBAL_OPTIONS)
    result['global_options'] = global_options
    return result


def _load_module(file_path: Path, module_name: Optional[str] = None):
    module_name = module_name or file_path.stem
    spec = importlib.util.spec_from_file_location(module_name, file_path)
    if spec is None or spec.loader is None:
        raise ImportError(f'Failed to create spec for {file_path}')
    mod = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = mod
    spec.loader.exec_module(mod)
    return mod


def _has_preloaded_bmgr_extension(root: Path) -> bool:
    expected = (root / 'idf_ext.py').resolve()
    loaded = sys.modules.get('idf_ext')
    loaded_path = getattr(loaded, '__file__', None)
    if not loaded_path:
        return False
    try:
        return Path(loaded_path).resolve() == expected
    except OSError:
        return False


def _create_assist_callback(project_path: str):
    def assist_callback(ctx, global_args, tasks) -> None:
        del ctx  # unused, kept for idf.py callback signature parity
        preflight.run_preflight(project_path, global_args, tasks)

    return assist_callback


def _resolve_actions_with_assist(base_actions: Dict[str, Any], project_path: str, root: Optional[Path]) -> Dict[str, Any]:
    assist_callback = _create_assist_callback(project_path)
    if root is None:
        return _fake_actions(assist_callback)

    if _has_preloaded_bmgr_extension(root):
        # ``idf.py`` may already have loaded the real extension from another actions path entry.
        # In that case only inject the assist callback and avoid importing the module twice.
        return _attach_assist_preflight({}, assist_callback)

    bmgr_config = _load_module(root / 'idf_ext.py')
    return _attach_assist_preflight(
        bmgr_config.action_extensions(base_actions, project_path),
        assist_callback,
    )


def action_extensions(base_actions: Dict, project_path: str) -> Dict:
    proj = Path(project_path)
    root = project_integration.find_board_manager_root(proj)
    return _resolve_actions_with_assist(base_actions, project_path, root)
