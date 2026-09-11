# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

"""Resolve YAML IDF macros with a known list plus prefix passthrough.

Each parser supplies its own known enumerator names. This helper does not
own those lists. The list is a hint, not a hard gate: a matching prefix is
emitted as-is with a warning so a new chip or IDF revision does not fail
``idf.py bmgr``.
"""

from __future__ import annotations

import re
from typing import FrozenSet, Iterable, Optional, Pattern, Sequence, Union

IdfMacroValue = Union[str, int]

_IDENT_RE = re.compile(r'^[A-Za-z_][A-Za-z0-9_]*$')
_COMPILED_PATTERNS: dict[str, Pattern[str]] = {}


def resolve_idf_macro(
    value,
    default: IdfMacroValue,
    prefixes: Sequence[str],
    known: Iterable[IdfMacroValue],
    field_name: str,
    extra_allowed: Iterable[IdfMacroValue] = (),
    pattern: Optional[str] = None,
) -> IdfMacroValue:
    """Return a compile-facing IDF identifier from YAML.

    Rules:
    - Empty / None uses ``default``.
    - Values in ``known`` or ``extra_allowed`` pass silently.
    - Strings that match ``pattern`` (if given) or start with one of
      ``prefixes`` pass through. Unknown-to-the-list values print a warning.
    - Anything else raises ``ValueError``.
    """
    if value is None or value == '':
        return default

    known_set: FrozenSet[IdfMacroValue] = frozenset(known)
    extra_set: FrozenSet[IdfMacroValue] = frozenset(extra_allowed)

    if isinstance(value, str):
        value = value.strip()
        if value == '':
            return default
        if value.isdigit() or (value.startswith('-') and value[1:].isdigit()):
            int_value = int(value)
            if int_value in extra_set:
                return int_value

    if value in extra_set or value in known_set:
        return value

    if isinstance(value, str) and _matches_shape(value, prefixes, pattern):
        print(
            f"⚠️  YAML WARNING: '{value}' is not in the known {field_name} list; "
            'passing it through literally. Confirm the current chip/IDF supports it.'
        )
        return value

    raise ValueError(_invalid_message(value, field_name, prefixes, pattern))


def _matches_shape(text: str, prefixes: Sequence[str], pattern: Optional[str]) -> bool:
    if not _IDENT_RE.fullmatch(text):
        return False
    if pattern:
        compiled = _COMPILED_PATTERNS.get(pattern)
        if compiled is None:
            compiled = re.compile(pattern)
            _COMPILED_PATTERNS[pattern] = compiled
        return compiled.fullmatch(text) is not None
    return any(text.startswith(prefix) and len(text) > len(prefix) for prefix in prefixes)


def _invalid_message(value, field_name: str, prefixes: Sequence[str], pattern: Optional[str]) -> str:
    if pattern:
        shape = f"matching '{pattern}'"
    elif prefixes:
        quoted = ', '.join(f"'{prefix}'" for prefix in prefixes)
        shape = f'starting with {quoted}'
    else:
        shape = 'a known IDF enumerator'
    return (
        f"Invalid {field_name} value '{value}'. "
        f'Use a C identifier {shape}.'
    )
