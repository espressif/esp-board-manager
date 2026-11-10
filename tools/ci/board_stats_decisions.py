# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""Project reviewed board decisions without changing the observed board graph."""

from copy import deepcopy
from pathlib import PurePosixPath
import re

from discover_board_stats import CollectionError


def _text(value, label):
    if not isinstance(value, str) or not value.strip() or any(character in value for character in '*?[]'):
        raise CollectionError('Invalid board decision ' + label)
    return value.strip()


def _repository(value):
    value = _text(value, 'repository')
    if (not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', value)
            or any(part in ('.', '..') for part in value.split('/'))):
        raise CollectionError('Invalid board decision repository')
    return value.casefold()


def _path(value):
    value = _text(value, 'path')
    if value.startswith('/') or '\\' in value or '..' in value.split('/'):
        raise CollectionError('Invalid board decision path')
    normalized = str(PurePosixPath(value))
    return '' if normalized == '.' else normalized


def _observed_path(value):
    path = str(PurePosixPath(value or ''))
    return '' if path == '.' else path


def _chip(value):
    return value.strip().casefold() if isinstance(value, str) else ''


def _source(origin):
    return origin.get('repository', '').casefold(), _observed_path(origin.get('path'))


def _selector(value):
    if not isinstance(value, dict):
        raise CollectionError('Invalid board decision selector')
    chip = _text(value.get('chip'), 'chip').casefold()
    if set(value) == {'repository', 'path', 'chip'}:
        return 'source', _repository(value['repository']), _path(value['path']), chip
    if set(value) == {'family', 'board', 'chip'}:
        return 'lineage', _text(value['family'], 'family').casefold(), _path(value['board']).casefold(), chip
    raise CollectionError('Invalid board decision selector fields')


def _target(value):
    if value is None or isinstance(value, dict) and not {'repository', 'path'} <= set(value):
        return None
    if not isinstance(value, dict):
        raise CollectionError('Invalid board decision target')
    return _repository(value['repository']), _path(value['path'])


def _distinct_from(entry):
    values = entry.get('distinct_from', [])
    if entry['decision'] != 'include' and 'distinct_from' in entry:
        raise CollectionError('Only included board decisions can declare distinct known targets')
    if not isinstance(values, list):
        raise CollectionError('Invalid distinct board targets')
    result = set()
    for value in values:
        if not isinstance(value, dict) or set(value) != {'repository', 'path'}:
            raise CollectionError('Invalid distinct board target')
        result.add(_target(value))
    return result


def _ledger_entries(ledger):
    if (not isinstance(ledger, dict) or ledger.get('schema_version') != 1
            or not isinstance(ledger.get('decisions'), list)):
        raise CollectionError('Unsupported board decision ledger')
    entries = []
    identities = {}
    for entry in ledger['decisions']:
        if not isinstance(entry, dict) or entry.get('decision') not in ('include', 'alias'):
            raise CollectionError('Invalid board decision action')
        selectors = entry.get('selectors')
        if not isinstance(selectors, list) or not selectors:
            raise CollectionError('Board decision selectors must be nonempty')
        if entry['decision'] == 'include' and 'target' in entry:
            raise CollectionError('An included board decision cannot have an alias target')
        validated = {'id': _text(entry.get('id'), 'id'), 'decision': entry['decision'],
                     'reason': _text(entry.get('reason'), 'reason'),
                     'selectors': {_selector(selector) for selector in selectors},
                     'target': _target(entry.get('target')) if entry['decision'] == 'alias' else None,
                     'distinct_from': _distinct_from(entry)}
        action = validated['decision'], validated['target']
        if validated['id'] in identities and identities[validated['id']] != action:
            raise CollectionError('Conflicting board decisions for identity: ' + validated['id'])
        identities[validated['id']] = action
        entries.append(validated)
    exemptions = {}
    for entry in entries:
        exemptions.setdefault(entry['id'], set()).update(entry['distinct_from'])
    for entry in entries:
        entry['distinct_from'] = exemptions[entry['id']]
    return entries


def _known_catalog(boards):
    packs = boards.get('packs', [])
    scopes = {(pack.get('repository', '').casefold(), _observed_path(pack.get('path')))
              for pack in packs if pack.get('listed', False)}
    known_count = sum(pack.get('count', 0) for pack in packs if pack.get('listed', False))
    catalog = {}
    for pack in packs:
        if not pack.get('listed', False):
            continue
        for definition in pack.get('definitions', []):
            for origin in definition.get('origins', []):
                repository, path = _source(origin)
                if (repository, _observed_path(str(PurePosixPath(path).parent))) not in scopes:
                    continue
                target = {'board': definition['board'], 'chip': definition['chip'],
                          'url': origin.get('url', ''), 'repository': origin['repository'], 'path': path}
                catalog.setdefault((repository, path), []).append(target)
    return known_count, catalog


def _lineages_by_source(boards):
    result = {}
    for pack in boards.get('packs', []):
        for definition in pack.get('definitions', []):
            lineage = definition.get('lineage')
            if not isinstance(lineage, dict) or not lineage.get('family') or not lineage.get('board'):
                continue
            identity = ('lineage', lineage['family'].casefold(), lineage['board'].casefold(), _chip(definition.get('chip')))
            for origin in definition.get('origins', []):
                result.setdefault((*_source(origin), _chip(definition.get('chip'))), set()).add(identity)
    return result


def _candidate_selectors(candidate, lineages_by_source):
    chip = _chip(candidate.get('chip'))
    selectors = set()
    for origin in candidate.get('origins', []):
        source = _source(origin)
        selectors.add(('source', *source, chip))
        selectors.update(lineages_by_source.get((*source, chip), ()))
    lineage = candidate.get('lineage')
    if isinstance(lineage, dict) and lineage.get('family') and lineage.get('board'):
        selectors.add(('lineage', lineage['family'].casefold(), lineage['board'].casefold(), chip))
    return selectors


def _include_groups(matches):
    groups = {}
    for index, entries in matches.items():
        for entry in entries:
            groups.setdefault(entry['id'], set()).add(index)
    return groups.values()


def _project_includes(candidates, matches, known):
    known_urls = {}
    for source, targets in known.items():
        for target in targets:
            if target.get('url'):
                known_urls.setdefault(target['url'], set()).add(source)
    for group in _include_groups(matches):
        blocked = ''
        distinct = set().union(*(entry['distinct_from'] for index in group for entry in matches[index]))
        hints = [hint for index in group for hint in candidates[index].get('known_matches', [])]
        if any(not isinstance(hint, dict) or not isinstance(hint.get('url'), str)
               or not known_urls.get(hint['url']) or not known_urls[hint['url']] <= distinct for hint in hints):
            blocked = '; New known-model hints require another review'
        elif len({_chip(candidates[index].get('chip')) for index in group}) != 1:
            blocked = '; The reviewed identity spans different chips and requires another review'
        if blocked:
            for index in group:
                candidates[index]['decision_reason'] += blocked
            continue
        representative = min(group, key=lambda index: (*_source(candidates[index].get('primary', {})),
                                                       candidates[index].get('id', '')))
        included = candidates[representative]
        origin = included.get('primary', {})
        target = {'board': included['board'], 'chip': included['chip'], 'url': origin.get('url', ''),
                  'repository': origin.get('repository', ''), 'path': _observed_path(origin.get('path'))}
        for index in group:
            candidate = candidates[index]
            candidate['decision'] = 'include' if index == representative else 'alias'
            if index != representative:
                candidate['decision_reason'] += '; Same reviewed model as the included source'
                candidate['target'] = deepcopy(target)


def apply_board_decisions(boards, review, ledger):
    """Apply exact selectors to current candidates; unmatched decisions add no boards.

    UI IDs do not authorize a source. All observed candidates and evidence remain
    in the returned review; this projection never changes collection or merging.
    """
    entries = _ledger_entries(ledger)
    known_count, known = _known_catalog(boards)
    lineages = _lineages_by_source(boards)
    candidates = deepcopy(review.get('candidates', []))
    active = set()
    include_matches = {}
    for index, candidate in enumerate(candidates):
        candidate.pop('target', None)
        selectors = _candidate_selectors(candidate, lineages)
        matched = [entry for entry in entries if entry['selectors'] & selectors]
        candidate['decision'] = 'pending'
        candidate['decision_reason'] = 'No reviewed decision matches this candidate'
        if not matched:
            continue
        active.update(entry['id'] for entry in matched)
        actions = {(entry['decision'], entry['target']) for entry in matched}
        if len(actions) != 1:
            raise CollectionError('Conflicting board decisions for candidate: ' + candidate.get('id', ''))
        action, target = actions.pop()
        reason = '; '.join(dict.fromkeys(entry['reason'] for entry in matched))
        candidate['decision_reason'] = reason
        if action == 'include':
            if len({entry['id'] for entry in matched}) != 1:
                raise CollectionError('Different included board identities match candidate: ' + candidate.get('id', ''))
            include_matches[index] = matched
            continue
        targets = [entry for entry in known.get(target, ()) if _chip(entry['chip']) == _chip(candidate.get('chip'))]
        if len(targets) != 1:
            candidate['decision_reason'] = reason + '; Alias target is missing, ambiguous, or has a different chip'
            continue
        candidate['decision'] = 'alias'
        candidate['decision_reason'] = reason
        candidate['target'] = deepcopy(targets[0])
    _project_includes(candidates, include_matches, known)
    included = sum(candidate['decision'] == 'include' for candidate in candidates)
    aliased = sum(candidate['decision'] == 'alias' for candidate in candidates)
    return {'known_count': known_count, 'included_count': included, 'aliased_count': aliased,
            'pending_count': len(candidates) - included - aliased, 'approved_count': known_count + included,
            'candidates': candidates, 'inactive_decision_ids': sorted({entry['id'] for entry in entries} - active)}
