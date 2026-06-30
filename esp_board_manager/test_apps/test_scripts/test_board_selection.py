"""
Test board selection functionality
Tests -b parameter and positional argument with names, indices, and error cases
"""

import subprocess

import pytest


def _write_minimal_board(board_path, marker):
    board_path.mkdir(parents=True)
    board_path.joinpath('board_info.yaml').write_text(
        f'board: {board_path.name}\nchip: esp32s3\ndescription: {marker}\n',
        encoding='utf-8',
    )
    board_path.joinpath('board_peripherals.yaml').write_text('peripherals: []\n', encoding='utf-8')
    board_path.joinpath('board_devices.yaml').write_text('devices: []\n', encoding='utf-8')


def test_scan_board_directories_later_sources_override_same_named_boards(tmp_path, bmgr_root):
    import sys
    sys.path.insert(0, str(bmgr_root))
    from generators.config_generator import ConfigGenerator

    bmgr_root_dir = tmp_path / 'bmgr'
    project_dir = tmp_path / 'project'
    board_name = 'same_board'
    builtin_board = bmgr_root_dir / 'boards' / board_name
    component_board = project_dir / 'components' / 'board_component' / 'boards' / board_name
    customer_board = tmp_path / 'customer_boards' / board_name

    _write_minimal_board(builtin_board, 'builtin')
    _write_minimal_board(component_board, 'component')
    _write_minimal_board(customer_board, 'customer')

    generator = ConfigGenerator(bmgr_root_dir, project_dir=str(project_dir))
    boards = generator.scan_board_directories(str(customer_board.parent))

    assert boards[board_name].path == str(customer_board.resolve())
    assert boards[board_name].source_type == 'customer'


def test_project_components_override_managed_components_for_same_named_boards(tmp_path, bmgr_root):
    import sys
    sys.path.insert(0, str(bmgr_root))
    from generators.config_generator import ConfigGenerator

    project_dir = tmp_path / 'project'
    board_name = 'same_board'
    managed_board = project_dir / 'managed_components' / 'espressif__esp_boards' / board_name
    component_board = project_dir / 'components' / 'local_boards' / board_name

    _write_minimal_board(managed_board, 'managed')
    _write_minimal_board(component_board, 'component')

    generator = ConfigGenerator(tmp_path / 'bmgr', project_dir=str(project_dir))
    boards = generator.scan_board_directories()

    assert boards[board_name].path == str(component_board.resolve())
    assert boards[board_name].source == 'local_boards'


def test_cli_same_named_board_priority_matches_real_scan_order(tmp_path, script_path):
    project_dir = tmp_path / 'project'
    customer_root = tmp_path / 'customer_boards'
    board_name = 'esp_box_3'
    component_board = project_dir / 'components' / 'priority_component' / 'boards' / board_name
    customer_board = customer_root / board_name

    _write_minimal_board(component_board, 'component shadows built-in')
    _write_minimal_board(customer_board, 'customer shadows component')

    result = subprocess.run(
        [
            'python3',
            str(script_path),
            '-b',
            board_name,
            '-c',
            str(customer_root),
            '--project-dir',
            str(project_dir),
            '--kconfig-only',
            '--log-level',
            'DEBUG',
        ],
        cwd=str(project_dir),
        capture_output=True,
        text=True,
    )

    assert result.returncode == 0, result.stdout + result.stderr
    assert f'Duplicate board "{board_name}"' in result.stdout
    assert f'✅ Board path: {customer_board.resolve()}' in result.stdout
    assert f'Peripherals: {customer_board.resolve() / "board_peripherals.yaml"}' in result.stdout
    assert f'Devices: {customer_board.resolve() / "board_devices.yaml"}' in result.stdout


def test_find_board_config_files_accepts_board_directory_entries(tmp_path, bmgr_root):
    import sys
    sys.path.insert(0, str(bmgr_root))
    from generators.config_generator import BoardDirectory, ConfigGenerator

    board_path = tmp_path / 'test_board'
    board_path.mkdir()
    periph_yaml = board_path / 'board_peripherals.yaml'
    dev_yaml = board_path / 'board_devices.yaml'
    periph_yaml.write_text('peripherals: []\n', encoding='utf-8')
    dev_yaml.write_text('devices: []\n', encoding='utf-8')

    generator = ConfigGenerator(tmp_path)
    all_boards = {
        'test_board': BoardDirectory(
            name='test_board',
            path=str(board_path),
            source='test',
            source_type='component',
        )
    }

    assert generator.find_board_config_files('test_board', all_boards) == (
        str(periph_yaml),
        str(dev_yaml),
    )


def test_find_board_config_files_still_accepts_legacy_path_entries(tmp_path, bmgr_root):
    import sys
    sys.path.insert(0, str(bmgr_root))
    from generators.config_generator import ConfigGenerator

    board_path = tmp_path / 'test_board'
    board_path.mkdir()
    periph_yaml = board_path / 'board_peripherals.yaml'
    dev_yaml = board_path / 'board_devices.yaml'
    periph_yaml.write_text('peripherals: []\n', encoding='utf-8')
    dev_yaml.write_text('devices: []\n', encoding='utf-8')

    generator = ConfigGenerator(tmp_path)

    assert generator.find_board_config_files('test_board', {'test_board': str(board_path)}) == (
        str(periph_yaml),
        str(dev_yaml),
    )


class TestBoardSelectionByName:
    """Test board selection using board names"""

    def test_select_valid_board_by_name(self, run_bmgr_cmd, valid_board):
        """Test selecting a valid board by name using -b"""
        result = run_bmgr_cmd(['-b', valid_board, '--kconfig-only'])
        assert result.returncode == 0
        assert valid_board in result.stdout
        assert 'Resolved board:' in result.stdout or 'Using board from command line:' in result.stdout

    def test_select_valid_board_by_name_positional(self, run_bmgr_cmd, valid_board):
        """Test selecting a valid board by name using positional argument"""
        result = run_bmgr_cmd([valid_board, '--kconfig-only'])
        assert result.returncode == 0
        assert valid_board in result.stdout
        assert 'Resolved board:' in result.stdout or 'Using board from command line:' in result.stdout

    def test_select_another_board(self, run_bmgr_cmd, board_list):
        """Test selecting another valid board using -b"""
        if len(board_list) >= 2:
            board = board_list[1]
            result = run_bmgr_cmd(['-b', board, '--kconfig-only'])
            assert result.returncode == 0
            assert board in result.stdout

    def test_select_another_board_positional(self, run_bmgr_cmd, board_list):
        """Test selecting another valid board using positional argument"""
        if len(board_list) >= 2:
            board = board_list[1]
            result = run_bmgr_cmd([board, '--kconfig-only'])
            assert result.returncode == 0
            assert board in result.stdout

    def test_invalid_board_name(self, run_bmgr_cmd):
        """Test selecting a non-existent board using -b"""
        result = run_bmgr_cmd(['-b', 'invalid_board_xyz', '--kconfig-only'], check=False)
        assert result.returncode != 0
        assert 'not found' in result.stdout or 'not found' in result.stderr

    def test_invalid_board_name_positional(self, run_bmgr_cmd):
        """Test selecting a non-existent board using positional argument"""
        result = run_bmgr_cmd(['invalid_board_xyz', '--kconfig-only'], check=False)
        assert result.returncode != 0
        assert 'not found' in result.stdout or 'not found' in result.stderr


class TestBoardSelectionByIndex:
    """Test board selection using numeric indices"""

    def test_select_board_by_index_1(self, run_bmgr_cmd):
        """Test selecting board by index 1 (first board) using -b"""
        result = run_bmgr_cmd(['-b', '1', '--kconfig-only'])
        assert result.returncode == 0
        assert 'Resolved board:' in result.stdout

    def test_select_board_by_index_1_positional(self, run_bmgr_cmd):
        """Test selecting board by index 1 (first board) using positional argument"""
        result = run_bmgr_cmd(['1', '--kconfig-only'])
        assert result.returncode == 0
        assert 'Resolved board:' in result.stdout

    def test_select_board_by_index_2(self, run_bmgr_cmd):
        """Test selecting board by index 2 using -b"""
        result = run_bmgr_cmd(['-b', '2', '--kconfig-only'])
        assert result.returncode == 0
        assert 'Resolved board:' in result.stdout

    def test_select_board_by_index_2_positional(self, run_bmgr_cmd):
        """Test selecting board by index 2 using positional argument"""
        result = run_bmgr_cmd(['2', '--kconfig-only'])
        assert result.returncode == 0
        assert 'Resolved board:' in result.stdout

    def test_select_last_board(self, run_bmgr_cmd, board_count):
        """Test selecting the last board by index using -b"""
        result = run_bmgr_cmd(['-b', str(board_count), '--kconfig-only'])
        assert result.returncode == 0
        assert 'Resolved board:' in result.stdout

    def test_select_last_board_positional(self, run_bmgr_cmd, board_count):
        """Test selecting the last board by index using positional argument"""
        result = run_bmgr_cmd([str(board_count), '--kconfig-only'])
        assert result.returncode == 0
        assert 'Resolved board:' in result.stdout

    @pytest.mark.parametrize('invalid_index', ['0', '999', '-1', '-5'])
    def test_invalid_indices(self, run_bmgr_cmd, invalid_index):
        """Test invalid board indices using -b"""
        result = run_bmgr_cmd(['-b', invalid_index, '--kconfig-only'], check=False)
        assert result.returncode != 0
        assert ('out of range' in result.stdout or 'out of range' in result.stderr or
                'not found' in result.stdout or 'not found' in result.stderr)

    @pytest.mark.parametrize('invalid_index', ['0', '999'])
    def test_invalid_indices_positional(self, run_bmgr_cmd, invalid_index):
        """Test invalid board indices using positional argument

        Note: Negative indices are not tested as positional arguments because
        they are interpreted as command-line options by the argument parser.
        """
        result = run_bmgr_cmd([invalid_index, '--kconfig-only'], check=False)
        assert result.returncode != 0
        assert ('out of range' in result.stdout or 'out of range' in result.stderr or
                'not found' in result.stdout or 'not found' in result.stderr)


class TestIndexNameCorrespondence:
    """Test that board indices correctly map to board names"""

    def test_index_2_maps_correctly(self, run_bmgr_cmd, board_list):
        """Test that index 2 resolves to the correct board using -b"""
        # Get board at index 2 from list
        expected_board = board_list[1] if len(board_list) >= 2 else None
        if not expected_board:
            pytest.skip('Not enough boards for this test')

        # Select by index 2
        result = run_bmgr_cmd(['-b', '2', '--kconfig-only'])
        assert result.returncode == 0

        # Verify it resolved to the correct board
        assert expected_board in result.stdout

    def test_index_2_maps_correctly_positional(self, run_bmgr_cmd, board_list):
        """Test that index 2 resolves to the correct board using positional argument"""
        # Get board at index 2 from list
        expected_board = board_list[1] if len(board_list) >= 2 else None
        if not expected_board:
            pytest.skip('Not enough boards for this test')

        # Select by index 2
        result = run_bmgr_cmd(['2', '--kconfig-only'])
        assert result.returncode == 0

        # Verify it resolved to the correct board
        assert expected_board in result.stdout

    def test_all_indices_map_correctly(self, run_bmgr_cmd, board_list):
        """Test that all indices map to their corresponding boards using -b"""
        for idx, expected_board in enumerate(board_list[:5], start=1):
            result = run_bmgr_cmd(['-b', str(idx), '--kconfig-only'])
            assert result.returncode == 0
            assert expected_board in result.stdout, f'Index {idx} did not resolve to {expected_board}'

    def test_all_indices_map_correctly_positional(self, run_bmgr_cmd, board_list):
        """Test that all indices map to their corresponding boards using positional argument"""
        for idx, expected_board in enumerate(board_list[:5], start=1):
            result = run_bmgr_cmd([str(idx), '--kconfig-only'])
            assert result.returncode == 0
            assert expected_board in result.stdout, f'Index {idx} did not resolve to {expected_board}'


class TestParameterPriority:
    """Test parameter priority when both positional and -b are provided"""

    def test_option_b_overrides_positional(self, run_bmgr_cmd, board_list):
        """Test that -b parameter overrides positional argument"""
        if len(board_list) >= 2:
            # Use different boards for positional and -b
            positional_board = board_list[0]
            b_option_board = board_list[1]

            # Run with both parameters
            result = run_bmgr_cmd([positional_board, '-b', b_option_board, '--kconfig-only'])
            assert result.returncode == 0
            # Should select the board from -b option
            assert b_option_board in result.stdout

    def test_option_b_overrides_positional_index(self, run_bmgr_cmd):
        """Test that -b parameter overrides positional index"""
        # Use index 1 as positional and index 2 as -b option
        result = run_bmgr_cmd(['1', '-b', '2', '--kconfig-only'])
        assert result.returncode == 0
        assert 'Resolved board:' in result.stdout


class TestEdgeCases:
    """Test edge cases in board selection"""

    def test_empty_board_parameter(self, run_bmgr_cmd):
        """Test empty board parameter using -b"""
        result = run_bmgr_cmd(['-b', '', '--kconfig-only'], check=False)
        assert result.returncode != 0

    def test_empty_board_parameter_positional(self, run_bmgr_cmd):
        """Test empty board parameter using positional argument"""
        result = run_bmgr_cmd(['', '--kconfig-only'], check=False)
        assert result.returncode != 0

    @pytest.mark.parametrize('special_board', [
        'board@#$%',
        'board with spaces',
        'board/with/slash',
        'board\\with\\backslash',
    ])
    def test_board_name_special_characters(self, run_bmgr_cmd, special_board):
        """Test board names with special characters using -b"""
        result = run_bmgr_cmd(['-b', special_board, '--kconfig-only'], check=False)
        assert result.returncode != 0
        assert 'not found' in result.stdout or 'not found' in result.stderr

    @pytest.mark.parametrize('special_board', [
        'board@#$%',
        'board with spaces',
        'board/with/slash',
        'board\\with\\backslash',
    ])
    def test_board_name_special_characters_positional(self, run_bmgr_cmd, special_board):
        """Test board names with special characters using positional argument"""
        result = run_bmgr_cmd([special_board, '--kconfig-only'], check=False)
        assert result.returncode != 0
        assert 'not found' in result.stdout or 'not found' in result.stderr

    def test_very_long_board_name(self, run_bmgr_cmd):
        """Test extremely long board name using -b"""
        long_name = 'a' * 200
        result = run_bmgr_cmd(['-b', long_name, '--kconfig-only'], check=False)
        assert result.returncode != 0

    def test_very_long_board_name_positional(self, run_bmgr_cmd):
        """Test extremely long board name using positional argument"""
        long_name = 'a' * 200
        result = run_bmgr_cmd([long_name, '--kconfig-only'], check=False)
        assert result.returncode != 0
