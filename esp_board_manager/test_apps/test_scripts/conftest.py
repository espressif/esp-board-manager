"""
pytest configuration file for ESP Board Manager tests
Provides fixtures and common utilities
"""

import pytest
import subprocess
import os
from pathlib import Path


@pytest.fixture(scope='session')
def bmgr_root():
    """Get the board manager root directory"""
    test_dir = Path(__file__).parent
    # test_dir = .../test_apps/test_scripts
    # test_dir.parent = .../test_apps
    # test_dir.parent.parent = .../esp_board_manager (board manager root)
    return test_dir.parent.parent


@pytest.fixture(scope='session')
def script_path(bmgr_root):
    """Get the path to gen_bmgr_config_codes.py"""
    return bmgr_root / 'gen_bmgr_config_codes.py'


@pytest.fixture(scope='session')
def test_project_dir(bmgr_root):
    """The ESP-IDF project used as the board discovery context for tests.

    Boards were extracted out of the board manager repository into separate
    board components. ``test_apps/main/idf_component.yml`` declares those
    components (via ``override_path``), so running the generator inside this
    project discovers them as board components.
    """
    return bmgr_root / 'test_apps'


@pytest.fixture
def resolve_board_dir(bmgr_root):
    """Return a resolver that locates a board directory across known sources.

    Boards were split out of the board manager repository into separate board
    components (``esp_boards``, ``esp_friends_boards``, ``m5stack_boards``),
    checked out alongside the repository as declared by the test project's
    ``idf_component.yml`` overrides. Tests that read a board file directly look
    it up across these sources and skip when the component is not available.
    """
    repo_root = bmgr_root.parent
    search_roots = [
        repo_root / 'esp_boards',
        repo_root / 'esp_friends_boards',
        repo_root / 'm5stack_boards',
    ]

    def _resolve(board_name):
        for root in search_roots:
            candidate = root / board_name
            if (candidate / 'board_info.yaml').is_file() or (candidate / 'board_devices.yaml').is_file():
                return candidate
        pytest.skip(f"board '{board_name}' is not available in any known board source")

    return _resolve


@pytest.fixture
def run_bmgr_cmd(script_path, test_project_dir):
    """Fixture to run board manager commands"""
    def _run(args, check=True, cwd=None, env=None):
        """
        Run board manager command with given arguments

        Args:
            args: List of command arguments
            check: Whether to check return code (default True)
            cwd: Working directory for command execution (default: the test
                project so boards are discovered from its component overrides)
            env: Extra environment variables to inject

        Returns:
            subprocess.CompletedProcess object
        """
        cmd = ['python3', str(script_path)] + args
        merged_env = os.environ.copy()
        if env:
            merged_env.update(env)
        result = subprocess.run(
            cmd,
            cwd=str(cwd) if cwd else str(test_project_dir),
            capture_output=True,
            text=True,
            env=merged_env,
        )
        if check and result.returncode != 0:
            print(f"Command failed: {' '.join(cmd)}")
            print(f'STDOUT:\n{result.stdout}')
            print(f'STDERR:\n{result.stderr}')
        return result
    return _run


@pytest.fixture(scope='session')
def board_list(script_path, test_project_dir):
    """Get list of available boards"""
    # Run command directly without depending on run_bmgr_cmd fixture
    cmd = ['python3', str(script_path), '-l']
    result = subprocess.run(
        cmd,
        cwd=str(test_project_dir),
        capture_output=True,
        text=True
    )
    boards = []
    for line in result.stdout.split('\n'):
        if line.strip().startswith('[') and ']' in line:
            # Extract board name from "[1] board_name" format
            parts = line.split(']', 1)
            if len(parts) == 2:
                board_name = parts[1].strip()
                boards.append(board_name)
    return boards


@pytest.fixture(scope='session')
def valid_board(board_list):
    """Get a valid board name for testing"""
    preferred = 'esp32_s3_korvo_2_3'
    if preferred in board_list:
        return preferred
    return board_list[0] if board_list else preferred


@pytest.fixture(scope='session')
def board_count(board_list):
    """Get total number of boards"""
    return len(board_list)
