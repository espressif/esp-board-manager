# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from generators.config_generator import (
    BoardDirectory,
    UnknownBoardListFilterError,
    filter_indexed_boards,
    format_board_groups,
    indexed_board_infos,
)


def _board(name, source='espressif/esp_boards', source_type='component', chip='esp32s3'):
    return BoardDirectory(
        name=name,
        path=f'/tmp/{name}',
        source=source,
        source_type=source_type,
        chip=chip,
    )


def test_indexed_board_infos_assigns_global_indices_in_list_order():
    boards = {
        'zeta': _board('zeta', source='espressif/m5stack_boards', chip='esp32s3'),
        'alpha': _board('alpha', source='espressif/esp_boards', chip='esp32c3'),
        'beta': _board('beta', source='customer', source_type='customer', chip='esp32s3'),
    }

    indexed = indexed_board_infos(boards)

    assert [(idx, info.name) for idx, info in indexed] == [
        (1, 'alpha'),
        (2, 'zeta'),
        (3, 'beta'),
    ]


def test_filter_indexed_boards_keeps_global_indices_for_chip_and_component():
    indexed = indexed_board_infos({
        'c3_board': _board('c3_board', chip='esp32c3'),
        's3_board': _board('s3_board', chip='esp32s3'),
        's3_m5': _board('s3_m5', source='espressif/m5stack_boards', chip='esp32s3'),
    })

    by_chip = filter_indexed_boards(indexed, 'ESP32S3')
    assert [(idx, info.name) for idx, info in by_chip] == [
        (2, 's3_board'),
        (3, 's3_m5'),
    ]

    by_short = filter_indexed_boards(indexed, 'esp_boards')
    assert [(idx, info.name) for idx, info in by_short] == [
        (1, 'c3_board'),
        (2, 's3_board'),
    ]

    by_full = filter_indexed_boards(indexed, 'espressif/m5stack_boards')
    assert [(idx, info.name) for idx, info in by_full] == [(3, 's3_m5')]


def test_unknown_list_filter_raises_with_available_tokens():
    indexed = indexed_board_infos({
        's3_board': _board('s3_board', chip='esp32s3'),
    })

    with pytest.raises(UnknownBoardListFilterError, match='esp32s33') as excinfo:
        filter_indexed_boards(indexed, 'esp32s33')

    message = str(excinfo.value)
    assert 'Available chips: esp32s3' in message
    assert 'Available components: espressif/esp_boards' in message


def test_format_board_groups_prints_provided_indices_with_gaps():
    lines = format_board_groups([
        (12, _board('esp32_s3_box_3')),
        (19, _board('esp32_s3_eye')),
    ])

    assert '    [12] esp32_s3_box_3' in lines
    assert '    [19] esp32_s3_eye' in lines
    assert '[1]' not in '\n'.join(lines)


def test_board_directory_chip_defaults_to_none():
    info = BoardDirectory(
        name='legacy',
        path='/tmp/legacy',
        source='test',
        source_type='component',
    )
    assert info.chip is None


def _write_board(path: Path, chip: str) -> None:
    path.mkdir(parents=True)
    (path / 'board_info.yaml').write_text(
        f'board: {path.name}\nchip: {chip}\n',
        encoding='utf-8',
    )
    (path / 'board_peripherals.yaml').write_text('peripherals: []\n', encoding='utf-8')
    (path / 'board_devices.yaml').write_text('devices: []\n', encoding='utf-8')


def test_record_board_directory_caches_chip_from_board_info(tmp_path):
    from generators.config_generator import ConfigGenerator

    board_path = tmp_path / 'esp32_s3_box_3'
    _write_board(board_path, 'esp32s3')
    generator = ConfigGenerator(tmp_path)
    boards = {}
    generator._record_board_directory(
        boards,
        'esp32_s3_box_3',
        str(board_path),
        'component',
        'espressif/esp_boards',
    )

    assert boards[str(board_path.resolve())].chip == 'esp32s3'


def test_record_board_directory_allows_missing_chip(tmp_path):
    from generators.config_generator import ConfigGenerator

    board_path = tmp_path / 'no_chip_board'
    board_path.mkdir()
    (board_path / 'board_info.yaml').write_text('board: no_chip_board\n', encoding='utf-8')
    (board_path / 'board_peripherals.yaml').write_text('peripherals: []\n', encoding='utf-8')
    (board_path / 'board_devices.yaml').write_text('devices: []\n', encoding='utf-8')
    generator = ConfigGenerator(tmp_path)
    boards = {}
    generator._record_board_directory(
        boards,
        'no_chip_board',
        str(board_path),
        'component',
        'espressif/esp_boards',
    )

    assert boards[str(board_path.resolve())].chip is None
