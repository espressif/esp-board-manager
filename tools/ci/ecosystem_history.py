# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""Build a private chronological ecosystem ledger without network or file writes."""

from copy import deepcopy
from datetime import datetime, timedelta, timezone
from hashlib import sha256
from html import escape
import json
from pathlib import PurePosixPath
import re
from urllib.parse import quote, urlsplit


SECTIONS = ('boards', 'projects')
SHANGHAI = timezone(timedelta(hours=8))


def _text(value):
    if not isinstance(value, str) or not value.strip():
        raise ValueError('History requires nonempty text fields')
    return value.strip()


def _timestamp(value):
    value = _text(value)
    try:
        parsed = datetime.strptime(value, '%Y-%m-%dT%H:%M:%SZ').replace(tzinfo=timezone.utc)
    except ValueError:
        raise ValueError('History requires an immutable UTC scan timestamp') from None
    if parsed.strftime('%Y-%m-%dT%H:%M:%SZ') != value:
        raise ValueError('Invalid history scan timestamp')
    return parsed


def _url(value, github=False):
    value = _text(value)
    parsed = urlsplit(value)
    if (parsed.scheme != 'https' or not parsed.hostname or parsed.username or parsed.password
            or any(character.isspace() for character in value)
            or github and parsed.netloc != 'github.com'):
        raise ValueError('Invalid history evidence URL')
    return value


def _run(value):
    if not isinstance(value, dict) or set(value) != {'id', 'url', 'ref'}:
        raise ValueError('History requires the run id, URL, and source ref')
    return {'id': _text(value['id']), 'url': _url(value['url']), 'ref': _text(value['ref'])}


def _count(value):
    if type(value) is not int or value < 0:
        raise ValueError('Invalid history count')
    return value


def _source(origin, chip):
    if not isinstance(origin, dict):
        raise ValueError('History board sources must be objects')
    repository = _text(origin.get('repository')).casefold()
    if not re.fullmatch(r'[a-z0-9_.-]+/[a-z0-9_.-]+', repository):
        raise ValueError('Invalid history board repository')
    path = _text(origin.get('path'))
    if path.startswith('/') or '\\' in path or '..' in path.split('/'):
        raise ValueError('Invalid history board path')
    path = str(PurePosixPath(path))
    return ('source', repository, path, chip), _url(origin.get('url'), github=True)


def _tokens(entity, section):
    if not isinstance(entity, dict):
        raise ValueError('Invalid history entity')
    identifier = _text(entity.get('id'))
    pattern = r'board-[0-9a-f]{64}' if section == 'boards' else r'project-[1-9][0-9]*'
    if not re.fullmatch(pattern, identifier):
        raise ValueError('Invalid history entity id')
    _text(entity.get('name'))
    _url(entity.get('url'), github=True)
    identities = entity.get('identities')
    if not isinstance(identities, list) or not identities:
        raise ValueError('History entities require stable identities')
    result = set()
    for identity in identities:
        if not isinstance(identity, list) or not all(isinstance(item, str) and item for item in identity):
            raise ValueError('Invalid stable history identity')
        token = tuple(identity)
        valid = (section == 'projects' and len(token) == 2 and token[0] == 'project'
                 and re.fullmatch(r'[1-9][0-9]*', token[1]) is not None)
        valid = valid or (section == 'boards' and ((len(token) == 4 and token[0] in ('source', 'lineage'))
                                                  or (len(token) == 3 and token[0] == 'review')))
        if not valid or token in result:
            raise ValueError('Invalid or duplicate stable history identity')
        result.add(token)
    if section == 'projects' and (len(result) != 1 or identifier != 'project-' + next(iter(result))[1]):
        raise ValueError('History project id must match exactly one canonical repository identity')
    if section == 'boards':
        chip = _text(entity.get('chip')).casefold()
        if any(token[-1] != chip for token in result):
            raise ValueError('Board history identity has a different chip')
        sources = entity.get('sources')
        if not isinstance(sources, list) or not sources:
            raise ValueError('Board history requires source links')
        for source in sources:
            _url(source, github=True)
    return result


def _validate_inventory(inventory):
    if not isinstance(inventory, dict) or set(inventory) != set(SECTIONS):
        raise ValueError('Invalid history inventory')
    for section in SECTIONS:
        if not isinstance(inventory[section], list):
            raise ValueError('Invalid history inventory entries')
        ids, tokens = set(), set()
        for entity in inventory[section]:
            current = _tokens(entity, section)
            if entity['id'] in ids or tokens & current:
                raise ValueError('Ambiguous or duplicate history identities')
            ids.add(entity['id'])
            tokens.update(current)


def _board_entity(definition, review_identity=None):
    if not isinstance(definition, dict):
        raise ValueError('Invalid board definition in history snapshot')
    chip = _text(definition.get('chip')).casefold()
    origins = definition.get('origins')
    if not isinstance(origins, list) or not origins:
        raise ValueError('Board history requires full source definitions')
    sources = sorted((_source(origin, chip) for origin in origins), key=lambda item: item[0])
    tokens = {token for token, _ in sources}
    lineage = definition.get('lineage')
    if lineage is not None:
        if not isinstance(lineage, dict):
            raise ValueError('Invalid board lineage in history')
        tokens.add(('lineage', _text(lineage.get('family')).casefold(),
                    _text(lineage.get('board')).casefold(), chip))
    if review_identity is not None:
        tokens.add(('review', _text(review_identity), chip))
    identities = [list(token) for token in sorted(tokens)]
    identity_hash = sha256(json.dumps(identities, separators=(',', ':')).encode('utf-8')).hexdigest()
    return {'id': 'board-' + identity_hash, 'name': _text(definition.get('board')), 'chip': chip,
            'url': sources[0][1], 'sources': sorted({url for _, url in sources}), 'identities': identities}


def _snapshot_inventory(snapshot):
    if (not isinstance(snapshot, dict) or type(snapshot.get('schema_version')) is not int
            or snapshot['schema_version'] != 1):
        raise ValueError('Only complete statistics snapshots can enter history')
    _timestamp(snapshot.get('updated_at'))
    boards, projects = snapshot.get('boards'), snapshot.get('projects')
    if not isinstance(boards, dict) or not isinstance(projects, dict):
        raise ValueError('History requires a complete board and project scan')
    packs = boards.get('packs')
    summary = boards.get('decision_summary')
    if not isinstance(packs, list) or not isinstance(summary, dict):
        raise ValueError('History requires reviewed complete board definitions')
    known, observed = [], 0
    for pack in packs:
        if not isinstance(pack, dict) or not isinstance(pack.get('definitions'), list):
            raise ValueError('History requires complete board pack definitions')
        if _count(pack.get('count')) != len(pack['definitions']):
            raise ValueError('History board pack count disagrees with definitions')
        observed += pack['count']
        if pack.get('listed'):
            known.extend(_board_entity(definition) for definition in pack['definitions'])
    candidates = summary.get('candidates')
    if not isinstance(candidates, list):
        raise ValueError('History requires current board decisions')
    decision_counts = {decision: 0 for decision in ('include', 'alias', 'pending')}
    included = []
    for candidate in candidates:
        if not isinstance(candidate, dict) or candidate.get('decision') not in decision_counts:
            raise ValueError('Invalid current board decision in history')
        decision_counts[candidate['decision']] += 1
        if candidate['decision'] == 'include':
            included.append(_board_entity(candidate, _text(candidate.get('review_identity'))))
    if (_count(boards.get('count')) != observed or observed != len(known) + len(candidates)
            or _count(summary.get('known_count')) != len(known)
            or _count(summary.get('included_count')) != len(included)
            or _count(summary.get('aliased_count')) != decision_counts['alias']
            or _count(summary.get('pending_count')) != decision_counts['pending']
            or _count(summary.get('approved_count')) != len(known) + len(included)
            or _count(boards.get('approved_count')) != len(known) + len(included)):
        raise ValueError('History board counts disagree with counted identities')
    repositories = projects.get('repositories')
    if not isinstance(repositories, list) or _count(projects.get('count')) != len(repositories):
        raise ValueError('History project counts disagree with repositories')
    normalized = []
    for project in repositories:
        if not isinstance(project, dict):
            raise ValueError('Invalid project in history snapshot')
        identifier = project.get('repository_id')
        if type(identifier) is not int or identifier <= 0:
            raise ValueError('Project history requires a stable GitHub repository id')
        normalized.append({'id': 'project-' + str(identifier), 'name': _text(project.get('repository')),
                           'url': _url(project.get('url'), github=True),
                           'identities': [['project', str(identifier)]]})
    inventory = {'boards': known + included, 'projects': normalized}
    _validate_inventory(inventory)
    for section in SECTIONS:
        inventory[section].sort(key=lambda entity: entity['id'])
    return inventory


def _validate_history(history):
    if (not isinstance(history, dict) or type(history.get('schema_version')) is not int
            or history['schema_version'] != 1):
        raise ValueError('Unsupported or malformed existing ecosystem history')
    _validate_inventory(history.get('latest'))
    entries = history.get('entries')
    if not isinstance(entries, list) or not entries:
        raise ValueError('Existing history cannot silently become a new baseline')
    previous_time, previous_counts, seen_runs = None, None, set()
    stream_ref = None
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict):
            raise ValueError('Invalid existing history entry')
        timestamp = _timestamp(entry.get('updated_at'))
        run = _run(entry.get('run'))
        if stream_ref is not None and run['ref'] != stream_ref:
            raise ValueError('Existing history source refs belong to different streams')
        stream_ref = run['ref']
        if run['id'] in seen_runs or previous_time is not None and timestamp < previous_time:
            raise ValueError('Duplicate or out-of-order existing history entries')
        seen_runs.add(run['id'])
        if entry.get('date') != timestamp.astimezone(SHANGHAI).date().isoformat():
            raise ValueError('Existing history date disagrees with its scan timestamp')
        if (type(entry.get('baseline')) is not bool or entry['baseline'] != (index == 0)
                or not isinstance(entry.get('snapshot_sha256'), str)
                or not re.fullmatch('[0-9a-f]{64}', entry['snapshot_sha256'])):
            raise ValueError('Invalid existing history baseline or snapshot identity')
        counts, delta = entry.get('counts'), entry.get('delta')
        if not isinstance(counts, dict) or set(counts) != set(SECTIONS) or not isinstance(delta, dict) or set(delta) != set(SECTIONS):
            raise ValueError('Invalid existing history counters')
        for category in ('added', 'removed'):
            _validate_inventory(entry.get(category))
        if entry.get('previous_counts') != previous_counts:
            raise ValueError('Existing history previous counts disagree')
        for section in SECTIONS:
            current = _count(counts[section])
            expected = current - previous_counts[section] if previous_counts is not None else 0
            if type(delta[section]) is not int or delta[section] != expected:
                raise ValueError('Existing history count delta disagrees')
            added, removed = len(entry['added'][section]), len(entry['removed'][section])
            if (index == 0 and (added or removed)) or index and added - removed != expected:
                raise ValueError('Existing history entity changes disagree with counts')
        previous_time, previous_counts = timestamp, counts
    if (history.get('updated_at') != entries[-1]['updated_at']
            or any(len(history['latest'][section]) != previous_counts[section] for section in SECTIONS)):
        raise ValueError('Existing history latest inventory disagrees with its last entry')
    # Walk changes backwards to ensure an event's added IDs actually belong to
    # that scan and removed IDs are absent. Counts alone miss corrupt ledgers.
    for section in SECTIONS:
        active = {entity['id']: entity for entity in history['latest'][section]}
        identity_history = {}

        def remember(entity):
            tokens = _tokens(entity, section)
            earlier = identity_history.setdefault(entity['id'], [])
            if any(not tokens.intersection(known) for known in earlier):
                raise ValueError('Existing history reuses an entity id for incompatible identities')
            if tokens not in earlier:
                earlier.append(tokens)
            return tokens

        for entity in active.values():
            remember(entity)
        for entry in reversed(entries[1:]):
            additions = {entity['id']: entity for entity in entry['added'][section]}
            removals = {entity['id']: entity for entity in entry['removed'][section]}
            if not additions.keys() <= active.keys() or removals.keys() & active.keys() or additions.keys() & removals.keys():
                raise ValueError('Existing history changes disagree with entity identities')
            for identifier, entity in additions.items():
                if not remember(entity) <= _tokens(active[identifier], section):
                    raise ValueError('Existing history added identities are missing from the active entity')
                del active[identifier]
            for identifier, entity in removals.items():
                remember(entity)
                active[identifier] = entity
            if len(active) != entry['previous_counts'][section]:
                raise ValueError('Existing history reconstructed inventory count disagrees')


def _match(previous, current):
    added, removed = {}, {}
    for section in SECTIONS:
        old = previous[section]
        old_tokens = {}
        for index, entity in enumerate(old):
            for token in _tokens(entity, section):
                old_tokens[token] = index
        used, additions = set(), []
        for entity in current[section]:
            tokens = _tokens(entity, section)
            matches = {old_tokens[token] for token in tokens if token in old_tokens}
            if len(matches) > 1:
                raise ValueError('Ambiguous history model merge requires review')
            if matches:
                index = matches.pop()
                if index in used:
                    raise ValueError('Ambiguous history model split requires review')
                used.add(index)
                entity['id'] = old[index]['id']
                entity['identities'] = [list(token) for token in sorted(tokens | _tokens(old[index], section))]
            else:
                additions.append(deepcopy(entity))
        added[section] = additions
        removed[section] = [deepcopy(entity) for index, entity in enumerate(old) if index not in used]
        current[section].sort(key=lambda entity: entity['id'])
    _validate_inventory(current)
    return added, removed


def build_history(previous, snapshot, run):
    """Return a new cumulative ledger; None alone authorizes the first baseline."""
    run = _run(run)
    current = _snapshot_inventory(snapshot)
    digest = sha256(json.dumps(snapshot, sort_keys=True, ensure_ascii=True, separators=(',', ':'),
                               allow_nan=False).encode('utf-8')).hexdigest()
    if previous is not None:
        _validate_history(previous)
        if previous['entries'][0]['run']['ref'] != run['ref']:
            raise ValueError('Incoming source ref does not match the existing history stream')
        for entry in previous['entries']:
            if entry['run']['id'] == run['id']:
                if entry['snapshot_sha256'] == digest and entry['run'] == run:
                    return deepcopy(previous)
                raise ValueError('The same history run id cannot describe different scan data')
        if _timestamp(snapshot['updated_at']) < _timestamp(previous['updated_at']):
            raise ValueError('A historical scan cannot replace a newer history baseline')
    baseline = previous is None
    result = deepcopy(previous) if previous is not None else {'schema_version': 1, 'entries': []}
    counts = {section: len(current[section]) for section in SECTIONS}
    old_counts = None if baseline else dict(result['entries'][-1]['counts'])
    added, removed = ({section: [] for section in SECTIONS}, {section: [] for section in SECTIONS})
    if not baseline:
        added, removed = _match(result['latest'], current)
    result['latest'] = current
    result['updated_at'] = snapshot['updated_at']
    result['entries'].append({'run': run, 'updated_at': snapshot['updated_at'],
                              'date': _timestamp(snapshot['updated_at']).astimezone(SHANGHAI).date().isoformat(),
                              'baseline': baseline, 'counts': counts, 'previous_counts': old_counts,
                              'delta': {section: counts[section] - old_counts[section] if old_counts else 0 for section in SECTIONS},
                              'added': added, 'removed': removed, 'snapshot_sha256': digest})
    _validate_history(result)
    return result


def _label(value):
    return re.sub(r'([\\\[\]`*_!|])', r'\\\1', escape(' '.join(value.splitlines())))


def _link(name, url):
    return '[{}]({})'.format(_label(name), quote(url, safe=':/?#@!$&\'+,;=%~._-'))


def _entity_table(entities, section):
    lines = ['| Name / 名称 | Source / 来源 |', '| --- | --- |']
    for entity in entities:
        name = entity['name'] + (' ({})'.format(entity['chip']) if section == 'boards' else '')
        urls = entity.get('sources', [entity['url']])
        lines.append('| {} | {} |'.format(_label(name), ' · '.join(_link('Source {}'.format(index + 1), url)
                                                                  for index, url in enumerate(urls))))
    return lines


def render_changes(history, run_id=None):
    """Render one archived scan, showing a full roster only for its baseline."""
    _validate_history(history)
    if run_id is None:
        entry = history['entries'][-1]
    else:
        selected = _text(run_id)
        entry = next((item for item in history['entries'] if item['run']['id'] == selected), None)
        if entry is None:
            raise ValueError('The selected history run was not found')
    if entry['baseline'] and len(history['entries']) != 1:
        raise ValueError('The original baseline inventory is unavailable in a later ledger; '
                         'open the baseline archive version (README.md for legacy archives, changes.md for new archives)')
    has_added = any(entry['added'][section] for section in SECTIONS)
    has_removed = any(entry['removed'][section] for section in SECTIONS)
    if entry['baseline']:
        status = '首次基线 / Baseline'
    elif has_added and has_removed:
        status = '有新增和移除 / Added and removed'
    elif has_added:
        status = '有新增 / Added'
    elif has_removed:
        status = '有移除 / Removed'
    else:
        status = '无变化 / No changes'
    timestamp = _timestamp(entry['updated_at']).astimezone(SHANGHAI).strftime('%Y-%m-%d %H:%M:%S (UTC+8)')
    lines = ['# 本轮更新 / Current update', '', '**{}**'.format(status), '',
             '扫描时间 / Scan time: **{}**'.format(timestamp), '',
             '{} · ref: {}'.format(_link('Run ' + entry['run']['id'], entry['run']['url']), _label(entry['run']['ref'])), '',
             '| 指标 / Metric | 上次 / Previous | 本次 / Current | 变化 / Change |',
             '| --- | ---: | ---: | ---: |']
    for section, label in (('boards', '适配的板卡 / Supported boards'),
                           ('projects', '支持的项目 / Projects using Board Manager')):
        previous = '—' if entry['baseline'] else str(entry['previous_counts'][section])
        delta = '—' if entry['baseline'] else '{:+d}'.format(entry['delta'][section])
        lines.append('| {} | {} | {} | {} |'.format(label, previous, entry['counts'][section], delta))
    lines.append('')
    if entry['baseline']:
        lines += ['首次建立基线，以下存量不记为新增。 / The initial inventory establishes the baseline; '
                  'existing entries are not newly added.', '']
        for section, label in (('boards', '板卡完整清单 / Complete board inventory'),
                               ('projects', '项目完整清单 / Complete project inventory')):
            lines += ['## ' + label, ''] + _entity_table(history['latest'][section], section) + ['']
    else:
        for action, title in (('added', '新增 / Added'), ('removed', '移除 / Removed')):
            for section, label in (('boards', '板卡 / Boards'), ('projects', '项目 / Projects')):
                entities = entry[action][section]
                if entities:
                    lines += ['## {} — {}'.format(title, label), ''] + _entity_table(entities, section) + ['']
        if not has_added and not has_removed:
            lines += ['本轮没有新增或移除的板卡和项目。 / No boards or projects were added or removed in this scan.', '']
    return '\n'.join(lines)


def render_history(history):
    """Render daily closing counts and scan changes without repeating the roster."""
    _validate_history(history)
    lines = ['# Ecosystem history / 生态变化历史', '',
             '[本轮更新 / Current update](changes.md)', '',
             '`history.json` retains the complete machine-readable state for the next comparison. '
             '/ `history.json` 保留完整机器状态，供下轮自动对比；本文件只展示每日汇总和变化明细。', '',
             'Observed scan dates use Asia/Shanghai (UTC+8). A missing day has no successful archived scan. '
             '/ 日期按北京时间（UTC+8）记录；缺少日期表示没有成功归档的扫描。', '',
             '## Daily summary / 每日汇总', '',
             'Last scan each day; changes are relative to the previous recorded day. '
             '/ 采用当日最后一次扫描，变化量相对于上一有记录日期。', '',
             '| Date / 日期 | Boards / 板卡 | Projects / 项目 | Board change / 板卡变化 | Project change / 项目变化 |',
             '| --- | ---: | ---: | ---: | ---: |']
    daily = {}
    for entry in history['entries']:
        daily[entry['date']] = entry
    old = None
    for date, entry in daily.items():
        counts = entry['counts']
        changes = ['Baseline / 基线'] * 2 if old is None else ['{:+d}'.format(counts[section] - old[section]) for section in SECTIONS]
        lines.append('| {} | {} | {} | {} | {} |'.format(date, counts['boards'], counts['projects'], *changes))
        old = counts
    lines += ['', '## Scan changes / 每轮扫描变化', '']
    for entry in history['entries']:
        lines += ['### {}'.format(entry['updated_at']), '',
                  '{} · ref: {}'.format(_link('Run ' + entry['run']['id'], entry['run']['url']), _label(entry['run']['ref'])), '',
                  'Boards / 板卡: **{}** ({:+d}); Projects / 项目: **{}** ({:+d}).'.format(
                      entry['counts']['boards'], entry['delta']['boards'], entry['counts']['projects'], entry['delta']['projects']), '']
        if entry['baseline']:
            lines += ['Baseline / 基线: existing inventory is not classified as newly added. / 存量不记为新增。', '']
            continue
        changed = False
        for action, title in (('added', 'Added / 新增'), ('removed', 'Removed / 移除')):
            for section, label in (('boards', 'Boards / 板卡'), ('projects', 'Projects / 项目')):
                entities = entry[action][section]
                if entities:
                    changed = True
                    lines += ['#### {} — {}'.format(title, label), ''] + _entity_table(entities, section) + ['']
        if not changed:
            lines += ['No changes / 无变化', '']
    return '\n'.join(lines)
