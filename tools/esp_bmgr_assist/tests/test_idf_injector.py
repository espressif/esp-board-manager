import builtins
import contextlib
import importlib.util
import errno
import io
import os
import sys
import tempfile
import threading
import time
import types
import unittest
from pathlib import Path
from unittest import mock

from esp_bmgr_py import bmgr_placeholder_ext, bmgr_request, bootstrap_target, idf_injector


def _actions_path_contains(path: Path) -> bool:
    needle = str(path.resolve())
    extra = os.environ.get('IDF_EXTRA_ACTIONS_PATH', '')
    return needle in extra


def _stub_idf_component_manager_modules(fake_download) -> dict:
    """Stub packages so project dependency bootstrap can import without ESP-IDF venv."""
    dep_mod = types.ModuleType('idf_component_manager.dependencies')
    dep_mod.download_project_dependencies = fake_download

    icm_pkg = types.ModuleType('idf_component_manager')
    icm_pkg.dependencies = dep_mod

    mgr_mod = types.ModuleType('idf_component_tools.manager')

    class _ManifestManager:
        def __init__(self, path, name):
            self._path = path
            self._name = name

        def load(self):
            return types.SimpleNamespace(real_name=self._name, path=str(Path(self._path) / 'idf_component.yml'))

    mgr_mod.ManifestManager = _ManifestManager

    util_mod = types.ModuleType('idf_component_tools.utils')
    util_mod.ProjectRequirements = lambda reqs: reqs

    return {
        'idf_component_manager': icm_pkg,
        'idf_component_manager.dependencies': dep_mod,
        'idf_component_tools.manager': mgr_mod,
        'idf_component_tools.utils': util_mod,
    }


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding='utf-8')


def _create_project(root: Path) -> None:
    _write(
        root / 'CMakeLists.txt',
        'include($ENV{IDF_PATH}/tools/cmake/project.cmake)\nproject(test)\n',
    )
    (root / 'main').mkdir(parents=True, exist_ok=True)
    _write(root / 'main' / 'CMakeLists.txt', "idf_component_register(SRCS \"main.c\")\n")


def _create_bmgr_component(component_dir: Path) -> None:
    _write(component_dir / 'idf_ext.py', 'def action_extensions(base_actions, project_path):\n    return {}\n')
    _write(component_dir / 'idf_component.yml', "dependencies:\n  idf: \">=5.4\"\n")
    _write(component_dir / 'gen_bmgr_config_codes.py', 'def main():\n    return 0\n')


class InjectorTests(unittest.TestCase):
    def setUp(self) -> None:
        self._env_backup = dict(os.environ)
        idf_injector._MAIN_EXECUTED = False

    def tearDown(self) -> None:
        os.environ.clear()
        os.environ.update(self._env_backup)
        idf_injector._MAIN_EXECUTED = False

    def test_parse_cli_context_supports_project_dir_variants(self) -> None:
        base = Path('/tmp/demo')
        context = idf_injector._parse_cli_context(
            ['idf.py', '-C', '/tmp/proj', 'gen-bmgr-config', '-b', 'board'],
            cwd=base,
        )
        self.assertTrue(context.is_idf_command)
        self.assertTrue(context.is_gen_bmgr_config)
        self.assertEqual(context.bmgr_command, 'gen-bmgr-config')
        self.assertEqual(context.project_path, Path('/tmp/proj'))

        context = idf_injector._parse_cli_context(
            ['idf.py', '--project-dir=/tmp/alt', 'build'],
            cwd=base,
        )
        self.assertEqual(context.project_path, Path('/tmp/alt'))
        self.assertFalse(context.is_gen_bmgr_config)
        self.assertIsNone(context.bmgr_command)

        context = idf_injector._parse_cli_context(
            ['idf.py', '-C', '/tmp/proj', 'bmgr', '-l'],
            cwd=base,
        )
        self.assertFalse(context.is_gen_bmgr_config)
        self.assertEqual(context.bmgr_command, 'bmgr')
        self.assertEqual(context.project_path, Path('/tmp/proj'))

    def test_resolve_bmgr_request_tracks_current_and_legacy_commands(self) -> None:
        request = bmgr_request.resolve_bmgr_request(['idf.py', 'bmgr', '-l'])
        self.assertEqual(request.command, 'bmgr')
        self.assertTrue(request.is_bmgr_cli)
        self.assertFalse(request.is_legacy_command)

        request = bmgr_request.resolve_bmgr_request(['idf.py', 'gen-bmgr-config', '-l'])
        self.assertEqual(request.command, 'gen-bmgr-config')
        self.assertTrue(request.is_bmgr_cli)
        self.assertTrue(request.is_legacy_command)

        request = bmgr_request.resolve_bmgr_request(['idf.py', 'build'])
        self.assertIsNone(request.command)
        self.assertFalse(request.is_bmgr_cli)
        self.assertFalse(request.is_legacy_command)

    def test_is_version_query_detects_idf_py_version_invocation(self) -> None:
        self.assertTrue(idf_injector._is_version_query(['idf.py', '--version']))
        self.assertTrue(idf_injector._is_version_query(['idf.py', 'version']))
        self.assertFalse(idf_injector._is_version_query(['idf.py', 'gen-bmgr-config', '-l']))

    def test_parse_idf_version_from_cmake(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            idf_path = Path(temp_dir)
            _write(
                idf_path / 'tools' / 'cmake' / 'version.cmake',
                'set(IDF_VERSION_MAJOR 5)\nset(IDF_VERSION_MINOR 5)\nset(IDF_VERSION_PATCH 3)\n',
            )
            self.assertEqual(
                idf_injector._parse_idf_version_from_cmake(idf_path),
                '5.5.3',
            )

    def test_placeholder_ext_defines_list_boards_option(self) -> None:
        names = []
        for opt in bmgr_placeholder_ext.GEN_BMGR_CONFIG_PLACEHOLDER_OPTIONS:
            names.extend(opt.get('names', []))
        self.assertIn('-l', names)
        self.assertIn('--list-boards', names)
        self.assertIn('-a', names)
        self.assertIn('--amend', names)
        self.assertIn('--skip-sdkconfig-check', names)
        self.assertNotIn('-o', names)
        self.assertNotIn('--override', names)

    def test_placeholder_ext_defines_preflight_level_global_option(self) -> None:
        names = []
        for opt in bmgr_placeholder_ext.ASSIST_PREFLIGHT_GLOBAL_OPTIONS:
            names.extend(opt.get('names', []))
        self.assertIn('--bmgr-preflight-level', names)

    def test_placeholder_registers_bmgr_and_legacy_actions_when_component_missing(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)

            with mock.patch.object(sys, 'argv', ['idf.py', 'build']):
                actions = bmgr_placeholder_ext.action_extensions({}, str(project_path))

            self.assertIn('global_action_callbacks', actions)
            self.assertIn('global_options', actions)
            self.assertIn('bmgr', actions['actions'])
            self.assertIn('gen-bmgr-config', actions['actions'])
            self.assertIs(
                actions['actions']['bmgr']['options'],
                bmgr_placeholder_ext.BMGR_PLACEHOLDER_OPTIONS,
            )
            option_names = []
            for opt in actions['global_options']:
                option_names.extend(opt.get('names', []))
            self.assertIn('--bmgr-preflight-level', option_names)

    def test_placeholder_skips_proxy_when_real_bmgr_extension_is_preloaded(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            managed_root = project_path / 'managed_components' / 'espressif__esp_board_manager'
            _create_bmgr_component(managed_root)

            fake_loaded = types.SimpleNamespace(__file__=str(managed_root / 'idf_ext.py'))
            with mock.patch.dict(sys.modules, {'idf_ext': fake_loaded}):
                actions = bmgr_placeholder_ext.action_extensions({}, str(project_path))

            self.assertIn('global_action_callbacks', actions)
            self.assertNotIn('actions', actions)

    def test_placeholder_keeps_fake_actions_for_bmgr_when_component_is_missing(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)

            with mock.patch.object(
                bmgr_placeholder_ext.project_integration,
                'find_board_manager_root',
                return_value=None,
            ), mock.patch.object(
                sys,
                'argv',
                ['idf.py', '-C', str(project_path), 'bmgr', '-b', 'esp32_s3_korvo2_v3'],
            ):
                actions = bmgr_placeholder_ext.action_extensions({}, str(project_path))

            self.assertIn('global_action_callbacks', actions)
            self.assertIn('actions', actions)
            self.assertIn('bmgr', actions['actions'])
            self.assertIn('gen-bmgr-config', actions['actions'])

    def test_prepend_actions_path_orders_package_before_existing(self) -> None:
        first = str(Path('/tmp/pkg'))
        second = str(Path('/tmp/bmgr'))
        merged = idf_injector._prepend_actions_path(second, first)
        self.assertTrue(merged.startswith(first))
        self.assertIn(second, merged)

    def test_merge_actions_path_deduplicates_and_preserves_existing_entries(self) -> None:
        first = str(Path('/tmp/one'))
        second = str(Path('/tmp/two'))
        existing = f'{first};{second}'
        merged = idf_injector._merge_actions_path(existing, second)
        self.assertEqual(merged, existing)

        merged = idf_injector._merge_actions_path(first, second)
        self.assertEqual(merged, f'{first};{second}')

    def test_split_actions_paths_matches_idf_py_semicolon_only(self) -> None:
        # idf.py uses extra_paths.split(';') only; ':' must not split (e.g. POSIX pathsep).
        posix_like = '/tmp/a:/tmp/b'
        self.assertEqual(idf_injector._split_actions_paths(posix_like), [posix_like])
        self.assertEqual(
            idf_injector._split_actions_paths('/tmp/a;/tmp/b'),
            ['/tmp/a', '/tmp/b'],
        )

    def test_collect_local_components_includes_main_and_components_dir(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            (project_path / 'components' / 'foo').mkdir(parents=True)
            (project_path / 'components' / 'bar').mkdir(parents=True)
            (project_path / 'components' / 'gen_bmgr_codes').mkdir(parents=True)

            components = idf_injector._collect_local_components(project_path)

            self.assertEqual(
                components,
                [
                    ('main', project_path / 'main'),
                    ('bar', project_path / 'components' / 'bar'),
                    ('foo', project_path / 'components' / 'foo'),
                ],
            )

    def test_find_local_bmgr_uses_parent_directory_for_test_apps_layout(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            pkg = Path(temp_dir) / 'esp_board_manager'
            _create_bmgr_component(pkg)
            test_apps = pkg / 'test_apps'
            (test_apps / 'main').mkdir(parents=True, exist_ok=True)
            found = idf_injector._find_local_bmgr(test_apps)
            self.assertEqual(found, pkg.resolve())

    def test_find_local_bmgr_supports_relative_override_path(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            component_dir = project_path / 'components' / 'esp_board_manager'
            _create_bmgr_component(component_dir)
            _write(
                project_path / 'main' / 'idf_component.yml',
                'dependencies:\n'
                '  espressif/esp_board_manager:\n'
                '    override_path: ../components/esp_board_manager\n',
            )

            found = idf_injector._find_local_bmgr(project_path)
            self.assertEqual(found, component_dir.resolve())

    def test_find_local_bmgr_supports_relative_local_path(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            component_dir = project_path / 'external' / 'esp_board_manager'
            _create_bmgr_component(component_dir)
            _write(
                project_path / 'main' / 'idf_component.yml',
                'dependencies:\n'
                '  espressif/esp_board_manager:\n'
                '    path: ../external/esp_board_manager\n',
            )

            found = idf_injector._find_local_bmgr(project_path)
            self.assertEqual(found, component_dir.resolve())

    def test_find_local_bmgr_scans_component_manifests_for_local_path(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            component_dir = project_path / 'external' / 'esp_board_manager'
            _create_bmgr_component(component_dir)
            helper_dir = project_path / 'components' / 'helper'
            _write(helper_dir / 'CMakeLists.txt', "idf_component_register(SRCS \"helper.c\")\n")
            _write(
                helper_dir / 'idf_component.yml',
                'dependencies:\n'
                '  espressif/esp_board_manager:\n'
                '    path: ../../external/esp_board_manager\n',
            )

            found = idf_injector._find_local_bmgr(project_path)
            self.assertEqual(found, component_dir.resolve())

    def test_find_board_manager_component_uses_shared_resolution_order(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            component_dir = project_path / 'components' / 'esp_board_manager'
            _create_bmgr_component(component_dir)

            found = idf_injector.find_board_manager_component(project_path)
            self.assertEqual(found, component_dir.resolve())

    def test_sanitize_download_dependency_normalizes_string_subclasses(self) -> None:
        class FancyStr(str):
            pass

        dependency = {
            'version': FancyStr('*'),
            'require': FancyStr('public'),
            'registry_url': FancyStr('https://components.example.com'),
        }

        sanitized = idf_injector._sanitize_download_dependency(dependency)
        self.assertEqual(sanitized['version'], '*')
        self.assertEqual(type(sanitized['version']), str)
        self.assertEqual(type(sanitized['require']), str)
        dumped = idf_injector.yaml.safe_dump(
            {'dependencies': {'espressif/esp_board_manager': sanitized}},
            sort_keys=False,
            default_flow_style=False,
        )
        self.assertIn('version', dumped)

    def test_sanitize_download_dependency_preserves_git_source_fields(self) -> None:
        dependency = {
            'git': 'ssh://git@example.com/repo.git',
            'path': 'packages/esp_board_manager',
            'version': '511260a6e1f31494dfa6bcb5b43352e114b93204',
            'matches': [{'if': 'target == esp32s3', 'version': '*'}],
            'rules': [{'if': 'idf_version >=5.5', 'version': '*'}],
        }

        sanitized = idf_injector._sanitize_download_dependency(dependency)

        self.assertEqual(sanitized['git'], dependency['git'])
        self.assertEqual(sanitized['path'], dependency['path'])
        self.assertEqual(sanitized['version'], dependency['version'])
        self.assertNotIn('matches', sanitized)
        self.assertNotIn('rules', sanitized)

    def test_module_import_does_not_require_pyyaml(self) -> None:
        module_path = Path(idf_injector.__file__)
        spec = importlib.util.spec_from_file_location('esp_bmgr_py._idf_injector_no_yaml', module_path)
        self.assertIsNotNone(spec)
        self.assertIsNotNone(spec.loader)
        module = importlib.util.module_from_spec(spec)

        real_import = builtins.__import__

        def guarded_import(name, globals=None, locals=None, fromlist=(), level=0):
            if name == 'yaml':
                raise AssertionError('idf_injector import should not require PyYAML')
            return real_import(name, globals, locals, fromlist, level)

        try:
            with mock.patch('builtins.__import__', side_effect=guarded_import):
                spec.loader.exec_module(module)
        finally:
            sys.meta_path = [entry for entry in sys.meta_path if not isinstance(entry, module.IdfPyActionsHook)]

    def test_passive_bootstrap_preserves_preset_idf_extra_without_merging_discovered(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            pkg = Path(temp_dir) / 'esp_board_manager'
            _create_bmgr_component(pkg)
            test_apps = pkg / 'test_apps'
            _create_project(test_apps)
            user_extra = Path(temp_dir) / 'my_extensions'
            user_extra.mkdir()

            with mock.patch.dict(os.environ, {'IDF_EXTRA_ACTIONS_PATH': str(user_extra)}):
                with mock.patch.object(idf_injector, '_bootstrap_project_dependencies') as mocked_download:
                    context = idf_injector.CliContext(True, test_apps, False, ('idf.py', 'build'))
                    idf_injector._bootstrap_for_context(context)
                    mocked_download.assert_not_called()
                merged = os.environ.get('IDF_EXTRA_ACTIONS_PATH', '')
                self.assertIn(str(user_extra), merged)
                self.assertNotIn(str(pkg.resolve()), merged)
                non_empty = [p.strip() for p in merged.split(';') if p.strip()]
                self.assertEqual(len(non_empty), 2)

    def test_passive_bootstrap_injects_existing_local_component_without_download(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            component_dir = project_path / 'components' / 'esp_board_manager'
            _create_bmgr_component(component_dir)
            context = idf_injector.CliContext(True, project_path, False, ('idf.py', 'build'))

            with mock.patch.object(idf_injector, '_bootstrap_project_dependencies') as mocked_download:
                idf_injector._bootstrap_for_context(context)

            self.assertTrue(_actions_path_contains(component_dir))
            mocked_download.assert_not_called()

    def test_bootstrap_project_dependencies_restores_idf_target_when_unset(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(project_path / 'main' / 'idf_component.yml', "dependencies:\n  test/a: \"*\"\n")
            _write(project_path / 'sdkconfig', 'CONFIG_IDF_TARGET="esp32s3"\n')
            captured = {}

            def fake_download(req, lock, dest):
                captured['target'] = os.environ.get('IDF_TARGET')
                captured['idf_version'] = os.environ.get('IDF_VERSION')
                captured['skip'] = os.environ.get(idf_injector.SKIP_BOOTSTRAP_ENV)
                captured['manifests'] = req
                captured['lock'] = lock
                captured['dest'] = dest

            os.environ.pop('IDF_TARGET', None)
            with mock.patch.dict(sys.modules, _stub_idf_component_manager_modules(fake_download)):
                with mock.patch.object(
                    idf_injector,
                    '_resolve_idf_version_for_bootstrap',
                    return_value='5.5.3',
                ):
                    idf_injector._bootstrap_project_dependencies(project_path)

            self.assertEqual(captured.get('target'), 'esp32s3')
            self.assertEqual(captured.get('idf_version'), '5.5.3')
            self.assertEqual(captured.get('skip'), '1')
            self.assertEqual(len(captured.get('manifests', [])), 1)
            self.assertEqual(captured.get('lock'), str(project_path / 'dependencies.lock'))
            self.assertEqual(captured.get('dest'), str(project_path / 'managed_components'))
            self.assertNotIn('IDF_TARGET', os.environ)
            self.assertNotIn('IDF_VERSION', os.environ)
            self.assertNotIn(idf_injector.SKIP_BOOTSTRAP_ENV, os.environ)

    def test_bootstrap_project_dependencies_preserves_existing_idf_target(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(project_path / 'main' / 'idf_component.yml', "dependencies:\n  test/a: \"*\"\n")
            captured = {}

            def fake_download(req, lock, dest):
                captured['target'] = os.environ.get('IDF_TARGET')

            with mock.patch.dict(os.environ, {'IDF_TARGET': 'esp32s3'}):
                with mock.patch.dict(sys.modules, _stub_idf_component_manager_modules(fake_download)):
                    idf_injector._bootstrap_project_dependencies(project_path)
                self.assertEqual(captured.get('target'), 'esp32s3')
                self.assertEqual(os.environ.get('IDF_TARGET'), 'esp32s3')

    def test_resolve_bootstrap_target_prefers_sdkconfig(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(project_path / 'sdkconfig', 'CONFIG_IDF_TARGET="esp32c6"\n')
            _write(project_path / 'sdkconfig.defaults', 'CONFIG_IDF_TARGET="esp32s3"\n')

            resolved = bootstrap_target.resolve_bootstrap_target(project_path, env={})

            self.assertEqual(resolved.target, 'esp32c6')
            self.assertEqual(resolved.source, 'sdkconfig')
            self.assertFalse(resolved.is_fallback)

    def test_resolve_bootstrap_target_uses_override_env_before_project_files(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(project_path / 'sdkconfig', 'CONFIG_IDF_TARGET="esp32s3"\n')

            resolved = bootstrap_target.resolve_bootstrap_target(
                project_path,
                env={bootstrap_target.BOOTSTRAP_TARGET_ENV: 'esp32c5'},
            )

            self.assertEqual(resolved.target, 'esp32c5')
            self.assertEqual(
                resolved.source,
                f'env:{bootstrap_target.BOOTSTRAP_TARGET_ENV}',
            )
            self.assertFalse(resolved.is_fallback)

    def test_resolve_bootstrap_target_uses_board_manager_defaults_before_fallback(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            gen_dir = project_path / 'components' / 'gen_bmgr_codes'
            gen_dir.mkdir(parents=True, exist_ok=True)
            _write(gen_dir / 'board_manager.defaults', 'CONFIG_IDF_TARGET="esp32p4"\n')

            resolved = bootstrap_target.resolve_bootstrap_target(project_path, env={})

            self.assertEqual(resolved.target, 'esp32p4')
            self.assertEqual(resolved.source, 'board_manager.defaults')
            self.assertFalse(resolved.is_fallback)

    def test_resolve_bootstrap_target_falls_back_to_default_when_project_has_no_target(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)

            resolved = bootstrap_target.resolve_bootstrap_target(project_path, env={})

            self.assertEqual(
                resolved.target,
                bootstrap_target.DEFAULT_BOOTSTRAP_IDF_TARGET,
            )
            self.assertEqual(resolved.source, 'fallback-default')
            self.assertTrue(resolved.is_fallback)

    def test_find_bmgr_from_lock_resolves_transitive_local_component(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            external_bmgr = project_path / 'external' / 'esp_board_manager'
            _create_bmgr_component(external_bmgr)
            _write(
                project_path / 'dependencies.lock',
                'version: 3.0.0\n'
                'dependencies:\n'
                '  espressif/esp_board_manager:\n'
                '    source:\n'
                f'      path: {external_bmgr}\n'
                '      type: local\n'
                '    version: 0.5.7\n',
            )
            found = idf_injector._find_bmgr_from_lock(project_path)

            self.assertEqual(found, external_bmgr.resolve())

    def test_active_bootstrap_resolves_project_dependencies_and_injects_managed_component(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(project_path / 'main' / 'idf_component.yml', "dependencies:\n  test/a: \"*\"\n")
            managed_dir = project_path / 'managed_components' / 'espressif__esp_board_manager'
            context = idf_injector.CliContext(True, project_path, True, ('idf.py', 'gen-bmgr-config'))

            def fake_bootstrap(target_project_path: Path) -> None:
                self.assertEqual(target_project_path, project_path)
                _create_bmgr_component(managed_dir)

            with mock.patch.object(idf_injector, '_bootstrap_project_dependencies', side_effect=fake_bootstrap) as mocked_download:
                with mock.patch.object(idf_injector.os, 'execvpe') as mocked_exec:
                    idf_injector._bootstrap_for_context(context)

            self.assertTrue(_actions_path_contains(managed_dir))
            mocked_download.assert_called_once()
            mocked_exec.assert_called_once()
            argv = mocked_exec.call_args.args[1]
            env = mocked_exec.call_args.args[2]
            self.assertEqual(argv, [sys.executable, *context.argv])
            self.assertEqual(env.get(idf_injector.REEXEC_AFTER_BOOTSTRAP_ENV), '1')

    def test_active_bootstrap_accepts_bmgr_command(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(project_path / 'main' / 'idf_component.yml', "dependencies:\n  test/a: \"*\"\n")
            managed_dir = project_path / 'managed_components' / 'espressif__esp_board_manager'
            context = idf_injector.CliContext(
                True,
                project_path,
                False,
                ('idf.py', 'bmgr', '-l'),
                'bmgr',
            )

            def fake_bootstrap(target_project_path: Path) -> None:
                self.assertEqual(target_project_path, project_path)
                _create_bmgr_component(managed_dir)

            with mock.patch.object(idf_injector, '_bootstrap_project_dependencies', side_effect=fake_bootstrap) as mocked_download:
                with mock.patch.object(idf_injector.os, 'execvpe') as mocked_exec:
                    idf_injector._bootstrap_for_context(context)

            self.assertTrue(_actions_path_contains(managed_dir))
            mocked_download.assert_called_once()
            mocked_exec.assert_called_once()

    def test_active_bootstrap_does_not_reexec_twice_when_flag_is_set(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(project_path / 'main' / 'idf_component.yml', "dependencies:\n  test/a: \"*\"\n")
            managed_dir = project_path / 'managed_components' / 'espressif__esp_board_manager'
            context = idf_injector.CliContext(
                True,
                project_path,
                False,
                ('idf.py', 'bmgr', '-l'),
                'bmgr',
            )

            def fake_bootstrap(target_project_path: Path) -> None:
                self.assertEqual(target_project_path, project_path)
                _create_bmgr_component(managed_dir)

            with mock.patch.dict(os.environ, {idf_injector.REEXEC_AFTER_BOOTSTRAP_ENV: '1'}, clear=False):
                with mock.patch.object(idf_injector, '_bootstrap_project_dependencies', side_effect=fake_bootstrap) as mocked_download:
                    with mock.patch.object(idf_injector.os, 'execvpe') as mocked_exec:
                        idf_injector._bootstrap_for_context(context)

            mocked_download.assert_called_once()
            mocked_exec.assert_not_called()

    def test_missing_board_manager_dependency_raises_friendly_error(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            context = idf_injector.CliContext(True, project_path, True, ('idf.py', 'gen-bmgr-config', '-l'))

            with mock.patch.object(idf_injector, '_bootstrap_project_dependencies') as mocked_bootstrap:
                with self.assertRaises(idf_injector.MissingBoardManagerDependencyError) as cm:
                    idf_injector._bootstrap_for_context(context)

            mocked_bootstrap.assert_called_once()
            # Project has no main manifest → Direct Path is skipped → Discovery Path
            # succeeds without producing bmgr → new "not depending" message is raised.
            self.assertIn('does not depend on esp_board_manager', str(cm.exception))

    def test_main_exits_cleanly_for_missing_board_manager_dependency(self) -> None:
        with mock.patch.object(
            idf_injector,
            '_bootstrap_for_context',
            side_effect=idf_injector.MissingBoardManagerDependencyError(
                'Project does not depend on esp_board_manager (directly or transitively).'
            ),
        ):
            context = idf_injector.CliContext(
                True,
                Path('/tmp/project'),
                True,
                ('idf.py', 'gen-bmgr-config', '-l'),
            )
            with mock.patch.object(idf_injector, '_parse_cli_context', return_value=context):
                with mock.patch('sys.stderr') as stderr:
                    with self.assertRaises(SystemExit) as cm:
                        idf_injector._main()

        self.assertEqual(cm.exception.code, 1)
        written = ''.join(call.args[0] for call in stderr.write.call_args_list if call.args)
        self.assertIn(
            'Project does not depend on esp_board_manager',
            written,
        )

    def test_scan_direct_bmgr_declaration_finds_main_manifest_key(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(
                project_path / 'main' / 'idf_component.yml',
                'dependencies:\n'
                '  espressif/esp_board_manager:\n'
                '    version: "^0.5.7"\n',
            )

            found = idf_injector._scan_direct_bmgr_declaration(project_path)

            self.assertIsNotNone(found)
            manifest_path, key, dep = found
            self.assertEqual(manifest_path, project_path / 'main' / 'idf_component.yml')
            self.assertEqual(key, 'espressif/esp_board_manager')
            self.assertEqual(dep.get('version'), '^0.5.7')

    def test_scan_direct_bmgr_declaration_accepts_matches_gated_entry(self) -> None:
        # The user's intent is the presence of the key; matches/rules gating should not
        # affect declaration detection because running a bmgr command already implies
        # unconditional intent to use bmgr.
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(
                project_path / 'main' / 'idf_component.yml',
                'dependencies:\n'
                '  espressif/esp_board_manager:\n'
                '    version: "^0.5.7"\n'
                '    matches:\n'
                '      - if: "target in [esp32s3]"\n',
            )

            found = idf_injector._scan_direct_bmgr_declaration(project_path)
            self.assertIsNotNone(found)

    def test_scan_direct_bmgr_declaration_finds_component_manifest_key(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            component_dir = project_path / 'components' / 'my_lib'
            component_dir.mkdir(parents=True, exist_ok=True)
            _write(component_dir / 'CMakeLists.txt', "idf_component_register(SRCS \"a.c\")\n")
            _write(
                component_dir / 'idf_component.yml',
                'dependencies:\n'
                '  esp_board_manager: "^0.5.0"\n',
            )

            found = idf_injector._scan_direct_bmgr_declaration(project_path)

            self.assertIsNotNone(found)
            manifest_path, key, _dep = found
            self.assertEqual(manifest_path, component_dir / 'idf_component.yml')
            self.assertEqual(key, 'esp_board_manager')

    def test_scan_direct_bmgr_declaration_propagates_yaml_syntax_error(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(
                project_path / 'main' / 'idf_component.yml',
                'dependencies:\n  : not-a-valid-key\n',
            )

            with self.assertRaises(Exception):
                idf_injector._scan_direct_bmgr_declaration(project_path)

    def test_direct_path_downloads_bmgr_when_main_manifest_declares_it(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(
                project_path / 'main' / 'idf_component.yml',
                'dependencies:\n'
                '  espressif/esp_board_manager:\n'
                '    version: "^0.5.7"\n',
            )
            managed_dir = project_path / 'managed_components' / 'espressif__esp_board_manager'
            context = idf_injector.CliContext(
                True, project_path, True, ('idf.py', 'gen-bmgr-config')
            )

            def fake_direct(target_project, dep):
                self.assertEqual(target_project, project_path)
                # Sanitized payload should survive and still have the user's version.
                self.assertEqual(dep.get('version'), '^0.5.7')
                _create_bmgr_component(managed_dir)

            with mock.patch.object(
                idf_injector, '_download_bmgr_component', side_effect=fake_direct
            ) as mocked_direct:
                with mock.patch.object(
                    idf_injector, '_bootstrap_project_dependencies'
                ) as mocked_discovery:
                    with mock.patch.object(idf_injector.os, 'execvpe') as mocked_exec:
                        idf_injector._bootstrap_for_context(context)

            mocked_direct.assert_called_once()
            mocked_discovery.assert_not_called()
            mocked_exec.assert_called_once()
            self.assertTrue(_actions_path_contains(managed_dir))

    def test_direct_path_rejects_broken_override_path(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(
                project_path / 'main' / 'idf_component.yml',
                'dependencies:\n'
                '  espressif/esp_board_manager:\n'
                '    override_path: ../not/a/real/bmgr\n',
            )
            context = idf_injector.CliContext(
                True, project_path, True, ('idf.py', 'gen-bmgr-config')
            )

            with mock.patch.object(
                idf_injector, '_download_bmgr_component'
            ) as mocked_direct:
                with self.assertRaises(
                    idf_injector.MissingBoardManagerDependencyError
                ) as cm:
                    idf_injector._bootstrap_for_context(context)

            mocked_direct.assert_not_called()
            message = str(cm.exception)
            self.assertIn('override_path', message)
            self.assertIn('../not/a/real/bmgr', message)

    def test_direct_path_rejects_broken_local_path(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(
                project_path / 'main' / 'idf_component.yml',
                'dependencies:\n'
                '  espressif/esp_board_manager:\n'
                '    path: ../not/a/real/bmgr\n',
            )
            context = idf_injector.CliContext(
                True, project_path, True, ('idf.py', 'gen-bmgr-config')
            )

            with mock.patch.object(
                idf_injector, '_download_bmgr_component'
            ) as mocked_direct:
                with self.assertRaises(
                    idf_injector.MissingBoardManagerDependencyError
                ) as cm:
                    idf_injector._bootstrap_for_context(context)

            mocked_direct.assert_not_called()
            message = str(cm.exception)
            self.assertIn('path', message)
            self.assertIn('../not/a/real/bmgr', message)

    def test_direct_path_download_failure_preserves_cause(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(
                project_path / 'main' / 'idf_component.yml',
                'dependencies:\n'
                '  espressif/esp_board_manager:\n'
                '    version: "^0.5.7"\n',
            )
            context = idf_injector.CliContext(
                True, project_path, True, ('idf.py', 'gen-bmgr-config')
            )
            sentinel = RuntimeError('registry down: detail-XYZ')

            with mock.patch.object(
                idf_injector, '_download_bmgr_component', side_effect=sentinel
            ):
                with self.assertRaises(
                    idf_injector.MissingBoardManagerDependencyError
                ) as cm:
                    idf_injector._bootstrap_for_context(context)

            self.assertIs(cm.exception.__cause__, sentinel)
            message = str(cm.exception)
            self.assertIn('Failed to download the declared esp_board_manager', message)
            self.assertIn('registry down: detail-XYZ', message)

    def test_direct_path_uses_temp_lock_and_cleans_it_up(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            _write(project_path / 'main' / 'idf_component.yml', 'dependencies: {}\n')
            captured = {}

            def fake_download(req, lock, dest):
                captured['lock'] = lock
                captured['dest'] = dest
                # The temp lock path must NOT be the project's dependencies.lock.
                self.assertNotEqual(
                    Path(lock).resolve(),
                    (project_path / 'dependencies.lock').resolve(),
                )
                # Managed components destination stays inside the project.
                self.assertEqual(
                    Path(dest).resolve(),
                    (project_path / 'managed_components').resolve(),
                )
                # Snapshot the temp lock's parent directory so we can verify cleanup.
                captured['temp_dir'] = Path(lock).parent

            with mock.patch.dict(
                sys.modules, _stub_idf_component_manager_modules(fake_download)
            ):
                idf_injector._download_bmgr_component(project_path, {'version': '^0.5.7'})

            self.assertIn('temp_dir', captured)
            self.assertFalse(
                captured['temp_dir'].exists(),
                'Temporary bootstrap directory should be removed after download',
            )
            self.assertFalse(
                (project_path / 'dependencies.lock').exists(),
                "Direct Path must never write into the project's dependencies.lock",
            )

    def test_safe_temp_dir_cleans_up_on_normal_exit(self) -> None:
        with idf_injector._safe_temp_dir('esp_bmgr_unit_') as temp_dir:
            path = Path(temp_dir)
            self.assertTrue(path.is_dir())
            (path / 'sentinel').write_text('ok', encoding='utf-8')

        self.assertFalse(path.exists())

    def test_safe_temp_dir_tolerates_cleanup_permission_error(self) -> None:
        # Simulate the Windows flake where the first rmtree fails with PermissionError
        # (e.g. antivirus still holds a handle); our fallback rmtree(ignore_errors=True)
        # must succeed without propagating the original exception.
        import shutil as _shutil

        captured_dir = {}
        real_rmtree = _shutil.rmtree
        call_count = {'n': 0}

        def flaky_rmtree(path, ignore_errors=False, **kwargs):
            call_count['n'] += 1
            if call_count['n'] == 1 and not ignore_errors:
                raise PermissionError('simulated AV lock on cleanup')
            return real_rmtree(path, ignore_errors=ignore_errors, **kwargs)

        with mock.patch('shutil.rmtree', side_effect=flaky_rmtree):
            with idf_injector._safe_temp_dir('esp_bmgr_unit_flaky_') as temp_dir:
                captured_dir['path'] = Path(temp_dir)
                self.assertTrue(captured_dir['path'].is_dir())

        self.assertFalse(captured_dir['path'].exists())
        self.assertEqual(call_count['n'], 2)

    def test_discovery_path_failure_preserves_cause_in_message(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            # No bmgr declaration anywhere → Discovery Path is taken.
            _write(project_path / 'main' / 'idf_component.yml', 'dependencies:\n  test/a: "*"\n')
            context = idf_injector.CliContext(
                True, project_path, True, ('idf.py', 'gen-bmgr-config')
            )
            sentinel = RuntimeError('SolverError: no version for esp32 target')

            with mock.patch.object(
                idf_injector, '_bootstrap_project_dependencies', side_effect=sentinel
            ):
                with self.assertRaises(
                    idf_injector.MissingBoardManagerDependencyError
                ) as cm:
                    idf_injector._bootstrap_for_context(context)

            message = str(cm.exception)
            self.assertIn('Failed to resolve project dependency graph', message)
            self.assertIn('SolverError: no version for esp32 target', message)
            # Both major sections carry the ASSIST_TAG so they stand out in mixed logs.
            self.assertIn(f'{idf_injector.ASSIST_TAG} Underlying cause:', message)
            self.assertIn(f'{idf_injector.ASSIST_TAG} Hints:', message)
            # The hint must surface the actual bootstrap target and its resolution
            # source so the user can understand *why* this path was triggered.
            self.assertIn('IDF_TARGET=esp32', message)
            self.assertIn('source: fallback-default', message)
            self.assertIn('idf.py set-target', message)
            self.assertIn('main/idf_component.yml', message)
            self.assertIs(cm.exception.__cause__, sentinel)

    def test_main_prints_cause_traceback_always(self) -> None:
        idf_injector._MAIN_EXECUTED = False
        cause = RuntimeError('network unreachable')
        wrapper = idf_injector.MissingBoardManagerDependencyError(
            'Failed to resolve project dependency graph while looking for esp_board_manager.\n'
        )
        wrapper.__cause__ = cause
        context = idf_injector.CliContext(
            True, Path('/tmp/project'), True, ('idf.py', 'gen-bmgr-config', '-l')
        )

        # Ensure DEBUG is off to prove traceback is printed unconditionally.
        os.environ.pop('ESP_BMGR_DEBUG', None)

        with mock.patch.object(idf_injector, '_parse_cli_context', return_value=context):
            with mock.patch.object(
                idf_injector, '_bootstrap_for_context', side_effect=wrapper
            ):
                with mock.patch('sys.stderr') as stderr:
                    with self.assertRaises(SystemExit):
                        idf_injector._main()

        written = ''.join(call.args[0] for call in stderr.write.call_args_list if call.args)
        self.assertIn('Failed to resolve project dependency graph', written)
        self.assertIn('Underlying cause traceback', written)
        self.assertIn('RuntimeError', written)
        self.assertIn('network unreachable', written)

    def test_main_skips_bootstrap_for_version_query(self) -> None:
        idf_injector._MAIN_EXECUTED = False
        context = idf_injector.CliContext(True, Path('/tmp/project'), False, ('idf.py', '--version'))
        with mock.patch.object(idf_injector, '_parse_cli_context', return_value=context):
            with mock.patch.object(idf_injector, '_emit_import_notice_once') as mocked_notice:
                with mock.patch.object(idf_injector, '_bootstrap_for_context') as mocked_bootstrap:
                    idf_injector._main()

        mocked_notice.assert_not_called()
        mocked_bootstrap.assert_not_called()

    def test_main_keeps_completion_quiet_while_preserving_placeholder_path(self) -> None:
        idf_injector._MAIN_EXECUTED = False
        context = idf_injector.CliContext(True, Path('/tmp/project'), False, ('idf.py', 'bm'))
        with mock.patch.dict(os.environ, {'_IDF.PY_COMPLETE': 'zsh_source'}, clear=False):
            with mock.patch.object(idf_injector, '_parse_cli_context', return_value=context):
                with mock.patch.object(idf_injector, '_emit_import_notice_once') as mocked_notice:
                    with mock.patch.object(idf_injector, '_bootstrap_for_context') as mocked_bootstrap:
                        idf_injector._main()
            self.assertTrue(_actions_path_contains(Path(idf_injector.__file__).resolve().parent))

        mocked_notice.assert_not_called()
        mocked_bootstrap.assert_not_called()

    def test_set_target_is_passive_and_does_not_clean_generated_files(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            component_dir = project_path / 'components' / 'esp_board_manager'
            _create_bmgr_component(component_dir)
            generated_file = project_path / 'components' / 'gen_bmgr_codes' / 'marker.txt'
            _write(generated_file, 'keep me')
            context = idf_injector.CliContext(True, project_path, False, ('idf.py', 'set-target', 'esp32s3'))

            with mock.patch.object(idf_injector, '_bootstrap_project_dependencies') as mocked_download:
                idf_injector._bootstrap_for_context(context)

            self.assertTrue(generated_file.exists())
            self.assertTrue(_actions_path_contains(component_dir))
            mocked_download.assert_not_called()

    def test_project_lock_times_out_when_already_held(self) -> None:
        if idf_injector.FileLock is None:
            self.skipTest('filelock is not installed in this environment')

        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            held_lock = idf_injector.FileLock(str(idf_injector._get_lock_file(project_path)))

            with held_lock.acquire(timeout=1):
                with self.assertRaises(RuntimeError):
                    with idf_injector._project_lock(project_path, timeout=0.01):
                        self.fail('lock should not have been acquired')

    def test_project_lock_timeout_error_includes_lock_diagnostics(self) -> None:
        if idf_injector.FileLock is None:
            self.skipTest('filelock is not installed in this environment')

        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            lock_path = idf_injector._get_lock_file(project_path)
            held_lock = idf_injector.FileLock(str(lock_path))

            with held_lock.acquire(timeout=1):
                with self.assertRaises(RuntimeError) as cm:
                    with idf_injector._project_lock(project_path, timeout=0.01):
                        self.fail('lock should not have been acquired')

            message = str(cm.exception)
            self.assertIn('Timed out waiting for board manager bootstrap lock', message)
            self.assertIn(str(project_path), message)
            self.assertIn(str(lock_path), message)
            self.assertIn(f'lsof {lock_path}', message)
            self.assertIn(f'fuser -v {lock_path}', message)

    def test_project_lock_wait_notice_includes_lock_diagnostics(self) -> None:
        if idf_injector.FileLock is None:
            self.skipTest('filelock is not installed in this environment')

        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            lock_path = idf_injector._get_lock_file(project_path)
            stderr = io.StringIO()

            with contextlib.redirect_stderr(stderr):
                with idf_injector._project_lock(project_path, timeout=0.01):
                    pass

            output = stderr.getvalue()
            self.assertIn('Waiting for board manager bootstrap lock', output)
            self.assertIn(str(project_path), output)
            self.assertIn(str(lock_path), output)
            self.assertIn(f'lsof {lock_path}', output)
            self.assertIn(f'fuser -v {lock_path}', output)

    def test_lock_timeout_defaults_to_30_seconds(self) -> None:
        with mock.patch.dict(os.environ, {}, clear=True):
            self.assertEqual(idf_injector._resolve_lock_timeout(), 30.0)

    def test_lock_timeout_can_be_overridden_by_env(self) -> None:
        with mock.patch.dict(os.environ, {idf_injector.LOCK_TIMEOUT_ENV: '12.5'}):
            self.assertEqual(idf_injector._resolve_lock_timeout(), 12.5)

    def test_get_lock_file_prefers_tmp_directory_when_available(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            lock_file = idf_injector._get_lock_file(project_path)

            self.assertNotEqual(lock_file.parent, project_path)
            self.assertEqual(lock_file.parent.name, idf_injector.LOCK_FALLBACK_DIR)

    def test_get_lock_file_falls_back_to_project_when_tmp_directory_is_unavailable(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            with mock.patch.object(
                idf_injector,
                '_fallback_lock_file',
                side_effect=OSError(errno.EROFS, 'tmp lock dir unavailable'),
            ):
                lock_file = idf_injector._get_lock_file(project_path)

            self.assertEqual(lock_file, project_path / idf_injector.LOCK_FILE_NAME)

    def test_project_lock_continues_without_lock_when_lock_path_is_unavailable(self) -> None:
        if idf_injector.FileLock is None:
            self.skipTest('filelock is not installed in this environment')

        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            steps = []

            with mock.patch.object(
                idf_injector,
                '_get_lock_file',
                side_effect=OSError(errno.EACCES, 'lock path unavailable'),
            ):
                with idf_injector._project_lock(project_path, timeout=0.01):
                    steps.append('entered')

            self.assertEqual(steps, ['entered'])

    def test_lock_serializes_competing_bootstrap_attempts(self) -> None:
        if idf_injector.FileLock is None:
            self.skipTest('filelock is not installed in this environment')

        with tempfile.TemporaryDirectory() as temp_dir:
            project_path = Path(temp_dir)
            _create_project(project_path)
            order = []

            def worker(name: str) -> None:
                with idf_injector._project_lock(project_path, timeout=1):
                    order.append(f'{name}-start')
                    time.sleep(0.05)
                    order.append(f'{name}-end')

            first = threading.Thread(target=worker, args=('first',))
            second = threading.Thread(target=worker, args=('second',))
            first.start()
            time.sleep(0.01)
            second.start()
            first.join()
            second.join()

            self.assertEqual(order, ['first-start', 'first-end', 'second-start', 'second-end'])


if __name__ == '__main__':
    unittest.main()
