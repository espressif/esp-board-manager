# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

import hashlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
from urllib.error import HTTPError, URLError

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
registry_module = (__import__('gitlab_ecosystem_history')
                   if importlib.util.find_spec('gitlab_ecosystem_history') else None)
CHANGES = '# Current scan changes\n'


class Response:
    def __init__(self, content, status=200, headers=None):
        self.content = content
        self.status = status
        self.headers = headers or {}

    def read(self, length):
        return self.content[:length]

    def __enter__(self):
        return self

    def __exit__(self, *arguments):
        return False


class FakeRegistry:
    def __init__(self):
        self.files = {}
        self.calls = []
        self.pages = {}
        self.failures = {}
        self.requests = []

    def open(self, request, timeout):
        url = request.full_url
        method = request.get_method()
        self.calls.append((method, url, request.data))
        self.requests.append(request)
        failure = self.failures.get((method, url), [])
        if failure:
            error, commit = failure.pop(0)
            if commit:
                self.files[url] = request.data
            raise error
        if method == 'PUT':
            self.files[url] = request.data
            return Response(b'{"message":"201 Created"}', 201)
        if '/packages?' in url:
            return Response(json.dumps(self.pages.get(url, [])).encode())
        if url not in self.files:
            raise HTTPError(url, 404, 'private missing response', {}, io.BytesIO())
        return Response(self.files[url])


class HistoryRegistryTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(registry_module, 'Private ecosystem history registry must exist')
        self.env = {'CI_API_V4_URL': 'https://gitlab.example/api/v4', 'CI_PROJECT_ID': '42',
                    'CI_JOB_TOKEN': 'private-job-token', 'CI_JOB_ID': '987',
                    'CI_COMMIT_BRANCH': 'main', 'CI_PROJECT_URL': 'https://gitlab.example/group/project',
                    'CI_PROJECT_VISIBILITY': 'private'}
        self.client = registry_module.HistoryRegistry.from_environment(self.env)
        self.remote = FakeRegistry()
        self.client.opener = self.remote
        self.waits = []
        self.client.sleep = self.waits.append

    def file(self, name, version='1.0.987'):
        return 'https://gitlab.example/api/v4/projects/42/packages/generic/ecosystem-history/{}/{}'.format(version, name)

    def package(self, number, name='ecosystem-history'):
        return {'id': number, 'name': name, 'version': '1.0.' + str(number), 'package_type': 'generic'}

    def page(self, number=1):
        return ('https://gitlab.example/api/v4/projects/42/packages?package_type=generic'
                '&package_name=ecosystem-history&order_by=created_at&sort=desc&per_page=100&page=' + str(number))

    def test_environment_provides_stable_run_and_branch_isolated_stream(self):
        self.assertEqual(self.client.run, {'id': '987', 'url': 'https://gitlab.example/group/project/-/jobs/987', 'ref': 'main'})
        self.assertEqual(self.client.package_name, 'ecosystem-history')
        self.assertEqual(self.client.version, '1.0.987')
        branch = 'feature/history'
        test = registry_module.HistoryRegistry.from_environment(dict(self.env, CI_COMMIT_BRANCH=branch))
        self.assertEqual(test.package_name, 'ecosystem-history-test-' + hashlib.sha256(branch.encode()).hexdigest()[:12])

    def test_missing_unsafe_or_public_settings_fail_before_network(self):
        changes = ({'CI_PROJECT_VISIBILITY': 'public'}, {'CI_PROJECT_VISIBILITY': 'internal'},
                   {'CI_PROJECT_VISIBILITY': ''}, {'CI_API_V4_URL': 'http://gitlab.example/api/v4'},
                   {'CI_API_V4_URL': 'https://user:password@gitlab.example/api/v4'},
                   {'CI_API_V4_URL': 'https://gitlab.example/api/v4?secret=x'},
                   {'CI_PROJECT_ID': '../42'}, {'CI_JOB_ID': '0'}, {'CI_JOB_TOKEN': 'bad\ntoken'},
                   {'CI_COMMIT_BRANCH': ''}, {'CI_PROJECT_URL': 'https://other.example/project'},
                   {'CI_JOB_URL': 'https://other.example/jobs/987'})
        for change in changes:
            with self.subTest(change=change), self.assertRaises(registry_module.RegistryError):
                registry_module.HistoryRegistry.from_environment(dict(self.env, **change))

    def test_empty_registry_is_initial_baseline(self):
        self.assertIsNone(self.client.load())
        self.assertEqual(len(self.remote.calls), 1)

    def test_load_filters_fuzzy_matches_skips_incomplete_and_loads_previous_marker(self):
        self.remote.pages[self.page()] = [self.package(1002, 'ecosystem-history-test-other'),
                                         self.package(1001), self.package(1000)]
        self.remote.files[self.file('history.json', '1.0.1000')] = b'{"entries": []}'
        self.assertEqual(self.client.load(), {'entries': []})
        self.assertEqual([call[1] for call in self.remote.calls], [self.page(), self.file('history.json', '1.0.1001'), self.file('history.json', '1.0.1000')])

    def test_load_follows_derived_pages_when_first_page_contains_only_other_names(self):
        self.remote.pages[self.page()] = [self.package(number, 'ecosystem-history-test-other') for number in range(1000, 1100)]
        self.remote.pages[self.page(2)] = [self.package(5)]
        self.remote.files[self.file('history.json', '1.0.5')] = b'{"entries": []}'
        self.assertEqual(self.client.load(), {'entries': []})
        self.assertEqual(self.remote.calls[1][1], self.page(2))

    def test_malformed_or_non_object_or_duplicate_key_history_is_not_silently_reset(self):
        for raw in (b'{bad', b'[]', b'{"entries":[],"entries":[]}', b'{"value":NaN}'):
            self.remote.pages[self.page()] = [self.package(5)]
            self.remote.files[self.file('history.json', '1.0.5')] = raw
            with self.subTest(raw=raw), self.assertRaises(registry_module.RegistryError):
                self.client.load()

    def test_save_preflights_all_three_files_then_uploads_companions_before_marker(self):
        history = {'entries': [{'date': '2026-11-16'}]}
        self.client.save(history, '# History\n', CHANGES)
        puts = [call for call in self.remote.calls if call[0] == 'PUT']
        self.assertEqual([(call[0], call[1]) for call in self.remote.calls[:3]], [
            ('GET', self.file('history.json')), ('GET', self.file('README.md')), ('GET', self.file('changes.md'))])
        self.assertEqual([call[1] for call in puts], [self.file('README.md'), self.file('changes.md'), self.file('history.json')])
        self.assertEqual(puts[1][2], CHANGES.encode())
        self.assertEqual(json.loads(puts[-1][2]), history)

    def test_same_version_is_idempotent_and_different_content_is_never_overwritten(self):
        history = {'entries': []}
        self.client.save(history, '# History\n', CHANGES)
        self.remote.calls.clear()
        self.client.save(history, '# History\n', CHANGES)
        self.assertFalse(any(call[0] == 'PUT' for call in self.remote.calls))
        for changed_history, markdown, changes in (({'entries': [1]}, '# History\n', CHANGES),
                                                  (history, 'different', CHANGES),
                                                  (history, '# History\n', 'different changes')):
            self.remote.calls.clear()
            with self.subTest(markdown=markdown), self.assertRaises(registry_module.RegistryError):
                self.client.save(changed_history, markdown, changes)
            self.assertFalse(any(call[0] == 'PUT' for call in self.remote.calls))

    def test_partial_readme_upload_is_resumed_only_when_identical(self):
        self.remote.files[self.file('README.md')] = b'# History\n'
        self.client.save({'entries': []}, '# History\n', CHANGES)
        self.assertEqual([call[1] for call in self.remote.calls if call[0] == 'PUT'], [self.file('changes.md'), self.file('history.json')])
        self.remote.files.pop(self.file('history.json'))
        with self.assertRaises(registry_module.RegistryError):
            self.client.save({'entries': []}, '# Other\n', CHANGES)

    def test_put_unknown_outcome_reads_back_identical_content_instead_of_reuploading(self):
        error = URLError('private-job-token and private server detail')
        self.remote.failures[('PUT', self.file('history.json'))] = [(error, True)]
        self.client.save({'entries': []}, '# History\n', CHANGES)
        puts = [call for call in self.remote.calls if call[0] == 'PUT' and call[1].endswith('history.json')]
        self.assertEqual(len(puts), 1)

    def test_get_503_retry_and_put_503_retry_keep_finite_budget(self):
        for method, url in (('GET', self.page()), ('PUT', self.file('history.json'))):
            errors = [HTTPError(url, 503, 'secret body', {'Retry-After': '3'}, io.BytesIO()) for _ in range(2)]
            self.remote.failures[(method, url)] = [(error, False) for error in errors]
        self.assertIsNone(self.client.load())
        self.client.save({'entries': []}, '# History\n', CHANGES)
        self.assertEqual(self.waits, [3.0, 4, 3.0, 4])

    def test_retry_after_over_budget_and_auth_fail_sanitized(self):
        for code, headers in ((503, {'Retry-After': '99999'}), (403, {})):
            self.remote.failures[('GET', self.page())] = [(HTTPError(self.page(), code, 'private-job-token secret body', headers, io.BytesIO()), False)] * 3
            with self.subTest(code=code), self.assertRaises(registry_module.RegistryError) as caught:
                self.client.load()
            self.assertNotIn('private-job-token', str(caught.exception))
            self.assertNotIn('secret', str(caught.exception))
        self.assertFalse(self.waits)

    def test_exhausted_get_attempts_do_not_return_an_empty_baseline(self):
        self.remote.failures[('GET', self.page())] = [(URLError('secret error'), False)] * 3
        with self.assertRaises(registry_module.RegistryError):
            self.client.load()
        self.assertEqual(self.waits, [2, 4])

    def test_list_http_404_does_not_masquerade_as_first_scan(self):
        self.remote.failures[('GET', self.page())] = [(HTTPError(self.page(), 404, 'private body', {}, io.BytesIO()), False)]
        with self.assertRaises(registry_module.RegistryError):
            self.client.load()

    def test_missing_readme_from_committed_version_fails_without_rewriting(self):
        self.client.save({'entries': []}, '# History\n', CHANGES)
        self.remote.files.pop(self.file('README.md'))
        self.remote.calls.clear()
        with self.assertRaises(registry_module.RegistryError):
            self.client.save({'entries': []}, '# History\n', CHANGES)
        self.assertFalse(any(call[0] == 'PUT' for call in self.remote.calls))

    def test_invalid_upload_never_makes_a_remote_request(self):
        for history, markdown in (([], '# History\n'), ({'bad': float('nan')}, '# History\n'), ({}, '')):
            with self.subTest(history=history), self.assertRaises(registry_module.RegistryError):
                self.client.save(history, markdown, CHANGES)
        self.assertFalse(self.remote.calls)

    def test_partial_changes_resumes_only_after_both_companions_match(self):
        self.remote.files[self.file('changes.md')] = CHANGES.encode()
        self.client.save({'entries': []}, '# History\n', CHANGES)
        self.assertEqual([call[1] for call in self.remote.calls if call[0] == 'PUT'],
                         [self.file('README.md'), self.file('history.json')])

    def test_mismatched_partial_changes_is_found_before_any_upload(self):
        self.remote.files[self.file('changes.md')] = b'Other scan changes'
        with self.assertRaises(registry_module.RegistryError):
            self.client.save({'entries': []}, '# History\n', CHANGES)
        self.assertFalse(any(call[0] == 'PUT' for call in self.remote.calls))
        self.assertNotIn(self.file('README.md'), self.remote.files)

    def test_missing_changes_from_committed_legacy_version_is_not_backfilled(self):
        history = {'entries': []}
        self.remote.files[self.file('history.json')] = (json.dumps(history, ensure_ascii=False, sort_keys=True, indent=2) + '\n').encode()
        self.remote.files[self.file('README.md')] = b'# History\n'
        with self.assertRaises(registry_module.RegistryError):
            self.client.save(history, '# History\n', CHANGES)
        self.assertFalse(any(call[0] == 'PUT' for call in self.remote.calls))
        self.remote.pages[self.page()] = [self.package(987)]
        self.assertEqual(self.client.load(), history)

    def test_changes_payload_must_be_nonempty_text_and_fit_size_limit(self):
        for changes in ('', ' \n', None, b'bytes', '\ud800'):
            with self.subTest(changes=repr(changes)), self.assertRaises(registry_module.RegistryError):
                self.client.save({}, '# History\n', changes)
        with patch.object(registry_module, 'MAX_FILE_BYTES', 32), self.assertRaises(registry_module.RegistryError):
            self.client.save({}, '# History\n', 'x' * 33)
        self.assertFalse(self.remote.calls)

    def test_package_listing_page_limit_cannot_restart_history(self):
        self.remote.pages[self.page()] = [self.package(number, 'ecosystem-history-test-other') for number in range(1000, 1100)]
        with patch.object(registry_module, 'MAX_PAGES', 1), self.assertRaises(registry_module.RegistryError):
            self.client.load()

    def test_429_can_retry_without_exposing_server_details(self):
        self.remote.failures[('GET', self.page())] = [(HTTPError(self.page(), 429, 'secret', {'Retry-After': '5'}, io.BytesIO()), False)]
        self.assertIsNone(self.client.load())
        self.assertEqual(self.waits, [5])

    def test_response_size_limit_cannot_be_used_as_empty_baseline(self):
        self.remote.pages[self.page()] = [self.package(5)]
        with patch.object(registry_module, 'MAX_FILE_BYTES', 16), self.assertRaises(registry_module.RegistryError):
            self.client.load()

    def test_object_storage_download_redirect_never_receives_job_token(self):
        self.remote.pages[self.page()] = [self.package(5)]
        original = self.file('history.json', '1.0.5')
        storage = 'https://storage.example/history.json?signature=private-presigned-value'
        self.remote.failures[('GET', original)] = [(HTTPError(original, 302, 'redirect', {'Location': storage}, io.BytesIO()), False)]
        self.remote.files[storage] = b'{"entries": []}'
        self.assertEqual(self.client.load(), {'entries': []})
        self.assertEqual(self.remote.requests[-2].get_header('Job-token'), 'private-job-token')
        self.assertIsNone(self.remote.requests[-1].get_header('Job-token'))
        self.assertEqual(self.remote.requests[-1].full_url, storage)

    def test_relative_same_origin_redirect_keeps_gitlab_authentication(self):
        self.remote.pages[self.page()] = [self.package(5)]
        original = self.file('history.json', '1.0.5')
        location = '/private-download/history.json?signature=private-value'
        self.remote.failures[('GET', original)] = [(HTTPError(original, 302, 'redirect', {'Location': location}, io.BytesIO()), False)]
        self.remote.files['https://gitlab.example' + location] = b'{"entries": []}'
        self.assertEqual(self.client.load(), {'entries': []})
        self.assertEqual(self.remote.requests[-1].get_header('Job-token'), 'private-job-token')

    def test_unsafe_download_redirects_fail_before_second_host_request(self):
        self.remote.pages[self.page()] = [self.package(5)]
        original = self.file('history.json', '1.0.5')
        for location in ('http://storage.example/file', 'https://user:secret@storage.example/file',
                         'https://storage.example/file with-space', 'https://storage.example/file#fragment'):
            self.remote.failures[('GET', original)] = [(HTTPError(original, 302, 'secret', {'Location': location}, io.BytesIO()), False)]
            self.remote.calls.clear()
            with self.subTest(location=location), self.assertRaises(registry_module.RegistryError) as caught:
                self.client.load()
            self.assertEqual(len(self.remote.calls), 2)
            self.assertNotIn('secret', str(caught.exception))

    def test_download_redirect_chain_is_bounded(self):
        self.remote.pages[self.page()] = [self.package(5)]
        original = self.file('history.json', '1.0.5')
        chain = [original] + ['https://storage.example/file' + str(number) for number in range(4)]
        for source, destination in zip(chain, chain[1:]):
            self.remote.failures[('GET', source)] = [(HTTPError(source, 302, 'secret', {'Location': destination}, io.BytesIO()), False)]
        with self.assertRaises(registry_module.RegistryError):
            self.client.load()
        self.assertEqual(len(self.remote.calls), 5)
        self.assertFalse(any(call[1] == chain[-1] for call in self.remote.calls))

    def test_upload_redirect_is_not_followed(self):
        original = self.file('README.md')
        self.remote.failures[('PUT', original)] = [(HTTPError(original, 307, 'secret', {'Location': 'https://storage.example/upload'}, io.BytesIO()), False)]
        with self.assertRaises(registry_module.RegistryError):
            self.client.save({'entries': []}, '# History\n', CHANGES)
        self.assertFalse(any('storage.example' in call[1] for call in self.remote.calls))

    def test_missing_storage_object_does_not_silently_select_an_older_baseline(self):
        self.remote.pages[self.page()] = [self.package(5)]
        original = self.file('history.json', '1.0.5')
        storage = 'https://storage.example/history.json?signature=private-value'
        self.remote.failures[('GET', original)] = [(HTTPError(original, 302, 'redirect', {'Location': storage}, io.BytesIO()), False)]
        with self.assertRaises(registry_module.RegistryError):
            self.client.load()

    def test_storage_503_retries_original_download_without_leaking_job_token(self):
        self.remote.pages[self.page()] = [self.package(5)]
        original = self.file('history.json', '1.0.5')
        storage = 'https://storage.example/history.json?signature=private-value'
        self.remote.failures[('GET', original)] = [(HTTPError(original, 302, 'redirect', {'Location': storage}, io.BytesIO()), False) for _ in range(2)]
        self.remote.failures[('GET', storage)] = [(HTTPError(storage, 503, 'secret', {}, io.BytesIO()), False)]
        self.remote.files[storage] = b'{"entries": []}'
        self.assertEqual(self.client.load(), {'entries': []})
        self.assertEqual(self.waits, [2])
        requests = [request for request in self.remote.requests if request.full_url == storage]
        self.assertEqual(len(requests), 2)
        self.assertTrue(all(request.get_header('Job-token') is None for request in requests))


if __name__ == '__main__':
    unittest.main()
