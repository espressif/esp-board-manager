# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""Store private ecosystem history in this GitLab project's Generic Registry.

Each job owns an immutable package version. Upload history.json last, so a
partially uploaded package cannot replace the last complete history baseline.
"""

from datetime import datetime, timezone
from email.utils import parsedate_to_datetime
import hashlib
import json
import os
import re
import time
from urllib.error import HTTPError
from urllib.parse import quote, urljoin, urlsplit
from urllib.request import HTTPRedirectHandler, Request, build_opener


MAX_FILE_BYTES = 32 * 1024 * 1024
MAX_PAGES = 100
MAX_RETRY_WAIT = 60
MAX_REDIRECTS = 3
RETRYABLE = (429, 500, 502, 503, 504)


class RegistryError(RuntimeError):
    """Sanitized failure safe to print without private inventories or tokens."""


class NoRedirect(HTTPRedirectHandler):
    def redirect_request(self, request, response, code, message, headers, new_url):
        return None


class RequestFailure(Exception):
    def __init__(self, status=None, headers=None, redirected=False):
        self.status = status
        self.headers = headers or {}
        self.redirected = redirected


def _url(value, name, allow_query=False):
    try:
        parsed = urlsplit(value)
        valid = (parsed.scheme == 'https' and parsed.hostname and parsed.port != 0
                 and not parsed.username and not parsed.password and (allow_query or not parsed.query)
                 and not parsed.fragment and not any(character.isspace() for character in value))
    except (TypeError, ValueError):
        valid = False
    if not valid:
        raise RegistryError('Invalid HTTPS URL variable: ' + name)
    return value.rstrip('/')


def _origin(value):
    parsed = urlsplit(value)
    return parsed.scheme, parsed.hostname, parsed.port or 443


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
        return json.loads(data.decode('utf-8'), object_pairs_hook=_pairs, parse_constant=_invalid_constant)
    except (UnicodeError, ValueError, RecursionError):
        raise RegistryError('Invalid JSON in private history registry') from None


class HistoryRegistry:
    def __init__(self, api_url, project, token, job, branch, project_url, job_url):
        self.api_url = api_url
        self.project = project
        self.token = token
        self.run = {'id': job, 'url': job_url, 'ref': branch}
        self.package_name = ('ecosystem-history' if branch == 'main' else
                             'ecosystem-history-test-' + hashlib.sha256(branch.encode('utf-8')).hexdigest()[:12])
        self.version = '1.0.' + job
        self.opener = build_opener(NoRedirect())
        self.sleep = time.sleep
        self.retry_wait = 0

    @classmethod
    def from_environment(cls, environ=None):
        environ = os.environ if environ is None else environ
        names = ('CI_API_V4_URL', 'CI_PROJECT_ID', 'CI_JOB_TOKEN', 'CI_JOB_ID',
                 'CI_COMMIT_BRANCH', 'CI_PROJECT_URL')
        settings = {}
        for name in names:
            value = environ.get(name)
            if (not isinstance(value, str) or not value
                    or any(character.isspace() for character in value)):
                raise RegistryError('Missing or invalid required variable: ' + name)
            settings[name] = value
        if environ.get('CI_PROJECT_VISIBILITY') != 'private':
            raise RegistryError('Private history requires CI_PROJECT_VISIBILITY=private')
        for name in ('CI_PROJECT_ID', 'CI_JOB_ID'):
            if not re.fullmatch(r'[1-9][0-9]*', settings[name]):
                raise RegistryError('Invalid numeric identifier: ' + name)
        api = _url(settings['CI_API_V4_URL'], 'CI_API_V4_URL')
        project_url = _url(settings['CI_PROJECT_URL'], 'CI_PROJECT_URL')
        job_url = _url(environ.get('CI_JOB_URL', project_url + '/-/jobs/' + settings['CI_JOB_ID']), 'CI_JOB_URL')
        if _origin(api) != _origin(project_url) or _origin(api) != _origin(job_url):
            raise RegistryError('History URLs must belong to the same GitLab origin')
        return cls(api, settings['CI_PROJECT_ID'], settings['CI_JOB_TOKEN'], settings['CI_JOB_ID'],
                   settings['CI_COMMIT_BRANCH'], project_url, job_url)

    def _file_url(self, name, version=None):
        return '{}/projects/{}/packages/generic/{}/{}/{}'.format(
            self.api_url, self.project, self.package_name, version or self.version, name)

    def _send(self, method, url, data=None):
        visited = set()
        for redirects in range(MAX_REDIRECTS + 1):
            if url in visited:
                raise RegistryError('Private history download contains a redirect loop')
            visited.add(url)
            headers = {'User-Agent': 'esp-board-manager-ecosystem-history'}
            if _origin(url) == _origin(self.api_url):
                headers['JOB-TOKEN'] = self.token
            if data is not None:
                headers['Content-Type'] = 'application/octet-stream'
            request = Request(url, data=data, headers=headers, method=method)
            try:
                with self.opener.open(request, timeout=30) as response:
                    content = response.read(MAX_FILE_BYTES + 1)
                    status = response.status
                    response_headers = {key.lower(): value for key, value in response.headers.items()}
            except HTTPError as error:
                status = error.code
                response_headers = {key.lower(): value for key, value in (error.headers or {}).items()}
                error.close()
                content = b''
            except Exception:
                raise RequestFailure(redirected=redirects > 0) from None
            if method == 'GET' and status in (301, 302, 303, 307, 308):
                if redirects == MAX_REDIRECTS:
                    raise RegistryError('Private history download exceeded its redirect limit')
                location = response_headers.get('location')
                if not isinstance(location, str) or not location or any(character.isspace() for character in location):
                    raise RegistryError('Invalid private history download redirect')
                try:
                    destination = urljoin(url, location)
                    _url(destination, 'history download redirect', allow_query=True)
                except (RegistryError, TypeError, ValueError):
                    raise RegistryError('Unsafe private history download redirect') from None
                url = destination
                continue
            if len(content) > MAX_FILE_BYTES:
                raise RegistryError('Private history registry response exceeds the size limit')
            if status != (201 if method == 'PUT' else 200):
                raise RequestFailure(status, response_headers, redirected=redirects > 0)
            return content, response_headers

    def _wait(self, failure, attempt):
        delay = 2 ** (attempt + 1)
        retry_after = failure.headers.get('retry-after')
        if retry_after:
            try:
                seconds = int(retry_after)
            except (TypeError, ValueError):
                try:
                    date = parsedate_to_datetime(retry_after)
                    if date.tzinfo is None:
                        date = date.replace(tzinfo=timezone.utc)
                    seconds = max(0, (date - datetime.now(timezone.utc)).total_seconds())
                except (TypeError, ValueError, OverflowError):
                    seconds = 0
            delay = max(delay, seconds)
        if self.retry_wait + delay > MAX_RETRY_WAIT:
            raise RegistryError('Private history registry retry wait budget exhausted')
        self.retry_wait += delay
        self.sleep(delay)

    def _failure(self, failure):
        detail = 'HTTP {}'.format(failure.status) if failure.status is not None else 'transport failure'
        return RegistryError('Private history registry request failed ({})'.format(detail))

    def _get(self, url, missing=False, include_headers=False):
        for attempt in range(3):
            try:
                content, headers = self._send('GET', url)
                return (content, headers) if include_headers else content
            except RequestFailure as failure:
                if missing and failure.status == 404 and not failure.redirected:
                    return None
                if failure.status not in RETRYABLE + (None,) or attempt == 2:
                    raise self._failure(failure) from None
                self._wait(failure, attempt)

    def _put(self, url, content):
        for attempt in range(3):
            try:
                self._send('PUT', url, content)
                return
            except RequestFailure as failure:
                if failure.status not in RETRYABLE + (None,):
                    raise self._failure(failure) from None
                recovered = self._get(url, missing=True)
                if recovered is not None:
                    if recovered != content:
                        raise RegistryError('Immutable history package has different content')
                    return
                if attempt == 2:
                    raise self._failure(failure) from None
                self._wait(failure, attempt)

    def load(self):
        seen = set()
        for page in range(1, MAX_PAGES + 1):
            url = ('{}/projects/{}/packages?package_type=generic&package_name={}'
                   '&order_by=created_at&sort=desc&per_page=100&page={}').format(
                       self.api_url, self.project, quote(self.package_name, safe=''), page)
            data, headers = self._get(url, include_headers=True)
            packages = _json(data)
            if not isinstance(packages, list) or len(packages) > 100:
                raise RegistryError('Invalid private history package list')
            for package in packages:
                if (not isinstance(package, dict) or type(package.get('id')) is not int
                        or package['id'] <= 0 or not isinstance(package.get('name'), str)):
                    raise RegistryError('Invalid private history package metadata')
                if package['id'] in seen:
                    raise RegistryError('Duplicate private history package across list pages')
                seen.add(package['id'])
                if package['name'] != self.package_name:
                    continue
                version = package.get('version')
                if package.get('package_type') != 'generic' or not isinstance(version, str) or not re.fullmatch(r'1\.0\.[1-9][0-9]*', version):
                    raise RegistryError('Invalid private history package version')
                content = self._get(self._file_url('history.json', version), missing=True)
                if content is None:
                    continue
                history = _json(content)
                if not isinstance(history, dict):
                    raise RegistryError('Invalid private history document')
                return history
            next_page = headers.get('x-next-page')
            if next_page is not None:
                if next_page == '':
                    return None
                if next_page != str(page + 1):
                    raise RegistryError('Invalid private history pagination')
            elif len(packages) < 100:
                return None
        raise RegistryError('Private history package listing exceeded its page limit')

    def save(self, history, markdown, changes):
        try:
            if (not isinstance(history, dict)
                    or any(not isinstance(value, str) or not value.strip() for value in (markdown, changes))):
                raise ValueError('invalid history')
            data = (json.dumps(history, ensure_ascii=False, sort_keys=True, indent=2, allow_nan=False) + '\n').encode('utf-8')
            files = [('README.md', markdown.encode('utf-8')), ('changes.md', changes.encode('utf-8')),
                     ('history.json', data)]
        except (ValueError, TypeError, UnicodeError, RecursionError):
            raise RegistryError('Invalid private history upload') from None
        if any(len(content) > MAX_FILE_BYTES for name, content in files):
            raise RegistryError('Private history upload exceeds the size limit')
        previous = {name: self._get(self._file_url(name), missing=True)
                    for name in ('history.json', 'README.md', 'changes.md')}
        if previous['history.json'] is not None:
            if any(previous[name] != content for name, content in files):
                raise RegistryError('Immutable history package has different or missing content')
            return
        if any(previous[name] is not None and previous[name] != content for name, content in files):
            raise RegistryError('Incomplete history package has different content')
        for name, content in files:
            if previous[name] is None:
                self._put(self._file_url(name), content)
