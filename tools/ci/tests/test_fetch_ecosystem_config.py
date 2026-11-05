# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

import copy
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
from urllib.error import HTTPError

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
SPEC = importlib.util.find_spec('fetch_ecosystem_config')
fetcher = __import__('fetch_ecosystem_config') if SPEC else None
LIST_PATH = '/projects/42/secure_files?per_page=100&page=1'


def documents():
    sources = {'board_packs': [{'component': 'vendor/boards', 'repository': 'vendor/boards', 'path': 'boards'}],
               'board_excluded_repositories': [], 'board_excluded_paths': [], 'board_lineages': [],
               'board_review_matches': [], 'excluded_repositories': [], 'excluded_evidence_paths': [],
               'board_review_ledger': 'ecosystem_board_review.json'}
    review = {'schema_version': 1, 'reviewed_at': '2026-11-06', 'decisions': [
        {'id': 'model-a', 'decision': 'include', 'reason': 'Reviewed',
         'selectors': [{'repository': 'user/app', 'path': 'boards/a', 'chip': 'esp32s3'}]}]}
    return sources, review


class FakeClient:
    def __init__(self, sources=None, review=None):
        sources, review = (sources, review) if sources is not None else documents()
        self.responses = {}
        self.headers = {}
        self.calls = []
        files = []
        for file_id, name, document in ((11, 'ecosystem_sources.json', sources),
                                        (12, 'ecosystem_board_review.json', review)):
            content = json.dumps(document).encode('utf-8')
            path = '/projects/42/secure_files/' + str(file_id)
            metadata = {'id': file_id, 'name': name, 'checksum_algorithm': 'sha256',
                        'checksum': hashlib.sha256(content).hexdigest()}
            self.responses[path] = json.dumps(metadata).encode('utf-8')
            self.responses[path + '/download'] = content
            files.append(metadata)
        self.responses[LIST_PATH] = json.dumps(files).encode('utf-8')
        self.headers[LIST_PATH] = {'X-Page': '1', 'X-Per-Page': '100', 'X-Total': '2',
                                   'X-Total-Pages': '1', 'X-Next-Page': ''}

    def get(self, path, include_headers=False):
        self.calls.append(path)
        value = self.responses[path]
        if isinstance(value, Exception):
            raise value
        return (value, self.headers.get(path, {})) if include_headers else value


class FetchConfigTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(fetcher, 'Secure Files configuration downloader must exist')
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.output = Path(self.temporary.name) / 'private-config'
        self.env = {'CI_API_V4_URL': 'https://gitlab.example/api/v4', 'CI_PROJECT_ID': '42',
                    'CI_JOB_TOKEN': 'fake-private-job-token', 'ECOSYSTEM_SOURCES_FILE_ID': '11',
                    'ECOSYSTEM_REVIEW_FILE_ID': '12'}

    def fetch(self, client=None, env=None):
        return fetcher.fetch_config(self.output, self.env if env is None else env, client or FakeClient())

    def automatic_env(self):
        return {key: value for key, value in self.env.items() if not key.endswith('_FILE_ID')}

    def test_no_file_ids_discovers_exact_names_and_then_verifies_both_files(self):
        client = FakeClient()
        self.fetch(client, self.automatic_env())
        self.assertEqual(client.calls, [LIST_PATH, '/projects/42/secure_files/11',
                                        '/projects/42/secure_files/11/download', '/projects/42/secure_files/12',
                                        '/projects/42/secure_files/12/download'])
        self.assertEqual(json.loads((self.output / 'ecosystem_sources.json').read_text()), documents()[0])

    def test_missing_or_duplicate_exact_names_fail_before_download(self):
        original = json.loads(FakeClient().responses[LIST_PATH])
        duplicate = dict(original[0], id=99)
        for files in (original[:1], original + [duplicate]):
            client = FakeClient()
            client.responses[LIST_PATH] = json.dumps(files).encode()
            client.headers[LIST_PATH]['X-Total'] = str(len(files))
            with self.subTest(files=len(files)), self.assertRaises(fetcher.ConfigError):
                self.fetch(client, self.automatic_env())
            self.assertEqual(client.calls, [LIST_PATH])
            self.assertFalse(self.output.exists())

    def test_duplicate_ids_in_list_are_rejected(self):
        client = FakeClient()
        files = json.loads(client.responses[LIST_PATH])
        files.append(dict(files[0], name='unrelated.json'))
        client.responses[LIST_PATH] = json.dumps(files).encode()
        client.headers[LIST_PATH]['X-Total'] = '3'
        with self.assertRaises(fetcher.ConfigError):
            self.fetch(client, self.automatic_env())
        self.assertEqual(client.calls, [LIST_PATH])
        self.assertFalse(self.output.exists())

    def test_incomplete_or_inconsistent_pagination_is_rejected(self):
        changes = ({'X-Next-Page': '2'}, {'X-Total': '3'}, {'X-Total-Pages': '2'}, {'X-Page': '2'},
                   {'X-Per-Page': '20'}, {'Link': '<https://other.example/list?page=2>; rel="next"'})
        for change in changes:
            client = FakeClient()
            client.headers[LIST_PATH].update(change)
            with self.subTest(change=change), self.assertRaises(fetcher.ConfigError):
                self.fetch(client, self.automatic_env())
            self.assertEqual(client.calls, [LIST_PATH])
            self.assertFalse(self.output.exists())

    def test_short_page_does_not_require_optional_pagination_headers(self):
        client = FakeClient()
        client.headers[LIST_PATH] = {}
        self.fetch(client, self.automatic_env())
        self.assertTrue(self.output.is_dir())

    def test_full_page_without_complete_count_evidence_is_rejected(self):
        client = FakeClient()
        files = json.loads(client.responses[LIST_PATH])
        files.extend({'id': number, 'name': 'unrelated-{}.json'.format(number)} for number in range(100, 198))
        client.responses[LIST_PATH] = json.dumps(files).encode()
        client.headers[LIST_PATH] = {}
        with self.assertRaises(fetcher.ConfigError):
            self.fetch(client, self.automatic_env())
        self.assertFalse(self.output.exists())

    def test_list_http_error_is_sanitized_and_leaves_no_output(self):
        client = FakeClient()
        client.responses[LIST_PATH] = HTTPError('https://example/fake-private-job-token', 403,
                                                'private-list-response', {}, None)
        with self.assertRaises(fetcher.ConfigError) as error:
            self.fetch(client, self.automatic_env())
        self.assertIn('403', str(error.exception))
        self.assertNotIn('fake-private-job-token', str(error.exception))
        self.assertNotIn('private-list-response', str(error.exception))
        self.assertFalse(self.output.exists())

    def test_single_explicit_id_does_not_fall_back_to_name_discovery(self):
        env = self.automatic_env()
        env['ECOSYSTEM_SOURCES_FILE_ID'] = '11'
        client = FakeClient()
        with self.assertRaises(fetcher.ConfigError):
            self.fetch(client, env)
        self.assertEqual(client.calls, [])

    def test_list_http_request_uses_job_header_and_only_the_fixed_list_query(self):
        response = io.BytesIO(b'[]')
        response.headers = {'X-Total': '0'}
        opener = unittest.mock.Mock()
        opener.open.return_value = response
        with patch.object(fetcher, 'build_opener', return_value=opener):
            client = fetcher.SecureFilesClient(self.env['CI_API_V4_URL'], self.env['CI_JOB_TOKEN'])
            self.assertEqual(client.get(LIST_PATH, include_headers=True), (b'[]', {'X-Total': '0'}))
        request = opener.open.call_args[0][0]
        self.assertEqual(request.get_header('Job-token'), self.env['CI_JOB_TOKEN'])
        self.assertEqual(request.full_url, self.env['CI_API_V4_URL'] + LIST_PATH)
        self.assertNotIn(self.env['CI_JOB_TOKEN'], request.full_url)
        for path in ('/projects/42/secure_files?per_page=20&page=1',
                     '/projects/42/secure_files?per_page=100&page=2',
                     '/projects/42/secure_files?per_page=100&page=1&url=https://other.example'):
            with self.subTest(path=path), self.assertRaises(fetcher.ConfigError):
                client.get(path)

    def test_success_downloads_and_verifies_both_fixed_files(self):
        client = FakeClient()
        before = copy.deepcopy(self.env)
        self.fetch(client)
        self.assertEqual(self.env, before)
        self.assertEqual(set(path.name for path in self.output.iterdir()),
                         {'ecosystem_sources.json', 'ecosystem_board_review.json'})
        for name, expected in zip(('ecosystem_sources.json', 'ecosystem_board_review.json'), documents()):
            self.assertEqual(json.loads((self.output / name).read_text()), expected)
        self.assertEqual(client.calls, ['/projects/42/secure_files/11', '/projects/42/secure_files/11/download',
                                        '/projects/42/secure_files/12', '/projects/42/secure_files/12/download'])
        if os.name == 'posix':
            self.assertEqual(self.output.stat().st_mode & 0o777, 0o700)
            self.assertTrue(all(path.stat().st_mode & 0o777 == 0o600 for path in self.output.iterdir()))

    def test_missing_parameters_fail_before_any_download(self):
        for key in self.env:
            env = dict(self.env)
            del env[key]
            client = FakeClient()
            with self.subTest(key=key), self.assertRaises(fetcher.ConfigError):
                self.fetch(client, env)
            self.assertEqual(client.calls, [])
        self.assertFalse(self.output.exists())

    def test_unsafe_api_url_or_identifiers_are_rejected(self):
        invalid = [('CI_API_V4_URL', 'http://gitlab.example/api/v4'),
                   ('CI_API_V4_URL', 'https://user:secret@gitlab.example/api/v4'),
                   ('CI_API_V4_URL', 'https://gitlab.example/api/v4?token=secret'),
                   ('CI_API_V4_URL', 'https://gitlab.example/api/v4#secret'),
                   ('CI_PROJECT_ID', '../other'), ('ECOSYSTEM_REVIEW_FILE_ID', '0'),
                   ('ECOSYSTEM_REVIEW_FILE_ID', '11'), ('CI_JOB_TOKEN', 'bad\r\nheader')]
        for key, value in invalid:
            client = FakeClient()
            with self.subTest(key=key, value=value), self.assertRaises(fetcher.ConfigError):
                self.fetch(client, dict(self.env, **{key: value}))
            self.assertEqual(client.calls, [])

    def test_401_or_404_during_second_file_leaves_no_configuration(self):
        for status in (401, 404):
            client = FakeClient()
            client.responses['/projects/42/secure_files/12'] = HTTPError(
                'https://example/fake-private-job-token', status, 'private-response-marker', {}, None)
            with self.subTest(status=status), self.assertRaises(fetcher.ConfigError) as error:
                self.fetch(client)
            self.assertNotIn('fake-private-job-token', str(error.exception))
            self.assertNotIn('private-response-marker', str(error.exception))
            self.assertFalse(self.output.exists())
            self.assertEqual(list(self.output.parent.iterdir()), [])

    def test_checksum_mismatch_is_rejected_before_json_is_used(self):
        client = FakeClient()
        client.responses['/projects/42/secure_files/12/download'] = b'changed-private-content'
        with self.assertRaises(fetcher.ConfigError):
            self.fetch(client)
        self.assertFalse(self.output.exists())

    def test_invalid_metadata_is_rejected(self):
        changes = ({'checksum_algorithm': 'md5'}, {'checksum': 'invalid'}, {'id': 99},
                   {'name': '../ecosystem_sources.json'})
        for change in changes:
            client = FakeClient()
            path = '/projects/42/secure_files/11'
            metadata = json.loads(client.responses[path])
            metadata.update(change)
            client.responses[path] = json.dumps(metadata).encode()
            with self.subTest(change=change), self.assertRaises(fetcher.ConfigError):
                self.fetch(client)
            self.assertFalse(self.output.exists())

    def test_invalid_json_is_rejected_without_exposing_content(self):
        client = FakeClient()
        path = '/projects/42/secure_files/12'
        content = b'private-response-marker invalid JSON'
        metadata = json.loads(client.responses[path])
        metadata['checksum'] = hashlib.sha256(content).hexdigest()
        client.responses[path] = json.dumps(metadata).encode()
        client.responses[path + '/download'] = content
        with self.assertRaises(fetcher.ConfigError) as error:
            self.fetch(client)
        self.assertNotIn('private-response-marker', str(error.exception))
        self.assertFalse(self.output.exists())

    def test_incomplete_configuration_or_ledger_path_escape_is_rejected(self):
        sources, review = documents()
        invalid = [({}, review), (dict(sources, board_packs=[]), review),
                   (dict(sources, board_review_ledger='../review.json'), review),
                   (sources, {'schema_version': 1}),
                   (sources, dict(review, decisions=[{'id': 'a', 'decision': 'include'}]))]
        for source_document, review_document in invalid:
            with self.subTest(source=source_document), self.assertRaises(fetcher.ConfigError):
                self.fetch(FakeClient(source_document, review_document))
            self.assertFalse(self.output.exists())

    def test_existing_output_is_rejected_without_overwriting_it(self):
        self.output.mkdir()
        sentinel = self.output / 'existing.txt'
        sentinel.write_text('keep')
        client = FakeClient()
        with self.assertRaises(fetcher.ConfigError):
            self.fetch(client)
        self.assertEqual(sentinel.read_text(), 'keep')
        self.assertEqual(client.calls, [])

    def test_http_client_uses_job_header_and_never_puts_token_in_url(self):
        response = io.BytesIO(b'{}')
        opener = unittest.mock.Mock()
        opener.open.return_value = response
        with patch.object(fetcher, 'build_opener', return_value=opener):
            client = fetcher.SecureFilesClient(self.env['CI_API_V4_URL'], self.env['CI_JOB_TOKEN'])
            self.assertEqual(client.get('/projects/42/secure_files/11'), b'{}')
        request = opener.open.call_args[0][0]
        self.assertEqual(request.get_header('Job-token'), self.env['CI_JOB_TOKEN'])
        self.assertNotIn(self.env['CI_JOB_TOKEN'], request.full_url)
        self.assertEqual(request.get_method(), 'GET')

    def test_redirects_are_not_followed_with_job_credentials(self):
        handler = fetcher.NoRedirect()
        result = handler.redirect_request(None, None, 302, 'Found', {}, 'https://storage.example/file')
        self.assertIsNone(result)

    def test_cli_failure_is_sanitized_and_returns_nonzero(self):
        client = FakeClient()
        client.responses['/projects/42/secure_files/11'] = OSError('fake-private-job-token private-body')
        output = io.StringIO()
        with patch('sys.stderr', output):
            status = fetcher.main(['--output', str(self.output)], environ=self.env, client=client)
        self.assertEqual(status, 1)
        self.assertNotIn('fake-private-job-token', output.getvalue())
        self.assertNotIn('private-body', output.getvalue())
        self.assertFalse(self.output.exists())


if __name__ == '__main__':
    unittest.main()
