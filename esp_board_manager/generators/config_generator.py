# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

"""
Configuration generator for ESP Board Manager
"""

import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Any

from .utils.logger import LoggerMixin
from .utils.file_utils import (
    normalize_project_dir,
    parse_customer_paths,
    resolve_project_root_dir,
    safe_write_file,
    should_skip_directory,
)
from .utils.main_idf_override import collect_main_override_board_paths
from .utils.yaml_utils import load_yaml_safe
from .amend import MANIFEST_FILENAME as AMEND_MANIFEST_FILENAME
from .settings import BoardManagerConfig
sys.path.insert(0, str(Path(__file__).parent.parent))

from .parser_loader import load_parsers
from .peripheral_parser import PeripheralParser
from .device_parser import DeviceParser
from .name_validator import parse_component_name


@dataclass(frozen=True)
class BoardDirectory:
    """A discovered board directory with source information for UI and stable ordering."""

    name: str
    path: str
    source: str
    source_type: str
    chip: Optional[str] = None


class UnknownBoardListFilterError(ValueError):
    """Raised when ``idf.py bmgr -l FILTER`` does not match chip or component tokens."""


class AmbiguousBoardSelectorError(ValueError):
    """Raised when a qualified ``-b`` selector matches more than one record."""


def records_in_scan_order(board_infos: Dict[str, BoardDirectory]) -> List[BoardDirectory]:
    return list(board_infos.values())


def select_last_named_board(board_infos: Dict[str, BoardDirectory], board_name: str) -> Optional[BoardDirectory]:
    matches = [info for info in records_in_scan_order(board_infos) if info.name == board_name]
    return matches[-1] if matches else None


def _board_sort_key(info: BoardDirectory) -> Tuple[int, str, str]:
    source_type_order = {'component': 0, 'customer': 1}
    return (
        source_type_order.get(info.source_type, 9),
        info.source.lower(),
        info.name.lower(),
    )


def indexed_board_infos(board_infos: Dict[str, BoardDirectory]) -> List[Tuple[int, BoardDirectory]]:
    """Assign 1-based global indices from the unfiltered list-view order."""
    ordered = sorted(board_infos.values(), key=_board_sort_key)
    return list(enumerate(ordered, start=1))


def _filter_tokens(info: BoardDirectory) -> List[str]:
    tokens = []
    if info.chip:
        tokens.append(info.chip.lower())
    if info.source:
        source = info.source.lower()
        tokens.append(source)
        tokens.append(source.rsplit('/', 1)[-1])
    if info.source_type == 'customer':
        tokens.append('customer')
    return tokens


def board_matches_list_filter(info: BoardDirectory, filter_text: str) -> bool:
    needle = filter_text.strip().lower()
    return needle in _filter_tokens(info)


def filter_indexed_boards(
    indexed_boards: List[Tuple[int, BoardDirectory]],
    filter_text: str,
) -> List[Tuple[int, BoardDirectory]]:
    needle = filter_text.strip().lower()
    chips = sorted({
        info.chip
        for _, info in indexed_boards
        if info.chip
    }, key=str.lower)
    components = sorted({
        info.source
        for _, info in indexed_boards
        if info.source
    }, key=str.lower)
    known = {token for _, info in indexed_boards for token in _filter_tokens(info)}
    if needle not in known:
        chip_text = ', '.join(chips) if chips else '(none)'
        component_text = ', '.join(components) if components else '(none)'
        raise UnknownBoardListFilterError(
            f'Unknown board list filter "{filter_text}".\n'
            f'Available chips: {chip_text}\n'
            f'Available components: {component_text}'
        )
    return [
        (idx, info)
        for idx, info in indexed_boards
        if board_matches_list_filter(info, filter_text)
    ]


def format_list_boards_header(matched_count: int, filter_text: Optional[str], global_total: int) -> str:
    if filter_text:
        return (
            f'Found {matched_count} matching board(s) '
            f'(filter: {filter_text}, global total: {global_total})'
        )
    return f'Found {matched_count} board(s):'


def build_list_board_output(
    board_infos: Dict[str, BoardDirectory],
    filter_text: Optional[str] = None,
    project_root: Optional[str] = None,
) -> Tuple[str, List[str]]:
    """Build the list-boards header and grouped lines, applying an optional filter."""
    indexed = indexed_board_infos(board_infos)
    global_total = len(indexed)
    needle = filter_text.strip() if filter_text else None
    if needle:
        indexed = filter_indexed_boards(indexed, needle)
    header = format_list_boards_header(len(indexed), needle, global_total)
    return header, format_board_groups(indexed, project_root=project_root)


def _read_board_chip(board_path: str) -> Optional[str]:
    """Read optional ``chip`` from ``board_info.yaml``. Missing chip is not an error."""
    board_info_path = os.path.join(board_path, 'board_info.yaml')
    board_info = load_yaml_safe(board_info_path)
    if not isinstance(board_info, dict) or not board_info:
        return None
    chip = board_info.get('chip')
    if chip is None:
        return None
    chip_text = str(chip).strip()
    return chip_text or None


def _board_record_key(board_path: str) -> str:
    return str(Path(board_path).resolve())


def _resolved_log_path(path) -> str:
    """Return an absolute, resolved path for user-visible logs."""
    return str(Path(path).resolve())


@dataclass(frozen=True)
class AutoAmendDirectory:
    """An auto-discovered amend directory for the selected board.

    ``path`` is the absolute directory containing ``board_amend.yaml`` and whose
    basename matches the selected board name. ``source`` is a short label used
    for logging / diagnostics (e.g. ``'component'`` or ``'customer'``).
    """

    path: str
    source: str


def _customer_root_from_record(info: BoardDirectory) -> Optional[Path]:
    """Recover the ``-c`` root from ``path`` and ``source`` without a new field."""
    if info.source_type != 'customer' or not info.source:
        return None
    board = Path(info.path)
    source_parts = info.source.split('/')
    relative_parent = Path(*source_parts[1:]) if len(source_parts) > 1 else Path('.')
    if relative_parent == Path('.'):
        if len(source_parts) == 1 and source_parts[0] == board.name:
            return board
        return board.parent
    depth = len(relative_parent.parts)
    if depth >= len(board.parents):
        return None
    return board.parents[depth]


def _customer_group_label(
    source: str,
    entries: List[Tuple[int, BoardDirectory]],
    project_root: Optional[str],
) -> Dict[str, str]:
    """Map resolved customer root -> display label. Suffix appears only on collisions."""
    roots = []
    for _, info in entries:
        root = _customer_root_from_record(info)
        if root is not None and root not in roots:
            roots.append(root)
    labels = {}
    collide = len(roots) > 1
    for root in roots:
        if not collide:
            labels[str(root)] = source
            continue
        if project_root:
            suffix = Path(os.path.relpath(root, project_root)).as_posix()
        else:
            suffix = root.as_posix()
        labels[str(root)] = f'{source} ({suffix})'
    return labels


def format_board_groups(
    indexed_boards: List[Tuple[int, BoardDirectory]],
    project_root: Optional[str] = None,
) -> List[str]:
    """Format discovered boards for CLI list output using provided global indices."""
    if not indexed_boards:
        return []

    ordered = sorted(indexed_boards, key=lambda item: _board_sort_key(item[1]))
    groups: Dict[Tuple[str, str], List[Tuple[int, BoardDirectory]]] = {}
    for idx, info in ordered:
        groups.setdefault((info.source_type, info.source), []).append((idx, info))

    lines: List[str] = []
    component_groups = [
        (source, entries)
        for (source_type, source), entries in groups.items()
        if source_type == 'component'
    ]
    if component_groups:
        lines.append('Board Components:')
        for source, entries in component_groups:
            lines.append(f'  {source}:')
            for idx, info in entries:
                lines.append(f'    [{idx}] {info.name}')
            lines.append('')

    customer_groups = [
        (source, entries)
        for (source_type, source), entries in groups.items()
        if source_type == 'customer'
    ]
    if customer_groups:
        lines.append('Customer Boards:')
        for source, entries in customer_groups:
            labels = _customer_group_label(source, entries, project_root)
            by_root: Dict[str, List[Tuple[int, BoardDirectory]]] = {}
            for idx, info in entries:
                root = _customer_root_from_record(info)
                root_key = str(root) if root is not None else ''
                by_root.setdefault(root_key, []).append((idx, info))
            for root_key, root_entries in by_root.items():
                label = labels.get(root_key, source)
                lines.append(f'  {label}:')
                for idx, info in root_entries:
                    lines.append(f'    [{idx}] {info.name}')
                lines.append('')
        if lines and lines[-1] == '':
            lines.pop()

    return lines


class ConfigGenerator(LoggerMixin):
    """Main configuration generator class for board scanning and configuration file discovery"""

    def __init__(self, root_dir: Path, project_dir: Optional[str] = None):
        super().__init__()
        self.root_dir = root_dir
        # Cache project root to avoid repeated lookups
        self._project_root: Optional[str] = normalize_project_dir(project_dir)

    def set_project_root(self, project_dir: Optional[str]) -> None:
        """Update cached project root explicitly."""
        self._project_root = normalize_project_dir(project_dir)

    def get_project_root(self) -> Optional[str]:
        """Get project root directory, with caching

        Returns:
            Project root path as string, or None if not found
        """
        # Return cached value if available
        if self._project_root is not None:
            return self._project_root

        project_root = resolve_project_root_dir(start_dir=Path(os.getcwd()))
        if project_root:
            self._project_root = project_root
            return project_root

        return None

    def is_c_constant(self, value: Any) -> bool:
        """Check if a value is a C constant based on naming patterns and prefixes"""
        if not isinstance(value, str):
            return False
        if value.isupper() and '_' in value:
            return True
        if value.startswith('sizeof(') and value.endswith(')'):
            return True
        if value.startswith('&esp_bmgr_'):
            return True
        return any(value.startswith(prefix) for prefix in BoardManagerConfig.get_c_constant_prefixes())

    def _format_value_to_c(self, v: Any) -> str:
        """Helper to format a single value into C syntax recursively"""
        if isinstance(v, bool):
            return 'true' if v else 'false'
        elif v is None:
            return 'NULL'
        elif isinstance(v, str):
            # Don't quote if it's:
            # 1. A C constant (e.g. GPIO_NUM_1)
            # 2. A hex number (e.g. 0x12)
            # 3. Already quoted string (e.g. "value")
            x_stripped = v.strip()
            if self.is_c_constant(v) or x_stripped.lower().startswith('0x'):
                return str(v)
            elif (x_stripped.startswith('"') and x_stripped.endswith('"')) or \
                 (x_stripped.startswith("'") and x_stripped.endswith("'")):
                return v
            else:
                return f'"{v}"'
        elif isinstance(v, int):
            return str(v)
        elif isinstance(v, float):
            return str(v)
        elif isinstance(v, list):
            # Recursive formatting for lists -> C arrays { ... }
            elements = [self._format_value_to_c(item) for item in v]
            return '{' + ', '.join(elements) + '}'
        elif isinstance(v, dict):
            # Recursive formatting for dicts -> C structs { .k = v, ... }
            # Note: Inline structs usually don't need newlines/indentation for simple cases,
            # but for complex ones it might be long. Keeping it compact for now.
            fields = []
            for sub_k, sub_v in v.items():
                fields.append(f'.{sub_k} = {self._format_value_to_c(sub_v)}')
            return '{' + ', '.join(fields) + '}'
        else:
            return str(v)

    def dict_to_c_initializer(self, d: Dict[str, Any], indent: int = 4) -> List[str]:
        """Convert dictionary to C initializer format with fixed per-level indentation."""
        step = indent if indent > 0 else 4

        def _render(obj: Any) -> List[str]:
            lines: List[str] = []
            if isinstance(obj, list):
                arr = ', '.join(self._format_value_to_c(x) for x in obj)
                lines.append(f'{arr}')
                return lines

            for k, v in obj.items():
                # Special handling for large integers or specific fields that need hex
                if isinstance(v, int):
                    if k == 'pin_bit_mask' and v > 0xFFFFFFFF:
                        lines.append(f'.{k} = 0x{v:X}ULL,')
                        continue
                    elif k in ['adc_channel_mask', 'dac_channel_mask']:
                        lines.append(f'.{k} = 0x{v:X},')
                        continue

                if isinstance(v, dict):
                    lines.append(f'.{k} = {{')
                    lines += [(' ' * step) + l for l in _render(v)]
                    lines.append('},')
                elif isinstance(v, list) and v and isinstance(v[0], dict):
                    # Array of structs - multiline formatting
                    lines.append(f'.{k} = {{')
                    for item in v:
                        lines.append((' ' * step) + '{')
                        lines += [(' ' * (step * 2)) + l for l in _render(item)]
                        lines.append((' ' * step) + '},')
                    lines.append('},')
                else:
                    val_str = self._format_value_to_c(v)
                    lines.append(f'.{k} = {val_str},')

            return lines

        return _render(d)

    def extract_id_from_name(self, name: str) -> int:
        """Extract numeric ID from component name using regex pattern matching"""
        import re
        m = re.search(r'(\d+)$', name)
        return int(m.group(1)) if m else -1

    def _component_name_from_path(self, component_path: Path) -> str:
        """Return a display name for an IDF component directory."""
        name = component_path.name
        if '__' in name:
            namespace, component = name.split('__', 1)
            if namespace and component:
                return f'{namespace}/{component}'
        return name

    def _customer_source_name(self, customer_root: str, board_path: str) -> str:
        """Build the list/filter source for one customer board.

        ``<root-basename>`` when the board sits directly in the ``-c`` root.
        ``<root-basename>/<relative-parent>`` when it sits deeper.
        """
        root = Path(customer_root).resolve()
        board = Path(board_path).resolve()
        if root == board:
            return root.name
        try:
            relative_parent = board.parent.relative_to(root)
        except ValueError:
            # A customer board may be a symlink to a directory outside -c.
            return root.name
        if relative_parent == Path('.'):
            return root.name
        return f'{root.name}/{relative_parent.as_posix()}'

    def _source_for_board_path(self, board_path: str, customer_path: Optional[str] = None) -> Tuple[str, str]:
        """Infer board source type and display name from a discovered board path."""
        path = Path(board_path).resolve()
        resolved_customer = Path(customer_path).resolve() if customer_path else None
        if resolved_customer:
            try:
                path.relative_to(resolved_customer)
                return 'customer', 'customer'
            except ValueError:
                pass

        parts = path.parts
        for marker in ('managed_components', 'components'):
            if marker not in parts:
                continue
            marker_index = parts.index(marker)
            component_index = marker_index + 1
            if component_index < len(parts):
                return 'component', self._component_name_from_path(Path(parts[component_index]))

        return 'component', 'external'

    def _record_board_directory(
        self,
        boards: Dict[str, BoardDirectory],
        board_name: str,
        board_path: str,
        source_type: str,
        source: str,
    ) -> None:
        """Add a board record keyed by resolved path.

        Scan insertion order is the priority order. A later record with the same
        board name does not replace an earlier record. Recording the same path
        again moves that entry to the end, retaining the later scan priority.
        """
        board_path_abs = _board_record_key(board_path)
        # Reinsert repeated paths so a later scan source retains its priority.
        boards.pop(board_path_abs, None)
        boards[board_path_abs] = BoardDirectory(
            name=board_name,
            path=board_path_abs,
            source=source,
            source_type=source_type,
            chip=_read_board_chip(board_path_abs),
        )
        if boards[board_path_abs].chip is None:
            self.logger.debug(f'   Board "{board_name}" has no chip in board_info.yaml')

    def scan_board_directories(self, board_customer_path: Optional[str] = None) -> Dict[str, BoardDirectory]:
        """Scan all board directories and return them with source metadata.

        Returns a dict keyed by the board directory's resolved path. Same board names
        from different directories are all kept; insertion order is scan priority.
        Callers that need the path read ``info.path``; grouping uses ``info.source``
        and ``info.source_type``. A bare board name selects the last scanned match.
        """
        all_boards: Dict[str, BoardDirectory] = {}

        # 1. Scan board components downloaded by the component manager.
        project_root = self.get_project_root()

        if project_root:
            managed_components_dir = os.path.join(project_root, 'managed_components')
            if os.path.exists(managed_components_dir):
                self.logger.info(f'   Scanning managed components boards: {_resolved_log_path(managed_components_dir)}')
                try:
                    for d in os.listdir(managed_components_dir):
                        subdir_path = os.path.join(managed_components_dir, d)
                        if os.path.isdir(subdir_path) and 'boards' in d:
                            source = self._component_name_from_path(Path(d))
                            self.logger.debug(f'     Found potential board component in managed_components: {d}')

                            if self._is_board_directory(subdir_path):
                                board_name = d
                                if self.is_valid_board_name(board_name):
                                    self._record_board_directory(
                                        all_boards, board_name, subdir_path, 'component', source
                                    )
                                    self.logger.debug(f'     Found single board in managed_components: {board_name}')
                                else:
                                    self.logger.warning(
                                        f'⚠️  Skipping board with invalid name "{board_name}". Board names must contain only letters, numbers, and underscores.'
                                    )
                            else:
                                managed_boards = self._scan_all_directories_for_board_infos(
                                    subdir_path,
                                    f'component (managed: {d})',
                                    max_depth=3,
                                    source='component',
                                    source_name=source,
                                )
                                for info in managed_boards.values():
                                    self._record_board_directory(
                                        all_boards, info.name, info.path, info.source_type, info.source
                                    )
                except Exception as e:
                    self.logger.debug(f'Error scanning managed_components: {e}')

            # 2.1 Scan project component board directories. These are scanned
            # after managed_components so local project boards override managed
            # board components when names collide.
            components_dir = os.path.join(project_root, 'components')

            if os.path.exists(components_dir):
                self.logger.info(f'   Scanning component boards: {_resolved_log_path(components_dir)}')
                component_boards = self._scan_all_directories_for_board_infos(
                    components_dir,
                    'component',
                    max_depth=3,
                )
                for info in component_boards.values():
                    self._record_board_directory(all_boards, info.name, info.path, info.source_type, info.source)

            # 2.2 main/idf_component.yml local overrides for board-named dependencies.
            try:
                override_entries, override_missing = collect_main_override_board_paths(Path(project_root))
            except Exception as e:
                self.logger.debug(f'Error collecting main idf_component override board paths: {e}')
                override_entries, override_missing = [], []

            for dep_name, raw_path in override_missing:
                self.logger.warning(
                    f'⚠️  main idf_component: board-style dependency "{dep_name}" override path '
                    f'is missing or not a directory: {raw_path}'
                )

            if override_entries:
                self.logger.info('   Scanning board components:')
            mgr_root = self.root_dir.resolve()
            for dep_name, abs_p in override_entries:
                abs_path = _resolved_log_path(abs_p)
                try:
                    if abs_p.resolve() == mgr_root:
                        self.logger.debug(
                            f'     Skip {dep_name}: override resolves to esp_board_manager root (already scanned)'
                        )
                        continue
                except OSError:
                    pass
                self.logger.info(f'     {dep_name} -> {abs_path}')
                if self._is_board_directory(abs_path):
                    board_name = os.path.basename(abs_path)
                    if self.is_valid_board_name(board_name):
                        self._record_board_directory(all_boards, board_name, abs_path, 'component', dep_name)
                        self.logger.debug(f'Found single board (main override): {board_name}')
                    else:
                        self.logger.warning(
                            f'⚠️  Skipping board with invalid name "{board_name}" (main override). '
                            'Board names must contain only letters, numbers, and underscores.'
                        )
                else:
                    main_override_boards = self._scan_all_directories_for_board_infos(
                        abs_path,
                        f'main override ({dep_name})',
                        max_depth=3,
                        source='component',
                        source_name=dep_name,
                    )
                    for info in main_override_boards.values():
                        self._record_board_directory(all_boards, info.name, info.path, info.source_type, info.source)

        # 3. Scan customer boards directories if provided. ``board_customer_path``
        #    may be a single path or a ';'-separated list. Each entry is scanned in
        #    order. Same names are kept; a bare board name selects the last match.
        customer_paths = parse_customer_paths(board_customer_path, self.get_project_root())
        if customer_paths:
            for customer_path in customer_paths:
                self.logger.info(f'Scanning customer boards: {_resolved_log_path(customer_path)}')
                if os.path.exists(customer_path):
                    if self._is_board_directory(customer_path):
                        board_name = os.path.basename(customer_path)
                        if self.is_valid_board_name(board_name):
                            source_name = self._customer_source_name(customer_path, customer_path)
                            self._record_board_directory(
                                all_boards, board_name, customer_path, 'customer', source_name
                            )
                            self.logger.debug(f'Found single board: {board_name}')
                        else:
                            self.logger.warning(
                                f'⚠️  Skipping board with invalid name "{board_name}". Board names must contain only letters, numbers, and underscores.'
                            )
                    else:
                        customer_boards = self._scan_all_directories_for_board_infos(
                            customer_path,
                            'customer',
                            max_depth=3,
                            source='customer',
                            source_name=None,
                        )
                        for info in customer_boards.values():
                            source_name = self._customer_source_name(customer_path, info.path)
                            self._record_board_directory(
                                all_boards, info.name, info.path, 'customer', source_name
                            )
                else:
                    self.logger.warning(f'⚠️  Warning: Customer boards path does not exist: {customer_path}')
        else:
            self.logger.debug(f'   No customer boards path specified')

        return all_boards

    def sorted_board_infos(self, board_infos: Dict[str, BoardDirectory]) -> List[BoardDirectory]:
        """Return board infos in the same deterministic order used by the CLI list view."""
        return sorted(board_infos.values(), key=_board_sort_key)

    def ordered_board_names(self, board_customer_path: Optional[str] = None) -> List[str]:
        """Return board names in list-view order."""
        return [info.name for info in self.sorted_board_infos(self.scan_board_directories(board_customer_path))]

    def scan_auto_amend_directories(
        self,
        board_name: str,
        board_customer_path: Optional[str] = None,
    ) -> List[AutoAmendDirectory]:
        """Return auto-amend directories for ``board_name``, ordered low->high priority.

        A directory qualifies as an auto-amend directory when:

        - its basename equals ``board_name``;
        - it contains a ``board_amend.yaml`` manifest;
        - it is **not** a complete board directory (a full board with the same
          name and a manifest is treated as a board, not an auto-amend).

        The same root set used for board discovery is scanned (``{PROJECT}/components``,
        ``managed_components`` board components, ``main/idf_component.yml``
        board-style overrides), and finally every ``-c`` customer path in the
        order given. The returned list is ordered so that later entries have
        higher overlay priority (component sources first, customer last).
        """
        results: List[AutoAmendDirectory] = []
        seen: set = set()

        project_root = self.get_project_root()
        if project_root:
            components_dir = os.path.join(project_root, 'components')
            if os.path.exists(components_dir):
                self._scan_auto_amend_root(components_dir, board_name, 'component', results, seen)

            managed_components_dir = os.path.join(project_root, 'managed_components')
            if os.path.exists(managed_components_dir):
                try:
                    for d in os.listdir(managed_components_dir):
                        subdir_path = os.path.join(managed_components_dir, d)
                        if os.path.isdir(subdir_path) and 'boards' in d:
                            self._scan_auto_amend_root(
                                subdir_path, board_name, f'component (managed: {d})', results, seen
                            )
                except Exception as e:
                    self.logger.debug(f'Error scanning managed_components for auto-amend: {e}')

            # main/idf_component.yml board-style overrides.
            try:
                override_entries, _override_missing = collect_main_override_board_paths(Path(project_root))
            except Exception as e:
                self.logger.debug(f'Error collecting main override paths for auto-amend: {e}')
                override_entries = []
            for dep_name, abs_p in override_entries:
                self._scan_auto_amend_root(
                    str(abs_p), board_name, f'main override ({dep_name})', results, seen
                )

        # Customer (-c) paths, in given order (later entries higher priority).
        customer_paths = parse_customer_paths(board_customer_path, project_root)
        for customer_path in customer_paths:
            self._scan_auto_amend_root(customer_path, board_name, 'customer', results, seen)

        return results

    def _scan_auto_amend_root(
        self,
        root_dir: str,
        board_name: str,
        source_label: str,
        results: List[AutoAmendDirectory],
        seen: set,
        depth: int = 0,
        max_depth: int = 3,
    ) -> None:
        """Recursively look for auto-amend directories named ``board_name`` under ``root_dir``."""
        if not root_dir or not os.path.isdir(root_dir):
            return
        if depth >= max_depth:
            return

        try:
            entries = sorted(os.listdir(root_dir))
        except (PermissionError, OSError) as e:
            self.logger.debug(f'Error listing {root_dir} for auto-amend: {e}')
            return

        for d in entries:
            sub_path = os.path.join(root_dir, d)
            if not os.path.isdir(sub_path):
                continue
            if should_skip_directory(d):
                continue

            if d == board_name and os.path.isfile(os.path.join(sub_path, AMEND_MANIFEST_FILENAME)):
                if self._is_board_directory(sub_path):
                    self.logger.debug(
                        f'   {sub_path} is a full board with {AMEND_MANIFEST_FILENAME}; '
                        f'treated as board, not auto-amend'
                    )
                else:
                    key = str(Path(sub_path).resolve())
                    if key not in seen:
                        seen.add(key)
                        results.append(AutoAmendDirectory(path=key, source=source_label))
                    # Matched directory: do not recurse into it.
                    continue

            self._scan_auto_amend_root(
                sub_path, board_name, source_label, results, seen, depth + 1, max_depth
            )

    def _scan_all_directories_for_board_infos(
        self,
        root_dir: str,
        dir_name: str,
        depth: int = 0,
        max_depth: int = 3,
        source: Optional[str] = None,
        source_name: Optional[str] = None,
    ) -> Dict[str, BoardDirectory]:
        board_dirs: Dict[str, BoardDirectory] = {}

        if not os.path.exists(root_dir):
            self.logger.warning(f'⚠️  {dir_name} directory does not exist: {root_dir}')
            return board_dirs

        if depth >= max_depth:
            self.logger.debug(f'Max recursion depth ({max_depth}) reached at: {root_dir}')
            return board_dirs

        if depth == 0:
            self.logger.debug(f'Scanning all directories in {dir_name} path: {_resolved_log_path(root_dir)}')

        try:
            for d in os.listdir(root_dir):
                board_path = os.path.join(root_dir, d)

                if not os.path.isdir(board_path):
                    continue

                if self._is_board_directory(board_path):
                    if self.is_valid_board_name(d):
                        source_type, inferred_source = self._source_for_board_path(board_path)
                        info = BoardDirectory(
                            name=d,
                            path=_board_record_key(board_path),
                            source=source_name or inferred_source,
                            source_type=source or source_type,
                        )
                        board_dirs[info.path] = info
                        self.logger.debug(f'Found valid board in {dir_name}: {d}')
                    else:
                        self.logger.warning(
                            f'⚠️  Skipping board with invalid name "{d}". Board names must contain only letters, numbers, and underscores.'
                        )
                else:
                    yaml_files = ['board_info.yaml', 'board_peripherals.yaml', 'board_devices.yaml']
                    found_files = [f for f in yaml_files if os.path.isfile(os.path.join(board_path, f))]

                    if found_files:
                        missing_files = [f for f in yaml_files if f not in found_files]
                        self.logger.warning(
                            f'⚠️  Skipping invalid board "{d}" - missing required files: {", ".join(missing_files)}'
                        )
                    else:
                        sub_boards = self._scan_all_directories_for_board_infos(
                            board_path,
                            f'{dir_name}/{d}',
                            depth=depth + 1,
                            max_depth=max_depth,
                            source=source,
                            source_name=source_name,
                        )
                        for info in sub_boards.values():
                            self._record_board_directory(
                                board_dirs, info.name, info.path, info.source_type, info.source
                            )

        except PermissionError:
            self.logger.warning(f'⚠️  Permission denied accessing {root_dir}')
        except Exception as e:
            self.logger.debug(f'Error scanning {root_dir}: {e}')

        return board_dirs

    def _scan_all_directories_for_boards(
        self,
        root_dir: str,
        dir_name: str,
        depth: int = 0,
        max_depth: int = 3
    ) -> Dict[str, str]:
        """
        Recursively scan all subdirectories for valid boards.

        A valid board must have all 3 required files:
        - board_info.yaml
        - board_peripherals.yaml
        - board_devices.yaml

        Args:
            root_dir: Root directory to scan
            dir_name: Name for logging purposes
            depth: Current recursion depth
            max_depth: Maximum recursion depth (call sites use 3; entries with depth >= max_depth are not listed)

        Returns:
            Dictionary of board_name -> board_path
        """
        infos = self._scan_all_directories_for_board_infos(
            root_dir,
            dir_name,
            depth=depth,
            max_depth=max_depth,
        )
        return {info.path: info.path for info in infos.values()}

    def _is_board_directory(self, path: str) -> bool:
        """
        Check if a directory is a valid board directory by verifying all required files exist.

        A valid board must have:
        1. board_info.yaml
        2. board_peripherals.yaml
        3. board_devices.yaml
        """
        if not os.path.isdir(path):
            return False

        required_files = [
            'board_info.yaml',
            'board_peripherals.yaml',
            'board_devices.yaml'
        ]

        for filename in required_files:
            file_path = os.path.join(path, filename)
            if not os.path.isfile(file_path):
                return False

        return True

    def is_valid_board_name(self, name: str) -> bool:
        """
        Check if board name is valid.
        Valid names can contain letters (uppercase/lowercase), numbers, and underscores.
        Must not contain hyphens or other special characters.
        """
        return bool(re.match(r'^[a-zA-Z0-9_]+$', name))

    def check_board_name_consistency(self, board_path: str, dir_name: str) -> None:
        """Check if board name in board_info.yaml matches directory name"""
        try:
            board_info_path = os.path.join(board_path, 'board_info.yaml')
            if os.path.exists(board_info_path):
                # Use simple parsing to avoid full YAML load overhead if possible,
                # but load_yaml_safe is safer
                board_info = load_yaml_safe(board_info_path)
                if board_info and 'board' in board_info:
                    board_name_in_yaml = board_info['board']
                    if board_name_in_yaml != dir_name:
                        self.logger.warning(
                            f'⚠️  Board name mismatch in "{dir_name}": '
                            f'directory name is "{dir_name}", but board_info.yaml says "{board_name_in_yaml}". '
                            f'The directory name will be used as the board ID.'
                        )
        except Exception as e:
            self.logger.debug(f'Error checking board name consistency for {dir_name}: {e}')

    def get_selected_board_from_sdkconfig(self) -> str:
        """Read sdkconfig file to determine which board is selected, fallback to default if not found"""
        # Get project root using cached method
        project_root = self.get_project_root()
        if project_root:
            self.logger.info(f'Using project root: {project_root}')
        else:
            self.logger.warning('⚠️  Could not find project root, using default board')
            return BoardManagerConfig.DEFAULT_BOARD

        sdkconfig_path = Path(project_root) / 'sdkconfig'
        if not sdkconfig_path.exists():
            self.logger.warning('⚠️  sdkconfig file not found, using default board')
            return BoardManagerConfig.DEFAULT_BOARD

        self.logger.info(f'Reading sdkconfig from: {sdkconfig_path}')

        # Read sdkconfig and look for board selection configs.
        selected_board = None
        with open(sdkconfig_path, 'r') as f:
            for line in f:
                line = line.strip()
                if '=y' not in line:
                    continue

                config_name = line.split('=')[0]
                if line.startswith('CONFIG_ESP_BOARD_') and not line.endswith('_SUPPORT=y'):
                    board_name = config_name.replace('CONFIG_ESP_BOARD_', '').lower()
                    selected_board = board_name
                    self.logger.info(f'Found selected board in sdkconfig: {board_name}')
                    break

        if not selected_board:
            self.logger.warning('⚠️  No board selected in sdkconfig, using default board')
            return BoardManagerConfig.DEFAULT_BOARD

        return selected_board

    def find_board_config_files(self, board_name: str, all_boards: Dict[str, Any]) -> Tuple[Optional[str], Optional[str]]:
        """Find board_peripherals.yaml and board_devices.yaml for the selected board"""
        board_info = all_boards.get(board_name)
        if board_info is None and all_boards:
            sample = next(iter(all_boards.values()))
            if isinstance(sample, BoardDirectory):
                board_info = select_last_named_board(all_boards, board_name)

        if board_info is None:
            self.logger.error(f"Board '{board_name}' not found in any boards directory")
            if all_boards and isinstance(next(iter(all_boards.values())), BoardDirectory):
                available = sorted({info.name for info in all_boards.values()})
            else:
                available = list(all_boards.keys())
            self.logger.error(f'Available boards: {available}')
            return None, None
        board_path = board_info.path if isinstance(board_info, BoardDirectory) else board_info
        periph_yaml_path = os.path.join(board_path, 'board_peripherals.yaml')
        dev_yaml_path = os.path.join(board_path, 'board_devices.yaml')

        if not os.path.exists(periph_yaml_path):
            self.logger.error(f"board_peripherals.yaml not found for board '{board_name}' at {periph_yaml_path}")
            return None, None

        if not os.path.exists(dev_yaml_path):
            self.logger.error(f"board_devices.yaml not found for board '{board_name}' at {dev_yaml_path}")
            return None, None

        self.logger.debug(f"   Using board configuration files for '{board_name}':")
        self.logger.debug(f'   Peripherals: {_resolved_log_path(periph_yaml_path)}')
        self.logger.debug(f'   Devices: {_resolved_log_path(dev_yaml_path)}')

        return periph_yaml_path, dev_yaml_path
