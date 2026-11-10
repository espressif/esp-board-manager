# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

import copy
import importlib.util
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
SPEC = importlib.util.find_spec('review_board_stats')
review = __import__('review_board_stats') if SPEC else None


def origin(repo, path, info=None, files=None):
    result = {'repository': repo, 'path': path, 'revision': 'a' * 40,
              'url': 'https://github.com/{}/tree/{}/{}'.format(repo, 'a' * 40, path),
              'tree_sha': 'tree-' + path}
    if info is not None:
        result['info'] = info
    if files is not None:
        result['files'] = files
    return result


def definition(board, chip, *origins):
    return {'board': board, 'chip': chip, 'path': origins[0]['path'],
            'tree_sha': origins[0]['tree_sha'], 'origins': list(origins)}


def pack(repo, path, *definitions, listed=False):
    return {'repository': repo, 'path': path, 'listed': listed,
            'count': len(definitions), 'definitions': list(definitions)}


class OutsideBoardReviewTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(review, 'The independent board review module must exist')

    def test_declared_model_matches_known_board_without_reclassifying_counts(self):
        known = origin('vendor/known', 'boards/esp32_s3_box_3',
                       info={'board': 'ESP32-S3-BOX-3', 'manufacturer': 'Espressif'})
        outside = origin('user/application', 'boards/box/bmgr',
                         info={'board': 'ESP32 S3 BOX 3', 'version': '2.0'})
        boards = {'count': 2, 'packs': [
            pack('vendor/known', 'boards', definition('esp32_s3_box_3', 'esp32s3', known), listed=True),
            pack('user/application', 'boards/box', definition('bmgr', 'ESP32-S3', outside))]}
        before = copy.deepcopy(boards)
        result = review.review_outside_boards(boards)
        self.assertEqual(boards, before)
        self.assertEqual((result['candidate_count'], result['possible_known_count'], result['no_known_match_count']), (1, 1, 0))
        candidate = result['candidates'][0]
        self.assertEqual(candidate['status'], 'possible_known_model')
        self.assertEqual(candidate['known_matches'][0]['url'], known['url'])
        self.assertEqual(candidate['origins'], [outside])
        self.assertEqual(candidate['primary']['info'], outside['info'])

    def test_generic_bmgr_uses_parent_model_and_preserves_unmatched_models(self):
        known = origin('vendor/known', 'boards/esp32_s3_eye')
        matching = origin('user/app', 'boards/ESP32-S3-EYE/bmgr')
        unrelated = origin('user/app', 'boards/other_model/bmgr')
        result = review.review_outside_boards({'packs': [
            pack('vendor/known', 'boards', definition('esp32_s3_eye', 'esp32s3', known), listed=True),
            pack('user/app', 'boards', definition('bmgr', 'esp32s3', matching), definition('bmgr', 'esp32s3', unrelated))]})
        self.assertEqual(result['possible_known_count'], 1)
        self.assertEqual(result['no_known_match_count'], 1)
        self.assertNotEqual(result['candidates'][0]['id'], result['candidates'][1]['id'])

    def test_candidate_display_prefers_declared_model_then_bmgr_parent(self):
        declared = origin('vision/upstream', 'boards/model_a/bmgr', info={'board': 'Declared Model A'})
        generic = origin('vision/upstream', 'boards/model_b/bmgr', info={'board': 'bmgr'})
        plain = origin('vision/upstream', 'boards/model_c')
        result = review.review_outside_boards({'packs': [pack(
            'vision/upstream', 'boards', definition('bmgr', 'esp32s3', declared),
            definition('bmgr', 'esp32s3', generic), definition('model_c', 'esp32s3', plain))]})
        self.assertEqual([row['board'] for row in result['candidates']], ['Declared Model A', 'model_b', 'model_c'])
        self.assertEqual(result['candidates'][0]['primary']['info']['board'], 'Declared Model A')

    def test_different_chip_and_unreviewed_suffix_do_not_match(self):
        known = origin('vendor/known', 'boards/model_a')
        result = review.review_outside_boards({'packs': [
            pack('vendor/known', 'boards', definition('model_a', 'esp32s3', known), listed=True),
            pack('user/app', 'boards', definition('model_a', 'esp32p4', origin('user/app', 'boards/model_a')),
                 definition('model_a_pro', 'esp32s3', origin('user/app', 'boards/model_a_pro')))]})
        self.assertEqual(result['possible_known_count'], 0)
        self.assertTrue(all(row['status'] == 'no_known_match' for row in result['candidates']))

    def test_verified_legacy_aliases_mark_possible_match_without_merging(self):
        rules = [{'family': 'legacy', 'upstream': 'vendor/known', 'roots': [],
                  'aliases': {'old_box': 'new_box'}}]
        result = review.review_outside_boards({'packs': [
            pack('vendor/known', 'boards', definition('new_box', 'esp32s3', origin('vendor/known', 'boards/new_box')), listed=True),
            pack('user/app', 'boards', definition('old_box', 'esp32s3', origin('user/app', 'boards/old_box')))]}, rules)
        self.assertEqual(result['candidate_count'], 1)
        self.assertEqual(result['possible_known_count'], 1)

    def test_object_source_alias_does_not_escape_its_lineage_scope(self):
        known = origin('espressif/esp-board-manager', 'esp_boards/esp32_c3_devkitm_1')
        outside = origin('thirdparty/application', 'boards/acme/esp32c3_devkitm')
        lineages = [{'family': 'esp-claw', 'upstream': 'espressif/esp-claw',
                     'roots': [{'path': 'application/edge_agent/boards'}],
                     'aliases': {'espressif/esp32c3_devkitm': {
                         'repository': 'espressif/esp-board-manager',
                         'path': 'esp_boards/esp32_c3_devkitm_1'}}}]
        result = review.review_outside_boards({'packs': [
            pack('espressif/esp-board-manager', 'esp_boards',
                 definition('esp32_c3_devkitm_1', 'esp32c3', known), listed=True),
            pack('thirdparty/application', 'boards/acme', definition('esp32c3_devkitm', 'esp32c3', outside))]}, lineages)
        self.assertEqual(result['possible_known_count'], 0)
        self.assertEqual(result['candidates'][0]['status'], 'no_known_match')
        self.assertEqual(result['candidates'][0]['known_matches'], [])

    def test_both_configuration_hashes_are_required_for_similarity(self):
        files = {'board_devices.yaml': 'devices', 'board_peripherals.yaml': 'peripherals'}
        known = origin('vendor/known', 'boards/known_a', files=files)
        match = origin('user/app', 'boards/custom_a', files=files)
        partial = origin('user/app', 'boards/custom_b', files={'board_devices.yaml': 'devices'})
        changed = origin('user/app', 'boards/custom_c', files=dict(files, **{'board_peripherals.yaml': 'different'}))
        result = review.review_outside_boards({'packs': [
            pack('vendor/known', 'boards', definition('known_a', 'esp32s3', known), listed=True),
            pack('user/app', 'boards', *(definition('custom_' + suffix, 'esp32s3', item)
                                      for suffix, item in zip('abc', (match, partial, changed))))]})
        self.assertEqual((result['possible_known_count'], result['no_known_match_count']), (1, 2))
        matched = next(row for row in result['candidates'] if row['known_matches'])
        self.assertIn('device and peripheral', matched['known_matches'][0]['reason'])

    def test_merged_outside_origins_are_retained_using_exact_known_scope(self):
        known = origin('vendor/known', 'boards/a')
        copies = [origin('copy/repo', 'boards/a'), origin('vendor/known', 'boards_copy/a'),
                  origin('vendor/known', 'boards/nested/a')]
        result = review.review_outside_boards({'packs': [
            pack('vendor/known', 'boards', definition('a', 'esp32s3', known, *copies), listed=True)]})
        self.assertEqual(result['candidate_count'], 0)
        self.assertEqual(result['merged_known_origin_count'], 3)
        self.assertEqual({row['origin']['path'] for row in result['merged_known_origins']}, {item['path'] for item in copies})
        self.assertTrue(all(row['known']['url'] == known['url'] for row in result['merged_known_origins']))

    def test_candidate_ids_survive_revision_changes_and_ordering(self):
        a = origin('user/app', 'boards/a')
        b = origin('user/app', 'boards/b')
        boards = {'packs': [pack('user/app', 'boards', definition('a', 'esp32s3', a), definition('b', 'esp32s3', b))]}
        first = review.review_outside_boards(boards)
        boards['packs'][0]['definitions'].reverse()
        for item in boards['packs'][0]['definitions']:
            item['origins'][0]['revision'] = 'b' * 40
            item['origins'][0]['tree_sha'] = 'changed'
        second = review.review_outside_boards(boards)
        self.assertEqual({row['board']: row['id'] for row in first['candidates']},
                         {row['board']: row['id'] for row in second['candidates']})

    def test_ids_preserve_chip_distinctions_used_by_the_collector(self):
        first_board = definition('model_a', 'esp32s3', origin('first/app', 'boards/model_a'))
        second_board = definition('model_a', 'ESP32-S3', origin('second/app', 'boards/model_a'))
        for board in (first_board, second_board):
            board['lineage'] = {'family': 'upstream', 'board': 'vendor/model_a'}
        boards = {'updated_at': '2026-11-05T00:00:00Z', 'packs': [
            pack('first/app', 'boards', first_board), pack('second/app', 'boards', second_board)]}
        before = review.review_outside_boards(boards)
        self.assertEqual(len({row['id'] for row in before['candidates']}), 2)
        boards['updated_at'] = '2026-11-06T00:00:00Z'
        for board in (first_board, second_board):
            board['origins'][0]['revision'] = 'b' * 40
            board['origins'][0]['tree_sha'] = 'new-tree'
        after = review.review_outside_boards(boards)
        self.assertEqual({row['chip']: row['id'] for row in before['candidates']},
                         {row['chip']: row['id'] for row in after['candidates']})

    def test_reviewed_map_applies_to_any_origin_and_keeps_notes_and_evidence(self):
        known = origin('vendor/known', 'boards/a')
        primary = origin('upstream/application', 'boards/custom')
        copy_origin = origin('copy/application', 'nested/boards/custom')
        mapping = [{'repository': 'copy/application', 'path': copy_origin['path'],
                    'known_matches': [{'repository': 'vendor/known', 'path': 'boards/a', 'reason': 'Reviewed model lineage'}],
                    'note': 'Hardware revision remains unverified', 'evidence': ['https://example.com/evidence']}]
        result = review.review_outside_boards({'packs': [
            pack('vendor/known', 'boards', definition('a', 'esp32s3', known), listed=True),
            pack('upstream/application', 'boards', definition('custom', 'esp32s3', primary, copy_origin))]}, reviewed_matches=mapping)
        candidate = result['candidates'][0]
        self.assertEqual(candidate['status'], 'possible_known_model')
        self.assertEqual(candidate['known_matches'][0]['reason'], 'Reviewed model lineage')
        self.assertIn(mapping[0]['note'], candidate['note'])
        self.assertEqual(candidate['evidence'], mapping[0]['evidence'])
        self.assertEqual(candidate['origins'], [primary, copy_origin])

    def test_missing_review_target_is_not_counted_as_a_known_match(self):
        mapping = [{'repository': 'user/app', 'path': 'boards/a', 'note': 'Needs manual review',
                    'known_matches': [{'repository': 'missing/repo', 'path': 'boards/a', 'reason': 'Unresolved target'}]}]
        result = review.review_outside_boards({'packs': [
            pack('user/app', 'boards', definition('a', 'esp32s3', origin('user/app', 'boards/a')))]}, reviewed_matches=mapping)
        self.assertEqual(result['no_known_match_count'], 1)
        self.assertEqual(result['candidates'][0]['known_matches'], [])
        self.assertIn('target', result['candidates'][0]['note'].lower())

    def test_legacy_snapshot_without_metadata_or_listed_flags_is_supported(self):
        legacy = pack('user/app', 'boards', definition('custom', 'esp32s3', origin('user/app', 'boards/custom')))
        legacy.pop('listed')
        result = review.review_outside_boards({'packs': [legacy]})
        self.assertEqual(result['candidate_count'], 1)
        self.assertEqual(result['no_known_match_count'], 1)
        self.assertEqual(review.review_outside_boards({'packs': []})['candidate_count'], 0)


if __name__ == '__main__':
    unittest.main()
