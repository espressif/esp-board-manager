# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""Discover public BMGR definitions without executing repository contents."""

from concurrent.futures import ThreadPoolExecutor
from copy import deepcopy
from fnmatch import fnmatchcase
from pathlib import PurePosixPath
import re
from urllib.parse import quote

import yaml


SEARCH_QUERY = 'chip filename:board_info.yaml in:file'
REQUIRED_FILES = ('board_info.yaml', 'board_devices.yaml', 'board_peripherals.yaml')


class CollectionError(Exception):
    """A collection failure whose message is safe to show in CI logs."""
    pass


class BoardDiscoveryError(CollectionError):
    pass


def _call(function, message, *arguments):
    try:
        return function(*arguments)
    except CollectionError:
        raise
    except Exception as error:
        raise BoardDiscoveryError('{} ({})'.format(message, type(error).__name__)) from None


def _path(value):
    if not isinstance(value, str) or value.startswith('/') or '..' in value.split('/'):
        raise BoardDiscoveryError('Invalid repository path in board discovery')
    return '' if value in ('', '.') else str(PurePosixPath(value))


def _read_info(client, candidate):
    repo, revision, path, expected_sha = candidate
    result = _call(client.content, 'Could not read committed board information: ' + repo + '/' + path,
                   repo, path, revision)
    if not isinstance(result, tuple) or len(result) != 2:
        raise BoardDiscoveryError('Invalid board content response: ' + repo + '/' + path)
    content, actual_sha = result
    if not isinstance(content, str) or actual_sha != expected_sha:
        raise BoardDiscoveryError('Committed board information is unavailable or changed: ' + repo + '/' + path)
    try:
        document = yaml.safe_load(content)
    except yaml.YAMLError:
        return None
    if (not isinstance(document, dict) or not isinstance(document.get('chip'), str)
            or not document['chip'].strip()):
        return None
    return document


def _lineage(origin, chip, rules):
    """Match complete path segments, retaining the full vendor/model suffix."""
    parts = PurePosixPath(origin['path']).parts
    for rule in rules:
        for root in rule['roots']:
            if root.get('repository', origin['repository']).lower() != origin['repository'].lower():
                continue
            marker = PurePosixPath(root['path']).parts
            for start in range(len(parts) - len(marker)):
                end = start + len(marker)
                if parts[start:end] != marker:
                    continue
                identity = '/'.join(parts[end:]).casefold()
                identity = root.get('aliases', {}).get(identity, identity)
                if isinstance(identity, str):
                    identity = rule.get('aliases', {}).get(identity, identity)
                if isinstance(identity, dict):
                    repo = identity.get('repository')
                    if not isinstance(repo, str) or not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repo):
                        raise BoardDiscoveryError('Invalid board source alias repository')
                    return ('source', repo.lower(), _path(identity.get('path')), chip.casefold()), rule['upstream']
                if not isinstance(identity, str) or not identity:
                    raise BoardDiscoveryError('Invalid board identity alias')
                return (rule['family'], identity, chip.casefold()), rule['upstream']
    return None, None


def consolidate_board_sources(boards, lineages):
    """Merge inherited board identities without counting historical tree variants.

    Exact directory copies still merge across families. Every origin retains its
    own tree SHA; an upstream identity merge does not imply identical contents.
    The function can also reprocess older tree-only snapshots without network I/O.
    """
    families = set()
    for rule in lineages:
        if (not isinstance(rule.get('family'), str) or not rule['family'] or rule['family'] in families
                or not isinstance(rule.get('upstream'), str) or not isinstance(rule.get('roots'), list)):
            raise BoardDiscoveryError('Invalid board lineage rule')
        families.add(rule['family'])
        for root in rule['roots']:
            if not _path(root.get('path')):
                raise BoardDiscoveryError('Board lineage root must be nonempty')

    result = deepcopy(boards)
    source_groups = {(pack['repository'].lower(), _path(pack['path']), pack['revision']): index
                     for index, pack in enumerate(result['packs'])}
    nodes = []
    seen = set()
    for pack_index, pack in enumerate(result['packs']):
        for definition in pack.get('definitions', []):
            for original in definition['origins']:
                origin = dict(original, tree_sha=original.get('tree_sha', definition['tree_sha']))
                identity = (origin['repository'].lower(), origin['path'], origin['revision'])
                if identity in seen:
                    raise BoardDiscoveryError('Duplicate board origin in collected snapshot')
                seen.add(identity)
                lineage, upstream = _lineage(origin, definition['chip'], lineages)
                parent = _path(str(PurePosixPath(origin['path']).parent))
                source_index = source_groups.get((origin['repository'].lower(), parent, origin['revision']), pack_index)
                nodes.append({'origin': origin, 'chip': definition['chip'], 'pack': source_index,
                              'board': PurePosixPath(origin['path']).name or definition['board'],
                              'lineage': lineage, 'upstream': upstream})
        pack.update(count=0, raw_count=0, boards=[], definitions=[])

    # Union both exact copies and declared upstream identities. Transitivity is
    # needed when an exact copy moved outside the recognizable upstream layout.
    parents = list(range(len(nodes)))

    def find(index):
        while parents[index] != index:
            parents[index] = parents[parents[index]]
            index = parents[index]
        return index

    trees = {}
    identities = {}
    sources = {('source', node['origin']['repository'].lower(), node['origin']['path'], node['chip'].casefold())
               for node in nodes}
    authoritative = {rule['upstream'].lower() for rule in lineages}
    for index, node in enumerate(nodes):
        lineage = node['lineage']
        if lineage and len(lineage) == 4 and lineage not in sources:
            raise BoardDiscoveryError('Reviewed board alias target is missing or has a different chip: ' + lineage[1] + '/' + lineage[2])
        source = ('source', node['origin']['repository'].lower(), node['origin']['path'], node['chip'].casefold())
        for lookup, key in ((trees, node['origin']['tree_sha']), (identities, source), (identities, lineage)):
            if key is None:
                continue
            if key in lookup:
                parents[find(index)] = find(lookup[key])
            else:
                lookup[key] = index

    def priority(index):
        node = nodes[index]
        pack = result['packs'][node['pack']]
        repo = node['origin']['repository'].lower()
        rank = 0 if pack.get('listed') else (1 if repo in authoritative else 2)
        return rank, node['pack'] if rank == 0 else 0, repo, node['origin']['path']

    components = {}
    for index, node in enumerate(nodes):
        result['packs'][node['pack']]['raw_count'] += 1
        components.setdefault(find(index), []).append(index)
    for indexes in components.values():
        indexes.sort(key=priority)
        chosen = nodes[indexes[0]]
        origin = chosen['origin']
        definition = {'board': chosen['board'], 'path': origin['path'], 'chip': chosen['chip'],
                      'tree_sha': origin['tree_sha'], 'origins': [nodes[index]['origin'] for index in indexes],
                      'variant_tree_count': len({nodes[index]['origin']['tree_sha'] for index in indexes})}
        if chosen['lineage'] and len(chosen['lineage']) == 3:
            definition['lineage'] = {'family': chosen['lineage'][0], 'board': chosen['lineage'][1]}
        pack = result['packs'][chosen['pack']]
        pack['definitions'].append(definition)
        pack['count'] += 1
    for pack in result['packs']:
        pack['definitions'].sort(key=lambda definition: (definition['path'], definition['chip'], definition['tree_sha']))
        pack['boards'] = [definition['board'] for definition in pack['definitions']]
    raw_count = len(nodes)
    result.update(count=len(components), raw_count=raw_count,
                  listed_count=sum(pack['count'] for pack in result['packs'] if pack.get('listed')),
                  exact_duplicates_removed=raw_count - len(trees), lineage_variants_merged=len(trees) - len(components),
                  duplicates_removed=raw_count - len(components))
    result['discovered_count'] = result['count'] - result['listed_count']
    return result


def discover_boards(client, packs, excluded_repositories=(), excluded_paths=(), lineages=()):
    """Return exclusive canonical source groups plus provenance for exact copies.

    ``client.tree`` must return a complete recursive tree or raise an error. A
    synthetic empty-path tree entry retains the root SHA for root-level boards.
    Rejected candidates count filtered paths and structurally invalid board dirs.
    """
    matches = _call(client.search_query, 'Could not search for board definitions', SEARCH_QUERY)
    if not isinstance(matches, list):
        raise BoardDiscoveryError('Invalid board search response')
    excluded = {name.lower() for name in excluded_repositories}
    requested = [pack['repository'] for pack in packs] + [rule['upstream'] for rule in lineages]
    for match in matches:
        try:
            requested.append(match['repository']['full_name'])
        except (KeyError, TypeError):
            raise BoardDiscoveryError('Invalid repository in board search results') from None

    repositories = {}
    aliases = {}
    for requested_name in dict.fromkeys(requested):
        if (not isinstance(requested_name, str)
                or not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', requested_name)):
            raise BoardDiscoveryError('Invalid board repository name')
        key = requested_name.lower()
        if key in excluded or key in aliases:
            continue
        metadata = _call(client.repository, 'Could not read board repository: ' + requested_name, requested_name)
        if not isinstance(metadata, dict):
            raise BoardDiscoveryError('Board repository is unavailable: ' + requested_name)
        repo = metadata.get('full_name')
        if not isinstance(repo, str) or not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repo):
            raise BoardDiscoveryError('Invalid board repository metadata')
        if repo.lower() in excluded or any(metadata.get(flag) for flag in ('private', 'fork', 'is_template')):
            aliases[key] = None
            continue
        identity = metadata.get('id')
        if identity is None:
            raise BoardDiscoveryError('Missing board repository identity: ' + repo)
        aliases[key] = identity
        aliases[repo.lower()] = identity
        if identity in repositories:
            continue
        branch = metadata.get('default_branch')
        if not isinstance(branch, str) or not branch:
            raise BoardDiscoveryError('Missing board default branch: ' + repo)
        revision = _call(client.default_revision, 'Could not pin board repository: ' + repo, repo, branch)
        if not isinstance(revision, str) or not revision:
            raise BoardDiscoveryError('Missing board repository revision: ' + repo)
        entries = _call(client.tree, 'Could not read complete board tree: ' + repo, repo, revision)
        if isinstance(entries, dict):
            if entries.get('truncated') is not False:
                raise BoardDiscoveryError('Incomplete board tree: ' + repo)
            entries = list(entries.get('tree', [])) + [
                {'path': '', 'type': 'tree', 'mode': '040000', 'sha': entries.get('sha')}]
        if not isinstance(entries, list):
            raise BoardDiscoveryError('Invalid board tree: ' + repo)
        tree = {}
        for entry in entries:
            if not isinstance(entry, dict) or 'path' not in entry:
                raise BoardDiscoveryError('Invalid board tree entry: ' + repo)
            path = _path(entry['path'])
            if path in tree and tree[path] != entry:
                raise BoardDiscoveryError('Conflicting board tree entries: ' + repo)
            tree[path] = entry
        repositories[identity] = {'repository': repo, 'revision': revision, 'tree': tree}

    groups = []
    group_by_path = {}
    for pack in packs:
        identity = aliases.get(pack['repository'].lower())
        if identity is None:
            raise BoardDiscoveryError('Configured board source was excluded or unavailable: ' + pack['repository'])
        source = repositories[identity]
        path = _path(pack['path'])
        key = (identity, path)
        if key in group_by_path:
            continue
        group = {'component': pack['component'], 'repository': source['repository'],
                 'path': path, 'revision': source['revision'], 'count': 0,
                 'boards': [], 'definitions': [], 'raw_count': 0, 'listed': True}
        group_by_path[key] = group
        groups.append(group)

    candidates = []
    rejected = 0
    for identity, source in sorted(repositories.items(), key=lambda item: item[1]['repository'].lower()):
        repo, revision, tree = source['repository'], source['revision'], source['tree']
        for info_path in sorted(tree):
            if PurePosixPath(info_path).name != 'board_info.yaml':
                continue
            directory = _path(str(PurePosixPath(info_path).parent))
            if any(fnmatchcase('/' + info_path, pattern) for pattern in excluded_paths):
                rejected += 1
                continue
            name = PurePosixPath(directory).name if directory else None
            if name is not None and not re.fullmatch(r'[A-Za-z0-9_]+', name):
                rejected += 1
                continue
            files = [tree.get((directory + '/' if directory else '') + filename) for filename in REQUIRED_FILES]
            if any(not entry or entry.get('type') != 'blob' or entry.get('mode') not in ('100644', '100755')
                   for entry in files):
                rejected += 1
                continue
            directory_entry = tree.get(directory)
            if not directory_entry or directory_entry.get('type') != 'tree' or not directory_entry.get('sha'):
                raise BoardDiscoveryError('Missing board directory tree hash: ' + repo + '/' + directory)
            if any(not isinstance(entry.get('sha'), str) or not entry['sha'] for entry in files):
                raise BoardDiscoveryError('Missing committed board file hash: ' + repo + '/' + directory)
            parent = _path(str(PurePosixPath(directory).parent)) if directory else ''
            key = (identity, parent)
            if key not in group_by_path:
                group = {'component': repo, 'repository': repo, 'path': parent, 'revision': revision,
                         'count': 0, 'boards': [], 'definitions': [], 'raw_count': 0, 'listed': False}
                group_by_path[key] = group
                groups.append(group)
            candidates.append({'repository': repo, 'revision': revision, 'path': directory, 'board': name,
                               'info_path': info_path, 'info_sha': files[0]['sha'],
                               'files': {filename: entry['sha'] for filename, entry in zip(REQUIRED_FILES, files)},
                               'tree_sha': directory_entry['sha'], 'group': group_by_path[key]})

    # One pinned content request per unique info blob; workers only read data.
    reads = {}
    with ThreadPoolExecutor(max_workers=4) as executor:
        for candidate in candidates:
            sha = candidate['info_sha']
            if sha not in reads:
                arguments = (candidate['repository'], candidate['revision'], candidate['info_path'], sha)
                reads[sha] = executor.submit(_read_info, client, arguments)
        documents = {sha: future.result() for sha, future in reads.items()}

    positions = {id(group): index for index, group in enumerate(groups) if group['listed']}
    candidates.sort(key=lambda item: (0, positions[id(item['group'])], item['path']) if item['group']['listed']
                    else (1, item['repository'].lower(), item['path']))
    canonical = {}
    for candidate in candidates:
        document = documents[candidate['info_sha']]
        if document is None:
            rejected += 1
            continue
        name = candidate['board'] or document.get('board') or candidate['repository'].split('/')[-1]
        if not isinstance(name, str) or not re.fullmatch(r'[A-Za-z0-9_]+', name):
            rejected += 1
            continue
        group = candidate['group']
        group['raw_count'] += 1
        origin = {field: candidate[field] for field in ('repository', 'path', 'revision')}
        origin['tree_sha'] = candidate['tree_sha']
        origin['files'] = candidate['files']
        origin['info'] = {key: str(document[key]).strip() for key in ('board', 'manufacturer', 'description', 'version')
                          if isinstance(document.get(key), (str, int, float)) and not isinstance(document[key], bool)}
        origin['url'] = 'https://github.com/{}/tree/{}{}'.format(
            candidate['repository'], quote(candidate['revision'], safe=''),
            '/' + quote(candidate['path'], safe='/') if candidate['path'] else '')
        sha = candidate['tree_sha']
        if sha in canonical:
            canonical[sha]['origins'].append(origin)
            continue
        definition = {'board': name, 'path': candidate['path'], 'chip': document['chip'].strip(),
                      'tree_sha': sha, 'origins': [origin]}
        canonical[sha] = definition
        group['definitions'].append(definition)
        group['boards'].append(name)
        group['count'] += 1

    listed = [group for group in groups if group['listed']]
    for group in listed:
        if group['raw_count'] == 0:
            raise BoardDiscoveryError('No valid board definitions in configured source: {}/{}'.format(
                group['repository'], group['path']))
    discovered = sorted((group for group in groups if not group['listed']),
                        key=lambda group: (group['repository'].lower(), group['path']))
    raw_count = sum(group['raw_count'] for group in groups)
    listed_count = sum(group['count'] for group in listed)
    result = {'count': len(canonical), 'packs': listed + discovered, 'raw_count': raw_count,
            'listed_count': listed_count, 'discovered_count': len(canonical) - listed_count,
            'duplicates_removed': raw_count - len(canonical), 'rejected_candidates': rejected,
            'search_query': SEARCH_QUERY, 'search_matches': len(matches)}
    return consolidate_board_sources(result, lineages) if lineages else result
