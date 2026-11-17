#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""Archive private daily count changes and source links in this GitLab project."""

import argparse
import json
from pathlib import Path
import sys
import tempfile

from ecosystem_history import build_history, render_changes, render_history
from gitlab_ecosystem_history import HistoryRegistry
from publish_ecosystem_stats import validate_snapshot


def archive_history(source, output, registry=None):
    """Validate a complete scan, retain its local report, then durably archive it."""
    source, output = Path(source), Path(output)
    snapshot = validate_snapshot(source)
    registry = registry if registry is not None else HistoryRegistry.from_environment()
    history = build_history(registry.load(), snapshot, registry.run)
    markdown = render_history(history)
    changes = render_changes(history, registry.run['id'])
    files = {'history.json': json.dumps(history, ensure_ascii=False, indent=2) + '\n',
             'README.md': markdown, 'changes.md': changes}
    if output.is_symlink():
        raise ValueError('History output must be a regular directory')
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=str(output)) as temporary:
        for name, content in files.items():
            (Path(temporary) / name).write_text(content, encoding='utf-8')
        for name in files:
            (Path(temporary) / name).replace(output / name)
    registry.save(history, markdown, changes)
    print('Private scan history archived to package {} version {}; {} scan records.'.format(
        registry.package_name, registry.version, len(history['entries'])))
    return history


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=Path, help='Directory containing the complete private scan')
    parser.add_argument('--output', type=Path, help='Local private history report; defaults to SOURCE/history')
    args = parser.parse_args(argv)
    try:
        archive_history(args.source, args.output if args.output is not None else args.source / 'history')
    except (OSError, ValueError, RuntimeError) as error:
        print('Private history archive failed; public statistics were not updated: {}'.format(error), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
