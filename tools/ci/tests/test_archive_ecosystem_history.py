# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

import copy
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

import yaml

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
SPEC = importlib.util.find_spec('archive_ecosystem_history')
archive = __import__('archive_ecosystem_history') if SPEC else None


def snapshot(timestamp='2026-11-16T00:00:00Z'):
    origin = {'repository': 'vendor/boards', 'path': 'boards/model-a',
              'url': 'https://github.com/vendor/boards/tree/pinned/boards/model-a'}
    definition = {'board': 'model-a', 'chip': 'esp32s3', 'path': origin['path'],
                  'origins': [origin]}
    return {
        'schema_version': 1, 'updated_at': timestamp,
        'boards': {'count': 1, 'approved_count': 1, 'discovery_enabled': True,
                   'packs': [{'repository': 'vendor/boards', 'path': 'boards', 'listed': True,
                              'count': 1, 'boards': ['model-a'], 'definitions': [definition]}],
                   'decision_summary': {'known_count': 1, 'included_count': 0, 'aliased_count': 0,
                                        'pending_count': 0, 'approved_count': 1, 'candidates': []}},
        'projects': {'count': 1, 'repositories': [
            {'repository_id': 123, 'repository': 'owner/application',
             'url': 'https://github.com/owner/application', 'revision': 'pinned',
             'evidence': [{'path': 'main/idf_component.yml', 'kind': 'direct',
                           'url': 'https://github.com/owner/application/blob/pinned/main/idf_component.yml'}]}]},
    }


class Registry:
    def __init__(self):
        self.run = {'id': '106123456', 'url': 'https://gitlab.example/team/project/-/jobs/106123456',
                    'ref': 'main'}
        self.package_name = 'ecosystem-history'
        self.version = '1.0.106123456'
        self.history = None
        self.saved = []

    def load(self):
        return copy.deepcopy(self.history)

    def save(self, history, markdown, changes):
        self.saved.append((copy.deepcopy(history), markdown, changes))
        self.history = copy.deepcopy(history)


class ArchiveHistoryTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(archive, 'The private history archive command must exist')
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / 'report'
        self.source.mkdir()
        self.output = self.source / 'history'
        self.registry = Registry()
        self.write_snapshot(snapshot())

    def write_snapshot(self, value):
        (self.source / 'stats.json').write_text(json.dumps(value), encoding='utf-8')

    def test_baseline_and_next_day_changes_are_archived_and_readable_locally(self):
        archive.archive_history(self.source, self.output, self.registry)
        first, markdown, changes = self.registry.saved[-1]
        self.assertTrue(first['entries'][0]['baseline'])
        self.assertIn('owner/application', changes or '')
        current = snapshot('2026-11-17T00:00:00Z')
        current['projects'] = {'count': 0, 'repositories': []}
        self.write_snapshot(current)
        self.registry.run['id'] = '106123457'
        self.registry.version = '1.0.106123457'
        result = archive.archive_history(self.source, self.output, self.registry)
        self.assertEqual(len(result['entries']), 2)
        self.assertEqual(json.loads((self.output / 'history.json').read_text()), result)
        self.assertIn('https://github.com/owner/application', (self.output / 'README.md').read_text())
        self.assertEqual(self.registry.saved[-1][0], result)

    def test_incomplete_collection_cannot_reset_or_update_archive(self):
        current = snapshot()
        current['projects'] = None
        self.write_snapshot(current)
        with self.assertRaises(ValueError):
            archive.archive_history(self.source, self.output, self.registry)
        self.assertEqual(self.registry.saved, [])
        self.assertFalse(self.output.exists())

    def test_corrupt_previous_history_is_not_silently_replaced(self):
        self.registry.history = {'schema_version': 999}
        with self.assertRaises(ValueError):
            archive.archive_history(self.source, self.output, self.registry)
        self.assertEqual(self.registry.saved, [])
        self.assertFalse(self.output.exists())

    def test_archive_failure_is_reported_and_local_diagnostic_report_remains(self):
        with patch.object(self.registry, 'save', side_effect=RuntimeError('Registry unavailable')):
            with self.assertRaisesRegex(RuntimeError, 'Registry unavailable'):
                archive.archive_history(self.source, self.output, self.registry)
        self.assertTrue((self.output / 'history.json').is_file())
        self.assertTrue((self.output / 'README.md').is_file())
        self.assertTrue((self.output / 'changes.md').is_file())

    def test_current_update_is_full_only_for_baseline_and_excludes_unchanged_inventory_later(self):
        archive.archive_history(self.source, self.output, self.registry)
        first_report = self.output / 'changes.md'
        self.assertTrue(first_report.is_file(), 'The per-run changes report must be generated')
        self.assertIn('https://github.com/owner/application', first_report.read_text())
        self.assertIn('https://github.com/vendor/boards/tree/pinned/boards/model-a', first_report.read_text())
        current = snapshot('2026-11-17T00:00:00Z')
        project = current['projects']['repositories'][0]
        project.update(repository_id=456, repository='owner/new-app', url='https://github.com/owner/new-app')
        self.write_snapshot(current)
        self.registry.run['id'] = '106123457'
        self.registry.version = '1.0.106123457'
        archive.archive_history(self.source, self.output, self.registry)
        report = first_report.read_text()
        self.assertIn('https://github.com/owner/new-app', report)
        self.assertIn('https://github.com/owner/application', report)
        self.assertNotIn('vendor/boards/tree/pinned/boards/model-a', report)
        self.assertEqual(self.registry.saved[-1][2], report)
        self.assertEqual({p.name for p in self.output.iterdir()}, {'history.json', 'README.md', 'changes.md'})

    def test_unchanged_scan_current_update_does_not_repeat_baseline_links(self):
        archive.archive_history(self.source, self.output, self.registry)
        self.write_snapshot(snapshot('2026-11-17T00:00:00Z'))
        self.registry.run['id'] = '106123457'
        self.registry.version = '1.0.106123457'
        archive.archive_history(self.source, self.output, self.registry)
        self.assertTrue((self.output / 'changes.md').is_file(), 'The per-run changes report must be generated')
        report = (self.output / 'changes.md').read_text()
        self.assertIn('无变化', report)
        self.assertNotIn('https://github.com/', report)
        self.assertNotIn('Current inventory', (self.output / 'README.md').read_text())

    def test_command_failure_exits_nonzero_without_printing_private_inventory(self):
        with patch.object(archive, 'archive_history', side_effect=RuntimeError('Archive unavailable')):
            self.assertEqual(archive.main(['--source', str(self.source), '--output', str(self.output)]), 1)

    def test_real_registry_round_trip_records_replacements_with_unchanged_totals(self):
        from gitlab_ecosystem_history import HistoryRegistry
        from public_ecosystem_stats import public_files
        from test_gitlab_ecosystem_history import FakeRegistry

        env = {'CI_API_V4_URL': 'https://gitlab.example/api/v4', 'CI_PROJECT_ID': '42',
               'CI_JOB_TOKEN': 'synthetic-job-token', 'CI_JOB_ID': '1234', 'CI_COMMIT_BRANCH': 'main',
               'CI_PROJECT_URL': 'https://gitlab.example/team/project', 'CI_PROJECT_VISIBILITY': 'private'}
        remote = FakeRegistry()
        first_client = HistoryRegistry.from_environment(env)
        first_client.opener = remote
        archive.archive_history(self.source, self.output, first_client)
        list_url = ('https://gitlab.example/api/v4/projects/42/packages?package_type=generic'
                    '&package_name=ecosystem-history&order_by=created_at&sort=desc&per_page=100&page=1')
        remote.pages[list_url] = [{'id': 1, 'name': 'ecosystem-history', 'version': '1.0.1234',
                                   'package_type': 'generic'}]
        current = snapshot('2026-11-17T00:00:00Z')
        project = current['projects']['repositories'][0]
        project.update(repository_id=456, repository='owner/new-app', url='https://github.com/owner/new-app')
        project['evidence'][0]['url'] = 'https://github.com/owner/new-app/blob/pinned/main/idf_component.yml'
        self.write_snapshot(current)
        second_client = HistoryRegistry.from_environment(dict(env, CI_JOB_ID='1235'))
        second_client.opener = remote
        history = archive.archive_history(self.source, self.output, second_client)
        change = history['entries'][-1]
        self.assertEqual(change['delta']['projects'], 0)
        self.assertEqual(change['added']['projects'][0]['url'], 'https://github.com/owner/new-app')
        self.assertEqual(change['removed']['projects'][0]['url'], 'https://github.com/owner/application')
        saved = next(data for url, data in remote.files.items() if '/1.0.1235/history.json' in url)
        self.assertEqual(json.loads(saved), history)
        self.assertNotIn('owner/new-app', json.dumps(public_files(current)))

    def test_legacy_two_file_archive_continues_into_new_three_file_job_version(self):
        from gitlab_ecosystem_history import HistoryRegistry
        from test_gitlab_ecosystem_history import FakeRegistry

        env = {'CI_API_V4_URL': 'https://gitlab.example/api/v4', 'CI_PROJECT_ID': '42',
               'CI_JOB_TOKEN': 'synthetic-job-token', 'CI_JOB_ID': '1235', 'CI_COMMIT_BRANCH': 'main',
               'CI_PROJECT_URL': 'https://gitlab.example/team/project', 'CI_PROJECT_VISIBILITY': 'private'}
        legacy = archive.build_history(None, snapshot(), {
            'id': '1234', 'url': 'https://gitlab.example/team/project/-/jobs/1234', 'ref': 'main'})
        self.assertEqual(legacy['schema_version'], 1)
        base = 'https://gitlab.example/api/v4/projects/42/packages/generic/ecosystem-history/'
        old_prefix, new_prefix = base + '1.0.1234/', base + '1.0.1235/'
        old_files = {old_prefix + 'history.json': json.dumps(legacy).encode(),
                     old_prefix + 'README.md': b'# Legacy history\nhttps://github.com/owner/application\n'}
        remote = FakeRegistry()
        remote.files.update(old_files)
        list_url = ('https://gitlab.example/api/v4/projects/42/packages?package_type=generic'
                    '&package_name=ecosystem-history&order_by=created_at&sort=desc&per_page=100&page=1')
        remote.pages[list_url] = [{'id': 1, 'name': 'ecosystem-history', 'version': '1.0.1234',
                                   'package_type': 'generic'}]
        client = HistoryRegistry.from_environment(env)
        client.opener = remote
        self.write_snapshot(snapshot('2026-11-17T00:00:00Z'))

        result = archive.archive_history(self.source, self.output, client)

        self.assertEqual(len(result['entries']), 2)
        self.assertEqual(result['entries'][0], legacy['entries'][0])
        self.assertFalse(result['entries'][-1]['baseline'])
        self.assertEqual({url: data for url, data in remote.files.items() if url.startswith(old_prefix)}, old_files)
        self.assertFalse(any(method == 'PUT' and url.startswith(old_prefix) for method, url, data in remote.calls))
        self.assertEqual({url[len(new_prefix):] for url in remote.files if url.startswith(new_prefix)},
                         {'history.json', 'README.md', 'changes.md'})
        self.assertEqual(json.loads(remote.files[new_prefix + 'history.json']), result)
        changes = remote.files[new_prefix + 'changes.md'].decode()
        self.assertIn('无变化', changes)
        self.assertNotIn('https://github.com/', changes)
        self.assertEqual(changes, (self.output / 'changes.md').read_text())

    def test_pipeline_archives_before_publication_and_keeps_private_report_in_artifacts(self):
        config = Path(__file__).resolve().parents[3] / '.gitlab/ci/ecosystem-stats.yml'
        job = yaml.safe_load(config.read_text())['update_ecosystem_stats']
        script = job['script']
        collect = next(index for index, command in enumerate(script) if 'collect_ecosystem_stats.py' in command)
        retain = next(index for index, command in enumerate(script) if 'archive_ecosystem_history.py' in command)
        publish = next(index for index, command in enumerate(script) if 'publish_ecosystem_stats.py' in command)
        self.assertLess(collect, retain)
        self.assertLess(retain, publish)
        self.assertEqual(job['resource_group'], 'ecosystem-stats')
        self.assertEqual(job['artifacts']['access'], 'developer')
        self.assertIn('.ecosystem-private/report/history/', job['artifacts']['paths'])


if __name__ == '__main__':
    unittest.main()
