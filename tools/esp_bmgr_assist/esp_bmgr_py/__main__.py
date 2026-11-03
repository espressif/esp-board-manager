"""
自检：在「与 idf.py 相同的」解释器下运行:

    python -m esp_bmgr_py

若此处显示版本不是 0.7.0+、缺少 bmgr_placeholder_ext 或缺少 esp_bmgr_py.pth，
则 `idf.py bmgr -l`（旧命令：`idf.py gen-bmgr-config -l`）仍可能报 No such option: -l。
"""

from __future__ import annotations

import os
import sys
from pathlib import Path
from typing import List, Set


def _site_packages_candidates() -> List[Path]:
    out: List[Path] = []
    # Check the live interpreter search path first. This shows where the current ``python`` will
    # actually look for the ``.pth`` hook at runtime.
    for entry in sys.path:
        if not entry or entry == '.':
            continue
        p = Path(entry)
        if p.name == 'site-packages' and p.is_dir():
            out.append(p)
    # Fall back to ``sysconfig`` because some environments do not expose the final site-packages
    # directory directly in ``sys.path`` during startup.
    try:
        from sysconfig import get_paths

        sp = get_paths().get('purelib')
        if sp:
            out.append(Path(sp))
    except (ImportError, KeyError, OSError) as exc:
        if os.environ.get('ESP_BMGR_DEBUG', '0') == '1':
            print(
                f'[ESP_BMGR_ASSIST] sysconfig fallback skipped: '
                f'{type(exc).__name__}: {exc}',
                file=sys.stderr,
                flush=True,
            )
    seen: Set[str] = set()
    uniq: List[Path] = []
    for p in out:
        key = str(p.resolve())
        if key not in seen:
            seen.add(key)
            uniq.append(p)
    return uniq


def main() -> int:
    import esp_bmgr_py as pkg

    print(
        f'esp-bmgr-assist version: {pkg.__version__} '
        '(Python package: esp_bmgr_py; expect >= 0.7.0 for bmgr tooling)'
    )
    print(f'sys.executable: {sys.executable}')

    placeholder = Path(pkg.__file__).resolve().parent / 'bmgr_placeholder_ext.py'
    print(f'bmgr_placeholder_ext.py: {placeholder} exists={placeholder.is_file()}')

    injector = Path(pkg.__file__).resolve().parent / 'idf_injector.py'
    text = injector.read_text(encoding='utf-8', errors='replace') if injector.is_file() else ''
    print(f"idf_injector has prepend hook: {'_ensure_placeholder_ext_path_first' in text}")

    found_pth = False
    for sp in _site_packages_candidates():
        pth = sp / 'esp_bmgr_py.pth'
        if pth.is_file():
            print(f"esp_bmgr_py.pth OK: {pth} -> {pth.read_text(encoding='utf-8').strip()!r}")
            found_pth = True
    if not found_pth:
        print('esp_bmgr_py.pth: NOT FOUND under sys.path site-packages — hook will not run; reinstall with:')
        print('  pip install --no-cache-dir --no-build-isolation -U /path/to/esp-board-manager/tools/esp_bmgr_assist')

    extra = os.environ.get('IDF_EXTRA_ACTIONS_PATH', '')
    if extra:
        print(f'IDF_EXTRA_ACTIONS_PATH={extra!r}')
        print(
            '  If this lists managed_components/.../espressif__esp_board_manager, an OLD idf_ext there '
            'can override the placeholder and drop -l. Remove that folder or bump espressif/esp_board_manager.'
        )
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
