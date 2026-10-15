"""
Test log level functionality
Tests --log-level parameter with different values
"""

import re

import pytest


SCAN_EXPAND = 'Scanning all directories in'
FOUND_VALID_BOARD = 'Found valid board in'
FOUND_BOARDS_DUMP = 'boards: ['
YAML_PERIPH = 'board_peripherals.yaml'


class TestValidLogLevels:
    """Test all valid log levels"""

    @pytest.mark.parametrize('log_level', ['DEBUG', 'INFO', 'WARNING', 'ERROR'])
    def test_valid_log_levels(self, run_bmgr_cmd, valid_board, log_level):
        """Test that all valid log levels work"""
        result = run_bmgr_cmd(['-b', valid_board, '--log-level', log_level, '--kconfig-only'])
        assert result.returncode == 0


class TestLogLevelOutput:
    """Test that log levels affect output verbosity"""

    def test_info_keeps_roots_and_scan_sources(self, run_bmgr_cmd):
        result = run_bmgr_cmd(['-l', '--log-level', 'INFO'])
        assert result.returncode == 0
        out = result.stdout + result.stderr
        assert 'Using script directory as root directory:' in out
        assert 'Using project directory:' in out or 'Using user-specified project directory:' in out
        assert 'espressif/esp_boards' in out
        assert 'Board Components:' in out
        assert SCAN_EXPAND not in out
        assert FOUND_VALID_BOARD not in out

    def test_debug_list_shows_per_board_scan_trace(self, run_bmgr_cmd):
        result = run_bmgr_cmd(['-l', '--log-level', 'DEBUG'])
        assert result.returncode == 0
        out = result.stdout + result.stderr
        assert SCAN_EXPAND in out
        assert FOUND_VALID_BOARD in out
        assert out.count(FOUND_VALID_BOARD) > 1

    def test_info_generation_keeps_board_path_not_yaml_dump(self, run_bmgr_cmd, valid_board):
        result = run_bmgr_cmd(['-b', valid_board, '--kconfig-only', '--log-level', 'INFO'])
        assert result.returncode == 0
        out = result.stdout + result.stderr
        assert 'Using script directory as root directory:' in out
        assert 'Board path:' in out
        assert valid_board in out
        assert re.search(r'Found \d+ board\(s\)\.', out)
        assert FOUND_BOARDS_DUMP not in out
        assert SCAN_EXPAND not in out
        assert FOUND_VALID_BOARD not in out
        assert YAML_PERIPH not in out
        assert 'Writing Kconfig file to:' not in out
        assert 'Generated selected-board Kconfig.projbuild at:' not in out

    def test_debug_generation_shows_yaml_and_board_dump(self, run_bmgr_cmd, valid_board):
        result = run_bmgr_cmd(['-b', valid_board, '--kconfig-only', '--log-level', 'DEBUG'])
        assert result.returncode == 0
        out = result.stdout + result.stderr
        assert FOUND_VALID_BOARD in out
        assert YAML_PERIPH in out
        assert 'Boards:' in out
        assert 'Writing Kconfig file to:' in out
        info = run_bmgr_cmd(['-b', valid_board, '--kconfig-only', '--log-level', 'INFO'])
        assert len(result.stdout.splitlines()) > len(info.stdout.splitlines())

    def test_error_level_minimal(self, run_bmgr_cmd, valid_board):
        """Test that ERROR level suppresses info messages"""
        result = run_bmgr_cmd(['-b', valid_board, '--log-level', 'ERROR', '--kconfig-only'])
        assert result.returncode == 0
        # Count info symbols
        info_count = result.stdout.count('ℹ️') + result.stdout.count('INFO')
        # ERROR level should have fewer info messages
        assert info_count < 10  # Arbitrary threshold


class TestInvalidLogLevels:
    """Test invalid log level values"""

    @pytest.mark.parametrize('invalid_level', [
        'debug',      # lowercase
        'info',       # lowercase
        '1',          # numeric
        '5',          # numeric
        'INVALID',    # non-existent
        'TRACE',      # non-existent
        '',           # empty
    ])
    def test_invalid_log_levels(self, run_bmgr_cmd, valid_board, invalid_level):
        """Test that invalid log levels are rejected"""
        result = run_bmgr_cmd(['-b', valid_board, '--log-level', invalid_level, '--kconfig-only'], check=False)
        assert result.returncode != 0
        assert 'invalid choice' in result.stderr or 'error' in result.stderr.lower()


class TestDefaultBehavior:
    """Test default log level behavior"""

    def test_default_log_level_without_flag(self, run_bmgr_cmd, valid_board):
        """Test that default log level works without --log-level"""
        result = run_bmgr_cmd(['-b', valid_board, '--kconfig-only'])
        assert result.returncode == 0


class TestLogLevelWithOtherCommands:
    """Test log level with other commands"""

    def test_log_level_debug_with_list_boards(self, run_bmgr_cmd):
        """Test --log-level DEBUG with -l command"""
        result = run_bmgr_cmd(['-l', '--log-level', 'DEBUG'])
        assert result.returncode == 0
        # With DEBUG level, output should still contain board list
        assert 'Board Components:' in result.stdout or 'Board' in result.stdout

    def test_log_level_error_with_list_boards(self, run_bmgr_cmd):
        """Test --log-level ERROR with -l command"""
        result = run_bmgr_cmd(['-l', '--log-level', 'ERROR'])
        assert result.returncode == 0
        # With ERROR level, INFO messages may be suppressed
        # Just check command succeeds, don't check output content


class TestCaseSensitivity:
    """Test that log level is case-sensitive"""

    def test_lowercase_rejected(self, run_bmgr_cmd, valid_board):
        """Test that lowercase log levels are rejected"""
        result = run_bmgr_cmd(['-b', valid_board, '--log-level', 'info', '--kconfig-only'], check=False)
        assert result.returncode != 0

    def test_uppercase_accepted(self, run_bmgr_cmd, valid_board):
        """Test that uppercase log levels are accepted"""
        result = run_bmgr_cmd(['-b', valid_board, '--log-level', 'INFO', '--kconfig-only'])
        assert result.returncode == 0
