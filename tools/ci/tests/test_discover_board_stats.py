# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

import copy
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import discover_board_stats as discovery


class FakeGitHub:
    def __init__(self):
        self.repositories = {}
        self.trees = {}
        self.contents = {}
        self.matches = []
        self.reads = []

    def add_repo(self, repo, **metadata):
        self.repositories[repo] = dict(id=repo, full_name=repo, default_branch='main',
                                       private=False, fork=False, is_template=False, **metadata)
        self.trees[repo] = []

    def add_board(self, repo, path, tree_sha, info='chip: esp32s3\n', missing=(), symlink=None):
        self.trees[repo].append({'path': path, 'type': 'tree', 'mode': '040000', 'sha': tree_sha})
        for filename in ('board_info.yaml', 'board_devices.yaml', 'board_peripherals.yaml'):
            if filename not in missing:
                self.trees[repo].append({'path': path + '/' + filename, 'type': 'blob',
                                         'mode': '120000' if filename == symlink else '100644',
                                         'sha': tree_sha + '-' + filename})
        self.contents[(repo, path + '/board_info.yaml')] = info

    def search_query(self, query):
        self.query = query
        return copy.deepcopy(self.matches)

    def repository(self, repo):
        return self.repositories[repo]

    def default_revision(self, repo, branch):
        return 'commit-' + repo

    def tree(self, repo, revision):
        assert revision == 'commit-' + repo
        return self.trees[repo]

    def content(self, repo, path, revision):
        assert revision == 'commit-' + repo, 'Board reads must be pinned to a commit'
        self.reads.append((repo, path))
        blob_sha = next(entry['sha'] for entry in self.trees[repo] if entry['path'] == path)
        return self.contents.get((repo, path)), blob_sha

    def find(self, repo, path='boards/a/board_info.yaml'):
        self.matches.append({'repository': {'full_name': repo}, 'path': path})


class BoardDiscoveryTests(unittest.TestCase):
    def test_review_metadata_and_yaml_hashes_survive_lineage_consolidation(self):
        client = FakeGitHub()
        client.add_repo('vendor/upstream')
        client.find('vendor/upstream')
        client.add_board('vendor/upstream', 'boards/model/bmgr', 'tree-model',
                         info='board: ESP32_S3_MODEL\nchip: esp32s3\nmanufacturer: Vendor\ndescription: Model board\nversion: 1.0\n')
        boards = discovery.discover_boards(client, [])
        boards = discovery.consolidate_board_sources(boards, [])
        origin = boards['packs'][0]['definitions'][0]['origins'][0]
        self.assertEqual(origin['info']['board'], 'ESP32_S3_MODEL')
        self.assertEqual(origin['info']['manufacturer'], 'Vendor')
        self.assertEqual(origin['info']['description'], 'Model board')
        self.assertEqual(origin['info']['version'], '1.0')
        self.assertEqual(set(origin['files']), set(discovery.REQUIRED_FILES))
        self.assertTrue(all(origin['files'].values()))

    def test_unlisted_pack_and_application_board_do_not_require_a_manifest(self):
        client = FakeGitHub()
        client.add_repo('vendor/unlisted')
        client.add_repo('user/application')
        client.add_board('vendor/unlisted', 'boards/new_board', 'tree-a')
        client.add_board('user/application', 'examples/demo/boards/custom', 'tree-b')
        client.find('vendor/unlisted')
        client.find('user/application')
        result = discovery.discover_boards(client, [])
        self.assertEqual(result['count'], 2)
        self.assertEqual(result['discovered_count'], 2)
        self.assertIn('filename:board_info.yaml', client.query)
        self.assertNotIn('esp_board_manager', client.query)
        self.assertEqual(len(client.reads), 2)

    def test_valid_directory_requires_three_regular_files_and_a_chip(self):
        client = FakeGitHub()
        client.add_repo('user/app')
        client.find('user/app')
        client.add_board('user/app', 'boards/good', 'good', info='chip: esp32\nversion: anything\n')
        client.add_board('user/app', 'boards/missing', 'missing', missing=('board_devices.yaml',))
        client.add_board('user/app', 'boards/symlink', 'link', symlink='board_peripherals.yaml')
        client.add_board('user/app', 'boards/bad_chip', 'badchip', info='board: bad_chip\n')
        client.add_board('user/app', 'boards/not_mapping', 'badinfo', info='- chip\n')
        client.add_board('user/app', 'boards/bad-name', 'badname')
        result = discovery.discover_boards(client, [])
        self.assertEqual(result['count'], 1)
        self.assertEqual(result['packs'][0]['boards'], ['good'])
        self.assertTrue(result['rejected_candidates'])

    def test_caches_fixtures_forks_templates_and_excluded_repositories_are_omitted(self):
        client = FakeGitHub()
        for repo in ('user/app', 'user/fork', 'user/template', 'user/private', 'user/excluded'):
            client.add_repo(repo)
            client.find(repo)
            client.add_board(repo, 'boards/a', 'tree-' + repo)
        client.repositories['user/fork']['fork'] = True
        client.repositories['user/template']['is_template'] = True
        client.repositories['user/private']['private'] = True
        client.add_board('user/app', 'managed_components/vendor/boards/cached', 'cached')
        client.add_board('user/app', 'fixtures/boards/fixture', 'fixture')
        result = discovery.discover_boards(client, [], ['USER/EXCLUDED'],
                                            ['**/managed_components/**', '**/fixtures/**'])
        self.assertEqual(result['count'], 1)
        self.assertEqual(result['packs'][0]['repository'], 'user/app')

    def test_exact_directory_copies_merge_but_same_name_with_different_setup_is_retained(self):
        client = FakeGitHub()
        for repo in ('vendor/listed', 'user/copy', 'user/custom'):
            client.add_repo(repo)
            client.find(repo)
        client.add_board('vendor/listed', 'boards/a', 'same-entire-directory')
        client.add_board('user/copy', 'boards/a', 'same-entire-directory')
        client.add_board('user/custom', 'boards/a', 'different-setup-directory')
        seeds = [{'component': 'vendor/boards', 'repository': 'vendor/listed', 'path': 'boards'}]
        result = discovery.discover_boards(client, seeds)
        self.assertEqual(result['count'], 2)
        self.assertEqual(result['listed_count'], 1)
        self.assertEqual(result['discovered_count'], 1)
        self.assertEqual(result['duplicates_removed'], 1)
        self.assertEqual(sum(pack['count'] for pack in result['packs']), 2)
        canonical = next(pack for pack in result['packs'] if pack['repository'] == 'vendor/listed')
        self.assertEqual(canonical['boards'], ['a'])
        origins = canonical['definitions'][0]['origins']
        self.assertEqual({origin['repository'] for origin in origins}, {'vendor/listed', 'user/copy'})
        self.assertTrue(all('/tree/commit-' in origin['url'] for origin in origins))

    def test_all_boards_in_discovered_repository_are_checked_and_repository_aliases_count_once(self):
        client = FakeGitHub()
        client.add_repo('vendor/current')
        client.repositories['vendor/old'] = client.repositories['vendor/current']
        client.find('vendor/old')
        client.find('vendor/current')
        client.add_board('vendor/current', 'boards/a', 'a')
        client.add_board('vendor/current', 'boards/not_indexed_yet', 'b')
        result = discovery.discover_boards(client, [])
        self.assertEqual(result['count'], 2)
        self.assertEqual(len(client.reads), 2)

    def test_missing_directory_hash_and_disappearing_committed_file_fail_closed(self):
        for missing_hash in (True, False):
            client = FakeGitHub()
            client.add_repo('user/app')
            client.find('user/app')
            client.add_board('user/app', 'boards/a', 'tree-a')
            if missing_hash:
                client.trees['user/app'][0].pop('sha')
            else:
                client.contents.clear()
            with self.assertRaises(discovery.BoardDiscoveryError):
                discovery.discover_boards(client, [])

    def test_content_blob_must_match_the_pinned_repository_tree(self):
        client = FakeGitHub()
        client.add_repo('user/app')
        client.find('user/app')
        client.add_board('user/app', 'boards/a', 'tree-a')
        client.content = lambda repo, path, revision: ('chip: esp32s3\n', 'different-blob')
        with self.assertRaises(discovery.BoardDiscoveryError):
            discovery.discover_boards(client, [])

    def test_single_board_repository_root_uses_the_root_tree_identity(self):
        client = FakeGitHub()
        client.add_repo('vendor/single_board')
        client.find('vendor/single_board', 'board_info.yaml')
        client.trees['vendor/single_board'] = [{'path': '', 'type': 'tree', 'mode': '040000', 'sha': 'root-tree'}]
        for name in ('board_info.yaml', 'board_devices.yaml', 'board_peripherals.yaml'):
            client.trees['vendor/single_board'].append({'path': name, 'type': 'blob', 'mode': '100644', 'sha': name})
        client.contents[('vendor/single_board', 'board_info.yaml')] = 'board: single_board\nchip: esp32\n'
        result = discovery.discover_boards(client, [])
        self.assertEqual(result['count'], 1)
        definition = result['packs'][0]['definitions'][0]
        self.assertEqual(definition['tree_sha'], 'root-tree')
        self.assertEqual(definition['board'], 'single_board')

    def test_configured_source_cannot_silently_disappear_or_become_empty(self):
        seeds = [{'component': 'vendor/boards', 'repository': 'vendor/listed', 'path': 'missing'}]
        client = FakeGitHub()
        client.add_repo('vendor/listed')
        client.add_board('vendor/listed', 'boards/a', 'a')
        with self.assertRaises(discovery.BoardDiscoveryError):
            discovery.discover_boards(client, seeds)
        seeds[0]['path'] = 'boards'
        with self.assertRaises(discovery.BoardDiscoveryError):
            discovery.discover_boards(client, seeds, ['vendor/listed'])

    def test_sanitized_client_errors_keep_their_diagnostic_message(self):
        client = FakeGitHub()
        def fail(query):
            raise discovery.CollectionError('Search exceeds 1,000 results')
        client.search_query = fail
        with self.assertRaisesRegex(discovery.CollectionError, 'exceeds 1,000'):
            discovery.discover_boards(client, [])


class BoardLineageTests(unittest.TestCase):
    def setUp(self):
        self.client = FakeGitHub()
        for repo in ('aaa/copy', 'vendor/upstream'):
            self.client.add_repo(repo)
            self.client.find(repo)
        self.rules = [{'family': 'claw', 'upstream': 'vendor/upstream',
                       'roots': [{'path': 'application/edge_agent/boards'}]}]

    def collect(self):
        boards = discovery.discover_boards(self.client, [])
        return discovery.consolidate_board_sources(boards, self.rules)

    def test_recreated_repository_and_historical_variant_count_once_with_official_source(self):
        self.client.add_board('vendor/upstream', 'application/edge_agent/boards/vendor/a', 'current')
        self.client.add_board('aaa/copy', 'nested/application/edge_agent/boards/vendor/a', 'historical')
        result = self.collect()
        self.assertEqual(result['count'], 1)
        pack = next(pack for pack in result['packs'] if pack['count'])
        self.assertEqual(pack['repository'], 'vendor/upstream')
        self.assertEqual(result['lineage_variants_merged'], 1)
        self.assertEqual(result['exact_duplicates_removed'], 0)
        self.assertEqual({origin['tree_sha'] for origin in pack['definitions'][0]['origins']}, {'current', 'historical'})

    def test_new_models_different_vendor_and_chip_are_retained(self):
        self.client.add_board('vendor/upstream', 'application/edge_agent/boards/vendor/a', 'upstream')
        self.client.add_board('aaa/copy', 'application/edge_agent/boards/vendor/a', 'old')
        self.client.add_board('aaa/copy', 'application/edge_agent/boards/vendor/new_board', 'new')
        self.client.add_board('aaa/copy', 'application/edge_agent/boards/other_vendor/a', 'vendor')
        self.client.add_board('aaa/copy', 'application/edge_agent/boards/vendor/a_c3', 'chip', info='chip: esp32c3\n')
        self.rules[0]['aliases'] = {'vendor/a_c3': 'vendor/a'}
        result = self.collect()
        self.assertEqual(result['count'], 4)

    def test_generic_bmgr_directory_preserves_parent_model_and_repository_scope(self):
        self.rules = [{'family': 'vision', 'upstream': 'vendor/upstream', 'roots': [
            {'repository': repo, 'path': 'boards'} for repo in ('vendor/upstream', 'aaa/copy')]}]
        self.client.add_board('vendor/upstream', 'boards/model_a/bmgr', 'a')
        self.client.add_board('aaa/copy', 'boards/model_a/bmgr', 'old-a')
        self.client.add_board('aaa/copy', 'boards/model_b/bmgr', 'b')
        self.client.add_repo('other/independent')
        self.client.find('other/independent')
        self.client.add_board('other/independent', 'boards/model_a/bmgr', 'independent')
        self.assertEqual(self.collect()['count'], 3)

    def test_explicit_old_layout_alias_does_not_merge_similarly_named_new_board(self):
        self.rules[0]['roots'].append({'path': 'application/basic_demo/boards',
                                      'aliases': {'old_a': 'vendor/a'}})
        self.client.add_board('vendor/upstream', 'application/edge_agent/boards/vendor/a', 'a')
        self.client.add_board('aaa/copy', 'application/basic_demo/boards/old_a', 'old-a')
        self.client.add_board('aaa/copy', 'application/basic_demo/boards/old_a_pro', 'new-a')
        self.assertEqual(self.collect()['count'], 2)

    def test_root_marker_must_match_complete_path_segments(self):
        self.client.add_board('vendor/upstream', 'application/edge_agent/boards/vendor/a', 'a')
        self.client.add_board('aaa/copy', 'myapplication/edge_agent/boards/vendor/a', 'independent')
        self.assertEqual(self.collect()['count'], 2)

    def test_exact_copies_and_variants_have_separate_counts_and_replay_is_idempotent(self):
        self.client.add_board('vendor/upstream', 'application/edge_agent/boards/vendor/a', 'a')
        self.client.add_board('aaa/copy', 'application/edge_agent/boards/vendor/a', 'old-a')
        self.client.add_repo('zzz/exact_copy')
        self.client.find('zzz/exact_copy')
        self.client.add_board('zzz/exact_copy', 'application/edge_agent/boards/vendor/a', 'a')
        result = self.collect()
        self.assertEqual(result['count'], 1)
        self.assertEqual(result['exact_duplicates_removed'], 1)
        self.assertEqual(result['lineage_variants_merged'], 1)
        self.assertEqual(result['raw_count'], 3)
        self.assertEqual(discovery.consolidate_board_sources(result, self.rules), result)

    def test_collector_fetches_official_upstream_even_when_only_derivative_is_indexed(self):
        self.client.matches = [item for item in self.client.matches if item['repository']['full_name'] == 'aaa/copy']
        self.client.add_board('vendor/upstream', 'application/edge_agent/boards/vendor/a', 'a')
        self.client.add_board('aaa/copy', 'application/edge_agent/boards/vendor/a', 'old-a')
        result = discovery.discover_boards(self.client, [], lineages=self.rules)
        self.assertEqual(result['count'], 1)
        self.assertEqual(next(pack for pack in result['packs'] if pack['count'])['repository'], 'vendor/upstream')

    def test_reviewed_source_alias_merges_renamed_existing_board_without_global_name_matching(self):
        self.client.add_board('vendor/upstream', 'board_pack/original_board', 'seed')
        self.client.add_board('aaa/copy', 'application/edge_agent/boards/vendor/old_name', 'old')
        self.rules[0]['aliases'] = {'vendor/old_name': {'repository': 'vendor/upstream', 'path': 'board_pack/original_board'}}
        result = self.collect()
        self.assertEqual(result['count'], 1)

    def test_reviewed_source_alias_requires_a_matching_target_and_chip(self):
        self.client.add_board('vendor/upstream', 'board_pack/original_board', 'seed', info='chip: esp32c3\n')
        self.client.add_board('aaa/copy', 'application/edge_agent/boards/vendor/old_name', 'old')
        self.rules[0]['aliases'] = {'vendor/old_name': {'repository': 'vendor/upstream', 'path': 'board_pack/original_board'}}
        with self.assertRaises(discovery.BoardDiscoveryError):
            self.collect()


if __name__ == '__main__':
    unittest.main()
