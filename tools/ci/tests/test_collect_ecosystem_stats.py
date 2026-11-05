# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

import importlib.util
import io
import json
import re
import sys
import tempfile
import unittest
from http.client import IncompleteRead, RemoteDisconnected
from pathlib import Path
from unittest.mock import patch
from urllib.error import HTTPError, URLError
from urllib.parse import parse_qs, urlparse

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
SPEC = importlib.util.find_spec('collect_ecosystem_stats')
stats = importlib.import_module('collect_ecosystem_stats') if SPEC else None


class RateLimitTests(unittest.TestCase):
    @staticmethod
    def error(status=403, headers=None, message='API rate limit exceeded; private response text'):
        return HTTPError('https://api.github.com/search/code', status, 'private response text',
                         headers or {}, io.BytesIO(json.dumps({'message': message}).encode()))

    def test_primary_limit_waits_for_reset_and_closes_error_before_retry(self):
        error = self.error(headers={'X-RateLimit-Remaining': '0', 'X-RateLimit-Reset': '110',
                                    'X-RateLimit-Resource': 'code_search', 'X-GitHub-Request-Id': 'ABCD:1234'})

        def send(*args, **kwargs):
            if not error.closed:
                raise error
            return io.StringIO('{}')

        with patch.object(stats, 'urlopen', side_effect=send) as request, \
                patch.object(stats.time, 'time', return_value=100), patch.object(stats.time, 'sleep') as sleep, \
                patch('sys.stderr', new_callable=io.StringIO) as log:
            self.assertEqual(stats.GitHubClient('private-token').get('/search/code'), {})
        self.assertEqual(request.call_count, 2)
        sleep.assert_called_once_with(11)
        self.assertIn('primary_rate_limit', log.getvalue())
        self.assertIn('ABCD:1234', log.getvalue())
        self.assertIn('code_search', log.getvalue())
        self.assertNotIn('private', log.getvalue())

    def test_retry_after_and_reset_both_constrain_retry_time(self):
        error = self.error(429, {'Retry-After': '3', 'X-RateLimit-Remaining': '0', 'X-RateLimit-Reset': '110'})
        with patch.object(stats, 'urlopen', side_effect=[error, io.StringIO('{}')]), \
                patch.object(stats.time, 'time', return_value=100), patch.object(stats.time, 'sleep') as sleep, \
                patch('sys.stderr', new_callable=io.StringIO):
            self.assertEqual(stats.GitHubClient().get('/search/code'), {})
        sleep.assert_called_once_with(11)

    def test_secondary_limit_with_remaining_quota_uses_exponential_backoff(self):
        errors = [self.error(headers={'X-RateLimit-Remaining': '8'},
                             message='You have exceeded a secondary rate limit. private response text') for _ in range(2)]
        with patch.object(stats, 'urlopen', side_effect=errors + [io.StringIO('{}')]) as request, \
                patch.object(stats.time, 'sleep') as sleep, patch('sys.stderr', new_callable=io.StringIO) as log:
            self.assertEqual(stats.GitHubClient().get('/search/code'), {})
        self.assertEqual(request.call_count, 3)
        self.assertEqual([call.args[0] for call in sleep.call_args_list], [60, 120])
        self.assertIn('secondary_rate_limit', log.getvalue())
        self.assertNotIn('private', log.getvalue())

    def test_permission_error_has_safe_diagnostics_without_retry(self):
        for hints in ({}, {'X-RateLimit-Remaining': '0'}, {'Retry-After': '1'}):
            headers = dict(hints, **{'X-GitHub-Request-Id': 'ABCD:5678'})
            error = self.error(headers=headers,
                               message='Resource not accessible by personal access token. private response text')
            with self.subTest(headers=hints), \
                    patch.object(stats, 'urlopen', side_effect=[error, io.StringIO('{}')]) as request, \
                    patch.object(stats.time, 'sleep') as sleep:
                with self.assertRaises(stats.CollectionError) as caught:
                    stats.GitHubClient('private-token').get('/search/code')
                self.assertEqual(request.call_count, 1)
                sleep.assert_not_called()
                self.assertTrue(error.closed)
                self.assertIn('permission_denied', str(caught.exception))
                self.assertIn('ABCD:5678', str(caught.exception))
                self.assertNotIn('private', str(caught.exception))

    def test_deeply_nested_error_body_preserves_http_diagnostics(self):
        error = HTTPError('', 403, '', {'X-GitHub-Request-Id': 'ABCD:9999'},
                          io.BytesIO(b'[' * 1100 + b']' * 1100))
        with patch.object(stats, 'urlopen', side_effect=error) as request, patch.object(stats.time, 'sleep') as sleep:
            with self.assertRaises(stats.CollectionError) as caught:
                stats.GitHubClient().get('/search/code')
        self.assertEqual(request.call_count, 1)
        sleep.assert_not_called()
        self.assertTrue(error.closed)
        self.assertIn('HTTP 403', str(caught.exception))
        self.assertIn('ABCD:9999', str(caught.exception))

    def test_malformed_error_body_keeps_http_status_and_does_not_retry(self):
        class BrokenBody(io.BytesIO):
            def read(self, *args):
                raise IncompleteRead(b'private response', 20)

        error = HTTPError('', 403, '', {'X-GitHub-Request-Id': 'ABCD:9999'}, BrokenBody())
        with patch.object(stats, 'urlopen', side_effect=error) as request, patch.object(stats.time, 'sleep') as sleep:
            with self.assertRaises(stats.CollectionError) as caught:
                stats.GitHubClient().get('/search/code')
        self.assertEqual(request.call_count, 1)
        sleep.assert_not_called()
        self.assertTrue(error.closed)
        self.assertIn('HTTP 403', str(caught.exception))
        self.assertIn('ABCD:9999', str(caught.exception))

    def test_invalid_timing_headers_with_confirmed_limit_use_safe_fallback(self):
        for headers in ({'Retry-After': '-1', 'X-RateLimit-Reset': 'NaN'},
                        {'X-RateLimit-Remaining': '0', 'X-RateLimit-Reset': '90'}):
            with self.subTest(headers=headers), \
                    patch.object(stats, 'urlopen', side_effect=[self.error(headers=headers), io.StringIO('{}')]), \
                    patch.object(stats.time, 'time', return_value=100), patch.object(stats.time, 'sleep') as sleep, \
                    patch('sys.stderr', new_callable=io.StringIO):
                self.assertEqual(stats.GitHubClient().get('/search/code'), {})
                sleep.assert_called_once_with(60)

    def test_rate_limit_wait_budget_survives_queries_and_rejects_excessive_delays(self):
        client = stats.GitHubClient()
        responses = [self.error(headers={'Retry-After': '300'}), io.StringIO('{}'),
                     self.error(headers={'Retry-After': '1'})]
        with patch.object(stats, 'urlopen', side_effect=responses) as request, \
                patch.object(stats.time, 'sleep') as sleep, patch('sys.stderr', new_callable=io.StringIO):
            self.assertEqual(client.get('/first'), {})
            with self.assertRaisesRegex(stats.CollectionError, 'rate-limit wait budget'):
                client.get('/second')
        self.assertEqual(request.call_count, 3)
        sleep.assert_called_once_with(300)
        with patch.object(stats, 'urlopen', side_effect=self.error(headers={'Retry-After': '864000'})) as request, \
                patch.object(stats.time, 'sleep') as sleep:
            with self.assertRaisesRegex(stats.CollectionError, 'rate-limit wait budget'):
                stats.GitHubClient().get('/search/code')
        self.assertEqual(request.call_count, 1)
        sleep.assert_not_called()

    def test_transport_and_http_failures_share_three_attempts(self):
        failures = [RemoteDisconnected('private text'), self.error(), self.error()]
        with patch.object(stats, 'urlopen', side_effect=failures) as request, \
                patch.object(stats.time, 'sleep') as sleep, patch('sys.stderr', new_callable=io.StringIO) as log:
            with self.assertRaises(stats.CollectionError) as caught:
                stats.GitHubClient().get('/search/code')
        self.assertEqual(request.call_count, 3)
        self.assertEqual([call.args[0] for call in sleep.call_args_list], [2, 120])
        self.assertIn('3/3', str(caught.exception))
        self.assertNotIn('private', log.getvalue() + str(caught.exception))

    def test_search_budget_includes_http_rate_limit_retries(self):
        client = stats.PartitionedGitHubClient()
        client.SEARCH_REQUEST_LIMIT = 1
        with patch.object(stats, 'urlopen', side_effect=self.error(headers={'Retry-After': '1'})) as request, \
                patch.object(stats.time, 'sleep'), patch('sys.stderr', new_callable=io.StringIO):
            with self.assertRaisesRegex(stats.CollectionError, 'search request budget'):
                client.get('/search/code', search=True)
        self.assertEqual(request.call_count, 1)
        self.assertEqual(client.search_requests, 1)


class PartitionedSearchTests(unittest.TestCase):
    query = 'chip filename:board_info.yaml in:file'

    def setUp(self):
        self.assertTrue(hasattr(stats, 'PartitionedGitHubClient'), 'Partitioned search must exist')

    @staticmethod
    def item(index, size=100):
        return {'repository': {'full_name': 'user/app'}, 'path': 'board_{}/board_info.yaml'.format(index),
                'sha': '{:040x}'.format(index), 'size': size}

    @staticmethod
    def response(items, total=None):
        return {'total_count': len(items) if total is None else total,
                'incomplete_results': False, 'items': items}

    def test_small_query_is_verified_without_changing_its_scope(self):
        items = [self.item(1)]
        client = stats.PartitionedGitHubClient()
        with patch.object(client, 'get', return_value=self.response(items)) as request:
            self.assertEqual(client.search_query(self.query), items)
        self.assertEqual(request.call_count, 2)
        self.assertTrue(all(parse_qs(urlparse(call.args[0]).query)['q'] == [self.query]
                            for call in request.call_args_list))

    def test_large_parent_is_discarded_and_unbounded_tail_is_included(self):
        client = stats.PartitionedGitHubClient()
        left, right = [self.item(1)], [self.item(2, 5000000)]
        responses = [self.response([self.item(99)], 1001), self.response(left), self.response(right),
                     self.response(left), self.response(right)]
        with patch.object(client, 'get', side_effect=responses) as request, \
                patch('sys.stderr', new_callable=io.StringIO):
            self.assertEqual(client.search_query(self.query), left + right)
        queries = [parse_qs(urlparse(call.args[0]).query)['q'][0] for call in request.call_args_list]
        self.assertEqual(queries, [self.query, self.query + ' size:0..1023', self.query + ' size:>=1024',
                                   self.query + ' size:0..1023', self.query + ' size:>=1024'])

    def _size_search(self, items, requests):
        def read(path, search=False):
            params = parse_qs(urlparse(path).query)
            query, page = params['q'][0], int(params['page'][0])
            requests.append((query, page))
            selected = items
            match = re.search(r' size:(\d+)\.\.(\d+)$', query)
            tail = re.search(r' size:>=(\d+)$', query)
            if match:
                low, high = map(int, match.groups())
                selected = [item for item in items if low <= item['size'] <= high]
            elif tail:
                selected = [item for item in items if item['size'] >= int(tail.group(1))]
            return self.response(selected[(page - 1) * 100:page * 100], len(selected))
        return read

    def test_exact_size_uses_strict_pagination_and_preserves_interval_boundaries(self):
        items = [self.item(index, 511) for index in range(101)] + [self.item(999, 512)]
        client, requests = stats.PartitionedGitHubClient(), []
        with patch.object(client, 'get', side_effect=self._size_search(items, requests)), \
                patch('sys.stderr', new_callable=io.StringIO):
            self.assertEqual(client.search_query(self.query), items)
        atomic = self.query + ' size:511..511'
        self.assertEqual([page for query, page in requests if query == atomic], [1, 2, 1, 2])
        self.assertIn((self.query + ' size:512..1023', 1), requests)

    def test_exact_size_over_api_cap_stops_collection(self):
        items = [self.item(index) for index in range(1001)]
        client, requests = stats.PartitionedGitHubClient(), []
        with patch.object(client, 'get', side_effect=self._size_search(items, requests)), \
                patch('sys.stderr', new_callable=io.StringIO):
            with self.assertRaisesRegex(stats.CollectionError, '1,000'):
                client.search_query(self.query)

    def test_cross_partition_duplicate_stops_after_three_complete_attempts(self):
        client = stats.PartitionedGitHubClient()
        item = self.item(1)
        responses = [self.response([self.item(99)], 101), self.response([item]), self.response([item])] * 3
        with patch.object(client, 'get', side_effect=responses) as request, \
                patch('sys.stderr', new_callable=io.StringIO):
            with self.assertRaisesRegex(stats.CollectionError, 'duplicate'):
                client.search_query(self.query)
        self.assertEqual(request.call_count, 9)

    def test_new_item_in_previously_empty_leaf_restarts_the_whole_query(self):
        client = stats.PartitionedGitHubClient()
        left, right = self.item(1), self.item(2, 2048)
        responses = [self.response([self.item(99)], 101), self.response([left]), self.response([]),
                     self.response([left]), self.response([right]),
                     self.response([left, right]), self.response([left, right])]
        with patch.object(client, 'get', side_effect=responses) as request, \
                patch('sys.stderr', new_callable=io.StringIO):
            self.assertEqual(client.search_query(self.query), [left, right])
        self.assertEqual([parse_qs(urlparse(call.args[0]).query)['q'][0]
                          for call in request.call_args_list].count(self.query), 3)

    def test_changed_blob_is_not_combined_with_previous_observation(self):
        client = stats.PartitionedGitHubClient()
        old, new = self.item(1), dict(self.item(1), sha='f' * 40)
        responses = [self.response([old]), self.response([new]), self.response([new]), self.response([new])]
        with patch.object(client, 'get', side_effect=responses) as request, \
                patch('sys.stderr', new_callable=io.StringIO):
            self.assertEqual(client.search_query(self.query), [new])
        self.assertEqual(request.call_count, 4)

    def test_missing_blob_identity_is_rejected(self):
        item = self.item(1)
        del item['sha']
        client = stats.PartitionedGitHubClient()
        with patch.object(client, 'get', return_value=self.response([item])) as request:
            with self.assertRaisesRegex(stats.CollectionError, 'SHA'):
                client.search_query(self.query)
        self.assertEqual(request.call_count, 1)

    def test_request_budget_includes_transport_retries_and_survives_query_boundaries(self):
        client = stats.PartitionedGitHubClient()
        with patch.object(client, 'SEARCH_REQUEST_LIMIT', 3), \
                patch.object(stats, 'urlopen', side_effect=[RemoteDisconnected('closed'), io.StringIO('{}'),
                                                          io.StringIO('{}')]) as request, \
                patch.object(stats.time, 'sleep'), patch('sys.stderr', new_callable=io.StringIO):
            self.assertEqual(client.get('/search/code?q=first', search=True), {})
            self.assertEqual(client.get('/search/code?q=second', search=True), {})
            with self.assertRaisesRegex(stats.CollectionError, 'budget'):
                client.search_query(self.query)
        self.assertEqual(request.call_count, 3)


class CollectorTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(stats, 'The ecosystem collector must exist')

    def test_board_count_excludes_nested_fixtures_and_other_packs(self):
        entries = [{'path': p, 'type': 'blob'} for p in (
            'boards/a/board_info.yaml', 'boards/b/board_info.yaml',
            'boards/tests/a/board_info.yaml', 'other/c/board_info.yaml')]
        entries.append({'path': 'boards/symlink/board_info.yaml', 'type': 'blob', 'mode': '120000'})
        self.assertEqual(stats.board_paths(entries, 'boards'), ['a', 'b'])

    def test_board_packs_share_one_immutable_repository_snapshot(self):
        packs = [{'component': name, 'repository': 'vendor/boards', 'path': name} for name in ('first', 'second')]
        entries = [{'path': name + '/board/board_info.yaml', 'type': 'blob'} for name in ('first', 'second')]
        with patch.object(stats, 'repository_tree', create=True, return_value=('commit-sha', entries)) as read:
            result = stats.collect_boards(packs)
        self.assertEqual(result['count'], 2)
        self.assertEqual(read.call_count, 1)
        self.assertEqual([pack['revision'] for pack in result['packs']], ['commit-sha', 'commit-sha'])

    def test_manifest_parses_dependencies_instead_of_mentions(self):
        self.assertEqual(stats.dependency_kind('dependencies:\n  espressif/esp_board_manager: "^0.7"', 'idf_component.yml'), 'direct')
        self.assertEqual(stats.dependency_kind('dependencies:\n  esp_board_manager:\n    version: "*"', 'idf_component.yml'), 'direct')
        self.assertIsNone(stats.dependency_kind('# espressif/esp_board_manager\ndescription: esp_board_manager', 'idf_component.yml'))
        self.assertIsNone(stats.dependency_kind('dependencies:\n  vendor/esp_board_manager: "*"', 'idf_component.yml'))

    def test_lock_file_marks_resolved_dependency_separately(self):
        self.assertEqual(stats.dependency_kind('dependencies:\n  espressif/esp_board_manager:\n    version: 0.7.4', 'dependencies.lock'), 'resolved')

    def test_projects_deduplicate_repositories_and_filter_non_projects(self):
        class Client:
            def search(self, filename):
                return [{'repository': {'full_name': repo}, 'path': 'main/' + filename}
                        for repo in ('user/app', 'user/fork', 'user/template', 'user/private', 'user/excluded')]

            def repository(self, repo):
                return {'fork': repo.endswith('fork'), 'is_template': repo.endswith('template'),
                        'private': repo.endswith('private'), 'default_branch': 'main', 'id': repo, 'full_name': repo}

            def default_revision(self, repo, branch):
                return 'commit-sha'

            def content(self, repo, path, ref):
                assert ref == 'commit-sha', 'Dependency reads must use an immutable commit'
                return 'dependencies:\n  espressif/esp_board_manager: "*"', 'blob-sha'

        result = stats.collect_projects(Client(), ['USER/EXCLUDED'])
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['repository'], 'user/app')
        self.assertEqual(len(result[0]['evidence']), 2)
        self.assertIn('/blob/commit-sha/', result[0]['evidence'][0]['url'])

    def test_dependency_caches_and_embedded_component_examples_are_not_project_evidence(self):
        patterns = ['**/managed_components/**', '**/vendor/**', '**/third_party/**', '**/third-party/**',
                    '**/components/espressif__*/**', '**/components/esp-adf-libs/**',
                    '**/components/*esp_board_manager/examples/**',
                    '**/components/*esp_board_manager/test_apps/**', '**/esp-gmf/components/**']
        excluded = [
            'managed_components/espressif__esp_capture/examples/audio/main/idf_component.yml',
            'app/managed_components/espressif__esp_muxer/test_apps/main/idf_component.yml',
            'src/vendor/esp_audio_codec/test_apps/main/idf_component.yml',
            'third_party/library/examples/main/idf_component.yml',
            'app/third-party/library/idf_component.yml',
            'components/esp_board_manager/examples/display/main/idf_component.yml',
            'components/esp-adf-libs/esp_audio_effects/example/demo/main/idf_component.yml',
            'components/esp-adf-libs/esp_muxer/test_apps/main/idf_component.yml',
            'components/espressif__esp_audio_codec/test_apps/main/idf_component.yml',
            'audio/esp-gmf/components/esp_muxer/examples/demo/main/idf_component.yml',
        ]
        for path in excluded:
            self.assertFalse(stats.is_project_evidence(path, patterns), path)
        for path in ('main/idf_component.yml', 'dependencies.lock', 'examples/demo/main/idf_component.yml',
                     'test_apps/main/idf_component.yml', 'components/ui/idf_component.yml',
                     'components/esp_apa_doa/examples/live_example/main/idf_component.yml',
                     'components/board/guition_boards/idf_component.yml'):
            self.assertTrue(stats.is_project_evidence(path, patterns), path)

        class Client:
            def search(self, filename):
                return [{'repository': {'full_name': 'user/cache-only'}, 'path': path} for path in excluded]

            def repository(self, repo):
                raise AssertionError('Excluded paths must be ignored before fetching repository data')

        self.assertEqual(stats.collect_projects(Client(), [], patterns), [])

    def test_renamed_repository_counts_once_and_respects_canonical_exclusions(self):
        class Client:
            def search(self, filename):
                return [{'repository': {'full_name': name}, 'path': filename} for name in ('user/old', 'user/current')]

            def repository(self, repo):
                return {'id': 123, 'full_name': 'user/current', 'default_branch': 'main'}

            def default_revision(self, repo, branch):
                return 'commit-sha'

            def content(self, repo, path, ref):
                return 'dependencies:\n  espressif/esp_board_manager: "*"', 'blob-sha'

        result = stats.collect_projects(Client(), [])
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['repository'], 'user/current')
        self.assertEqual(len(result[0]['evidence']), 2)
        self.assertEqual(stats.collect_projects(Client(), ['user/current']), [])

    @staticmethod
    def _search_items(count, prefix='item'):
        return [{'repository': {'full_name': 'user/app'}, 'path': '{}_{}'.format(prefix, index)}
                for index in range(count)]

    @staticmethod
    def _search_response(total, items, incomplete=False):
        return {'total_count': total, 'incomplete_results': incomplete, 'items': items}

    @staticmethod
    def _requested_pages(request):
        return [int(parse_qs(urlparse(call.args[0]).query)['page'][0]) for call in request.call_args_list]

    def test_request_recovers_from_transport_failures_without_logging_secrets(self):
        secret = 'secret-token-not-for-logs'
        failures = (RemoteDisconnected(secret), ConnectionResetError(secret), TimeoutError(secret),
                    URLError(TimeoutError(secret)), URLError(secret), IncompleteRead(secret.encode(), 10))
        path = '/repos/user/app/git/trees/commit-sha?recursive=1'
        for failure in failures:
            with self.subTest(error=type(failure).__name__):
                response = io.StringIO('{"sha": "commit-sha"}')
                with patch.object(stats, 'urlopen', side_effect=[failure, response]) as request, \
                        patch.object(stats.time, 'sleep') as sleep, \
                        patch('sys.stderr', new_callable=io.StringIO) as diagnostics:
                    try:
                        result = stats.GitHubClient(secret).get(path)
                    except Exception as error:
                        self.fail('Transport failure was not retried: {}'.format(type(error).__name__))
                self.assertEqual(result, {'sha': 'commit-sha'})
                self.assertEqual(request.call_count, 2)
                self.assertTrue(all(call.args[0].full_url == 'https://api.github.com' + path
                                    for call in request.call_args_list))
                sleep.assert_called_once()
                self.assertGreater(sleep.call_args.args[0], 0)
                log = diagnostics.getvalue()
                for expected in (path, '1/3', type(failure).__name__):
                    self.assertIn(expected, log)
                self.assertNotIn(secret, log)
                self.assertTrue(response.closed)

    def test_request_stops_after_three_transport_failures_with_safe_diagnostics(self):
        client = stats.GitHubClient('secret-token-not-for-logs')
        path = '/repos/user/app'
        with patch.object(stats, 'urlopen', side_effect=RemoteDisconnected('private response text')) as request, \
                patch.object(stats.time, 'sleep') as sleep, \
                patch('sys.stderr', new_callable=io.StringIO) as diagnostics:
            with self.assertRaises(stats.CollectionError) as error:
                client.get(path)
        self.assertEqual(request.call_count, 3)
        self.assertEqual(sleep.call_count, 2)
        delays = [call.args[0] for call in sleep.call_args_list]
        self.assertTrue(0 < delays[0] < delays[1])
        log = diagnostics.getvalue() + str(error.exception)
        for expected in (path, '1/3', '2/3', '3/3', 'RemoteDisconnected'):
            self.assertIn(expected, log)
        for secret in ('secret-token-not-for-logs', 'private response text'):
            self.assertNotIn(secret, log)

    def test_request_closes_interrupted_response_before_retrying(self):
        class InterruptedResponse(io.StringIO):
            def read(self, *args):
                raise IncompleteRead(b'private response body', 10)

        interrupted = InterruptedResponse()
        complete = io.StringIO('{"ok": true}')

        def send(*args, **kwargs):
            if not interrupted.closed:
                return interrupted
            return complete

        with patch.object(stats, 'urlopen', side_effect=send) as request, \
                patch.object(stats.time, 'sleep'), patch('sys.stderr', new_callable=io.StringIO):
            self.assertEqual(stats.GitHubClient().get('/repos/user/app'), {'ok': True})
        self.assertEqual(request.call_count, 2)
        self.assertTrue(interrupted.closed)
        self.assertTrue(complete.closed)

    def test_request_does_not_retry_invalid_json_or_http_errors(self):
        failures = [HTTPError('https://api.github.com/repos/user/app', status, 'private text', {}, None)
                    for status in (401, 403, 422, 429, 500)]
        failures.append(io.StringIO('private invalid JSON'))
        for failure in failures:
            with self.subTest(error=getattr(failure, 'code', 'invalid JSON')):
                options = {'side_effect': failure} if isinstance(failure, HTTPError) else {'return_value': failure}
                with patch.object(stats, 'urlopen', **options) as request, \
                        patch.object(stats.time, 'sleep') as sleep:
                    with self.assertRaises(stats.CollectionError) as error:
                        stats.GitHubClient().get('/repos/user/app')
                self.assertEqual(request.call_count, 1)
                sleep.assert_not_called()
                self.assertNotIn('private', str(error.exception))
        with patch.object(stats, 'urlopen', side_effect=HTTPError('', 404, '', {}, None)) as request:
            self.assertIsNone(stats.GitHubClient().get('/repos/user/missing'))
            self.assertEqual(request.call_count, 1)

    def test_request_throttles_each_search_transport_attempt(self):
        clock = [100.0]
        starts = []

        def sleep(seconds):
            clock[0] += seconds

        def send(*args, **kwargs):
            starts.append(clock[0])
            if len(starts) == 1:
                raise RemoteDisconnected('connection closed')
            return io.StringIO('{}')

        with patch.object(stats, 'urlopen', side_effect=send), \
                patch.object(stats.time, 'monotonic', side_effect=lambda: clock[0]), \
                patch.object(stats.time, 'sleep', side_effect=sleep), \
                patch('sys.stderr', new_callable=io.StringIO):
            self.assertEqual(stats.GitHubClient().get('/search/code?q=chip', search=True), {})
        self.assertEqual(len(starts), 2)
        self.assertGreaterEqual(starts[1] - starts[0], 6.1 - 1e-9)

    def test_search_transport_retries_current_page_without_duplicating_results(self):
        items = self._search_items(101)
        responses = [io.StringIO(json.dumps(self._search_response(101, items[:100]))),
                     RemoteDisconnected('connection closed'),
                     io.StringIO(json.dumps(self._search_response(101, items[100:])))]
        with patch.object(stats, 'urlopen', side_effect=responses) as request, \
                patch.object(stats.time, 'sleep'), patch('sys.stderr', new_callable=io.StringIO):
            self.assertEqual(stats.GitHubClient().search('idf_component.yml'), items)
        self.assertEqual([int(parse_qs(urlparse(call.args[0].full_url).query)['page'][0])
                          for call in request.call_args_list], [1, 2, 2])

    def test_search_revalidates_totals_after_transport_recovery(self):
        stale = self._search_items(102, 'stale')
        fresh = self._search_items(101, 'fresh')
        responses = [io.StringIO(json.dumps(self._search_response(101, stale[:100]))),
                     RemoteDisconnected('connection closed'),
                     io.StringIO(json.dumps(self._search_response(102, stale[100:]))),
                     io.StringIO(json.dumps(self._search_response(101, fresh[:100]))),
                     io.StringIO(json.dumps(self._search_response(101, fresh[100:])))]
        with patch.object(stats, 'urlopen', side_effect=responses) as request, \
                patch.object(stats.time, 'sleep'), patch('sys.stderr', new_callable=io.StringIO):
            self.assertEqual(stats.GitHubClient().search('idf_component.yml'), fresh)
        self.assertEqual([int(parse_qs(urlparse(call.args[0].full_url).query)['page'][0])
                          for call in request.call_args_list], [1, 2, 2, 1, 2])

    def test_search_transport_exhaustion_does_not_restart_entire_query(self):
        with patch.object(stats, 'urlopen', side_effect=RemoteDisconnected('connection closed')) as request, \
                patch.object(stats.time, 'sleep'), patch('sys.stderr', new_callable=io.StringIO):
            with self.assertRaises(stats.CollectionError):
                stats.GitHubClient().search('idf_component.yml')
        self.assertEqual(request.call_count, 3)

    def test_search_paginates_and_refuses_capped_results(self):
        client = stats.GitHubClient('unused')
        items = self._search_items(101)
        with patch.object(client, 'get', side_effect=[
            {'total_count': 101, 'incomplete_results': False, 'items': items[:100]},
            {'total_count': 101, 'incomplete_results': False, 'items': items[100:]},
        ]):
            self.assertEqual(len(client.search('idf_component.yml')), 101)
        with patch.object(client, 'get', return_value=self._search_response(1001, [])) as request:
            with self.assertRaisesRegex(stats.CollectionError, '1,000'):
                client.search('idf_component.yml')
            self.assertEqual(request.call_count, 1)

    def test_search_refuses_duplicate_files_after_three_attempts(self):
        item = {'repository': {'full_name': 'user/app'}, 'path': 'main/idf_component.yml'}
        with patch.object(stats.GitHubClient, 'get', return_value={
            'total_count': 2, 'incomplete_results': False, 'items': [item, item],
        }) as request, patch('sys.stderr', new_callable=io.StringIO):
            with self.assertRaises(stats.CollectionError):
                stats.GitHubClient('unused').search('idf_component.yml')
            self.assertEqual(self._requested_pages(request), [1, 1, 1])

    def test_search_refuses_lost_pages_after_three_attempts(self):
        client = stats.GitHubClient('unused')
        with patch.object(client, 'get', return_value=self._search_response(2, [])) as request, \
                patch('sys.stderr', new_callable=io.StringIO):
            with self.assertRaises(stats.CollectionError):
                client.search('idf_component.yml')
            self.assertEqual(self._requested_pages(request), [1, 1, 1])

    def test_generic_search_preserves_board_query(self):
        client = stats.GitHubClient('unused')
        query = 'chip filename:board_info.yaml in:file'
        with patch.object(client, 'get', return_value={
                'total_count': 0, 'incomplete_results': False, 'items': []}) as request:
            self.assertEqual(client.search_query(query), [])
        self.assertIn('chip+filename%3Aboard_info.yaml+in%3Afile', request.call_args.args[0])

    def test_search_restarts_drifted_query_and_discards_all_previous_results(self):
        client = stats.GitHubClient('secret-token-not-for-logs')
        query = 'chip filename:board_info.yaml in:file'
        stale = self._search_items(100, 'stale')
        # Reusing one prior path also proves the failed attempt's seen set is reset.
        fresh = stale[:1] + self._search_items(100, 'fresh')
        responses = [self._search_response(201, stale), self._search_response(202, self._search_items(100, 'tail')),
                     self._search_response(101, fresh[:100]), self._search_response(101, fresh[100:])]
        with patch.object(client, 'get', side_effect=responses) as request, \
                patch('sys.stderr', new_callable=io.StringIO) as diagnostics:
            self.assertEqual(client.search_query(query), fresh)
        self.assertEqual(self._requested_pages(request), [1, 2, 1, 2])
        log = diagnostics.getvalue()
        for value in (query, '1/3', 'page', '201', '202'):
            self.assertIn(value, log)
        self.assertNotIn('secret-token-not-for-logs', log)
        self.assertNotIn('stale_1', log)

    def test_search_repeated_drift_stops_after_three_attempts_with_diagnostics(self):
        client = stats.GitHubClient('secret-token-not-for-logs')
        query = 'esp_board_manager filename:idf_component.yml in:file'
        responses = [self._search_response(101, self._search_items(100)),
                     self._search_response(102, self._search_items(2, 'tail'))] * 3
        with patch.object(client, 'get', side_effect=responses) as request, \
                patch('sys.stderr', new_callable=io.StringIO) as diagnostics:
            with self.assertRaises(stats.CollectionError) as error:
                client.search_query(query)
        self.assertEqual(self._requested_pages(request), [1, 2, 1, 2, 1, 2])
        log = diagnostics.getvalue() + str(error.exception)
        for value in (query, '1/3', '2/3', '3/3', 'page', '101', '102'):
            self.assertIn(value, log)
        self.assertNotIn('secret-token-not-for-logs', log)
        self.assertNotIn('tail_0', log)

    def test_search_duplicate_short_and_incomplete_attempts_can_recover(self):
        old = self._search_items(100, 'old')
        fresh = self._search_items(2, 'fresh')
        cases = (
            ('duplicate', [self._search_response(101, old), self._search_response(101, old[:1])], [1, 2, 1]),
            ('short', [self._search_response(2, old[:1])], [1, 1]),
            ('incomplete', [self._search_response(2, [], incomplete=True)], [1, 1]),
        )
        for name, failed, pages in cases:
            with self.subTest(name=name):
                client = stats.GitHubClient('unused')
                with patch.object(client, 'get', side_effect=failed + [self._search_response(2, fresh)]) as request, \
                        patch('sys.stderr', new_callable=io.StringIO):
                    self.assertEqual(client.search('idf_component.yml'), fresh)
                self.assertEqual(self._requested_pages(request), pages)

    def test_search_http_errors_are_not_retried(self):
        for status in (401, 403, 500):
            with self.subTest(status=status):
                client = stats.GitHubClient('secret-token-not-for-logs')
                error = HTTPError('https://api.github.com/search/code', status, 'failed', {}, None)
                with patch.object(stats, 'urlopen', side_effect=error) as request, \
                        patch.object(stats.time, 'sleep'), patch('sys.stderr', new_callable=io.StringIO) as diagnostics:
                    with self.assertRaisesRegex(stats.CollectionError, 'HTTP {}'.format(status)):
                        client.search('idf_component.yml')
                self.assertEqual(request.call_count, 1)
                self.assertNotIn('secret-token-not-for-logs', diagnostics.getvalue())

    def test_search_invalid_responses_are_not_retried(self):
        item = self._search_items(1)
        invalid = (
            None, [], {},
            {'incomplete_results': False, 'items': []},
            {'total_count': 1, 'items': item},
            {'total_count': 1, 'incomplete_results': False},
            self._search_response(-1, []), self._search_response(True, item),
            self._search_response('1', item), self._search_response(1, item, incomplete='false'),
            self._search_response(1, {}), self._search_response(1, [{'path': 'file'}]),
            self._search_response(1, [{'repository': {'full_name': 'user/app'}, 'path': False}]),
            self._search_response(1, [{'path': False}], incomplete=True),
            self._search_response(0, item),
        )
        for response in invalid:
            with self.subTest(response=response):
                client = stats.GitHubClient('unused')
                # A valid follow-up response must never be consumed for a schema error.
                with patch.object(client, 'get', side_effect=[response, self._search_response(0, [])]) as request, \
                        patch('sys.stderr', new_callable=io.StringIO):
                    with self.assertRaises(stats.CollectionError):
                        client.search('idf_component.yml')
                self.assertEqual(request.call_count, 1)

    def test_search_excess_results_across_pages_are_not_retried(self):
        client = stats.GitHubClient('unused')
        responses = [self._search_response(101, self._search_items(100)),
                     self._search_response(101, self._search_items(2, 'extra')),
                     self._search_response(0, [])]
        with patch.object(client, 'get', side_effect=responses) as request, \
                patch('sys.stderr', new_callable=io.StringIO):
            with self.assertRaises(stats.CollectionError):
                client.search('idf_component.yml')
        self.assertEqual(self._requested_pages(request), [1, 2])

    def test_repository_tree_rejects_truncated_and_missing_responses(self):
        client = stats.GitHubClient('unused')
        for response in (None, {'truncated': True, 'tree': []}, {'tree': []}):
            with patch.object(client, 'get', return_value=response):
                with self.assertRaises(stats.CollectionError):
                    client.tree('user/app', 'commit-sha')
        with patch.object(client, 'get', return_value={'truncated': False, 'tree': []}):
            self.assertEqual(client.tree('user/app', 'commit-sha'), [])

    def test_failed_collection_preserves_previous_snapshot(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'public'
            output.mkdir()
            previous = {name: ('previous ' + name).encode('utf-8') for name in
                        ('stats.json', 'supported-boards.svg', 'dependent-projects.svg', 'README.md')}
            for name, content in previous.items():
                (output / name).write_bytes(content)
            sources = Path(directory) / 'sources.json'
            sources.write_text(json.dumps({'board_packs': [], 'excluded_repositories': []}))
            with patch.dict(stats.os.environ, {'ECOSYSTEM_GITHUB_TOKEN': 'unused'}), \
                    patch.object(stats.GitHubClient, 'get', return_value=self._search_response(1, [], incomplete=True)) as request, \
                    patch.object(stats, 'write_report') as write_report, patch('sys.stderr', new_callable=io.StringIO):
                self.assertEqual(stats.main(['--sources', str(sources), '--boards-only', '--output', str(output)]), 1)
                self.assertEqual(self._requested_pages(request), [1, 1, 1])
                write_report.assert_not_called()
            for name, content in previous.items():
                self.assertEqual((output / name).read_bytes(), content)

    def test_default_configuration_is_loaded_from_the_private_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            script = root / 'tools' / 'ci' / 'collect_ecosystem_stats.py'
            script.parent.mkdir(parents=True)
            script.with_name('ecosystem_sources.json').write_text('obsolete public configuration')
            private = root / '.ecosystem-private' / 'config'
            private.mkdir(parents=True)
            (private / 'ecosystem_sources.json').write_text(json.dumps({'board_packs': [], 'excluded_repositories': []}))
            with patch.object(stats, '__file__', str(script)), \
                    patch.dict(stats.os.environ, {'ECOSYSTEM_GITHUB_TOKEN': 'unused'}), \
                    patch.object(stats, 'discover_boards', return_value={'count': 0, 'packs': []}) as discover:
                self.assertEqual(stats.main(['--boards-only', '--output', str(root / 'report')]), 0)
                discover.assert_called_once()

    def test_complete_collection_requires_review_decisions_before_network_access(self):
        with tempfile.TemporaryDirectory() as directory:
            sources = Path(directory) / 'sources.json'
            sources.write_text(json.dumps({'board_packs': [], 'excluded_repositories': []}))
            with patch.dict(stats.os.environ, {'ECOSYSTEM_GITHUB_TOKEN': 'unused'}), \
                    patch.object(stats, 'discover_boards', return_value={'count': 0, 'packs': []}) as discover, \
                    patch.object(stats, 'collect_projects', return_value=[]) as projects:
                result = stats.main(['--sources', str(sources), '--output', str(Path(directory) / 'report')])
            self.assertEqual(result, 1)
            discover.assert_not_called()
            projects.assert_not_called()

    def test_boards_only_does_not_display_zero_projects(self):
        snapshot = {'updated_at': '2026-11-05T00:00:00Z', 'boards': {'count': 2, 'packs': []}, 'projects': None}
        with tempfile.TemporaryDirectory() as directory:
            stats.write_report(snapshot, Path(directory))
            badge = (Path(directory) / 'dependent-projects.svg').read_text(encoding='utf-8')
            self.assertIn('not scanned', badge)
            self.assertIsNone(json.loads((Path(directory) / 'stats.json').read_text())['projects'])

    def test_default_preview_discovers_boards_but_listed_preview_never_scans_projects(self):
        boards = {'count': 1, 'packs': []}
        with tempfile.TemporaryDirectory() as directory:
            sources = Path(directory) / 'sources.json'
            sources.write_text(json.dumps({'board_packs': [], 'excluded_repositories': []}))
            arguments = ['--sources', str(sources), '--boards-only', '--output', directory]
            with patch.dict(stats.os.environ, {'ECOSYSTEM_GITHUB_TOKEN': 'unused'}), \
                    patch.object(stats, 'discover_boards', return_value=boards.copy()) as discover, \
                    patch.object(stats, 'collect_projects') as projects:
                self.assertEqual(stats.main(arguments), 0)
                discover.assert_called_once()
                projects.assert_not_called()
            with patch.dict(stats.os.environ, {}, clear=True), \
                    patch.object(stats, 'collect_boards', return_value=boards.copy()) as listed, \
                    patch.object(stats, 'discover_boards') as discover, \
                    patch.object(stats, 'collect_projects') as projects:
                self.assertEqual(stats.main(arguments + ['--listed-boards']), 0)
                listed.assert_called_once()
                discover.assert_not_called()
                projects.assert_not_called()
                self.assertIsNone(json.loads((Path(directory) / 'stats.json').read_text())['projects'])
                self.assertEqual(stats.main(['--listed-boards', '--output', directory]), 1)

    def test_report_escapes_untrusted_dependency_paths(self):
        snapshot = {'updated_at': '2026-11-05T00:00:00Z', 'boards': {'count': 1, 'packs': []},
                    'projects': {'count': 1, 'repositories': [{'repository': 'user/app',
                        'url': 'https://github.com/user/app', 'evidence': [{
                            'path': '<tag>/x]name\nfolder/idf_component.yml', 'kind': 'direct',
                            'url': 'https://github.com/user/app/blob/sha/main/idf_component.yml'}]}]}}
        with tempfile.TemporaryDirectory() as directory:
            stats.write_report(snapshot, Path(directory))
            report = (Path(directory) / 'README.md').read_text(encoding='utf-8')
        self.assertNotIn('<tag>', report)
        self.assertIn('x\\]name folder', report)

    def test_board_report_shows_merged_origins_and_escapes_source_paths(self):
        boards = {'count': 1, 'listed_count': 0, 'discovered_count': 1, 'duplicates_removed': 1,
                  'search_query': 'chip filename:board_info.yaml in:file', 'search_matches': 2,
                  'packs': [{'component': 'user/app', 'repository': 'user/app',
                             'path': 'boards/<tag>| name', 'revision': 'commit-sha', 'count': 1, 'raw_count': 2,
                             'definitions': [{'board': 'custom', 'chip': 'esp32', 'tree_sha': 'directory-sha', 'origins': [
                                 {'repository': 'user/app', 'path': 'boards/<tag>| name/custom',
                                  'url': 'https://github.com/user/app/tree/commit-sha/boards/custom'},
                                 {'repository': 'user/copy', 'path': 'boards/custom',
                                  'url': 'https://github.com/user/copy/tree/copy-sha/boards/custom'}]}]}]}
        boards['packs'].append({'component': 'user/copy', 'repository': 'user/copy', 'path': 'boards',
                                'revision': 'copy-sha', 'count': 0, 'raw_count': 1, 'definitions': [], 'boards': []})
        snapshot = {'updated_at': '2026-11-05T00:00:00Z', 'boards': boards, 'projects': None}
        with tempfile.TemporaryDirectory() as directory:
            stats.write_report(snapshot, Path(directory))
            report = (Path(directory) / 'README.md').read_text()
        self.assertIn('user/copy', report)
        self.assertNotIn('user/copy', report.split('## Board definition evidence')[0])
        self.assertIn('Merged copies and upstream variants', report)
        self.assertIn('合并相同副本: **1**', report)
        self.assertNotIn('<tag>', report)
        self.assertIn('%3Ctag%3E%7C%20name', report)

    def test_review_report_separates_known_components_and_individual_outside_candidates(self):
        def origin(repo, path):
            return {'repository': repo, 'path': path, 'revision': 'sha', 'tree_sha': repo,
                    'url': 'https://github.com/' + repo + '/tree/sha/' + path}
        listed = origin('vendor/listed', 'boards/model')
        copied = origin('vendor/vision', 'boards/MODEL/bmgr')
        custom = origin('user/project', 'boards/new_board')
        boards = {'count': 3, 'listed_count': 1, 'discovered_count': 2, 'duplicates_removed': 0,
                  'search_query': 'chip filename:board_info.yaml in:file', 'search_matches': 3,
                  'packs': []}
        for source, is_known, name in ((listed, True, 'model'), (copied, False, 'bmgr'), (custom, False, 'new_board')):
            boards['packs'].append({'component': source['repository'], 'repository': source['repository'],
                'path': str(Path(source['path']).parent), 'revision': 'sha', 'count': 1,
                'raw_count': 1, 'listed': is_known,
                'definitions': [{'board': name, 'path': source['path'], 'chip': 'esp32s3',
                                 'tree_sha': source['tree_sha'], 'origins': [source]}]})
        snapshot = {'updated_at': '2026-11-05T00:00:00Z', 'boards': boards, 'projects': None}
        with tempfile.TemporaryDirectory() as directory:
            stats.write_report(snapshot, Path(directory))
            report = (Path(directory) / 'README.md').read_text()
            data = json.loads((Path(directory) / 'stats.json').read_text())
        known = report.split('## Known components / 已知组件')[1].split('## Outside-known candidates / 已知组件范围外候选')[0]
        outside = report.split('## Outside-known candidates / 已知组件范围外候选')[1].split('## Board definition evidence')[0]
        self.assertIn('vendor/listed', known)
        self.assertNotIn('vendor/vision', known)
        self.assertIn('vendor/vision', outside)
        self.assertIn('user/project', outside)
        self.assertIn('可能对应已知型号', outside)
        self.assertIn('未找到已知型号匹配', outside)
        self.assertIn('待你过滤', outside)
        self.assertEqual(data['boards']['outside_review']['candidate_count'], 2)
        self.assertEqual(data['boards']['outside_review']['possible_known_count'], 1)

    def test_reviewed_badge_counts_approved_boards_and_keeps_new_candidates_pending(self):
        def source(repo, name):
            return {'repository': repo, 'path': 'boards/' + name, 'revision': 'sha',
                    'url': 'https://github.com/' + repo + '/tree/sha/boards/' + name}
        boards = {'count': 4, 'packs': []}
        for repo, name, listed in (('vendor/known', 'model', True), ('user/new', 'new_board', False),
                                   ('user/copy', 'model', False), ('user/unreviewed', 'unreviewed_board', False)):
            origin = source(repo, name)
            boards['packs'].append({'component': repo, 'repository': repo, 'path': 'boards', 'revision': 'sha',
                'count': 1, 'boards': [name], 'listed': listed,
                'definitions': [{'board': name, 'path': origin['path'], 'chip': 'esp32s3',
                                 'tree_sha': repo, 'origins': [origin]}]})
        def decision(repo, name, action):
            return {'id': repo, 'decision': action, 'reason': 'Reviewed',
                    'selectors': [{'repository': repo, 'path': 'boards/' + name, 'chip': 'esp32s3'}]}
        included = decision('user/new', 'new_board', 'include')
        alias = decision('user/copy', 'model', 'alias')
        alias['target'] = {'repository': 'vendor/known', 'path': 'boards/model'}
        snapshot = {'updated_at': '2026-11-06T00:00:00Z', 'boards': boards, 'projects': None,
                    'board_filters': {'board_decisions': {'schema_version': 1, 'reviewed_at': '2026-11-06',
                                                         'decisions': [included, alias]}}}
        with tempfile.TemporaryDirectory() as directory:
            stats.write_report(snapshot, Path(directory))
            data = json.loads((Path(directory) / 'stats.json').read_text())
            report = (Path(directory) / 'README.md').read_text()
            badge = (Path(directory) / 'supported-boards.svg').read_text()
        self.assertEqual(data['boards']['count'], 4)
        self.assertEqual(data['boards']['approved_count'], 2)
        self.assertEqual(data['boards']['decision_summary']['aliased_count'], 1)
        self.assertEqual(data['boards']['decision_summary']['pending_count'], 1)
        self.assertIn('Supported boards: 2', badge)
        self.assertIn('支持板卡（已审阅）: **2**', report)
        self.assertIn('Approved additional boards / 已确认的范围外板卡', report)
        self.assertIn('Confirmed aliases / 已确认的同型号来源', report)
        self.assertIn('Pending review / 待审候选', report)
        self.assertNotIn('approved_count', boards)


if __name__ == '__main__':
    unittest.main()
