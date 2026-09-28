# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from gen_bmgr_config_codes import resolve_board_name_or_index
from generators.config_generator import (
    AmbiguousBoardSelectorError,
    BoardDirectory,
    ConfigGenerator,
    UnknownBoardListFilterError,
    filter_indexed_boards,
    format_board_groups,
    indexed_board_infos,
    select_last_named_board,
)


@pytest.mark.parametrize('metadata', ['plain scalar\n', '- list item\n'])
def test_scan_accepts_non_mapping_board_info_without_crashing(tmp_path, metadata):
    board = tmp_path / 'boards' / 'invalid_metadata'
    _write_board(board)
    (board / 'board_info.yaml').write_text(metadata, encoding='utf-8')
    generator = ConfigGenerator(tmp_path / 'bmgr')
    boards = generator.scan_board_directories(str(board.parent))
    assert boards[str(board.resolve())].chip is None


def test_repeated_customer_path_keeps_last_scan_priority(tmp_path):
    project = tmp_path / 'project'
    first = project / 'components' / 'local_boards' / 'same_board'
    second = tmp_path / 'boards' / 'same_board'
    _write_board(first)
    _write_board(second)
    generator = ConfigGenerator(tmp_path / 'bmgr', project_dir=str(project))
    boards = generator.scan_board_directories(f'{second.parent};{first}')
    assert len(boards) == 2
    assert select_last_named_board(boards, 'same_board').path == str(first.resolve())


def _info(name, path, source, source_type='customer', chip='esp32s3'):
    return BoardDirectory(
        name=name,
        path=path,
        source=source,
        source_type=source_type,
        chip=chip,
    )


def _write_board(path: Path, chip: str = 'esp32s3') -> None:
    path.mkdir(parents=True)
    (path / 'board_info.yaml').write_text(
        f'board: {path.name}\nchip: {chip}\n',
        encoding='utf-8',
    )
    (path / 'board_peripherals.yaml').write_text('peripherals: []\n', encoding='utf-8')
    (path / 'board_devices.yaml').write_text('devices: []\n', encoding='utf-8')


def test_scan_keeps_same_named_boards_from_different_directories(tmp_path):
    project = tmp_path / 'project'
    first = project / 'components' / 'local_boards' / 'same_board'
    second = tmp_path / 'customer_boards' / 'same_board'
    _write_board(first)
    _write_board(second)
    generator = ConfigGenerator(tmp_path / 'bmgr', project_dir=str(project))

    boards = generator.scan_board_directories(str(second.parent))

    paths = [info.path for info in boards.values() if info.name == 'same_board']
    assert paths == [str(first.resolve()), str(second.resolve())]
    assert list(boards) == paths


def test_customer_source_uses_root_basename_and_relative_parent(tmp_path):
    project = tmp_path / 'project'
    project.mkdir()
    boards_root = tmp_path / 'boards'
    _write_board(boards_root / 'espressif' / 'esp32_s3_devkitc')
    _write_board(boards_root / 'm5stack' / 'esp32_s3_devkitc')
    _write_board(boards_root / 'esp32_p4_core')
    generator = ConfigGenerator(tmp_path / 'bmgr', project_dir=str(project))

    boards = generator.scan_board_directories(str(boards_root))
    by_path = {info.path: info for info in boards.values()}

    assert by_path[str((boards_root / 'espressif' / 'esp32_s3_devkitc').resolve())].source == 'boards/espressif'
    assert by_path[str((boards_root / 'm5stack' / 'esp32_s3_devkitc').resolve())].source == 'boards/m5stack'
    assert by_path[str((boards_root / 'esp32_p4_core').resolve())].source == 'boards'
    assert all(info.source_type == 'customer' for info in by_path.values())


def test_multiple_customer_roots_keep_distinct_basenames(tmp_path):
    project = tmp_path / 'project'
    project.mkdir()
    root_a = tmp_path / 'boards_a'
    root_b = tmp_path / 'boards_b'
    _write_board(root_a / 'espressif' / 'foo')
    _write_board(root_b / 'espressif' / 'foo')
    generator = ConfigGenerator(tmp_path / 'bmgr', project_dir=str(project))

    boards = generator.scan_board_directories(f'{root_a};{root_b}')
    sources = [info.source for info in boards.values() if info.name == 'foo']

    assert sources == ['boards_a/espressif', 'boards_b/espressif']


def test_customer_source_when_dash_c_is_the_board_directory(tmp_path):
    project = tmp_path / 'project'
    project.mkdir()
    board = tmp_path / 'esp32_p4_core'
    _write_board(board)
    generator = ConfigGenerator(tmp_path / 'bmgr', project_dir=str(project))

    boards = generator.scan_board_directories(str(board))
    info = boards[str(board.resolve())]

    assert info.source == 'esp32_p4_core'
    assert info.source_type == 'customer'
    assert info.name == 'esp32_p4_core'


def test_customer_board_symlink_outside_root_does_not_abort_scan(tmp_path):
    project = tmp_path / 'project'
    project.mkdir()
    boards_root = tmp_path / 'boards'
    target = tmp_path / 'external' / 'linked_board'
    _write_board(target)
    boards_root.mkdir()
    (boards_root / 'linked_board').symlink_to(target, target_is_directory=True)
    generator = ConfigGenerator(tmp_path / 'bmgr', project_dir=str(project))

    boards = generator.scan_board_directories(str(boards_root))

    info = boards[str(target.resolve())]
    assert info.name == 'linked_board'
    assert info.source == 'boards'


def test_format_customer_groups_print_source_headers():
    lines = format_board_groups([
        (2, _info('esp32_s3_devkitc', '/tmp/boards/espressif/esp32_s3_devkitc', 'boards/espressif')),
        (3, _info('esp32_s3_devkitc', '/tmp/boards/m5stack/esp32_s3_devkitc', 'boards/m5stack')),
    ])
    text = '\n'.join(lines)
    assert 'Customer Boards:' in text
    assert '  boards/espressif:' in text
    assert '    [2] esp32_s3_devkitc' in text
    assert '  boards/m5stack:' in text
    assert '    [3] esp32_s3_devkitc' in text


def test_format_customer_groups_disambiguate_only_when_source_collides(tmp_path):
    project = tmp_path / 'project'
    project.mkdir()
    left = tmp_path / 'a' / 'boards' / 'espressif' / 'foo'
    right = tmp_path / 'b' / 'boards' / 'espressif' / 'foo'
    lines = format_board_groups(
        [
            (2, _info('foo', str(left), 'boards/espressif')),
            (3, _info('foo', str(right), 'boards/espressif')),
        ],
        project_root=str(project),
    )
    text = '\n'.join(lines)
    assert 'boards/espressif (../a/boards):' in text
    assert 'boards/espressif (../b/boards):' in text
    assert '  boards/espressif:\n' not in text + '\n'


def test_format_customer_groups_tolerates_unrecoverable_root():
    lines = format_board_groups([
        (1, _info('shallow', '/tmp/board', 'boards/vendor/deep')),
    ])
    text = '\n'.join(lines)
    assert 'Customer Boards:' in text
    assert '  boards/vendor/deep:' in text
    assert '    [1] shallow' in text


def test_format_customer_groups_single_board_root_uses_source_header():
    lines = format_board_groups(
        [(1, _info('esp32_p4_core', '/tmp/proj/esp32_p4_core', 'esp32_p4_core'))],
        project_root='/tmp/other',
    )
    text = '\n'.join(lines)
    assert '  esp32_p4_core:' in text
    assert '    [1] esp32_p4_core' in text


def test_list_filter_customer_source_full_and_short_name():
    indexed = [
        (1, _info('c3', '/tmp/c3', 'espressif/esp_boards', source_type='component', chip='esp32c3')),
        (2, _info('s3', '/tmp/boards/espressif/s3', 'boards/espressif')),
        (3, _info('core', '/tmp/boards/core', 'boards')),
    ]

    assert [idx for idx, _ in filter_indexed_boards(indexed, 'boards/espressif')] == [2]
    assert [idx for idx, _ in filter_indexed_boards(indexed, 'espressif')] == [2]
    assert [idx for idx, _ in filter_indexed_boards(indexed, 'esp_boards')] == [1]
    assert [idx for idx, _ in filter_indexed_boards(indexed, 'boards')] == [3]
    assert [idx for idx, _ in filter_indexed_boards(indexed, 'customer')] == [2, 3]


def test_list_filter_collision_returns_every_matching_record():
    indexed = [
        (2, _info('foo', '/tmp/a/boards/espressif/foo', 'boards/espressif')),
        (3, _info('foo', '/tmp/b/boards/espressif/foo', 'boards/espressif')),
    ]
    assert [idx for idx, _ in filter_indexed_boards(indexed, 'boards/espressif')] == [2, 3]


def test_list_filter_ignores_disambiguation_suffix():
    indexed = [
        (2, _info('foo', '/tmp/a/boards/espressif/foo', 'boards/espressif')),
    ]
    with pytest.raises(UnknownBoardListFilterError):
        filter_indexed_boards(indexed, 'boards/espressif (../a/boards)')


class _Gen:
    def __init__(self, config_generator):
        self.config_generator = config_generator


def test_resolve_plain_name_picks_last_scanned_record(tmp_path):
    project = tmp_path / 'project'
    low = project / 'components' / 'local_boards' / 'foo'
    high = tmp_path / 'boards_b' / 'foo'
    _write_board(low)
    _write_board(high)
    config = ConfigGenerator(tmp_path / 'bmgr', project_dir=str(project))
    boards = config.scan_board_directories(str(high.parent))

    selected = resolve_board_name_or_index('foo', boards, _Gen(config))

    assert selected.path == str(high.resolve())


def test_resolve_index_picks_the_listed_record_not_the_highest_priority(tmp_path):
    project = tmp_path / 'project'
    low = project / 'components' / 'local_boards' / 'foo'
    high = tmp_path / 'boards_b' / 'foo'
    _write_board(low)
    _write_board(high)
    config = ConfigGenerator(tmp_path / 'bmgr', project_dir=str(project))
    boards = config.scan_board_directories(str(high.parent))
    low_index = next(idx for idx, info in indexed_board_infos(boards) if info.path == str(low.resolve()))

    selected = resolve_board_name_or_index(str(low_index), boards, _Gen(config))

    assert selected.path == str(low.resolve())


def test_resolve_qualified_source_picks_one_board(tmp_path):
    project = tmp_path / 'project'
    project.mkdir()
    board_a = tmp_path / 'boards_a' / 'espressif' / 'foo'
    board_b = tmp_path / 'boards_b' / 'espressif' / 'foo'
    _write_board(board_a)
    _write_board(board_b)
    config = ConfigGenerator(tmp_path / 'bmgr', project_dir=str(project))
    boards = config.scan_board_directories(f'{board_a.parents[1]};{board_b.parents[1]}')

    selected = resolve_board_name_or_index('boards_a/espressif/foo', boards, _Gen(config))

    assert selected.path == str(board_a.resolve())


def test_resolve_ambiguous_qualified_source_raises(tmp_path):
    project = tmp_path / 'project'
    project.mkdir()
    left = tmp_path / 'a' / 'boards' / 'espressif' / 'foo'
    right = tmp_path / 'b' / 'boards' / 'espressif' / 'foo'
    _write_board(left)
    _write_board(right)
    config = ConfigGenerator(tmp_path / 'bmgr', project_dir=str(project))
    boards = config.scan_board_directories(f'{left.parents[1]};{right.parents[1]}')

    with pytest.raises(AmbiguousBoardSelectorError, match='matches more than one board'):
        resolve_board_name_or_index('boards/espressif/foo', boards, _Gen(config))
