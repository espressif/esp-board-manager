# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


PUBLISHER = Path(__file__).resolve().parents[1] / 'publish_ecosystem_stats.py'
SOURCE_ARTIFACTS = ('stats.json', 'supported-boards.svg', 'dependent-projects.svg', 'README.md')
ARTIFACTS = SOURCE_ARTIFACTS + (
    'ecosystem-banner-en-light.svg', 'ecosystem-banner-en-dark.svg',
    'ecosystem-banner-zh-light.svg', 'ecosystem-banner-zh-dark.svg',
)
BRANCH = 'refs/heads/ecosystem-stats'


class PublishEcosystemStatsTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.remote = self.root / 'remote.git'
        self.source = self.root / 'snapshot'
        self.source.mkdir()
        self.git('init', '--quiet', '--bare', str(self.remote))
        self.write_snapshot('first')

    def git(self, *arguments):
        result = subprocess.run(
            ['git', *arguments], capture_output=True, text=True, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        return result.stdout.strip()

    def write_snapshot(self, marker, updated_at='2026-11-05T00:00:00Z'):
        for name in SOURCE_ARTIFACTS:
            (self.source / name).write_text('{}: {}\n'.format(name, marker), encoding='utf-8')
        snapshot = {
            'schema_version': 1, 'updated_at': updated_at, 'marker': marker,
            'boards': {'count': 1, 'packs': [{'count': 1, 'boards': ['test-board']}]},
            'projects': {'count': 0, 'repositories': []},
        }
        (self.source / 'stats.json').write_text(json.dumps(snapshot), encoding='utf-8')

    def publish(self, remote=None):
        return subprocess.run(
            [sys.executable, str(PUBLISHER), '--source', str(self.source),
             '--remote', str(self.remote if remote is None else remote)],
            capture_output=True, text=True, check=False,
        )

    def revision(self):
        return self.git('--git-dir', str(self.remote), 'rev-parse', BRANCH)

    def test_four_source_artifacts_publish_only_the_eight_public_artifacts(self):
        self.assertEqual({path.name for path in self.source.iterdir()}, set(SOURCE_ARTIFACTS))
        (self.source / 'private-extra.txt').write_text('must not be published', encoding='utf-8')
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        names = self.git('--git-dir', str(self.remote), 'ls-tree', '-r', '--name-only', BRANCH)
        self.assertEqual(set(names.splitlines()), set(ARTIFACTS))
        branches = self.git('--git-dir', str(self.remote), 'for-each-ref', '--format=%(refname)', 'refs/heads')
        self.assertEqual(branches, BRANCH)

    def test_existing_four_artifact_branch_upgrades_without_replacing_history(self):
        work = self.root / 'legacy-work'
        work.mkdir()
        self.git('-C', str(work), 'init', '--quiet')
        self.git('-C', str(work), 'config', 'user.name', 'Publisher test')
        self.git('-C', str(work), 'config', 'user.email', 'publisher-test@example.invalid')
        for name in SOURCE_ARTIFACTS:
            (work / name).write_text('legacy aggregate artifact: ' + name, encoding='utf-8')
        self.git('-C', str(work), 'add', '--', *SOURCE_ARTIFACTS)
        self.git('-C', str(work), 'commit', '--quiet', '-m', 'Test legacy four-artifact publication')
        self.git('-C', str(work), 'push', '--quiet', str(self.remote), 'HEAD:' + BRANCH)
        previous = self.revision()

        result = self.publish()

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotEqual(previous, self.revision())
        self.git('--git-dir', str(self.remote), 'merge-base', '--is-ancestor', previous, BRANCH)
        names = self.git('--git-dir', str(self.remote), 'ls-tree', '-r', '--name-only', BRANCH)
        self.assertEqual(set(names.splitlines()), set(ARTIFACTS))
        self.assertEqual({path.name for path in self.source.iterdir()}, set(SOURCE_ARTIFACTS))

    def test_public_files_are_regenerated_without_private_fields_or_input_artifacts(self):
        marker = 'PRIVATE-REVIEW-AND-SOURCE-MARKER'
        snapshot = json.loads((self.source / 'stats.json').read_text())
        snapshot['marker'] = marker
        snapshot['board_filters'] = {'board_decisions': {'reason': marker}, 'private_path': marker}
        snapshot['boards']['packs'][0]['boards'] = [marker]
        snapshot['boards']['packs'][0]['private_review'] = marker
        snapshot['projects'] = {'count': 1, 'repositories': [{'repository': marker, 'evidence': [marker]}]}
        (self.source / 'stats.json').write_text(json.dumps(snapshot))
        for name in ARTIFACTS[1:]:
            (self.source / name).write_text(marker + ': ' + name)
        (self.source / 'private-review.json').write_text(marker)
        before = {path.name: path.read_bytes() for path in self.source.iterdir()}
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        published = {}
        for name in ARTIFACTS:
            published[name] = self.git('--git-dir', str(self.remote), 'show', BRANCH + ':' + name)
            self.assertNotIn(marker, published[name])
        self.assertEqual(json.loads(published['stats.json']), {
            'schema_version': 1, 'updated_at': '2026-11-05T00:00:00Z',
            'supported_boards': 1, 'public_dependent_projects': 1})
        self.assertTrue(published['supported-boards.svg'].startswith('<svg '))
        self.assertTrue(published['dependent-projects.svg'].startswith('<svg '))
        self.assertNotIn(marker, self.git('--git-dir', str(self.remote), 'log', '--all', '--format=fuller', '--patch'))
        self.assertEqual({path.name: path.read_bytes() for path in self.source.iterdir()}, before)

    def test_private_only_changes_do_not_create_public_history(self):
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        previous = self.revision()
        self.write_snapshot('private-only-change')
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(previous, self.revision())

    def test_invalid_update_time_cannot_be_published_as_private_text(self):
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        previous = self.revision()
        self.write_snapshot('private', updated_at='2026-11-05T00:00:00Z PRIVATE-TIME-MARKER')
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(previous, self.revision())

    def test_existing_main_branch_is_not_changed(self):
        work = self.root / 'main-work'
        work.mkdir()
        self.git('-C', str(work), 'init', '--quiet')
        self.git('-C', str(work), 'config', 'user.name', 'Publisher test')
        self.git('-C', str(work), 'config', 'user.email', 'publisher-test@example.invalid')
        (work / 'source.txt').write_text('main source', encoding='utf-8')
        self.git('-C', str(work), 'add', 'source.txt')
        self.git('-C', str(work), 'commit', '--quiet', '-m', 'Test main')
        self.git('-C', str(work), 'push', '--quiet', str(self.remote), 'HEAD:refs/heads/main')
        previous = self.git('--git-dir', str(self.remote), 'rev-parse', 'refs/heads/main')
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(previous, self.git('--git-dir', str(self.remote), 'rev-parse', 'refs/heads/main'))

    def test_existing_unexpected_files_are_not_silently_published_or_removed(self):
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        work = self.root / 'extra-file-work'
        self.git('clone', '--quiet', '--branch', 'ecosystem-stats', str(self.remote), str(work))
        self.git('-C', str(work), 'config', 'user.name', 'Publisher test')
        self.git('-C', str(work), 'config', 'user.email', 'publisher-test@example.invalid')
        (work / 'legacy-extra.txt').write_text('unmanaged file', encoding='utf-8')
        self.git('-C', str(work), 'add', 'legacy-extra.txt')
        self.git('-C', str(work), 'commit', '--quiet', '-m', 'Test unexpected file')
        self.git('-C', str(work), 'push', '--quiet', 'origin', 'HEAD:refs/heads/ecosystem-stats')
        previous = self.revision()
        self.write_snapshot('new')
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Unexpected', result.stderr)
        self.assertEqual(previous, self.revision())

    @unittest.skipIf(os.name == 'nt', 'Symlinks require additional Windows privileges')
    def test_replacing_a_remote_symlink_does_not_write_outside_the_temporary_repository(self):
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        external = self.root / 'outside.txt'
        external.write_text('must remain unchanged', encoding='utf-8')
        work = self.root / 'symlink-work'
        self.git('clone', '--quiet', '--branch', 'ecosystem-stats', str(self.remote), str(work))
        self.git('-C', str(work), 'config', 'user.name', 'Publisher test')
        self.git('-C', str(work), 'config', 'user.email', 'publisher-test@example.invalid')
        (work / 'stats.json').unlink()
        (work / 'stats.json').symlink_to(external)
        self.git('-C', str(work), 'add', 'stats.json')
        self.git('-C', str(work), 'commit', '--quiet', '-m', 'Test remote symlink')
        self.git('-C', str(work), 'push', '--quiet', 'origin', 'HEAD:refs/heads/ecosystem-stats')
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(external.read_text(encoding='utf-8'), 'must remain unchanged')
        content = self.git('--git-dir', str(self.remote), 'show', BRANCH + ':stats.json')
        self.assertEqual(json.loads(content)['supported_boards'], 1)
        self.assertNotIn('marker', json.loads(content))

    def test_second_publish_preserves_history_and_updates_contents(self):
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        previous = self.revision()
        self.write_snapshot('second', updated_at='2026-11-06T00:00:00Z')
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotEqual(previous, self.revision())
        self.git('--git-dir', str(self.remote), 'merge-base', '--is-ancestor', previous, BRANCH)
        content = self.git('--git-dir', str(self.remote), 'show', BRANCH + ':stats.json')
        self.assertEqual(json.loads(content)['updated_at'], '2026-11-06T00:00:00Z')

    def test_unchanged_snapshot_does_not_create_a_commit(self):
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        previous = self.revision()
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(previous, self.revision())

    def test_incomplete_snapshot_does_not_replace_published_contents(self):
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        previous = self.revision()
        self.write_snapshot('incomplete')
        (self.source / 'dependent-projects.svg').unlink()
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('dependent-projects.svg', result.stderr)
        self.assertEqual(previous, self.revision())

    def test_unavailable_remote_is_reported_as_remote_inspection_failure(self):
        result = self.publish(self.root / 'missing.git')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('ls-remote', result.stderr)
        self.assertNotIn('checkout', result.stderr)

    def test_boards_only_snapshot_does_not_replace_published_contents(self):
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        previous = self.revision()
        snapshot = json.loads((self.source / 'stats.json').read_text(encoding='utf-8'))
        snapshot['projects'] = None
        (self.source / 'stats.json').write_text(json.dumps(snapshot), encoding='utf-8')
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('complete', result.stderr)
        self.assertEqual(previous, self.revision())

    def test_inconsistent_counts_do_not_replace_published_contents(self):
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        previous = self.revision()
        for section in ('boards', 'projects'):
            with self.subTest(section=section):
                self.write_snapshot('inconsistent')
                snapshot = json.loads((self.source / 'stats.json').read_text(encoding='utf-8'))
                snapshot[section]['count'] += 1
                (self.source / 'stats.json').write_text(json.dumps(snapshot), encoding='utf-8')
                result = self.publish()
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(previous, self.revision())

    def test_inconsistent_reviewed_count_does_not_replace_published_contents(self):
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        previous = self.revision()
        snapshot = json.loads((self.source / 'stats.json').read_text())
        snapshot['boards']['packs'][0]['listed'] = True
        snapshot['boards']['approved_count'] = 99
        snapshot['boards']['decision_summary'] = {
            'known_count': 1, 'included_count': 0, 'aliased_count': 0, 'pending_count': 0,
            'approved_count': 1, 'candidates': [],
        }
        (self.source / 'stats.json').write_text(json.dumps(snapshot))
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(previous, self.revision())

    def test_discovery_without_reviewed_count_is_not_published(self):
        snapshot = json.loads((self.source / 'stats.json').read_text())
        snapshot['boards']['discovery_enabled'] = True
        (self.source / 'stats.json').write_text(json.dumps(snapshot))
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        refs = self.git('--git-dir', str(self.remote), 'for-each-ref', '--format=%(refname)')
        self.assertEqual(refs, '')

    def test_complete_reviewed_snapshot_can_be_published(self):
        snapshot = json.loads((self.source / 'stats.json').read_text())
        snapshot['boards'] = {
            'count': 3, 'approved_count': 2, 'discovery_enabled': True,
            'packs': [{'count': 1, 'boards': ['known'], 'listed': True},
                      {'count': 2, 'boards': ['new', 'copy'], 'listed': False}],
            'decision_summary': {'known_count': 1, 'included_count': 1, 'aliased_count': 1,
                                 'pending_count': 0, 'approved_count': 2,
                                 'candidates': [{'decision': 'include'}, {'decision': 'alias'}]},
        }
        (self.source / 'stats.json').write_text(json.dumps(snapshot))
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        published = json.loads(self.git('--git-dir', str(self.remote), 'show', BRANCH + ':stats.json'))
        self.assertEqual(published['supported_boards'], 2)
        self.assertNotIn('boards', published)

    def test_invalid_json_does_not_replace_published_contents(self):
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        previous = self.revision()
        (self.source / 'stats.json').write_text('not JSON', encoding='utf-8')
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(previous, self.revision())

    @unittest.skipIf(os.name == 'nt', 'Git executable hooks require a POSIX test host')
    def test_rejected_push_preserves_previous_snapshot(self):
        result = self.publish()
        self.assertEqual(result.returncode, 0, result.stderr)
        previous = self.revision()
        hook = self.remote / 'hooks' / 'pre-receive'
        hook.write_text('#!/bin/sh\nexit 1\n', encoding='utf-8')
        hook.chmod(0o755)
        self.write_snapshot('rejected', updated_at='2026-11-06T00:00:00Z')
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('push', result.stderr)
        self.assertEqual(previous, self.revision())


if __name__ == '__main__':
    unittest.main()
