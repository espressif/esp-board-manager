"""
Test board selection functionality
Tests -b parameter and positional argument with names, indices, and error cases
"""

import re
import subprocess

import pytest


def test_same_name_board_switch_uses_path_for_sdkconfig_preservation(tmp_path, script_path):
    project = tmp_path / 'project'
    project.mkdir()
    first = tmp_path / 'first' / 'same_board'
    second = tmp_path / 'second' / 'same_board'
    _write_minimal_board(first, 'first definition')
    _write_minimal_board(second, 'second definition')
    sdkconfig = project / 'sdkconfig'

    def generate(board):
        result = subprocess.run(
            ['python3', str(script_path), '-b', str(board), '--project-dir', str(project)],
            cwd=str(project), capture_output=True, text=True,
        )
        assert result.returncode == 0, result.stdout + result.stderr

    generate(first)
    original = 'CONFIG_IDF_TARGET="esp32s3"\nCONFIG_PROJECT_MARKER=y\n'
    sdkconfig.write_text(original, encoding='utf-8')
    generate(first)
    preserved = sdkconfig.read_text(encoding='utf-8')
    assert 'CONFIG_PROJECT_MARKER=y' in preserved
    generate(second)
    backup = project / 'components' / 'gen_bmgr_codes' / 'sdkconfig.bmgr_board.old'
    assert backup.read_text(encoding='utf-8') == preserved
    assert not sdkconfig.exists() or 'CONFIG_PROJECT_MARKER=y' not in sdkconfig.read_text(encoding='utf-8')


@pytest.mark.parametrize('conflicting', [False, True])
def test_revision_component_dependencies_reach_generated_manifest(tmp_path, script_path, conflicting):
    import yaml

    project = tmp_path / 'project'
    project.mkdir()
    board = tmp_path / 'dependency_board'
    _write_minimal_board(board, 'revision dependencies')
    board.joinpath('board_devices.yaml').write_text(
        'devices:\n  - name: base_device\n    type: power_ctrl\n    sub_type: custom\n'
        '    dependencies:\n      example/base_driver: "1.0.0"\n', encoding='utf-8',
    )
    board.joinpath('board_amend.yaml').write_text(
        'board_revisions:\n  new_revision:\n    - revision.yaml\n', encoding='utf-8',
    )
    component = 'example/base_driver' if conflicting else 'example/revision_driver'
    board.joinpath('revision.yaml').write_text(
        'devices:\n  - name: added_device\n    type: power_ctrl\n    sub_type: custom\n'
        f'    dependencies:\n      {component}: "2.0.0"\n', encoding='utf-8',
    )
    result = subprocess.run(
        ['python3', str(script_path), '-b', str(board), '--project-dir', str(project)],
        cwd=str(project), capture_output=True, text=True,
    )
    if conflicting:
        assert result.returncode != 0
        assert 'Conflicting dependency' in result.stdout + result.stderr
    else:
        assert result.returncode == 0, result.stdout + result.stderr
        generated = project / 'components' / 'gen_bmgr_codes'
        manifest = yaml.safe_load((generated / 'idf_component.yml').read_text(encoding='utf-8'))
        assert manifest['dependencies']['example/revision_driver'] == '2.0.0'
        assert manifest['dependencies']['example/base_driver'] == '1.0.0'
        kconfig = (generated / 'Kconfig.projbuild').read_text(encoding='utf-8')
        assert 'range 2 4096\n    default 2' in kconfig


def _write_minimal_board(board_path, marker):
    board_path.mkdir(parents=True)
    board_path.joinpath('board_info.yaml').write_text(
        f'board: {board_path.name}\nchip: esp32s3\ndescription: {marker}\n',
        encoding='utf-8',
    )
    board_path.joinpath('board_peripherals.yaml').write_text('peripherals: []\n', encoding='utf-8')
    board_path.joinpath('board_devices.yaml').write_text('devices: []\n', encoding='utf-8')


def _parse_customer_board_index(list_output: str, source_label: str, board_name: str) -> int:
    """Return the global list index for ``board_name`` under a customer source header."""
    lines = list_output.splitlines()
    in_group = False
    for line in lines:
        stripped = line.strip()
        if stripped.endswith(':') and source_label in stripped:
            in_group = True
            continue
        if in_group and stripped.endswith(':') and source_label not in stripped:
            in_group = False
        if in_group:
            match = re.match(rf'\[(\d+)\] {re.escape(board_name)}$', stripped)
            if match:
                return int(match.group(1))
    raise AssertionError(
        f'Board [{board_name}] under source {source_label!r} not found in list output:\n{list_output}'
    )


@pytest.mark.parametrize('from_amend', [False, True])
def test_cli_rejects_device_peripheral_name_collision(tmp_path, script_path, from_amend):
    project_dir = tmp_path / 'project'
    project_dir.mkdir()
    board_path = tmp_path / 'collision_board'
    _write_minimal_board(board_path, 'collision test')
    board_path.joinpath('board_devices.yaml').write_text(
        'devices:\n  - name: shared_name\n    type: custom\n', encoding='utf-8'
    )
    periph_yaml = 'peripherals:\n  - name: shared_name\n    type: custom\n'
    args = []
    if from_amend:
        amend_dir = tmp_path / 'amend'
        amend_dir.mkdir()
        amend_dir.joinpath('board_amend.yaml').write_text(
            'version: "1.0"\napply:\n  - collision.yaml\n', encoding='utf-8'
        )
        amend_dir.joinpath('collision.yaml').write_text(periph_yaml, encoding='utf-8')
        args = ['-a', str(amend_dir)]
    else:
        board_path.joinpath('board_peripherals.yaml').write_text(periph_yaml, encoding='utf-8')

    result = subprocess.run(
        ['python3', str(script_path), '-b', str(board_path), '--project-dir',
         str(project_dir), '--kconfig-only', *args],
        cwd=str(project_dir), capture_output=True, text=True,
    )
    assert result.returncode != 0
    assert 'Device and peripheral names must be distinct: shared_name' in result.stdout + result.stderr


def test_cli_collision_preflight_ignores_skipped_invalid_names(tmp_path, script_path):
    project_dir = tmp_path / 'project'
    project_dir.mkdir()
    board_path = tmp_path / 'skip_collision_board'
    _write_minimal_board(board_path, 'skipped collision preflight')
    board_path.joinpath('board_devices.yaml').write_text(
        'devices:\n  - name: InvalidLegacyName\n    type: custom\n    gen_skip: true\n',
        encoding='utf-8',
    )
    board_path.joinpath('board_peripherals.yaml').write_text(
        'peripherals:\n  - name: InvalidLegacyName\n    type: custom\n    gen_skip: true\n',
        encoding='utf-8',
    )

    result = subprocess.run(
        ['python3', str(script_path), '-b', str(board_path), '--project-dir',
         str(project_dir), '--kconfig-only'],
        cwd=str(project_dir), capture_output=True, text=True,
    )

    assert result.returncode == 0, result.stdout + result.stderr
    assert 'Invalid component name format' not in result.stdout + result.stderr


@pytest.mark.parametrize('selector', ['option', 'positional'])
def test_cli_customer_indices_match_listed_board_paths(tmp_path, script_path, selector):
    project_dir = tmp_path / 'project'
    project_dir.mkdir()
    customer_root = tmp_path / 'customer_boards'
    for name in ('selection_first', 'selection_second'):
        _write_minimal_board(customer_root / name, name)

    common_args = ['-c', str(customer_root), '--project-dir', str(project_dir)]
    listed = subprocess.run(
        ['python3', str(script_path), '-l', *common_args],
        cwd=str(project_dir), capture_output=True, text=True,
    )
    assert listed.returncode == 0, listed.stdout + listed.stderr
    for name in ('selection_first', 'selection_second'):
        index = _parse_customer_board_index(listed.stdout, 'customer_boards', name)
        board_arg = ['-b', str(index)] if selector == 'option' else [str(index)]
        result = subprocess.run(
            ['python3', str(script_path), *board_arg, *common_args, '--kconfig-only'],
            cwd=str(project_dir), capture_output=True, text=True,
        )
        assert result.returncode == 0, result.stdout + result.stderr
        assert f'Board path: {(customer_root / name).resolve()}' in result.stdout


def test_cli_index_selects_listed_record_not_last_same_name(tmp_path, script_path):
    project_dir = tmp_path / 'project'
    project_dir.mkdir()
    root_a = tmp_path / 'boards_a'
    root_b = tmp_path / 'boards_b'
    board_name = 'foo'
    board_a = root_a / board_name
    board_b = root_b / board_name

    _write_minimal_board(board_a, 'customer root a')
    _write_minimal_board(board_b, 'customer root b')

    customer_paths = f'{root_a};{root_b}'
    list_result = subprocess.run(
        [
            'python3',
            str(script_path),
            '-l',
            '-c',
            customer_paths,
            '--project-dir',
            str(project_dir),
        ],
        cwd=str(project_dir),
        capture_output=True,
        text=True,
    )
    assert list_result.returncode == 0, list_result.stdout + list_result.stderr
    list_output = list_result.stdout + list_result.stderr
    low_index = _parse_customer_board_index(list_output, 'boards_a', board_name)

    index_result = subprocess.run(
        [
            'python3',
            str(script_path),
            '-b',
            str(low_index),
            '-c',
            customer_paths,
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
    assert index_result.returncode == 0, index_result.stdout + index_result.stderr
    assert f'✅ Board path: {board_a.resolve()}' in index_result.stdout
    assert f'Peripherals: {board_a.resolve() / "board_peripherals.yaml"}' in index_result.stdout
    assert f'Peripherals: {board_b.resolve() / "board_peripherals.yaml"}' not in index_result.stdout

    name_result = subprocess.run(
        [
            'python3',
            str(script_path),
            '-b',
            board_name,
            '-c',
            customer_paths,
            '--project-dir',
            str(project_dir),
            '--kconfig-only',
        ],
        cwd=str(project_dir),
        capture_output=True,
        text=True,
    )
    assert name_result.returncode == 0, name_result.stdout + name_result.stderr
    assert f'✅ Board path: {board_b.resolve()}' in name_result.stdout


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

    matches = [info for info in boards.values() if info.name == board_name]
    assert [info.path for info in matches][-1] == str(customer_board.resolve())
    assert matches[-1].source_type == 'customer'
    assert len(matches) == 2


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

    matches = [info for info in boards.values() if info.name == board_name]
    assert [info.path for info in matches][-1] == str(component_board.resolve())
    assert matches[-1].source == 'local_boards'
    assert len(matches) == 2


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
