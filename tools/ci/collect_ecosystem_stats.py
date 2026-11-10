#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""Collect public board definitions and verified ESP Board Manager dependencies."""

import argparse
import base64
import json
import math
import os
import re
import subprocess
import sys
import tempfile
import time
from datetime import datetime, timezone
from fnmatch import fnmatchcase
from html import escape
from http.client import IncompleteRead
from pathlib import Path, PurePosixPath
from urllib.error import HTTPError, URLError
from urllib.parse import quote, urlencode
from urllib.request import Request, urlopen

import yaml

from discover_board_stats import CollectionError, discover_boards
from review_board_stats import review_outside_boards
from board_stats_decisions import apply_board_decisions
from public_ecosystem_stats import badge


class _SearchConsistencyError(CollectionError):
    """A search attempt that must be discarded and restarted from page one."""
    pass


class _SearchPartitionRequired(Exception):
    """Discard a broad response and query disjoint file-size ranges instead."""
    pass


class GitHubClient:
    MAX_RATE_LIMIT_WAIT = 300

    def __init__(self, token=None):
        self.token = token
        self.last_search = 0.0
        self.rate_limit_wait = 0.0

    def _before_search_request(self):
        time.sleep(max(0, 6.1 - (time.monotonic() - self.last_search)))
        self.last_search = time.monotonic()

    def _http_failure(self, error, attempt):
        """Classify bounded error data without logging arbitrary server text."""
        headers = error.headers or {}
        numbers = {}
        for name in ('Limit', 'Remaining', 'Used', 'Reset'):
            value = headers.get('X-RateLimit-' + name, '')
            if isinstance(value, str) and re.fullmatch(r'[0-9]{1,20}', value):
                numbers[name.lower()] = int(value)
        retry_after = headers.get('Retry-After', '')
        retry_after = (int(retry_after) if isinstance(retry_after, str)
                       and re.fullmatch(r'[0-9]{1,20}', retry_after) else None)
        message = ''
        if error.code in (401, 403, 429):
            try:
                body = json.loads(error.read(8192))
                if isinstance(body, dict) and isinstance(body.get('message'), str):
                    message = body['message'].lower()
            except (OSError, ValueError, IncompleteRead, RecursionError):
                pass
        secondary = 'you have exceeded a secondary rate limit' in message
        primary = 'api rate limit exceeded' in message or numbers.get('remaining') == 0
        permission = 'resource not accessible' in message
        authentication = 'bad credentials' in message
        limited = (error.code in (403, 429) and not (permission or authentication)
                   and (secondary or primary or retry_after is not None))
        reason, delay = 'unclassified_http', None
        if limited:
            reason = 'secondary_rate_limit' if secondary else 'primary_rate_limit' if primary else 'rate_limit'
            delays = []
            if retry_after is not None:
                delays.append(max(1, retry_after))
            if numbers.get('remaining') == 0 and 'reset' in numbers:
                reset_delay = numbers['reset'] - time.time()
                if reset_delay > 0:
                    delays.append(reset_delay + 1)
            delay = max(delays) if delays else 60 * 2 ** (attempt - 1)
        elif permission:
            reason = 'permission_denied'
        elif authentication:
            reason = 'authentication_failed'
        details = ['reason=' + reason]
        details.extend('{}={}'.format(name, value) for name, value in numbers.items())
        if retry_after is not None:
            details.append('retry_after={}'.format(retry_after))
        resource = headers.get('X-RateLimit-Resource')
        if resource in ('core', 'search', 'code_search'):
            details.append('resource=' + resource)
        request_id = headers.get('X-GitHub-Request-Id', '')
        if (isinstance(request_id, str) and re.fullmatch(r'[A-Za-z0-9:.-]{1,128}', request_id)
                and not (self.token and self.token in request_id)):
            details.append('request_id=' + request_id)
        return ', '.join(details), delay

    def get(self, path, search=False):
        headers = {'Accept': 'application/vnd.github+json',
                   'User-Agent': 'esp-board-manager-ecosystem-stats',
                   'X-GitHub-Api-Version': '2022-11-28'}
        if self.token:
            headers['Authorization'] = 'Bearer ' + self.token
        attempts = 3
        for attempt in range(1, attempts + 1):
            if search:
                self._before_search_request()
            try:
                with urlopen(Request('https://api.github.com' + path, headers=headers), timeout=30) as response:
                    return json.load(response)
            except HTTPError as error:
                with error:
                    if error.code == 404:
                        return None
                    details, delay = self._http_failure(error, attempt)
                message = 'GitHub HTTP {} for {} attempt={}/{} ({})'.format(
                    error.code, path, attempt, attempts, details)
                if delay is None:
                    raise CollectionError(message) from None
                if attempt == attempts:
                    raise CollectionError(message + '; request retries exhausted') from None
                if self.rate_limit_wait + delay > self.MAX_RATE_LIMIT_WAIT:
                    raise CollectionError(message + '; rate-limit wait budget exhausted ({}s)'.format(
                        self.MAX_RATE_LIMIT_WAIT)) from None
                self.rate_limit_wait += delay
                print(message + '; retrying in {:.1f}s'.format(delay), file=sys.stderr, flush=True)
                time.sleep(delay)
            except (URLError, TimeoutError, ConnectionError, IncompleteRead) as error:
                # Retry this GET only; search consistency is validated after the response arrives.
                message = 'GitHub request path={} attempt={}/{} failed ({})'.format(
                    json.dumps(path, ensure_ascii=True), attempt, attempts, type(error).__name__)
                if attempt == attempts:
                    raise CollectionError(message + '; transport retries exhausted') from None
                delay = 2 ** attempt
                print(message + '; retrying in {}s'.format(delay), file=sys.stderr, flush=True)
                time.sleep(delay)
            except ValueError as error:
                raise CollectionError('GitHub request failed for {} ({})'.format(path, type(error).__name__)) from None

    def repository(self, repo):
        return self.get('/repos/' + repo)

    def default_revision(self, repo, branch):
        commit = self.get('/repos/{}/commits/{}'.format(repo, quote(branch, safe='')))
        if not commit:
            raise CollectionError('Default branch disappeared during collection: ' + repo)
        return commit['sha']

    def search(self, filename):
        return self.search_query('esp_board_manager filename:{} in:file'.format(filename))

    def search_query(self, search_query):
        attempts = 3
        for attempt in range(1, attempts + 1):
            try:
                return self._search_query_once(search_query)
            except _SearchConsistencyError as error:
                message = 'GitHub search query={} attempt={}/{}: {}'.format(
                    json.dumps(search_query, ensure_ascii=True), attempt, attempts, error)
                if attempt == attempts:
                    raise CollectionError(message + '; no complete search obtained') from None
                print(message + '; discarding this attempt and restarting from page 1',
                      file=sys.stderr, flush=True)

    def _search_query_once(self, search_query, split_if_many=False):
        items = []
        seen = set()
        total = None
        page = 1
        while total is None or page <= math.ceil(total / 100):
            query = urlencode({'q': search_query,
                               'per_page': 100, 'page': page})
            response = self.get('/search/code?' + query, search=True)
            if (not isinstance(response, dict) or type(response.get('total_count')) is not int
                    or response['total_count'] < 0 or type(response.get('incomplete_results')) is not bool
                    or not isinstance(response.get('items'), list) or len(response['items']) > 100):
                raise CollectionError('Invalid GitHub search response for {} at page {}'.format(
                    json.dumps(search_query, ensure_ascii=True), page))
            if len(response['items']) > response['total_count']:
                raise CollectionError('Invalid GitHub search result count for {} at page {}'.format(
                    json.dumps(search_query, ensure_ascii=True), page))
            for item in response['items']:
                if (not isinstance(item, dict) or not isinstance(item.get('repository'), dict)
                        or not isinstance(item['repository'].get('full_name'), str)
                        or not item['repository']['full_name'] or not isinstance(item.get('path'), str)
                        or not item['path']):
                    raise CollectionError('Invalid GitHub search item for {} at page {}'.format(
                        json.dumps(search_query, ensure_ascii=True), page))
            if split_if_many and response['total_count'] > 100:
                raise _SearchPartitionRequired(response['total_count'])
            if response['total_count'] > 1000:
                raise CollectionError('GitHub search exceeds 1,000 results; split queries before publishing')
            if response['incomplete_results']:
                raise _SearchConsistencyError('page {}: incomplete_results=true, total_count={}'.format(
                    page, response['total_count']))
            if total is not None and response['total_count'] != total:
                raise _SearchConsistencyError('page {}: total_count changed from {} to {}'.format(
                    page, total, response['total_count']))
            total = response['total_count']
            for item in response['items']:
                key = (item['repository']['full_name'].lower(), item['path'])
                if key in seen:
                    raise _SearchConsistencyError('page {}: duplicate result, total_count={}'.format(page, total))
                seen.add(key)
            items.extend(response['items'])
            if len(items) > total:
                raise CollectionError('Invalid GitHub search result count for {} at page {}: '
                                      'received {} results, total_count={}'.format(
                                          json.dumps(search_query, ensure_ascii=True), page, len(items), total))
            page += 1
        if len(items) != total:
            raise _SearchConsistencyError('page {}: received {} results, total_count={}'.format(
                page - 1, len(items), total))
        return items

    def tree(self, repo, revision):
        response = self.get('/repos/{}/git/trees/{}?recursive=1'.format(repo, revision))
        if not response or response.get('truncated') is not False:
            raise CollectionError('Incomplete repository tree: ' + repo)
        entries = response['tree']
        if response.get('sha'):
            entries = entries + [{'path': '', 'type': 'tree', 'mode': '040000', 'sha': response['sha']}]
        return entries

    def content(self, repo, path, ref):
        response = self.get('/repos/{}/contents/{}?{}'.format(repo, quote(path, safe='/'), urlencode({'ref': ref})))
        if response is None:  # An indexed file may have been deleted from the default branch.
            return None, None
        if response.get('encoding') != 'base64' or response.get('type') != 'file':
            raise CollectionError('Unsupported GitHub file: {}/{}'.format(repo, path))
        try:
            return base64.b64decode(response['content']).decode('utf-8'), response['sha']
        except (ValueError, UnicodeError) as error:
            raise CollectionError('Invalid file encoding: {}/{}'.format(repo, path)) from error


class PartitionedGitHubClient(GitHubClient):
    """Collect disjoint size ranges and verify every leaf with a second observation."""

    SEARCH_REQUEST_LIMIT = 120

    def __init__(self, token=None):
        super().__init__(token)
        self.search_requests = 0

    def _before_search_request(self):
        if self.search_requests >= self.SEARCH_REQUEST_LIMIT:
            raise CollectionError('GitHub search request budget exhausted; no complete search obtained')
        self.search_requests += 1
        super()._before_search_request()

    @staticmethod
    def _signature(items):
        signature = {}
        for item in items:
            if not isinstance(item.get('sha'), str) or not item['sha']:
                raise CollectionError('GitHub search item is missing its blob SHA')
            signature[(item['repository']['full_name'].lower(), item['path'])] = item['sha']
        return signature

    def search_query(self, search_query):
        attempts = 3
        for attempt in range(1, attempts + 1):
            try:
                return self._partitioned_search_once(search_query)
            except _SearchConsistencyError as error:
                message = 'GitHub partitioned search query={} attempt={}/{}: {}'.format(
                    json.dumps(search_query, ensure_ascii=True), attempt, attempts, error)
                if attempt == attempts:
                    raise CollectionError(message + '; no complete search obtained') from None
                print(message + '; discarding all partitions and restarting', file=sys.stderr, flush=True)

    def _partitioned_search_once(self, search_query):
        pending = [(0, None)]
        leaves, collected, seen = [], [], set()
        while pending:
            low, high = pending.pop()
            query = search_query
            if high is not None:
                query += ' size:{}..{}'.format(low, high)
            elif low:
                query += ' size:>={}'.format(low)
            atomic = low == high
            try:
                items = super().search_query(query) if atomic else self._search_query_once(query, split_if_many=True)
            except _SearchPartitionRequired as error:
                middle = max(1023, low * 2 - 1) if high is None else (low + high) // 2
                print('GitHub search query={} results={}; splitting file-size range at {}'.format(
                    json.dumps(query, ensure_ascii=True), error.args[0], middle), file=sys.stderr, flush=True)
                pending.extend([(middle + 1, high), (low, middle)])
                continue
            signature = self._signature(items)
            if seen.intersection(signature):
                raise _SearchConsistencyError('duplicate file across size partitions')
            seen.update(signature)
            collected.extend(items)
            leaves.append((query, atomic, signature))

        # Revisit the same leaves, including empty ones, without changing their boundaries.
        for query, atomic, expected in leaves:
            try:
                items = super().search_query(query) if atomic else self._search_query_once(query, split_if_many=True)
            except _SearchPartitionRequired:
                raise _SearchConsistencyError('size partition grew during verification: ' + query) from None
            if self._signature(items) != expected:
                raise _SearchConsistencyError('size partition changed during verification: ' + query)
        print('GitHub search query={}: verified {} files across {} size partitions ({} search requests used)'.format(
            json.dumps(search_query, ensure_ascii=True), len(collected), len(leaves), self.search_requests),
            file=sys.stderr, flush=True)
        return collected


def board_paths(entries, prefix):
    root = PurePosixPath(prefix)
    boards = []
    for entry in entries:
        if entry['type'] != 'blob' or entry.get('mode') == '120000':
            continue
        path = PurePosixPath(entry['path'])
        if path.name == 'board_info.yaml' and path.parent.parent == root:
            boards.append(path.parent.name)
    return sorted(set(boards))


def repository_tree(repo):
    """Read committed paths through Git without fetching board images or code blobs."""
    if not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repo):
        raise CollectionError('Invalid board repository: ' + repo)
    environment = dict(os.environ, GIT_TERMINAL_PROMPT='0')
    try:
        with tempfile.TemporaryDirectory(prefix='bmgr-board-tree-') as temporary:
            def git(*arguments):
                result = subprocess.run(['git', *arguments], capture_output=True, check=True,
                                        timeout=120, env=environment)
                return result.stdout.decode('utf-8')
            git('clone', '--quiet', '--bare', '--depth=1', '--filter=blob:none', '--single-branch',
                'https://github.com/{}.git'.format(repo), temporary)
            revision = git('-C', temporary, 'rev-parse', 'HEAD').strip()
            tree = git('-C', temporary, 'ls-tree', '-r', '-z', 'HEAD')
            entries = []
            for item in tree.split('\0'):
                if item:
                    header, path = item.split('\t', 1)
                    mode, kind, _ = header.split(' ')
                    entries.append({'path': path, 'type': kind, 'mode': mode})
            return revision, entries
    except (OSError, subprocess.SubprocessError, UnicodeError) as error:
        raise CollectionError('Could not read board repository {} ({})'.format(repo, type(error).__name__)) from None


def collect_boards(packs):
    repositories = {}
    result = []
    for pack in packs:
        repo = pack['repository']
        if repo not in repositories:
            repositories[repo] = repository_tree(repo)
        revision, entries = repositories[repo]
        boards = board_paths(entries, pack['path'])
        if not boards:
            raise CollectionError('No board definitions found for ' + pack['component'])
        result.append(dict(pack, revision=revision, count=len(boards), boards=boards, listed=True))
    return {'count': sum(pack['count'] for pack in result), 'packs': result}


def dependency_kind(content, filename):
    if content is None:
        return None
    try:
        document = yaml.safe_load(content)
    except yaml.YAMLError as error:
        raise CollectionError('Invalid dependency YAML') from error
    if not isinstance(document, dict) or not isinstance(document.get('dependencies'), dict):
        return None
    dependencies = document['dependencies']
    if not any(name in dependencies for name in ('espressif/esp_board_manager', 'esp_board_manager')):
        return None
    return 'direct' if filename == 'idf_component.yml' else 'resolved'


def is_project_evidence(path, excluded_paths):
    return not any(fnmatchcase('/' + path, pattern) for pattern in excluded_paths)


def collect_projects(client, excluded, excluded_paths=()):
    excluded = {repo.lower() for repo in excluded}
    metadata = {}
    revisions = {}
    projects = {}
    seen_files = set()
    for filename in ('idf_component.yml', 'dependencies.lock'):
        for item in client.search(filename):
            repo = item['repository']['full_name']
            key = repo.lower()
            path = item['path']
            if key in excluded or PurePosixPath(path).name != filename or not is_project_evidence(path, excluded_paths):
                continue
            if key not in metadata:
                metadata[key] = client.repository(repo)
            info = metadata[key]
            if not info or info.get('private') or info.get('fork') or info.get('is_template'):
                continue
            repo = info['full_name']
            if repo.lower() in excluded:
                continue
            metadata[repo.lower()] = info
            key = info['id']
            if (key, path) in seen_files:
                continue
            seen_files.add((key, path))
            if key not in revisions:
                revisions[key] = client.default_revision(repo, info['default_branch'])
            revision = revisions[key]
            content, blob_sha = client.content(repo, path, revision)
            try:
                kind = dependency_kind(content, filename)
            except CollectionError:
                raise CollectionError('Invalid dependency YAML: {}/{}'.format(repo, path)) from None
            if kind:
                project = projects.setdefault(key, {'repository': repo, 'url': 'https://github.com/' + repo,
                                                    'revision': revision, 'evidence': []})
                project['evidence'].append({'path': path, 'kind': kind, 'blob_sha': blob_sha,
                                            'url': 'https://github.com/{}/blob/{}/{}'.format(
                                                repo, revision, quote(path, safe='/'))})
    return sorted(projects.values(), key=lambda project: project['repository'].lower())


def markdown_label(value):
    return re.sub(r'([\\\[\]`*_!|])', r'\\\1', escape(' '.join(str(value).splitlines())))


def write_report(snapshot, output):
    # Keep caller-owned snapshots intact when adding the reproducible review.
    snapshot = dict(snapshot, boards=dict(snapshot['boards']))
    boards = snapshot['boards']
    filters = snapshot.get('board_filters', {})
    review = review_outside_boards(boards, filters.get('board_lineages', []), filters.get('board_review_matches', []))
    boards['outside_review'] = review
    summary = None
    if 'board_decisions' in filters:
        summary = apply_board_decisions(boards, review, filters['board_decisions'])
        boards['decision_summary'] = summary
        boards['approved_count'] = summary['approved_count']
    projects = snapshot['projects']
    lines = ['# ESP Board Manager ecosystem / 生态统计', '',
             'Source scan / 来源扫描时间: **{}**'.format(snapshot['updated_at']), '']
    if snapshot.get('reprocessed_at'):
        lines += ['Rescreened from the same pinned revisions / 基于同一批固定版本重新筛选: **{}**'.format(
            snapshot['reprocessed_at']), '']
    if summary is not None:
        lines += ['Supported boards (reviewed) / 支持板卡（已审阅）: **{}**'.format(boards['approved_count']), '',
                  'Known components / 已知组件: **{}**; approved additional models / 确认保留的范围外型号: **{}**; '
                  'aliases / 已归并来源: **{}**; pending / 待审候选: **{}**.'.format(
                      summary['known_count'], summary['included_count'], summary['aliased_count'], summary['pending_count']), '',
                  'Review ledger / 人工审阅记录: **{}**. New unreviewed candidates do not increase this count. '
                  '/ 新发现但未经审阅的候选不增加上述数量。'.format(filters['board_decisions'].get('reviewed_at', '')), '']
    lines += ['Observed definitions before manual decisions / 人工审阅前保留的板级定义: **{}**'.format(boards['count']), '']
    if 'search_query' in boards:
        lines += ['Configured sources are seeds. GitHub code search discovers additional public repositories; '
                  'each candidate must contain board_info.yaml, board_devices.yaml and board_peripherals.yaml '
                  'in one directory, with a chip declared in board_info.yaml. Forks, templates, caches and '
                  'configured fixtures are excluded. Identical complete directory Git trees count once. '
                  'Within configured upstream families, the same vendor/model and chip count once across '
                  'historical versions and recreated repositories; genuinely new identities remain eligible. '
                  'This measures observed definitions, not unique physical models or hardware validation.', '',
                  '配置来源作为种子，同时通过 GitHub 代码搜索发现其他公开仓库。候选目录必须同时包含三个板级 YAML，'
                  '且 board_info.yaml 声明 chip；排除 fork、模板、缓存和配置列出的测试夹具。'
                  '完整目录 Git tree 相同的副本合并；在已配置的上游家族内，同一厂商/板目录及芯片的历史版本、'
                  '重建仓库副本只计一次，真正新增的板卡身份继续保留。'
                  '这不是唯一物理型号数，也不表示已完成硬件验证。', '',
                  'Definitions in listed sources / 已列来源中的定义: **{}**; retained outside-source definitions '
                  '/ 范围外保留定义: **{}**; '
                  'identical copies merged / 合并相同副本: **{}**; '
                  'upstream tree variants merged / 合并上游内容变体: **{}**.'.format(
                      boards['listed_count'], boards['discovered_count'],
                      boards.get('exact_duplicates_removed', boards['duplicates_removed']),
                      boards.get('lineage_variants_merged', 0)), '',
                  'Search / 搜索: `{}` ({} indexed matches / 条索引匹配). '
                  'All candidate board directories in each discovered repository are checked at a pinned commit. '
                  'GitHub indexing limits apply; this is not an exhaustive census.'.format(
                      boards['search_query'], boards['search_matches']), '']
    else:
        lines += ['Listed-source preview only; GitHub discovery was not run. / 仅已列来源预览，未执行 GitHub 发现。', '']
    lines += ['## Known components / 已知组件', '',
              'Only the configured board components belong to this baseline. '
              '/ 仅配置中明确列出的板卡组件属于已知范围。', '',
              '| Source / 来源 | Count after merging / 去重后 | Before merging / 去重前 | Source revision / 源版本 |',
              '| --- | ---: | ---: | --- |']
    for pack in snapshot['boards']['packs']:
        if not pack.get('listed'):
            continue
        source = 'https://github.com/{}/tree/{}/{}'.format(pack['repository'], pack['revision'], quote(pack['path'], safe='/'))
        label = '{} / {}'.format(pack['component'], pack['path'])
        lines.append('| {} | {} | {} | [{}]({}) |'.format(
            markdown_label(label), pack['count'], pack.get('raw_count', pack['count']), pack['revision'][:12], source))
    lines += ['', '## Outside-known candidates / 已知组件范围外候选', '',
              'Outside definitions / 范围外定义: **{}**; possible known-model matches / 可能对应已知型号: **{}**; '
              'no known-model match found / 未找到已知型号匹配: **{}**.'.format(
                  review['candidate_count'], review['possible_known_count'], review['no_known_match_count']), '',
              'Source location outside the listed components does not establish a new hardware model. '
              'Model-name and YAML matches are review hints; they do not silently merge definitions or change the total. '
              '/ 来源目录在已知组件之外不代表新增硬件型号。名称和 YAML 匹配是扫描提示；最终数量采用人工审阅决定。', '']
    groups = (
            ('possible_known_model', '### Possible known-model matches / 可能对应已知型号'),
            ('no_known_match', '### No known-model match found / 未找到已知型号匹配'))
    candidates_to_show = review['candidates']
    key = 'status'
    if summary is not None:
        key = 'decision'
        candidates_to_show = summary['candidates']
        groups = (('include', '### Approved additional boards / 已确认的范围外板卡'),
                  ('alias', '### Confirmed aliases / 已确认的同型号来源'),
                  ('pending', '### Pending review / 待审候选'))
    for status, heading in groups:
        candidates = [candidate for candidate in candidates_to_show if candidate[key] == status]
        lines += [heading, '',
                  '| ID | Board / 板卡 | Outside source / 范围外来源 | Known match and reason / 已知对应与依据 | Decision / 筛选结论 |',
                  '| --- | --- | --- | --- | --- |']
        for candidate in candidates:
            origin = candidate['primary']
            source = '[{} / {}]({})'.format(markdown_label(origin['repository']), markdown_label(origin['path']), origin['url'])
            info = origin.get('info', {})
            description = ' · '.join(str(info[key]) for key in ('manufacturer', 'description') if info.get(key))
            if description:
                source += '<br>' + markdown_label(description)
            matches = '<br>'.join('[{}]({}): {}'.format(
                markdown_label(match['board']), match['url'], markdown_label(match['reason'])) for match in candidate['known_matches'])
            if summary is not None:
                target = candidate.get('target')
                matches = markdown_label(candidate['decision_reason'])
                if target:
                    matches = '[{}]({})<br>{}'.format(markdown_label(target['board']), target['url'], matches)
            if not matches:
                matches = '未找到匹配；不代表确认新增 / No match; not confirmed new'
            if candidate.get('note'):
                matches += '<br>Prior matching notes / 原匹配提示: ' + markdown_label(candidate['note'])
            if candidate.get('evidence'):
                matches += '<br>' + ' · '.join('[evidence {}]({})'.format(index + 1, url)
                                               for index, url in enumerate(candidate['evidence']))
            decision = '待你过滤 / Pending' if summary is None else {
                'include': '已确认保留 / Included', 'alias': '已归并 / Alias', 'pending': '待审核 / Pending'}[candidate['decision']]
            lines.append('| {} | {}<br>{} | {} | {} | {} |'.format(
                candidate['id'], markdown_label(candidate['board']), markdown_label(candidate['chip']), source, matches, decision))
        if not candidates:
            lines += ['| — | — | — | None / 无 | — |']
        lines.append('')
    lines += ['### Outside source summary / 范围外来源汇总', '',
              '| Outside source / 范围外来源 | Candidates / 待复核定义 |', '| --- | ---: |']
    for pack in boards['packs']:
        if pack.get('listed') or not pack['count']:
            continue
        source = 'https://github.com/{}/tree/{}/{}'.format(pack['repository'], pack['revision'], quote(pack['path'], safe='/'))
        lines.append('| [{} / {}]({}) | {} |'.format(
            markdown_label(pack['repository']), markdown_label(pack['path']), source, pack['count']))
    lines.append('')
    definitions = [definition for pack in boards['packs'] for definition in pack.get('definitions', [])]
    if definitions:
        lines += ['', '## Board definition evidence / 板级定义依据', '',
                  '| Board directory / 板目录 | Chip | Primary source at immutable commit / 固定版本主来源 |', '| --- | --- | --- |']
        for definition in definitions:
            origins = ' · '.join('[{} / {}]({})'.format(
                markdown_label(origin['repository']), markdown_label(origin['path']), origin['url'])
                                 for origin in definition['origins'][:1])
            lines.append('| {} | {} | {} |'.format(
                markdown_label(definition['board']), markdown_label(definition['chip']), origins))
    inherited = [definition for definition in definitions if len(definition['origins']) > 1]
    if review['merged_known_origins']:
        lines += ['', '## Outside copies already merged into known components / 已合并到已知组件的范围外副本', '',
                  'These source directories are outside the baseline, but already add zero candidates. '
                  '/ 以下目录来源在已知范围外，已合并到已知定义，不占上面的候选名额。', '',
                  '| Known board / 已知板卡 | Outside source / 范围外来源 | Basis / 依据 |', '| --- | --- | --- |']
        for item in review['merged_known_origins']:
            origin = item['origin']
            lines.append('| [{}]({}) | [{} / {}]({}) | {} |'.format(
                markdown_label(item['known']['board']), item['known']['url'], markdown_label(origin['repository']),
                markdown_label(origin['path']), origin['url'], markdown_label(item['reason'])))
    if inherited:
        lines += ['', '## Merged copies and upstream variants / 已合并的副本与上游变体', '',
                  'These origins do not add boards. Content differences are retained as evidence; '
                  'a matching upstream identity does not imply byte-identical configuration. '
                  '/ 以下来源不增加计数；内容差异保留供核查，同一上游身份不表示配置文件完全相同。', '',
                  '| Board / 板卡 | Source / 来源 | Directory tree / 目录版本 |', '| --- | --- | --- |']
        for definition in inherited:
            for origin in definition['origins'][1:]:
                lines.append('| {} | [{} / {}]({}) | `{}` |'.format(
                    markdown_label(definition['board']), markdown_label(origin['repository']),
                    markdown_label(origin['path']), origin['url'], origin.get('tree_sha', definition['tree_sha'])[:12]))
    lines += ['', '## Projects using Board Manager / 使用 Board Manager 的项目', '']
    if projects is None:
        lines += ['Not scanned (boards-only preview). / 尚未扫描（仅板卡试跑）。']
    else:
        lines += ['Repositories / 仓库数: **{}**'.format(projects['count']), '',
                  'Public default-branch dependency declarations discovered by GitHub code search. '
                  'Each repository counts once; forks, templates and configured exclusions are omitted. '
                  'Dependency caches and configured vendored code paths are excluded. '
                  'Lock files can include transitive dependencies. This is not an exhaustive GitHub census.', '',
                  '按 GitHub 搜索发现的公开默认分支依赖声明核验，每个仓库计一次；排除 fork、模板和配置中的仓库。'
                  '依赖缓存和配置列出的内嵌上游代码不作为工程采用证据。'
                  '锁文件可能包含传递依赖。此数值不代表 GitHub 上全部使用者。', '',
                  '| Repository / 仓库 | Evidence / 依赖声明 |', '| --- | --- |']
        for project in projects['repositories']:
            evidence = ' · '.join('[{} ({})]({})'.format(
                markdown_label(item['path']), item['kind'], item['url'])
                                  for item in project['evidence'])
            lines.append('| [{}]({}) | {} |'.format(project['repository'], project['url'], evidence))
    project_value = str(projects['count']) if projects is not None else 'not scanned'
    files = {'stats.json': json.dumps(snapshot, ensure_ascii=False, indent=2) + '\n',
             'supported-boards.svg': badge('Supported boards' if summary is not None else 'board definitions found',
                                           str(boards.get('approved_count', boards['count'])), '#007ec6'),
             'dependent-projects.svg': badge('Projects using Board Manager', project_value, '#4c1' if projects is not None else '#9f9f9f'),
             'README.md': '\n'.join(lines) + '\n'}
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=str(output)) as temporary:
        for name, content in files.items():
            path = Path(temporary) / name
            path.write_text(content, encoding='utf-8')
        for name in files:
            (Path(temporary) / name).replace(output / name)
    return snapshot


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sources', type=Path,
                        default=Path(__file__).resolve().parents[2] / '.ecosystem-private/config/ecosystem_sources.json',
                        help='Private source configuration downloaded from GitLab Secure Files')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--boards-only', action='store_true', help='Preview board counts without scanning projects; never publish this preview')
    parser.add_argument('--listed-boards', action='store_true', help='With --boards-only, preview only configured sources without GitHub discovery')
    args = parser.parse_args(argv)
    token = next((os.environ[key] for key in ('ECOSYSTEM_GITHUB_TOKEN', 'GITHUB_TOKEN', 'GH_TOKEN') if os.environ.get(key)), None)
    try:
        if args.listed_boards and not args.boards_only:
            raise CollectionError('--listed-boards requires --boards-only; listed-only data is a preview')
        if not args.listed_boards and not token:
            raise CollectionError('Set ECOSYSTEM_GITHUB_TOKEN for authenticated GitHub code search')
        sources = json.loads(args.sources.read_text(encoding='utf-8'))
        ledger = None
        if sources.get('board_review_ledger'):
            ledger = json.loads((args.sources.parent / sources['board_review_ledger']).read_text(encoding='utf-8'))
        if not args.boards_only and ledger is None:
            raise CollectionError('A private board_review_ledger is required for complete statistics')
        client = PartitionedGitHubClient(token)
        boards = collect_boards(sources['board_packs']) if args.listed_boards else discover_boards(
            client, sources['board_packs'], sources.get('board_excluded_repositories', []),
            sources.get('board_excluded_paths', []), lineages=sources.get('board_lineages', []))
        boards['discovery_enabled'] = not args.listed_boards
        repositories = None if args.boards_only else collect_projects(
            client, sources['excluded_repositories'], sources.get('excluded_evidence_paths', []))
        snapshot = {'schema_version': 1, 'updated_at': datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),
                    'boards': boards, 'projects': None if repositories is None else {'count': len(repositories), 'repositories': repositories},
                    'board_filters': {key: sources.get(key, []) for key in (
                        'board_excluded_repositories', 'board_excluded_paths', 'board_lineages', 'board_review_matches')},
                    'project_filters': {key: sources.get(key, []) for key in ('excluded_repositories', 'excluded_evidence_paths')}}
        if ledger is not None:
            snapshot['board_filters']['board_decisions'] = ledger
        written = write_report(snapshot, args.output)
        print('Reviewed boards: {}; observed definitions: {}; public dependent projects: {}; output: {}'.format(
            written['boards'].get('approved_count', boards['count']), boards['count'],
            'not scanned' if repositories is None else len(repositories), args.output))
        return 0
    except (CollectionError, OSError, KeyError, ValueError) as error:
        print('Collection failed; previous published data is unchanged: {}'.format(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
