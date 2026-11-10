#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""Validate four private report artifacts and regenerate eight aggregate public files."""

import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile

from public_ecosystem_stats import public_files


PUBLISH_BRANCH = 'ecosystem-stats'
SOURCE_ARTIFACTS = ('stats.json', 'supported-boards.svg', 'dependent-projects.svg', 'README.md')
ARTIFACTS = SOURCE_ARTIFACTS + (
    'ecosystem-banner-en-light.svg', 'ecosystem-banner-en-dark.svg',
    'ecosystem-banner-zh-light.svg', 'ecosystem-banner-zh-dark.svg',
)


def validate_snapshot(source):
    snapshot = json.loads((source / 'stats.json').read_text(encoding='utf-8'))
    message = 'Only complete schema_version=1 statistics snapshots can be published'
    if (not isinstance(snapshot, dict) or snapshot.get('schema_version') != 1
            or not isinstance(snapshot.get('updated_at'), str) or not snapshot['updated_at']):
        raise ValueError(message)
    boards = snapshot.get('boards')
    projects = snapshot.get('projects')
    if not isinstance(boards, dict) or not isinstance(projects, dict):
        raise ValueError(message)
    for section in (boards, projects):
        if type(section.get('count')) is not int or section['count'] < 0:
            raise ValueError(message)
    packs = boards.get('packs')
    repositories = projects.get('repositories')
    if not isinstance(packs, list) or not packs or not isinstance(repositories, list):
        raise ValueError(message)
    total = 0
    for pack in packs:
        if (not isinstance(pack, dict) or type(pack.get('count')) is not int
                or not isinstance(pack.get('boards'), list) or pack['count'] != len(pack['boards'])):
            raise ValueError(message)
        total += pack['count']
    if boards['count'] != total or projects['count'] != len(repositories):
        raise ValueError(message)
    if boards.get('discovery_enabled') and 'approved_count' not in boards:
        raise ValueError('Discovered boards require a reviewed count before publication')
    if 'approved_count' in boards or 'decision_summary' in boards:
        summary = boards.get('decision_summary')
        if not isinstance(summary, dict):
            raise ValueError(message)
        counters = ('known_count', 'included_count', 'aliased_count', 'pending_count', 'approved_count')
        if any(type(summary.get(key)) is not int or summary[key] < 0 for key in counters):
            raise ValueError(message)
        candidates = summary.get('candidates')
        if not isinstance(candidates, list) or any(not isinstance(item, dict) for item in candidates):
            raise ValueError(message)
        decisions = [item.get('decision') for item in candidates]
        if any(decision not in ('include', 'alias', 'pending') for decision in decisions):
            raise ValueError(message)
        if (summary['known_count'] != sum(pack['count'] for pack in packs if pack.get('listed'))
                or summary['included_count'] != decisions.count('include')
                or summary['aliased_count'] != decisions.count('alias')
                or summary['pending_count'] != decisions.count('pending')
                or len(candidates) + summary['known_count'] != boards['count']
                or summary['approved_count'] != summary['known_count'] + summary['included_count']
                or type(boards.get('approved_count')) is not int
                or boards['approved_count'] != summary['approved_count']):
            raise ValueError(message)
    # Also validate the public scalar types and timestamp before touching Git.
    public_files(snapshot)
    return snapshot


def git(repository, *arguments, allowed_codes=(0,)):
    result = subprocess.run(
        ['git', '-C', str(repository), *arguments],
        capture_output=True, text=True, check=False, timeout=120,
    )
    if result.returncode not in allowed_codes:
        raise RuntimeError('git {} failed (exit {}): {}'.format(
            arguments[0], result.returncode, result.stderr.strip(),
        ))
    return result


def publish(source, remote):
    source = Path(source).resolve()
    for name in SOURCE_ARTIFACTS:
        artifact = source / name
        if not artifact.is_file() or artifact.is_symlink() or artifact.stat().st_size == 0:
            raise ValueError('Missing regular statistics artifact: {}'.format(artifact))
    snapshot = validate_snapshot(source)
    files = public_files(snapshot)

    with tempfile.TemporaryDirectory(prefix='bmgr-ecosystem-stats-') as temporary:
        repository = Path(temporary)
        git(repository, 'init', '--quiet')
        git(repository, 'config', 'user.name', 'ESP Board Manager CI')
        git(repository, 'config', 'user.email', 'esp-board-manager-ci@users.noreply.github.com')
        git(repository, 'remote', 'add', 'publish', remote)
        ref = 'refs/heads/' + PUBLISH_BRANCH
        branch = git(repository, 'ls-remote', '--exit-code', '--heads', 'publish', ref,
                     allowed_codes=(0, 2))
        if branch.returncode == 0:
            git(repository, 'fetch', '--quiet', '--no-tags', '--depth=1', 'publish', ref)
            tree = git(repository, 'ls-tree', '-rz', '--name-only', 'FETCH_HEAD')
            extra = set(tree.stdout.split('\0')) - {''} - set(ARTIFACTS)
            if extra:
                raise ValueError('Unexpected files on {}: {}'.format(PUBLISH_BRANCH, ', '.join(sorted(extra))))
            git(repository, 'checkout', '--quiet', '-B', PUBLISH_BRANCH, 'FETCH_HEAD')
        else:
            git(repository, 'checkout', '--quiet', '--orphan', PUBLISH_BRANCH)

        for name in ARTIFACTS:
            destination = repository / name
            if destination.is_symlink():
                destination.unlink()
            destination.write_text(files[name], encoding='utf-8')
        git(repository, 'add', '--', *ARTIFACTS)
        changed = git(repository, 'diff', '--cached', '--quiet', allowed_codes=(0, 1))
        if changed.returncode == 0:
            print('Statistics snapshot is unchanged; nothing to publish.')
            return

        git(repository, 'commit', '--quiet', '-m', 'chore: Update ecosystem statistics')
        git(repository, 'push', '--quiet', 'publish', 'HEAD:' + ref)
        print('Published statistics to {}.'.format(PUBLISH_BRANCH))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=Path,
                        help='Directory containing the four private report artifacts; public banners are regenerated')
    parser.add_argument('--remote', required=True, help='Git remote URL; authentication is supplied by the environment')
    arguments = parser.parse_args()
    try:
        publish(arguments.source, arguments.remote)
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        print('Statistics publication failed: {}'.format(error), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
