# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

import copy
import importlib.util
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
SPEC = importlib.util.find_spec('ecosystem_history')
history = __import__('ecosystem_history') if SPEC else None


def origin(name, revision='a'):
    return {'repository': 'vendor/boards', 'path': 'boards/' + name,
            'revision': revision, 'url': 'https://github.com/vendor/boards/tree/{}/boards/{}'.format(revision, name)}


def board(name='one', origins=None):
    origins = origins or [origin(name)]
    return {'board': name, 'path': origins[0]['path'], 'chip': 'esp32s3', 'origins': origins}


def candidate(name='extra', decision='include', review_identity='approved-extra'):
    item = board(name, [dict(origin(name), repository='user/app')])
    item.update(id='candidate-' + name, primary=item['origins'][0], decision=decision)
    if review_identity is not None:
        item['review_identity'] = review_identity
    return item


def project(identifier=10, name='user/app', revision='a'):
    return {'repository_id': identifier, 'repository': name, 'url': 'https://github.com/' + name,
            'revision': revision, 'evidence': [{'url': 'https://github.com/{}/blob/{}/idf_component.yml'.format(name, revision)}]}


def snapshot(at='2026-11-15T23:00:00Z', known=None, candidates=None, projects=None):
    known = [board()] if known is None else known
    candidates = [] if candidates is None else candidates
    projects = [project()] if projects is None else projects
    included = sum(item['decision'] == 'include' for item in candidates)
    packs = [{'repository': 'vendor/boards', 'path': 'boards', 'listed': True,
              'count': len(known), 'boards': [item['board'] for item in known], 'definitions': known}]
    if candidates:
        packs.append({'repository': 'user/app', 'path': 'boards', 'listed': False,
                      'count': len(candidates), 'boards': [item['board'] for item in candidates],
                      'definitions': copy.deepcopy(candidates)})
    summary = {'known_count': len(known), 'included_count': included,
               'aliased_count': sum(item['decision'] == 'alias' for item in candidates),
               'pending_count': sum(item['decision'] == 'pending' for item in candidates),
               'approved_count': len(known) + included, 'candidates': candidates}
    return {'schema_version': 1, 'updated_at': at,
            'boards': {'count': len(known) + len(candidates), 'approved_count': len(known) + included,
                       'discovery_enabled': True, 'packs': packs, 'decision_summary': summary},
            'projects': {'count': len(projects), 'repositories': projects}}


def run(identifier='100'):
    return {'id': identifier, 'url': 'https://gitlab.example/project/-/jobs/' + identifier, 'ref': 'main'}


class EcosystemHistoryTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(history, 'The private ecosystem history module must exist')

    def test_first_scan_is_baseline_without_calling_existing_entities_new(self):
        source = snapshot(candidates=[candidate(), candidate('alias', 'alias'), candidate('pending', 'pending')])
        before = copy.deepcopy(source)
        result = history.build_history(None, source, run())
        self.assertEqual(source, before)
        self.assertEqual(result['schema_version'], 1)
        self.assertEqual(len(result['latest']['boards']), 2)
        entry = result['entries'][0]
        self.assertTrue(entry['baseline'])
        self.assertEqual(entry['counts'], {'boards': 2, 'projects': 1})
        self.assertEqual(entry['delta'], {'boards': 0, 'projects': 0})
        self.assertEqual(entry['added'], {'boards': [], 'projects': []})
        self.assertEqual(entry['removed'], {'boards': [], 'projects': []})
        self.assertEqual(entry['date'], '2026-11-16')

    def test_counts_and_new_removed_links_are_recorded_even_when_net_total_is_unchanged(self):
        previous = history.build_history(None, snapshot(), run())
        new = snapshot('2026-11-16T23:00:00Z', known=[board('two')], projects=[project(11, 'user/new')])
        result = history.build_history(previous, new, run('101'))
        entry = result['entries'][-1]
        self.assertFalse(entry['baseline'])
        self.assertEqual(entry['delta'], {'boards': 0, 'projects': 0})
        self.assertEqual(entry['added']['boards'][0]['name'], 'two')
        self.assertIn('/boards/two', entry['added']['boards'][0]['url'])
        self.assertEqual(entry['removed']['boards'][0]['name'], 'one')
        self.assertEqual(entry['added']['projects'][0]['url'], 'https://github.com/user/new')
        self.assertEqual(entry['removed']['projects'][0]['url'], 'https://github.com/user/app')

    def test_revision_rename_and_source_order_changes_refresh_metadata_without_new_entities(self):
        a, b = origin('one'), origin('copy')
        first = snapshot(known=[board(origins=[a, b])], projects=[project()])
        previous = history.build_history(None, first, run())
        current = snapshot('2026-11-16T00:00:00Z', known=[board('renamed', [origin('copy', 'b'), origin('one', 'b')])],
                           projects=[project(10, 'new-owner/renamed', 'b')])
        result = history.build_history(previous, current, run('101'))
        self.assertEqual(result['entries'][-1]['added'], {'boards': [], 'projects': []})
        self.assertEqual(result['entries'][-1]['removed'], {'boards': [], 'projects': []})
        for section in ('boards', 'projects'):
            self.assertEqual(result['latest'][section][0]['id'], previous['latest'][section][0]['id'])
        self.assertEqual(result['latest']['projects'][0]['url'], 'https://github.com/new-owner/renamed')
        self.assertIn('/tree/b/', result['latest']['boards'][0]['url'])

    def test_review_identity_survives_representative_change_and_transition_to_known_component(self):
        previous = history.build_history(None, snapshot(known=[], candidates=[candidate()]), run())
        changed = candidate('different-copy')
        middle = history.build_history(previous, snapshot('2026-11-16T00:00:00Z', known=[], candidates=[changed]), run('101'))
        final = history.build_history(middle, snapshot('2026-11-16T01:00:00Z', known=[board('now-known', changed['origins'])]), run('102'))
        self.assertEqual(final['latest']['boards'][0]['id'], previous['latest']['boards'][0]['id'])
        self.assertEqual(final['entries'][-1]['added']['boards'], [])
        self.assertEqual(final['entries'][-1]['removed']['boards'], [])

    def test_every_successful_scan_is_kept_including_zero_change_and_same_day_reruns(self):
        previous = history.build_history(None, snapshot(), run())
        second = history.build_history(previous, snapshot('2026-11-16T00:00:00Z'), run('101'))
        third = history.build_history(second, snapshot('2026-11-16T01:00:00Z'), run('102'))
        self.assertEqual(len(third['entries']), 3)
        self.assertTrue(all(entry['date'] == '2026-11-16' for entry in third['entries']))
        rendered = history.render_history(third)
        self.assertIn('2026-11-16', rendered)
        self.assertIn('102', rendered)
        self.assertIn('No changes / 无变化', rendered)

    def test_same_run_is_idempotent_but_reuse_with_different_data_is_rejected(self):
        source = snapshot()
        previous = history.build_history(None, source, run())
        self.assertEqual(history.build_history(previous, copy.deepcopy(source), run()), previous)
        changed = copy.deepcopy(source)
        changed['projects']['repositories'][0]['revision'] = 'different'
        with self.assertRaises(ValueError):
            history.build_history(previous, changed, run())

    def test_backward_time_or_malformed_existing_history_does_not_reset_baseline(self):
        previous = history.build_history(None, snapshot(), run())
        with self.assertRaises(ValueError):
            history.build_history(previous, snapshot('2026-11-14T00:00:00Z'), run('101'))
        for malformed in ({}, {'schema_version': 2}, dict(previous, entries=[]), dict(previous, updated_at='bad')):
            with self.subTest(history=malformed):
                with self.assertRaises(ValueError):
                    history.build_history(malformed, snapshot('2026-11-16T00:00:00Z'), run('101'))

    def test_count_mismatches_missing_identities_and_duplicates_are_rejected(self):
        bad = snapshot()
        bad['boards']['approved_count'] += 1
        missing = snapshot()
        del missing['projects']['repositories'][0]['repository_id']
        for source in (bad, missing, snapshot(known=[board(), board()]), snapshot(projects=[project(), project()]),
                       snapshot(candidates=[candidate(review_identity=None)])):
            with self.subTest(source=source):
                with self.assertRaises(ValueError):
                    history.build_history(None, source, run())

    def test_ambiguous_cross_scan_merge_or_split_is_rejected(self):
        previous = history.build_history(None, snapshot(known=[board('one'), board('two')]), run())
        with self.assertRaises(ValueError):
            history.build_history(previous, snapshot('2026-11-16T00:00:00Z', known=[board(origins=[origin('one'), origin('two')])]), run('101'))
        previous = history.build_history(None, snapshot(known=[board(origins=[origin('one'), origin('two')])]), run())
        with self.assertRaises(ValueError):
            history.build_history(previous, snapshot('2026-11-16T00:00:00Z', known=[board('one'), board('two')]), run('101'))

    def test_nonzero_changes_match_counts_and_retained_historical_source_aliases(self):
        previous = history.build_history(None, snapshot(known=[board(origins=[origin('one'), origin('copy')])]), run())
        middle = history.build_history(previous, snapshot('2026-11-16T00:00:00Z', known=[board(origins=[origin('copy')]), board('new')], projects=[]), run('101'))
        final = history.build_history(middle, snapshot('2026-11-16T01:00:00Z', known=[board(origins=[origin('one')]), board('new')], projects=[]), run('102'))
        self.assertEqual(middle['entries'][-1]['previous_counts'], {'boards': 1, 'projects': 1})
        self.assertEqual(middle['entries'][-1]['delta'], {'boards': 1, 'projects': -1})
        self.assertEqual(len(middle['entries'][-1]['added']['boards']), 1)
        self.assertEqual(len(middle['entries'][-1]['removed']['projects']), 1)
        self.assertEqual(final['entries'][-1]['added'], {'boards': [], 'projects': []})
        self.assertEqual(final['entries'][-1]['removed'], {'boards': [], 'projects': []})

    def test_corrupted_existing_latest_ids_or_event_deltas_are_rejected(self):
        baseline = history.build_history(None, snapshot(), run())
        previous = history.build_history(baseline, snapshot('2026-11-16T00:00:00Z', known=[board(), board('new')]), run('101'))
        bad_identity = copy.deepcopy(previous)
        bad_identity['latest']['boards'][-1]['id'] = 'unrelated'
        bad_delta = copy.deepcopy(previous)
        bad_delta['entries'][-1]['delta']['boards'] = 20
        bad_schema = dict(previous, schema_version=True)
        for malformed in (bad_identity, bad_delta, bad_schema):
            with self.subTest(history=malformed):
                with self.assertRaises(ValueError):
                    history.build_history(malformed, snapshot('2026-11-16T01:00:00Z'), run('102'))

    def test_existing_project_id_must_match_exactly_one_canonical_repository_identity(self):
        baseline = history.build_history(None, snapshot(), run())
        cases = [('project-11', [['project', '10']]),
                 ('project-10', [['project', '10'], ['project', '11']]),
                 ('project-10', [['project', '010']]),
                 ('project-10', [['project', '\u0661\u0660']])]
        for identifier, identities in cases:
            malformed = copy.deepcopy(baseline)
            malformed['latest']['projects'][0].update(id=identifier, identities=identities)
            with self.subTest(identifier=identifier, identities=identities):
                with self.assertRaisesRegex(ValueError, 'project|identity'):
                    history.build_history(malformed, snapshot('2026-11-16T00:00:00Z'), run('101'))

    def test_history_cannot_mix_source_refs_in_existing_or_incoming_runs(self):
        baseline = history.build_history(None, snapshot(), run())
        different_ref = dict(run('101'), ref='feature/test')
        with self.assertRaisesRegex(ValueError, 'ref'):
            history.build_history(baseline, snapshot('2026-11-16T00:00:00Z'), different_ref)
        previous = history.build_history(baseline, snapshot('2026-11-16T00:00:00Z'), run('101'))
        previous['entries'][-1]['run']['ref'] = 'feature/test'
        with self.assertRaisesRegex(ValueError, 'ref'):
            history.render_history(previous)
        with self.assertRaisesRegex(ValueError, 'ref'):
            history.build_history(previous, snapshot('2026-11-16T01:00:00Z'), run('102'))

    def test_added_event_identity_must_be_retained_in_latest_inventory(self):
        baseline = history.build_history(None, snapshot(), run())
        previous = history.build_history(baseline, snapshot('2026-11-16T00:00:00Z', known=[board(), board('new')]), run('101'))
        added_id = previous['entries'][-1]['added']['boards'][0]['id']
        corrupted = next(entity for entity in previous['latest']['boards'] if entity['id'] == added_id)
        corrupted['identities'] = [['source', 'unrelated/repository', 'boards/other', 'esp32s3']]
        with self.assertRaisesRegex(ValueError, 'identity|identities'):
            history.render_history(previous)

    def test_reappearing_same_id_cannot_replace_its_historical_identity(self):
        baseline = history.build_history(None, snapshot(), run())
        added = history.build_history(baseline, snapshot('2026-11-16T00:00:00Z', known=[board(), board('new')]), run('101'))
        removed = history.build_history(added, snapshot('2026-11-16T01:00:00Z'), run('102'))
        returned = history.build_history(removed, snapshot('2026-11-16T02:00:00Z', known=[board(), board('new')]), run('103'))
        added_id = returned['entries'][-1]['added']['boards'][0]['id']
        entities = [returned['entries'][-1]['added']['boards'][0],
                    next(entity for entity in returned['latest']['boards'] if entity['id'] == added_id)]
        for entity in entities:
            entity['identities'] = [['source', 'unrelated/repository', 'boards/other', 'esp32s3']]
        with self.assertRaisesRegex(ValueError, 'identity|identities'):
            history.render_history(returned)

    def _changes(self, value, run_id=None):
        renderer = getattr(history, 'render_changes', None)
        self.assertTrue(callable(renderer), 'A readable per-run changes renderer must exist')
        return renderer(value, run_id=run_id)

    def test_changes_first_baseline_lists_complete_roster_without_new_classification(self):
        result = history.build_history(None, snapshot(candidates=[candidate(), candidate('pending', 'pending')]), run())
        before = copy.deepcopy(result)
        rendered = self._changes(result)
        self.assertIn('首次基线', rendered)
        self.assertIn('完整清单', rendered)
        self.assertIn('one (esp32s3)', rendered)
        self.assertIn('extra (esp32s3)', rendered)
        self.assertIn('https://github.com/user/app', rendered)
        self.assertIn('2026-11-16 07:00:00 (UTC+8)', rendered)
        self.assertIn('main', rendered)
        self.assertIn('Run 100', rendered)
        self.assertNotIn('pending (esp32s3)', rendered)
        self.assertNotIn('有新增', rendered)
        self.assertEqual(result, before)

    def test_changes_later_scan_lists_only_new_names_and_links(self):
        baseline = history.build_history(None, snapshot(), run())
        result = history.build_history(baseline, snapshot('2026-11-16T00:00:00Z', known=[board(), board('brand-new')],
                                       projects=[project(), project(11, 'owner/new-app')]), run('101'))
        rendered = self._changes(result)
        self.assertIn('有新增', rendered)
        self.assertIn('brand-new (esp32s3)', rendered)
        self.assertIn('https://github.com/owner/new-app', rendered)
        self.assertIn('| 1 | 2 | +1 |', rendered)
        self.assertNotIn('one (esp32s3)', rendered)
        self.assertNotIn('https://github.com/user/app', rendered)
        self.assertNotIn('完整清单', rendered)

    def test_changes_zero_change_same_day_scan_is_explicit_and_has_no_roster(self):
        baseline = history.build_history(None, snapshot(), run())
        result = history.build_history(baseline, snapshot('2026-11-16T00:00:00Z'), run('101'))
        rendered = self._changes(result)
        self.assertIn('无变化', rendered)
        self.assertIn('2026-11-16 08:00:00 (UTC+8)', rendered)
        self.assertIn('| 1 | 1 | +0 |', rendered)
        self.assertNotIn('one (esp32s3)', rendered)
        self.assertNotIn('https://github.com/user/app', rendered)
        self.assertNotIn('| Name / 名称 |', rendered)

    def test_changes_removed_only_and_zero_net_replacement_have_clear_statuses(self):
        baseline = history.build_history(None, snapshot(), run())
        removed = history.build_history(baseline, snapshot('2026-11-16T00:00:00Z', known=[], projects=[]), run('101'))
        removed_text = self._changes(removed)
        self.assertIn('有移除', removed_text)
        self.assertIn('one (esp32s3)', removed_text)
        self.assertIn('https://github.com/user/app', removed_text)
        self.assertIn('| 1 | 0 | -1 |', removed_text)
        replacement = history.build_history(baseline, snapshot('2026-11-16T00:00:00Z', known=[board('replacement')],
                                            projects=[project(11, 'owner/replacement')]), run('101'))
        rendered = self._changes(replacement)
        self.assertIn('有新增和移除', rendered)
        self.assertIn('replacement (esp32s3)', rendered)
        self.assertIn('one (esp32s3)', rendered)
        self.assertIn('| 1 | 1 | +0 |', rendered)

    def test_changes_explicit_old_run_uses_its_own_event_and_rejects_unknown_or_old_baseline(self):
        baseline = history.build_history(None, snapshot(), run())
        second = history.build_history(baseline, snapshot('2026-11-16T00:00:00Z', known=[board(), board('second')]), run('101'))
        third = history.build_history(second, snapshot('2026-11-16T01:00:00Z', known=[board(), board('second'), board('third')]), run('102'))
        rendered = self._changes(third, '101')
        self.assertIn('Run 101', rendered)
        self.assertIn('second (esp32s3)', rendered)
        self.assertNotIn('third (esp32s3)', rendered)
        self.assertNotIn('Run 102', rendered)
        self.assertIn('| 1 | 2 | +1 |', rendered)
        with self.assertRaisesRegex(ValueError, 'baseline archive version.*README.md for legacy archives, changes.md for new archives'):
            self._changes(third, '100')
        with self.assertRaisesRegex(ValueError, 'run'):
            self._changes(third, 'missing')

    def test_changes_escapes_names_and_urls_but_keeps_only_changed_items(self):
        baseline = history.build_history(None, snapshot(), run())
        special = board('new [name]|<b>')
        special['origins'][0]['url'] = 'https://github.com/vendor/boards/tree/a/boards/new_(name)'
        result = history.build_history(baseline, snapshot('2026-11-16T00:00:00Z', known=[board(), special]), run('101'))
        rendered = self._changes(result)
        self.assertIn('new \\[name\\]\\|&lt;b&gt;', rendered)
        self.assertIn('new_%28name%29', rendered)
        self.assertNotIn('<b>', rendered)
        self.assertNotIn('one (esp32s3)', rendered)

    def test_cumulative_report_links_current_update_and_omits_full_current_roster(self):
        baseline = history.build_history(None, snapshot(), run())
        second = history.build_history(baseline, snapshot('2026-11-16T00:00:00Z', known=[board(), board('second')]), run('101'))
        rendered = history.render_history(second)
        self.assertIn('[本轮更新 / Current update](changes.md)', rendered)
        self.assertIn('history.json', rendered)
        self.assertIn('second (esp32s3)', rendered)
        self.assertNotIn('Current inventory', rendered)
        self.assertNotIn('当前完整清单', rendered)
        self.assertNotIn('one (esp32s3)', rendered)
        self.assertNotIn('https://github.com/user/app', rendered)

    def test_daily_summary_uses_last_scan_and_links_are_escaped(self):
        previous = history.build_history(None, snapshot(), run())
        special = board('new [name]|<b>')
        special['origins'][0]['url'] = 'https://github.com/vendor/boards/tree/a/boards/new_(name)'
        second = history.build_history(previous, snapshot('2026-11-16T01:00:00Z', known=[board(), special]), run('101'))
        final = history.build_history(second, snapshot('2026-11-16T23:00:00Z', known=[board(), special, board('third')]), run('102'))
        rendered = history.render_history(final)
        self.assertIn('| 2026-11-16 | 2 | 1 |', rendered)
        self.assertIn('| 2026-11-17 | 3 | 1 | +1 | +0 |', rendered)
        self.assertIn('new \\[name\\]\\|&lt;b&gt;', rendered)
        self.assertIn('new_%28name%29', rendered)
        self.assertIn('Baseline / 基线', rendered)
        self.assertNotIn('<b>', rendered)


if __name__ == '__main__':
    unittest.main()
