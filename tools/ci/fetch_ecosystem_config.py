#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""Download private ecosystem configuration from this GitLab project's Secure Files.

Uses the metadata and download endpoints documented at
https://docs.gitlab.com/api/secure_files/ with the CI_JOB_TOKEN header.
"""

import argparse
import hashlib
import hmac
import json
import os
from pathlib import Path, PurePosixPath
import re
import sys
import tempfile
from urllib.error import HTTPError
from urllib.parse import urlsplit
from urllib.request import HTTPRedirectHandler, Request, build_opener


SOURCES_NAME = 'ecosystem_sources.json'
REVIEW_NAME = 'ecosystem_board_review.json'
MAX_FILE_BYTES = 5 * 1024 * 1024


class ConfigError(Exception):
    """A sanitized configuration-download failure, safe to show in job logs."""


class NoRedirect(HTTPRedirectHandler):
    """Never forward the job token to a redirect destination."""

    def redirect_request(self, request, response, code, message, headers, new_url):
        return None


def _api_url(value):
    try:
        parsed = urlsplit(value)
        valid = (parsed.scheme == 'https' and parsed.hostname and parsed.port != 0
                 and not parsed.username and not parsed.password and not parsed.query
                 and not parsed.fragment and not any(character.isspace() for character in value))
    except (TypeError, ValueError):
        valid = False
    if not valid:
        raise ConfigError('CI_API_V4_URL must be an HTTPS API URL without credentials, query, or fragment')
    return value.rstrip('/')


class SecureFilesClient:
    def __init__(self, api_url, token):
        self.api_url = _api_url(api_url)
        self.token = token
        self.opener = build_opener(NoRedirect())

    def get(self, path, include_headers=False):
        if not re.fullmatch(r'/projects/[1-9][0-9]*/secure_files(?:/[1-9][0-9]*(?:/download)?|\?per_page=100&page=1)', path):
            raise ConfigError('Invalid Secure Files request path')
        request = Request(self.api_url + path, headers={'JOB-TOKEN': self.token,
                                                       'User-Agent': 'esp-board-manager-ecosystem-config'})
        try:
            with self.opener.open(request, timeout=30) as response:
                data = response.read(MAX_FILE_BYTES + 1)
                headers = dict(response.headers.items()) if include_headers else None
        except HTTPError as error:
            raise ConfigError('Secure Files request failed (HTTP {})'.format(error.code)) from None
        except Exception:
            raise ConfigError('Secure Files request failed') from None
        if len(data) > MAX_FILE_BYTES:
            raise ConfigError('Secure Files response exceeds the size limit')
        return (data, headers) if include_headers else data


def _settings(environ):
    values = {}
    keys = ('CI_API_V4_URL', 'CI_PROJECT_ID', 'CI_JOB_TOKEN')
    file_keys = ('ECOSYSTEM_SOURCES_FILE_ID', 'ECOSYSTEM_REVIEW_FILE_ID')
    if any(environ.get(key) is not None for key in file_keys):
        keys += file_keys
    for key in keys:
        value = environ.get(key)
        if not isinstance(value, str) or not value.strip():
            raise ConfigError('Missing required variable: ' + key)
        if any(character.isspace() for character in value):
            raise ConfigError('Invalid required variable: ' + key)
        values[key] = value
    values['CI_API_V4_URL'] = _api_url(values['CI_API_V4_URL'])
    for key in ('CI_PROJECT_ID',) + tuple(key for key in file_keys if key in values):
        if not re.fullmatch(r'[1-9][0-9]*', values[key]):
            raise ConfigError('Invalid numeric identifier: ' + key)
    if file_keys[0] in values and values[file_keys[0]] == values[file_keys[1]]:
        raise ConfigError('The two configuration files must have different Secure File IDs')
    return values


def _pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('duplicate key')
        result[key] = value
    return result


def _invalid_constant(value):
    raise ValueError('non-JSON constant')


def _json(data):
    try:
        if not isinstance(data, bytes):
            raise ValueError('not bytes')
        return json.loads(data.decode('utf-8'), object_pairs_hook=_pairs, parse_constant=_invalid_constant)
    except (UnicodeError, ValueError, RecursionError):
        raise ConfigError('Secure Files configuration contains invalid JSON') from None


def _text(value):
    return isinstance(value, str) and bool(value.strip())


def _repository(value):
    return (_text(value) and re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', value)
            and all(part not in ('.', '..') for part in value.split('/')))


def _path(value):
    return (isinstance(value, str) and not value.startswith('/') and '\\' not in value
            and '..' not in PurePosixPath(value).parts and not any(character in value for character in '*?[]'))


def _source(value):
    return isinstance(value, dict) and _repository(value.get('repository')) and _path(value.get('path'))


def _selector(value):
    if not isinstance(value, dict) or not _text(value.get('chip')):
        return False
    if set(value) == {'repository', 'path', 'chip'}:
        return _source(value)
    return (set(value) == {'family', 'board', 'chip'} and _text(value['family'])
            and _text(value['board']) and _path(value['board']))


def _validate(sources, review):
    valid = isinstance(sources, dict) and isinstance(review, dict)
    if not valid:
        raise ConfigError('Incomplete ecosystem configuration')
    packs = sources.get('board_packs')
    valid = (isinstance(packs, list) and bool(packs)
             and all(_source(pack) and _text(pack.get('component')) for pack in packs)
             and sources.get('board_review_ledger') == REVIEW_NAME)
    for key in ('board_excluded_repositories', 'board_excluded_paths', 'excluded_repositories',
                'excluded_evidence_paths'):
        values = sources.get(key)
        valid = valid and isinstance(values, list) and all(_text(value) for value in values)
    lineages = sources.get('board_lineages')
    valid = valid and isinstance(lineages, list)
    if isinstance(lineages, list):
        for lineage in lineages:
            valid = (valid and isinstance(lineage, dict) and _text(lineage.get('family'))
                     and _repository(lineage.get('upstream')) and isinstance(lineage.get('roots'), list)
                     and bool(lineage['roots']) and all(isinstance(root, dict) and _path(root.get('path'))
                                                       for root in lineage['roots']))
    mappings = sources.get('board_review_matches')
    valid = valid and isinstance(mappings, list)
    if isinstance(mappings, list):
        valid = valid and all(_source(mapping) and isinstance(mapping.get('known_matches'), list)
                              and all(_source(target) for target in mapping['known_matches']) for mapping in mappings)
    entries = review.get('decisions')
    valid = (valid and type(review.get('schema_version')) is int and review['schema_version'] == 1
             and _text(review.get('reviewed_at')) and isinstance(entries, list))
    if isinstance(entries, list):
        for entry in entries:
            if not isinstance(entry, dict):
                valid = False
                continue
            selectors = entry.get('selectors')
            valid = (valid and _text(entry.get('id')) and _text(entry.get('reason'))
                     and entry.get('decision') in ('include', 'alias') and isinstance(selectors, list)
                     and bool(selectors) and all(_selector(selector) for selector in selectors))
            if entry.get('decision') == 'alias':
                valid = valid and _source(entry.get('target')) and 'distinct_from' not in entry
            else:
                distinct = entry.get('distinct_from', [])
                valid = valid and 'target' not in entry and isinstance(distinct, list) and all(_source(item) for item in distinct)
    if not valid:
        raise ConfigError('Incomplete or invalid ecosystem configuration')


def _request(client, path, include_headers=False):
    try:
        response = client.get(path, include_headers=True) if include_headers else client.get(path)
    except ConfigError:
        raise
    except HTTPError as error:
        raise ConfigError('Secure Files request failed (HTTP {})'.format(error.code)) from None
    except Exception:
        raise ConfigError('Secure Files request failed') from None
    if include_headers:
        if (not isinstance(response, tuple) or len(response) != 2 or not isinstance(response[1], dict)
                or any(not isinstance(key, str) or not isinstance(value, str) for key, value in response[1].items())):
            raise ConfigError('Invalid Secure Files list response headers')
        data, headers = response
    else:
        data = response
    if not isinstance(data, bytes) or len(data) > MAX_FILE_BYTES:
        raise ConfigError('Invalid Secure Files response size or type')
    return (data, headers) if include_headers else data


def _discover_ids(client, project):
    data, response_headers = _request(client, '/projects/{}/secure_files?per_page=100&page=1'.format(project),
                                      include_headers=True)
    files = _json(data)
    if not isinstance(files, list) or len(files) > 100:
        raise ConfigError('Invalid Secure Files list')
    headers = {key.lower(): value.strip() for key, value in response_headers.items()}
    if (headers.get('x-next-page') or ('x-page' in headers and headers['x-page'] != '1')
            or ('x-per-page' in headers and headers['x-per-page'] != '100')):
        raise ConfigError('Incomplete Secure Files pagination')
    for key in ('x-total', 'x-total-pages'):
        if key in headers and (not headers[key].isdigit()
                               or (key == 'x-total' and int(headers[key]) != len(files))
                               or (key == 'x-total-pages' and int(headers[key]) != (1 if files else 0))):
            raise ConfigError('Incomplete Secure Files pagination')
    for relation in re.finditer(r'\brel\s*=\s*(?:"([^"]*)"|\'([^\']*)\'|([^;,\s]+))', headers.get('link', ''), re.I):
        if 'next' in next(value for value in relation.groups() if value is not None).lower().split():
            raise ConfigError('Incomplete Secure Files pagination')
    if len(files) == 100 and not any(key in headers for key in ('x-total', 'x-total-pages', 'x-next-page')):
        raise ConfigError('Ambiguous full Secure Files page')
    identifiers = set()
    names = set()
    wanted = {}
    for metadata in files:
        if (not isinstance(metadata, dict) or type(metadata.get('id')) is not int or metadata['id'] <= 0
                or not _text(metadata.get('name')) or metadata['id'] in identifiers or metadata['name'] in names):
            raise ConfigError('Invalid or duplicate Secure Files list entry')
        identifiers.add(metadata['id'])
        names.add(metadata['name'])
        if metadata['name'] in (SOURCES_NAME, REVIEW_NAME):
            wanted[metadata['name']] = str(metadata['id'])
    if set(wanted) != {SOURCES_NAME, REVIEW_NAME}:
        raise ConfigError('Both named ecosystem configuration files must exist in Secure Files')
    return wanted


def _download(client, project, file_id, name):
    path = '/projects/{}/secure_files/{}'.format(project, file_id)
    metadata = _json(_request(client, path))
    if (not isinstance(metadata, dict) or type(metadata.get('id')) is not int
            or str(metadata['id']) != file_id or metadata.get('name') != name
            or metadata.get('checksum_algorithm') != 'sha256'
            or not isinstance(metadata.get('checksum'), str)
            or not re.fullmatch(r'[0-9a-fA-F]{64}', metadata['checksum'])):
        raise ConfigError('Invalid Secure Files metadata')
    content = _request(client, path + '/download')
    if not hmac.compare_digest(hashlib.sha256(content).hexdigest(), metadata['checksum'].lower()):
        raise ConfigError('Secure Files SHA256 verification failed')
    return content


def fetch_config(output, environ=None, client=None):
    """Verify both files before atomically creating a new private output directory."""
    settings = _settings(os.environ if environ is None else environ)
    output = Path(output).absolute()
    if output.exists() or output.is_symlink():
        raise ConfigError('Output configuration directory must not already exist')
    if client is None:
        client = SecureFilesClient(settings['CI_API_V4_URL'], settings['CI_JOB_TOKEN'])
    if 'ECOSYSTEM_SOURCES_FILE_ID' in settings:
        file_ids = {SOURCES_NAME: settings['ECOSYSTEM_SOURCES_FILE_ID'], REVIEW_NAME: settings['ECOSYSTEM_REVIEW_FILE_ID']}
    else:
        file_ids = _discover_ids(client, settings['CI_PROJECT_ID'])
    sources = _download(client, settings['CI_PROJECT_ID'], file_ids[SOURCES_NAME], SOURCES_NAME)
    review = _download(client, settings['CI_PROJECT_ID'], file_ids[REVIEW_NAME], REVIEW_NAME)
    _validate(_json(sources), _json(review))
    try:
        output.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='.ecosystem-config-', dir=str(output.parent)) as staging:
            for name, content in ((SOURCES_NAME, sources), (REVIEW_NAME, review)):
                with os.fdopen(os.open(str(Path(staging) / name), os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600), 'wb') as stream:
                    stream.write(content)
            Path(staging).replace(output)
    except OSError:
        raise ConfigError('Could not write private ecosystem configuration') from None
    return output


def main(argv=None, environ=None, client=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path, help='New directory for private configuration')
    arguments = parser.parse_args(argv)
    try:
        fetch_config(arguments.output, environ=environ, client=client)
    except ConfigError as error:
        print('Ecosystem configuration download failed: {}'.format(error), file=sys.stderr)
        return 1
    print('Ecosystem configuration downloaded and verified.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
