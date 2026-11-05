# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""Annotate outside board candidates for review without changing collected counts."""

from copy import deepcopy
from hashlib import sha256
import json
from pathlib import PurePosixPath
import unicodedata


def _normalize(value):
    if not isinstance(value, str):
        return ''
    return ''.join(character for character in unicodedata.normalize('NFKC', value).casefold()
                   if character.isalnum())


def _path(value):
    path = str(PurePosixPath(value or ''))
    return '' if path == '.' else path


def _source(origin):
    return origin.get('repository', '').casefold(), _path(origin.get('path'))


def _scope(origin, known_scopes):
    repository, path = _source(origin)
    return (repository, _path(str(PurePosixPath(path).parent))) in known_scopes


def _model(value):
    return _normalize(value.rsplit('/', 1)[-1]) if isinstance(value, str) else ''


def _names(definition, origins):
    names = set()
    board = _model(definition.get('board'))
    if board and board != 'bmgr':
        names.add(board)
    for origin in origins:
        path = PurePosixPath(origin.get('path', ''))
        name = path.parent.name if path.name.casefold() == 'bmgr' else path.name
        if _model(name):
            names.add(_model(name))
        info = origin.get('info')
        if isinstance(info, dict):
            name = _model(info.get('board'))
            if name and name != 'bmgr':
                names.add(name)
    return names


def _alias_rules(lineages):
    links = {}
    for lineage in lineages:
        mappings = [lineage.get('aliases', {})]
        mappings.extend(root.get('aliases', {}) for root in lineage.get('roots', []))
        for mapping in mappings:
            for old, target in mapping.items():
                old = _model(old)
                if not old:
                    continue
                if isinstance(target, str):
                    target = _model(target)
                    if target:
                        links.setdefault(old, set()).add(target)
                        links.setdefault(target, set()).add(old)
    return links


def _expanded(names, links):
    expanded = set(names)
    pending = list(names)
    while pending:
        for name in links.get(pending.pop(), ()):
            if name not in expanded:
                expanded.add(name)
                pending.append(name)
    return expanded


def _configuration_pairs(origins):
    pairs = set()
    for origin in origins:
        files = origin.get('files')
        if not isinstance(files, dict):
            continue
        values = [files.get(filename) for filename in ('board_devices.yaml', 'board_peripherals.yaml')]
        if all(isinstance(value, str) and value for value in values):
            pairs.add(tuple(values))
    return pairs


def _primary(pack, definition):
    origins = definition.get('origins', [])
    source = pack.get('repository', '').casefold(), _path(definition.get('path'))
    return next((origin for origin in origins if _source(origin) == source), origins[0] if origins else {})


def _display_board(definition, primary):
    info = primary.get('info')
    board = info.get('board') if isinstance(info, dict) else None
    if isinstance(board, str) and board.strip() and _normalize(board) != 'bmgr':
        return board.strip()
    path = PurePosixPath(primary.get('path', ''))
    if path.name.casefold() == 'bmgr' and path.parent.name:
        return path.parent.name
    return definition['board']


def _candidate_id(pack, definition):
    lineage = definition.get('lineage')
    if isinstance(lineage, dict) and lineage.get('family') and lineage.get('board'):
        identity = ['lineage', lineage['family'].casefold(), lineage['board'].casefold()]
    else:
        identity = ['source', pack.get('repository', '').casefold(), _path(definition.get('path'))]
    identity.append(definition.get('chip', '').strip().casefold())
    encoded = json.dumps(identity, ensure_ascii=True, separators=(',', ':')).encode('utf-8')
    return 'outside-' + sha256(encoded).hexdigest()[:16]


def review_outside_boards(boards, lineages=(), reviewed_matches=()):
    """Return review hints for every nonlisted canonical definition.

    Matches are suggestions, never deduplication decisions or proof of a new
    physical model. Known scope is each listed repository/root's direct boards.
    Metadata-free older snapshots remain eligible for directory-name review.
    """
    packs = boards.get('packs', [])
    known_scopes = {(pack.get('repository', '').casefold(), _path(pack.get('path')))
                    for pack in packs if pack.get('listed', False)}
    links = _alias_rules(lineages)
    known = []
    known_by_source = {}
    merged = []
    for pack in packs:
        if not pack.get('listed', False):
            continue
        for definition in pack.get('definitions', []):
            origins = definition.get('origins', [])
            scoped = [origin for origin in origins if _scope(origin, known_scopes)]
            primary = next(iter(scoped), _primary(pack, definition))
            names = _names(definition, scoped)
            entry = {'board': definition['board'], 'chip': definition['chip'],
                     'url': primary.get('url', ''), 'names': names,
                     'expanded': _expanded(names, links), 'pairs': _configuration_pairs(scoped)}
            known.append(entry)
            for origin in scoped:
                known_by_source.setdefault(_source(origin), []).append(entry)
            for origin in origins:
                if not _scope(origin, known_scopes):
                    merged.append({'board': definition['board'], 'chip': definition['chip'],
                                   'origin': deepcopy(origin),
                                   'known': {'board': definition['board'], 'url': entry['url']},
                                   'reason': 'Already merged into a known component definition; retained as outside evidence'})

    maps = {}
    for mapping in reviewed_matches:
        maps.setdefault(_source(mapping), []).append(mapping)
    candidates = []
    for pack in packs:
        if pack.get('listed', False):
            continue
        for definition in pack.get('definitions', []):
            origins = definition.get('origins', [])
            chip = _normalize(definition.get('chip'))
            names = _names(definition, origins)
            expanded = _expanded(names, links)
            pairs = _configuration_pairs(origins)
            matches = {}
            notes = []
            evidence = []

            def add_match(entry, reason):
                if not chip or chip != _normalize(entry['chip']):
                    return False
                key = entry['board'], entry['chip'], entry['url']
                match = matches.setdefault(key, {'board': entry['board'], 'chip': entry['chip'],
                                                 'url': entry['url'], 'reasons': []})
                if reason not in match['reasons']:
                    match['reasons'].append(reason)
                return True

            for entry in known:
                if names & entry['names']:
                    add_match(entry, 'Matching normalized declared board or directory model')
                elif expanded & entry['expanded']:
                    add_match(entry, 'Matching reviewed legacy name alias')
                if pairs & entry['pairs']:
                    add_match(entry, 'Identical device and peripheral file hashes')
            for origin in origins:
                for mapping in maps.get(_source(origin), []):
                    if isinstance(mapping.get('note'), str) and mapping['note'] not in notes:
                        notes.append(mapping['note'])
                    for url in mapping.get('evidence', []):
                        if isinstance(url, str) and url not in evidence:
                            evidence.append(url)
                    for target in mapping.get('known_matches', []):
                        entries = known_by_source.get(_source(target), [])
                        matched = False
                        for entry in entries:
                            matched = add_match(entry, target.get('reason') or 'Reviewed exact source overlap') or matched
                        if not matched:
                            notes.append('Review target unavailable in known components with the same chip: {}/{}'.format(
                                target.get('repository', ''), target.get('path', '')))
            public_matches = []
            for match in sorted(matches.values(), key=lambda item: (item['board'], item['chip'], item['url'])):
                public_matches.append({key: match[key] for key in ('board', 'chip', 'url')})
                public_matches[-1]['reason'] = '; '.join(match['reasons'])
            primary = _primary(pack, definition)
            candidates.append({'id': _candidate_id(pack, definition), 'board': _display_board(definition, primary),
                               'chip': definition['chip'], 'primary': deepcopy(primary),
                               'lineage': deepcopy(definition.get('lineage')),
                               'origins': deepcopy(origins),
                               'status': 'possible_known_model' if public_matches else 'no_known_match',
                               'known_matches': public_matches, 'note': '; '.join(dict.fromkeys(notes)),
                               'evidence': evidence})
    candidates.sort(key=lambda item: (item['primary'].get('repository', '').casefold(),
                                      item['primary'].get('path', ''), item['chip'], item['id']))
    merged.sort(key=lambda item: (_source(item['origin']), item['board'], item['chip']))
    possible = sum(candidate['status'] == 'possible_known_model' for candidate in candidates)
    return {'candidate_count': len(candidates), 'possible_known_count': possible,
            'no_known_match_count': len(candidates) - possible, 'merged_known_origin_count': len(merged),
            'candidates': candidates, 'merged_known_origins': merged}
