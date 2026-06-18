"""
Tests for sdkconfig consistency checking in board_manager_global_callback.
"""

import os
import sys
from pathlib import Path

import pytest



class _Task:
    def __init__(self, name, aliases=None):
        self.name = name
        self.aliases = aliases or []


def _make_project(tmp_path: Path, with_sdkconfig: bool = True) -> Path:
    proj = tmp_path / 'proj'
    gen_dir = proj / 'components' / 'gen_bmgr_codes'
    gen_dir.mkdir(parents=True, exist_ok=True)
    for filename in (
        'CMakeLists.txt',
        'idf_component.yml',
        'gen_board_info.c',
        'gen_board_periph_config.c',
        'gen_board_periph_handles.c',
        'gen_board_device_config.c',
        'gen_board_device_handles.c',
    ):
        (gen_dir / filename).write_text('', encoding='utf-8')
    (gen_dir / 'board_manager.defaults').write_text(
        '\n'.join(
            [
                'CONFIG_IDF_TARGET="esp32s3"',
                'CONFIG_ESP_BOARD_TEST_BOARD=y',
                'CONFIG_ESP_BOARD_NAME="test_board"',
                'CONFIG_ESP_BOARD_PERIPH_I2C_SUPPORT=y',
                'CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT=y',
                'CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_SPI_SUPPORT=y',
            ]
        )
        + '\n',
        encoding='utf-8',
    )
    if with_sdkconfig:
        (proj / 'sdkconfig').write_text(
            'CONFIG_IDF_TARGET="esp32s3"\nCONFIG_FREERTOS_HZ=1000\n',
            encoding='utf-8',
        )
    return proj


def _load_callback(bmgr_root: Path, project_dir: Path):
    sys.path.insert(0, str(bmgr_root))
    import idf_ext  # noqa: F401

    ext = idf_ext.action_extensions({}, str(project_dir))
    return ext['global_action_callbacks'][0], idf_ext


def test_parse_bmgr_defaults_symbols(bmgr_root, tmp_path):
    _, idf_ext = _load_callback(bmgr_root, tmp_path)
    defaults = tmp_path / 'board_manager.defaults'
    defaults.write_text(
        '\n'.join(
            [
                'CONFIG_ESP_BOARD_TEST_BOARD=y',
                'CONFIG_ESP_BOARD_PERIPH_I2C_SUPPORT=y',
                'CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT=y',
                'CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_SPI_SUPPORT=y',
            ]
        )
        + '\n',
        encoding='utf-8',
    )
    board, devs, periphs, subtypes = idf_ext._parse_bmgr_defaults_symbols(str(defaults))
    assert board == 'test_board'
    assert devs == {'audio_codec', 'display_lcd'}
    assert periphs == {'i2c'}
    assert subtypes == {'display_lcd': {'spi'}}


def test_callback_warns_on_inconsistent_sdkconfig(bmgr_root, tmp_path, monkeypatch, capsys):
    project_dir = _make_project(tmp_path, with_sdkconfig=True)
    callback, idf_ext = _load_callback(bmgr_root, project_dir)

    def _fake_check(self, **kwargs):
        return {
            'ok': False,
            'issues': ['CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT missing, expected y'],
        }

    monkeypatch.setattr(
        idf_ext.SDKConfigManager,
        'ensure_sdkconfig_consistency',
        _fake_check,
        raising=True,
    )
    callback(
        None,
        {'project_dir': str(project_dir), 'define_cache_entry': []},
        [_Task('build')],
    )
    out = capsys.readouterr().out
    assert '[Board Manager] Running global callback for tasks: build' in out
    assert '[Board Manager] Checking sdkconfig consistency before action execution...' in out
    assert '[Board Manager] Detected 1 sdkconfig inconsistency issue(s):' in out
    assert 'Please run: idf.py bmgr -b test_board' in out
    assert 'legacy: idf.py gen-bmgr-config -b test_board' in out

def test_callback_runs_once_for_duplicate_extension_load(bmgr_root, tmp_path, monkeypatch, capsys):
    project_dir = _make_project(tmp_path, with_sdkconfig=True)
    callback, idf_ext = _load_callback(bmgr_root, project_dir)
    calls = {'count': 0}

    def _fake_check(self, **kwargs):
        calls['count'] += 1
        return {'ok': True, 'issues': []}

    monkeypatch.setattr(
        idf_ext.SDKConfigManager,
        'ensure_sdkconfig_consistency',
        _fake_check,
        raising=True,
    )

    args = {'project_dir': str(project_dir), 'define_cache_entry': []}
    tasks = [_Task('all'), _Task('flash'), _Task('monitor')]
    callback(None, args, tasks)
    callback(None, args, tasks)

    out = capsys.readouterr().out
    assert out.count('[Board Manager] Running global callback for tasks: all, flash, monitor') == 1
    assert out.count('[Board Manager] Checking sdkconfig consistency before action execution...') == 1
    assert calls['count'] == 1

def test_extension_returns_no_callback_when_bmgr_action_already_loaded(bmgr_root, tmp_path):
    _, idf_ext = _load_callback(bmgr_root, tmp_path)

    ext = idf_ext.action_extensions(
        {
            'actions': {
                'bmgr': {
                    'aliases': [],
                },
            },
            'global_action_callbacks': [object()],
        },
        str(tmp_path),
    )

    assert ext['actions'] == {}
    assert ext['global_action_callbacks'] == []

def test_set_target_runs_sdkconfig_check_when_sdkconfig_present(bmgr_root, tmp_path, capsys):
    """Like build, set-target runs warn-only sdkconfig consistency when sdkconfig exists."""
    project_dir = _make_project(tmp_path, with_sdkconfig=True)
    (project_dir / 'sdkconfig').write_text(
        'CONFIG_IDF_TARGET="esp32c5"\nCONFIG_FREERTOS_HZ=1000\n',
        encoding='utf-8',
    )
    callback, _ = _load_callback(bmgr_root, project_dir)
    callback(
        None,
        {'project_dir': str(project_dir), 'define_cache_entry': []},
        [_Task('set-target')],
    )
    out = capsys.readouterr().out
    assert '[Board Manager] Checking sdkconfig consistency before action execution...' in out


@pytest.mark.parametrize(
    'task_name',
    ['menuconfig', 'confserver', 'config-report'],
)
def test_configure_tasks_run_sdkconfig_check_when_sdkconfig_present(
    bmgr_root, tmp_path, capsys, task_name
):
    project_dir = _make_project(tmp_path, with_sdkconfig=True)
    (project_dir / 'sdkconfig').write_text(
        'CONFIG_IDF_TARGET="esp32c5"\nCONFIG_FREERTOS_HZ=1000\n',
        encoding='utf-8',
    )
    callback, _ = _load_callback(bmgr_root, project_dir)
    callback(
        None,
        {'project_dir': str(project_dir), 'define_cache_entry': []},
        [_Task(task_name)],
    )
    out = capsys.readouterr().out
    assert '[Board Manager] Checking sdkconfig consistency before action execution...' in out


def test_set_target_injects_when_sdkconfig_missing(bmgr_root, tmp_path, monkeypatch):
    project_dir = _make_project(tmp_path, with_sdkconfig=False)
    callback, _ = _load_callback(bmgr_root, project_dir)
    monkeypatch.delenv('SDKCONFIG_DEFAULTS', raising=False)
    callback(
        None,
        {'project_dir': str(project_dir), 'define_cache_entry': []},
        [_Task('set-target')],
    )
    assert 'board_manager.defaults' in os.environ.get('SDKCONFIG_DEFAULTS', '')


def test_callback_skip_switch(bmgr_root, tmp_path, monkeypatch, capsys):
    project_dir = _make_project(tmp_path, with_sdkconfig=True)
    callback, idf_ext = _load_callback(bmgr_root, project_dir)

    def _should_not_be_called(self, **kwargs):
        raise AssertionError('ensure_sdkconfig_consistency should not be called when skip switch is enabled')

    monkeypatch.setattr(
        idf_ext.SDKConfigManager,
        'ensure_sdkconfig_consistency',
        _should_not_be_called,
        raising=True,
    )
    monkeypatch.setenv('ESP_BOARD_MANAGER_SKIP_SDKCONFIG_CHECK', '1')
    callback(
        None,
        {'project_dir': str(project_dir), 'define_cache_entry': []},
        [_Task('build')],
    )
    out = capsys.readouterr().out
    assert 'ESP_BOARD_MANAGER_SKIP_SDKCONFIG_CHECK' in out
    assert 'sdkconfig consistency check skipped' in out


def test_callback_injects_defaults_when_sdkconfig_missing(bmgr_root, tmp_path, monkeypatch):
    project_dir = _make_project(tmp_path, with_sdkconfig=False)
    callback, _ = _load_callback(bmgr_root, project_dir)
    monkeypatch.delenv('SDKCONFIG_DEFAULTS', raising=False)
    callback(
        None,
        {'project_dir': str(project_dir), 'define_cache_entry': []},
        [_Task('build')],
    )
    sdkdefaults = os.environ.get('SDKCONFIG_DEFAULTS', '')
    assert 'board_manager.defaults' in sdkdefaults


def test_callback_prepends_board_manager_defaults_when_sdkconfig_missing(
    bmgr_root, tmp_path, monkeypatch
):
    project_dir = _make_project(tmp_path, with_sdkconfig=False)
    (project_dir / 'sdkconfig.defaults').write_text(
        'CONFIG_SPIRAM_SPEED_120M=y\n',
        encoding='utf-8',
    )
    user_defaults = project_dir / 'sdkconfig.user'
    user_defaults.write_text('CONFIG_FREERTOS_HZ=1000\n', encoding='utf-8')
    callback, _ = _load_callback(bmgr_root, project_dir)
    monkeypatch.setenv('SDKCONFIG_DEFAULTS', str(user_defaults))

    callback(
        None,
        {'project_dir': str(project_dir), 'define_cache_entry': []},
        [_Task('build')],
    )

    defaults = os.environ.get('SDKCONFIG_DEFAULTS', '').split(';')
    assert defaults[0].endswith('components/gen_bmgr_codes/board_manager.defaults')
    assert defaults[1] == str(project_dir / 'sdkconfig.defaults')
    assert defaults[2] == str(user_defaults)


def test_callback_merges_and_rewrites_sdkconfig_defaults_from_define_cache_entry(
    bmgr_root, tmp_path, monkeypatch
):
    project_dir = _make_project(tmp_path, with_sdkconfig=False)
    (project_dir / 'sdkconfig.defaults').write_text(
        'CONFIG_SPIRAM_SPEED_120M=y\n',
        encoding='utf-8',
    )
    env_defaults = project_dir / 'sdkconfig.env'
    env_defaults.write_text('CONFIG_FREERTOS_HZ=1000\n', encoding='utf-8')
    cache_defaults = project_dir / 'sdkconfig.cache'
    cache_defaults.write_text('CONFIG_COMPILER_OPTIMIZATION_SIZE=y\n', encoding='utf-8')
    nested_defaults_dir = project_dir / 'nested'
    nested_defaults_dir.mkdir()
    nested_defaults = nested_defaults_dir / 'sdkconfig.extra'
    nested_defaults.write_text('CONFIG_PARTITION_TABLE_CUSTOM=y\n', encoding='utf-8')

    callback, _ = _load_callback(bmgr_root, project_dir)
    monkeypatch.setenv('SDKCONFIG_DEFAULTS', str(env_defaults))
    global_args = {
        'project_dir': str(project_dir),
        'define_cache_entry': [
            'OTHER_OPTION=1',
            'SDKCONFIG_DEFAULTS=sdkconfig.cache;nested/sdkconfig.extra',
        ],
    }

    callback(None, global_args, [_Task('build')])

    defaults = os.environ.get('SDKCONFIG_DEFAULTS', '').split(';')
    assert defaults[0].endswith('components/gen_bmgr_codes/board_manager.defaults')
    assert defaults[1] == str(project_dir / 'sdkconfig.defaults')
    assert defaults[2] == str(env_defaults)
    assert defaults[3] == str(cache_defaults.resolve())
    assert defaults[4] == str(nested_defaults.resolve())

    assert global_args['define_cache_entry'][0] == 'OTHER_OPTION=1'
    assert global_args['define_cache_entry'][1] == f'SDKCONFIG_DEFAULTS={";".join(defaults)}'


def test_callback_allows_user_defaults_to_override_non_bmgr_symbols(
    bmgr_root, tmp_path, monkeypatch
):
    project_dir = _make_project(tmp_path, with_sdkconfig=False)
    (project_dir / 'sdkconfig.defaults').write_text(
        'CONFIG_SPIRAM_SPEED_120M=y\n',
        encoding='utf-8',
    )
    callback, _ = _load_callback(bmgr_root, project_dir)
    monkeypatch.delenv('SDKCONFIG_DEFAULTS', raising=False)

    callback(
        None,
        {'project_dir': str(project_dir), 'define_cache_entry': []},
        [_Task('build')],
    )

    assert 'board_manager.defaults' in os.environ.get('SDKCONFIG_DEFAULTS', '')


@pytest.mark.parametrize(
    'managed_symbol_line',
    [
        '# CONFIG_ESP_BOARD_TEST_BOARD is not set',
        'CONFIG_ESP_BOARD_NAME="other_board"',
        '# CONFIG_ESP_BOARD_PERIPH_I2C_SUPPORT is not set',
        '# CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT is not set',
        '# CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_SPI_SUPPORT is not set',
        'CONFIG_ESP_BOARD_PERIPH_SPI_SUPPORT=y',
        'CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT=y',
        'CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUB_DSI_SUPPORT=y',
    ],
)
def test_callback_rejects_user_defaults_that_set_managed_bmgr_symbols(
    bmgr_root, tmp_path, monkeypatch, managed_symbol_line
):
    project_dir = _make_project(tmp_path, with_sdkconfig=False)
    (project_dir / 'sdkconfig.defaults').write_text(
        f'{managed_symbol_line}\n',
        encoding='utf-8',
    )
    callback, idf_ext = _load_callback(bmgr_root, project_dir)
    monkeypatch.delenv('SDKCONFIG_DEFAULTS', raising=False)

    with pytest.raises(idf_ext.FatalError, match='Board Manager managed symbol'):
        callback(
            None,
            {'project_dir': str(project_dir), 'define_cache_entry': []},
            [_Task('build')],
        )


def test_callback_skip_switch_allows_user_defaults_with_managed_bmgr_symbols(
    bmgr_root, tmp_path, monkeypatch, capsys
):
    project_dir = _make_project(tmp_path, with_sdkconfig=False)
    (project_dir / 'sdkconfig.defaults').write_text(
        'CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT=y\n',
        encoding='utf-8',
    )
    callback, _ = _load_callback(bmgr_root, project_dir)
    monkeypatch.delenv('SDKCONFIG_DEFAULTS', raising=False)
    monkeypatch.setenv('ESP_BOARD_MANAGER_SKIP_SDKCONFIG_CHECK', '1')

    callback(
        None,
        {'project_dir': str(project_dir), 'define_cache_entry': []},
        [_Task('build')],
    )

    out = capsys.readouterr().out
    assert 'ESP_BOARD_MANAGER_SKIP_SDKCONFIG_CHECK' in out
    assert 'user defaults managed-symbol check skipped' in out
    assert 'board_manager.defaults' in os.environ.get('SDKCONFIG_DEFAULTS', '')


def test_callback_allows_unmanaged_esp_board_prefixed_defaults(
    bmgr_root, tmp_path, monkeypatch
):
    project_dir = _make_project(tmp_path, with_sdkconfig=False)
    (project_dir / 'sdkconfig.defaults').write_text(
        'CONFIG_ESP_BOARD_FUTURE_OPTION=y\n',
        encoding='utf-8',
    )
    callback, _ = _load_callback(bmgr_root, project_dir)
    monkeypatch.delenv('SDKCONFIG_DEFAULTS', raising=False)

    callback(
        None,
        {'project_dir': str(project_dir), 'define_cache_entry': []},
        [_Task('build')],
    )

    assert 'board_manager.defaults' in os.environ.get('SDKCONFIG_DEFAULTS', '')


def test_callback_rejects_target_user_defaults_that_set_bmgr_symbols(
    bmgr_root, tmp_path, monkeypatch
):
    project_dir = _make_project(tmp_path, with_sdkconfig=False)
    (project_dir / 'sdkconfig.defaults').write_text(
        'CONFIG_SPIRAM_SPEED_120M=y\n',
        encoding='utf-8',
    )
    (project_dir / 'sdkconfig.defaults.esp32s3').write_text(
        'CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT=y\n',
        encoding='utf-8',
    )
    callback, idf_ext = _load_callback(bmgr_root, project_dir)
    monkeypatch.delenv('SDKCONFIG_DEFAULTS', raising=False)

    with pytest.raises(idf_ext.FatalError, match='sdkconfig.defaults.esp32s3'):
        callback(
            None,
            {'project_dir': str(project_dir), 'define_cache_entry': []},
            [_Task('build')],
        )
