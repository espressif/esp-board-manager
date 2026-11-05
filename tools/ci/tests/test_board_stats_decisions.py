# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

import copy
import importlib.util
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from discover_board_stats import CollectionError

SPEC = importlib.util.find_spec('board_stats_decisions')
decisions = __import__('board_stats_decisions') if SPEC else None


def source(repo, path):
    return {'repository': repo, 'path': path, 'url': 'https://github.com/{}/tree/pinned/{}'.format(repo, path)}


def candidate(identifier='outside-a', repo='user/app', path='boards/custom', chip='esp32s3', **extra):
    item = {'id': identifier, 'board': 'custom', 'chip': chip, 'primary': source(repo, path),
            'origins': [source(repo, path)], 'known_matches': [], 'note': '', 'evidence': []}
    item.update(extra)
    return item


def ledger(*entries):
    return {'schema_version': 1, 'reviewed_at': '2026-11-06', 'decisions': list(entries)}


def decision(action='include', identifier='outside-a', **extra):
    item = {'id': identifier, 'decision': action, 'reason': 'Reviewed by the maintainer',
            'selectors': [{'repository': 'user/app', 'path': 'boards/custom', 'chip': 'esp32s3'}]}
    item.update(extra)
    return item


class BoardDecisionTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(decisions, 'The independent decision projection module must exist')
        self.known_origin = source('vendor/known', 'boards/model_a')
        self.boards = {'count': 3, 'packs': [
            {'repository': 'vendor/known', 'path': 'boards', 'listed': True, 'count': 2,
             'definitions': [{'board': 'model_a', 'chip': 'esp32s3', 'path': 'boards/model_a',
                              'origins': [self.known_origin]}]},
            {'repository': 'user/app', 'path': 'boards', 'count': 1, 'definitions': []}]}
        self.review = {'candidates': [candidate()]}

    def test_include_adds_only_current_reviewed_candidates_without_mutating_inputs(self):
        before = copy.deepcopy((self.boards, self.review))
        result = decisions.apply_board_decisions(self.boards, self.review, ledger(decision()))
        self.assertEqual((result['known_count'], result['included_count'], result['approved_count']), (2, 1, 3))
        self.assertEqual((result['aliased_count'], result['pending_count']), (0, 0))
        self.assertEqual(result['candidates'][0]['decision'], 'include')
        self.assertEqual((self.boards, self.review), before)

    def test_unknown_candidate_stays_pending_even_when_its_name_looks_known(self):
        self.review['candidates'][0]['board'] = 'model_a'
        result = decisions.apply_board_decisions(self.boards, self.review, ledger())
        self.assertEqual((result['included_count'], result['pending_count'], result['approved_count']), (0, 1, 2))

    def test_id_alone_never_authorizes_a_changed_source(self):
        entry = decision(selectors=[{'repository': 'other/app', 'path': 'boards/custom', 'chip': 'esp32s3'}])
        result = decisions.apply_board_decisions(self.boards, self.review, ledger(entry))
        self.assertEqual(result['candidates'][0]['decision'], 'pending')
        self.assertEqual(result['inactive_decision_ids'], ['outside-a'])

    def test_source_selector_can_match_a_secondary_origin_after_canonical_changes(self):
        item = candidate(identifier='outside-new', repo='new/upstream', path='boards/new_primary')
        item['origins'].append(source('user/app', 'boards/custom'))
        result = decisions.apply_board_decisions(self.boards, {'candidates': [item]}, ledger(decision()))
        self.assertEqual(result['included_count'], 1)
        self.assertEqual(result['inactive_decision_ids'], [])

    def test_lineage_selector_keeps_family_vendor_model_and_chip_scope(self):
        item = candidate(lineage={'family': 'claw', 'board': 'vendor/model'})
        entry = decision(selectors=[{'family': 'claw', 'board': 'vendor/model', 'chip': 'ESP32S3'}])
        result = decisions.apply_board_decisions(self.boards, {'candidates': [item]}, ledger(entry))
        self.assertEqual(result['included_count'], 1)
        for change in ({'family': 'other', 'board': 'vendor/model'}, {'family': 'claw', 'board': 'other/model'}):
            item['lineage'] = change
            result = decisions.apply_board_decisions(self.boards, {'candidates': [item]}, ledger(entry))
            self.assertEqual(result['pending_count'], 1)
        item['lineage'] = {'family': 'claw', 'board': 'vendor/model'}
        item['chip'] = 'ESP32-S3'
        self.assertEqual(decisions.apply_board_decisions(self.boards, {'candidates': [item]}, ledger(entry))['pending_count'], 1)

    def test_older_review_can_recover_lineage_from_the_board_snapshot(self):
        self.boards['packs'][1]['definitions'] = [
            {'board': 'custom', 'chip': 'esp32s3', 'path': 'boards/custom',
             'lineage': {'family': 'legacy', 'board': 'vendor/custom'},
             'origins': self.review['candidates'][0]['origins']}]
        entry = decision(selectors=[{'family': 'legacy', 'board': 'vendor/custom', 'chip': 'esp32s3'}])
        self.assertEqual(decisions.apply_board_decisions(self.boards, self.review, ledger(entry))['included_count'], 1)

    def test_alias_resolves_current_known_source_and_does_not_add_to_approved_count(self):
        entry = decision('alias', target={'repository': 'vendor/known', 'path': 'boards/model_a'})
        result = decisions.apply_board_decisions(self.boards, self.review, ledger(entry))
        self.assertEqual((result['included_count'], result['aliased_count'], result['approved_count']), (0, 1, 2))
        self.assertEqual(result['candidates'][0]['target'], dict(self.known_origin, board='model_a', chip='esp32s3'))

    def test_missing_or_wrong_chip_alias_target_stays_pending_with_an_explanation(self):
        targets = [None, {}, {'repository': 'vendor/missing', 'path': 'boards/model_a'}]
        for target in targets:
            entry = decision('alias', **({'target': target} if target is not None else {}))
            result = decisions.apply_board_decisions(self.boards, self.review, ledger(entry))
            self.assertEqual(result['pending_count'], 1)
            self.assertIn('target', result['candidates'][0]['decision_reason'].lower())
        self.boards['packs'][0]['definitions'][0]['chip'] = 'esp32p4'
        entry = decision('alias', target={'repository': 'vendor/known', 'path': 'boards/model_a'})
        result = decisions.apply_board_decisions(self.boards, self.review, ledger(entry))
        self.assertEqual(result['pending_count'], 1)
        self.assertNotIn('target', result['candidates'][0])

    def test_include_returns_to_pending_when_new_known_hints_appear(self):
        self.review['candidates'][0]['known_matches'] = [{'board': 'model_a', 'url': self.known_origin['url']}]
        result = decisions.apply_board_decisions(self.boards, self.review, ledger(decision()))
        self.assertEqual((result['included_count'], result['pending_count']), (0, 1))
        self.assertIn('known', result['candidates'][0]['decision_reason'].lower())

    def test_reviewed_distinct_known_target_allows_the_false_positive_hint(self):
        self.review['candidates'][0]['known_matches'] = [{'board': 'model_a', 'url': self.known_origin['url']}]
        entry = decision(distinct_from=[{'repository': 'vendor/known', 'path': 'boards/model_a'}])
        result = decisions.apply_board_decisions(self.boards, self.review, ledger(entry))
        self.assertEqual((result['included_count'], result['pending_count'], result['approved_count']), (1, 0, 3))

    def test_new_or_unrecognized_known_hint_still_blocks_a_distinct_identity(self):
        extra = source('vendor/known', 'boards/model_b')
        self.boards['packs'][0]['definitions'].append(
            {'board': 'model_b', 'chip': 'esp32s3', 'path': 'boards/model_b', 'origins': [extra]})
        base = candidate(known_matches=[{'board': 'model_a', 'url': self.known_origin['url']}])
        other = candidate('outside-copy', 'copy/project', 'boards/custom')
        entry = decision(selectors=[
            {'repository': 'user/app', 'path': 'boards/custom', 'chip': 'esp32s3'},
            {'repository': 'copy/project', 'path': 'boards/custom', 'chip': 'esp32s3'}],
            distinct_from=[{'repository': 'vendor/known', 'path': 'boards/model_a'}])
        accepted = decisions.apply_board_decisions(self.boards, {'candidates': [base, other]}, ledger(entry))
        self.assertEqual((accepted['included_count'], accepted['aliased_count']), (1, 1))
        for new_url in (extra['url'], 'https://github.com/unrecognized/source'):
            other['known_matches'] = [{'board': 'model_b', 'url': new_url}]
            result = decisions.apply_board_decisions(self.boards, {'candidates': [base, other]}, ledger(entry))
            self.assertEqual((result['included_count'], result['aliased_count'], result['pending_count']), (0, 0, 2))

    def test_conflicting_actions_or_alias_targets_fail_closed(self):
        alias = decision('alias', identifier='outside-alias', target={'repository': 'vendor/known', 'path': 'boards/model_a'})
        with self.assertRaises(CollectionError):
            decisions.apply_board_decisions(self.boards, self.review, ledger(decision(), alias))
        different = decision('alias', identifier='outside-other', target={'repository': 'vendor/known', 'path': 'boards/model_b'})
        with self.assertRaises(CollectionError):
            decisions.apply_board_decisions(self.boards, self.review, ledger(alias, different))

    def test_repeated_same_decision_collapses_and_disappeared_entries_do_not_count(self):
        result = decisions.apply_board_decisions(self.boards, self.review, ledger(decision(), decision()))
        self.assertEqual(result['included_count'], 1)
        self.assertEqual(result['inactive_decision_ids'], [])
        result = decisions.apply_board_decisions(self.boards, {'candidates': []}, ledger(decision()))
        self.assertEqual((result['included_count'], result['approved_count']), (0, 2))
        self.assertEqual(result['inactive_decision_ids'], ['outside-a'])

    def test_one_include_identity_counts_once_for_multiple_current_sources(self):
        other = candidate('outside-copy', 'copy/project', 'boards/custom')
        entry = decision(selectors=[
            {'repository': 'user/app', 'path': 'boards/custom', 'chip': 'esp32s3'},
            {'repository': 'copy/project', 'path': 'boards/custom', 'chip': 'esp32s3'}])
        review = {'candidates': [candidate(), other]}
        before = copy.deepcopy(review)
        result = decisions.apply_board_decisions(self.boards, review, ledger(entry))
        self.assertEqual((result['included_count'], result['aliased_count'], result['approved_count']), (1, 1, 3))
        included = next(item for item in result['candidates'] if item['decision'] == 'include')
        alias = next(item for item in result['candidates'] if item['decision'] == 'alias')
        self.assertEqual(alias['target'], dict(included['primary'], board=included['board'], chip=included['chip']))
        self.assertEqual(review, before)
        reversed_result = decisions.apply_board_decisions(self.boards, {'candidates': list(reversed(review['candidates']))}, ledger(entry))
        self.assertEqual({item['id']: item['decision'] for item in result['candidates']},
                         {item['id']: item['decision'] for item in reversed_result['candidates']})

    def test_repeated_identity_records_can_extend_the_reviewed_sources(self):
        extra = decision(selectors=[{'repository': 'copy/project', 'path': 'boards/custom', 'chip': 'esp32s3'}])
        result = decisions.apply_board_decisions(
            self.boards, {'candidates': [candidate(), candidate('outside-copy', 'copy/project', 'boards/custom')]},
            ledger(decision(), extra))
        self.assertEqual((result['included_count'], result['aliased_count']), (1, 1))

    def test_different_include_identities_cannot_be_bridged_by_one_candidate(self):
        with self.assertRaises(CollectionError):
            decisions.apply_board_decisions(self.boards, self.review, ledger(decision(), decision(identifier='outside-other')))

    def test_one_identity_cannot_have_conflicting_actions_or_targets_across_sources(self):
        extra_source = [{'repository': 'copy/project', 'path': 'boards/custom', 'chip': 'esp32s3'}]
        alias = decision('alias', selectors=extra_source, target={'repository': 'vendor/known', 'path': 'boards/model_a'})
        review = {'candidates': [candidate(), candidate('outside-copy', 'copy/project', 'boards/custom')]}
        with self.assertRaises(CollectionError):
            decisions.apply_board_decisions(self.boards, review, ledger(decision(), alias))
        other_target = decision('alias', target={'repository': 'vendor/known', 'path': 'boards/model_b'})
        with self.assertRaises(CollectionError):
            decisions.apply_board_decisions(self.boards, review, ledger(alias, other_target))

    def test_new_known_hint_blocks_every_source_for_the_same_include_identity(self):
        hinted = candidate('outside-copy', 'copy/project', 'boards/custom',
                           known_matches=[{'board': 'model_a', 'url': self.known_origin['url']}])
        entry = decision(selectors=[
            {'repository': 'user/app', 'path': 'boards/custom', 'chip': 'esp32s3'},
            {'repository': 'copy/project', 'path': 'boards/custom', 'chip': 'esp32s3'}])
        result = decisions.apply_board_decisions(self.boards, {'candidates': [candidate(), hinted]}, ledger(entry))
        self.assertEqual((result['included_count'], result['aliased_count'], result['pending_count']), (0, 0, 2))
        self.assertTrue(all('known' in item['decision_reason'].lower() for item in result['candidates']))

    def test_an_include_identity_cannot_alias_a_copy_with_a_different_chip(self):
        other = candidate('outside-other-chip', 'copy/project', 'boards/custom', chip='esp32p4')
        entry = decision(selectors=[
            {'repository': 'user/app', 'path': 'boards/custom', 'chip': 'esp32s3'},
            {'repository': 'copy/project', 'path': 'boards/custom', 'chip': 'esp32p4'}])
        result = decisions.apply_board_decisions(self.boards, {'candidates': [candidate(), other]}, ledger(entry))
        self.assertEqual((result['included_count'], result['aliased_count'], result['pending_count']), (0, 0, 2))
        self.assertTrue(all('chip' in item['decision_reason'].lower() for item in result['candidates']))

    def test_unsafe_or_empty_selectors_are_rejected(self):
        invalid = [[], [{}], [{'repository': 'user/app', 'path': 'boards/*', 'chip': 'esp32s3'}],
                   [{'repository': 'user/app', 'path': 'boards/../custom', 'chip': 'esp32s3'}],
                   [{'repository': 'bad repo/app', 'path': 'boards/custom', 'chip': 'esp32s3'}],
                   [{'repository': 'user/app', 'path': 'boards/custom', 'chip': ''}],
                   [{'family': '', 'board': 'vendor/model', 'chip': 'esp32s3'}]]
        for selectors in invalid:
            with self.subTest(selectors=selectors), self.assertRaises(CollectionError):
                decisions.apply_board_decisions(self.boards, self.review, ledger(decision(selectors=selectors)))


if __name__ == '__main__':
    unittest.main()
