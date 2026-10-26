# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

"""Discover PHY configuration parsers within the active board generation context."""

from contextlib import contextmanager
from contextvars import ContextVar
import hashlib
import importlib.util
from pathlib import Path
import sys

from .amend import SOURCE_SUFFIXES


_BUILTIN_DIR = Path(__file__).resolve().parents[1] / 'devices' / 'dev_ethernet' / 'ethernet_extra'
_MODULE_CACHE = {}
_CURRENT_REGISTRY = ContextVar('bmgr_phy_parser_registry', default=None)


def _load_module(script_path):
    """Cache modules by absolute source path, refreshing changed source files."""
    script_path = Path(script_path).expanduser().resolve()
    if not script_path.is_file() or script_path.suffix.lower() != '.py':
        raise ValueError(f'PHY parser extension must be an existing Python file: {script_path}')
    source = script_path.read_bytes()
    fingerprint = hashlib.sha256(source).digest()
    cached = _MODULE_CACHE.get(script_path)
    if cached is not None and cached[0] == fingerprint:
        return cached[1]

    module_name = '_bmgr_phy_parser_' + hashlib.sha256(str(script_path).encode('utf-8')).hexdigest()
    spec = importlib.util.spec_from_file_location(module_name, script_path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f'Failed to load PHY parser extension: {script_path}')
    module = importlib.util.module_from_spec(spec)
    previous_module = sys.modules.get(module_name)
    sys.modules[module_name] = module
    try:
        # Read the source directly so quick edits cannot reuse stale .pyc files.
        exec(compile(source, str(script_path), 'exec'), module.__dict__)
    except Exception:
        if previous_module is None:
            sys.modules.pop(module_name, None)
        else:
            sys.modules[module_name] = previous_module
        raise
    _MODULE_CACHE[script_path] = (fingerprint, module)
    return module


def _validate_descriptor(chip, descriptor, script_path):
    """Validate configuration parsers and runtime sources before any device uses them."""
    label = f'PHY parser descriptor {chip!r} in {script_path}'
    if not isinstance(chip, str) or not chip:
        raise ValueError(f'{label} needs a non-empty chip name')
    if not isinstance(descriptor, dict) or not descriptor:
        raise ValueError(f'{label} must be a non-empty mapping')
    if set(descriptor) - {'parse', 'sources'}:
        raise ValueError(f'{label} supports only parse and sources')
    callback = descriptor.get('parse')
    if 'parse' in descriptor and not callable(callback):
        raise ValueError(f'{label} parse must be callable')
    source_paths = descriptor.get('sources', [])
    if not isinstance(source_paths, list):
        raise ValueError(f'{label} sources must be a list of source paths')
    sources = []
    for source_path in source_paths:
        if not isinstance(source_path, str) or not source_path:
            raise ValueError(f'{label} sources must contain non-empty source paths')
        source = (script_path.parent / source_path).resolve()
        if not source.is_file():
            raise ValueError(f'{label} source does not exist: {source}')
        if source.suffix.lower() not in SOURCE_SUFFIXES:
            raise ValueError(f'{label} source must be C, C++ or assembly: {source}')
        if str(source) not in sources:
            sources.append(str(source))
    return {'parse': callback, 'sources': sources}


def _directory_scripts(directory):
    """A file is a container: it can publish any number of PHY chip parsers."""
    directory = Path(directory).expanduser().resolve()
    return sorted(path for path in directory.glob('*.py') if path.is_file() and not path.name.startswith('_'))


def _apply_scope(registry, script_paths):
    seen_chips = {}
    seen_paths = set()
    for script_path in script_paths:
        script_path = Path(script_path).expanduser().resolve()
        if script_path in seen_paths:
            continue
        seen_paths.add(script_path)
        module = _load_module(script_path)
        getter = getattr(module, 'get_phy_parsers', None)
        if not callable(getter):
            raise ValueError(f'PHY parser extension {script_path} must define get_phy_parsers()')
        descriptors = getter()
        if not isinstance(descriptors, dict):
            raise ValueError(f'PHY parser extension {script_path} get_phy_parsers() must return a mapping')
        for chip, descriptor in descriptors.items():
            normalized = _validate_descriptor(chip, descriptor, script_path)
            if chip in seen_chips:
                raise ValueError(f'Duplicate PHY parser for {chip!r}: {seen_chips[chip]} and {script_path}')
            seen_chips[chip] = script_path
            registry[chip] = normalized


def _build_registry(board_dir=None, python_files=()):
    registry = {}
    _apply_scope(registry, _directory_scripts(_BUILTIN_DIR))
    if board_dir is not None:
        _apply_scope(registry, _directory_scripts(Path(board_dir) / 'ethernet_extra'))
    # Each explicitly listed amendment file has its own ordered precedence.
    for script_path in python_files:
        _apply_scope(registry, [script_path])
    return registry


def current_phy_parser_registry():
    """Return the active registry, or built-in parsers outside generation contexts."""
    registry = _CURRENT_REGISTRY.get()
    return _build_registry() if registry is None else registry


@contextmanager
def phy_parser_context(board_dir=None, python_files=()):
    """Select PHY extensions for one generation, restoring any enclosing context.

    Precedence is built-in, selected board ``ethernet_extra``, then explicitly
    listed amendment files in order. Repeated chips within one
    directory are errors instead of depending on filename traversal order.
    """
    registry = _build_registry(board_dir, python_files)
    token = _CURRENT_REGISTRY.set(registry)
    try:
        yield registry
    finally:
        _CURRENT_REGISTRY.reset(token)
