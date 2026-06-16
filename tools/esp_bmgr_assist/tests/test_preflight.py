import io
import os
import tempfile
import types
import unittest
from contextlib import redirect_stdout
from pathlib import Path

from esp_bmgr_py import artifact_check, preflight, project_integration, project_paths, target_check, task_policy


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding='utf-8')


def _create_project(root: Path) -> None:
    _write(
        root / 'CMakeLists.txt',
        'include($ENV{IDF_PATH}/tools/cmake/project.cmake)\nproject(test)\n',
    )
    (root / 'main').mkdir(parents=True, exist_ok=True)
    _write(root / 'main' / 'CMakeLists.txt', 'idf_component_register(SRCS "main.c")\n')


def _create_manifest_dependency(root: Path) -> None:
    _write(
        root / 'main' / 'idf_component.yml',
        'dependencies:\n'
        "  espressif/esp_board_manager: \"*\"\n",
    )


class PreflightModuleTests(unittest.TestCase):
    def setUp(self) -> None:
        self._env_backup = dict(os.environ)

    def tearDown(self) -> None:
        os.environ.clear()
        os.environ.update(self._env_backup)

    def test_task_policy_marks_build_like_commands(self) -> None:
        tasks = [types.SimpleNamespace(name='build', aliases=[])]
        self.assertTrue(task_policy.is_build_like(tasks))
        self.assertFalse(task_policy.is_bmgr_command(tasks))

    def test_project_dir_resolution_prefers_global_args(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            fallback = Path(temp_dir) / 'fallback'
            explicit = Path(temp_dir) / 'explicit'
            fallback.mkdir()
            explicit.mkdir()

            resolved = project_paths.resolve_project_dir(
                {'project_dir': str(explicit)},
                str(fallback),
            )
            self.assertEqual(resolved, explicit.resolve())

    def test_project_integration_accepts_direct_manifest_dependency(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project = Path(temp_dir)
            _create_project(project)
            _create_manifest_dependency(project)

            self.assertTrue(project_integration.project_uses_board_manager(project))

    def test_artifact_check_reports_missing_required_files(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project = Path(temp_dir)
            missing = artifact_check.list_missing_gen_files(project)
            self.assertIn('board_manager.defaults', missing)
            self.assertIn('gen_board_info.c', missing)

    def test_target_check_detects_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            defaults_path = root / 'board_manager.defaults'
            sdkconfig_path = root / 'sdkconfig'
            _write(defaults_path, 'CONFIG_IDF_TARGET="esp32s3"\n')
            _write(sdkconfig_path, 'CONFIG_IDF_TARGET="esp32c5"\n')

            message = target_check.find_target_mismatch(defaults_path, sdkconfig_path)
            self.assertIsNotNone(message)
            self.assertIn('esp32s3', message)
            self.assertIn('esp32c5', message)

    def test_preflight_is_enabled_by_default(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project = Path(temp_dir)
            _create_project(project)
            _create_manifest_dependency(project)
            output = io.StringIO()
            with self.assertRaises(preflight.PreflightAbortError) as ctx:
                with redirect_stdout(output):
                    preflight.run_preflight(
                        str(project),
                        {'project_dir': str(project)},
                        [types.SimpleNamespace(name='build', aliases=[])],
                    )
            text = str(ctx.exception)
            self.assertEqual(output.getvalue(), '')
            self.assertIn('missing generated board artifacts', text)
            self.assertIn('[ESP_BMGR_ASSIST] Error:', text)
            self.assertIn('ESP_BMGR_ASSIST_PREFLIGHT=0 idf.py build', text)
            self.assertEqual(os.environ.get(preflight.SKIP_GEN_CHECK_ENV), '1')

    def test_preflight_skip_hint_uses_current_build_like_task(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project = Path(temp_dir)
            _create_project(project)
            _create_manifest_dependency(project)
            output = io.StringIO()
            with self.assertRaises(preflight.PreflightAbortError) as ctx:
                with redirect_stdout(output):
                    preflight.run_preflight(
                        str(project),
                        {'project_dir': str(project)},
                        [types.SimpleNamespace(name='flash', aliases=[])],
                    )
            text = str(ctx.exception)
            self.assertEqual(output.getvalue(), '')
            self.assertIn('missing generated board artifacts', text)
            self.assertIn('ESP_BMGR_ASSIST_PREFLIGHT=0 idf.py flash', text)

    def test_preflight_can_be_disabled_via_env_zero(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project = Path(temp_dir)
            _create_project(project)
            _create_manifest_dependency(project)
            os.environ[preflight.ASSIST_PREFLIGHT_ENV] = '0'
            output = io.StringIO()
            with redirect_stdout(output):
                preflight.run_preflight(
                    str(project),
                    {'project_dir': str(project)},
                    [types.SimpleNamespace(name='build', aliases=[])],
                )
            self.assertEqual(output.getvalue(), '')
            self.assertEqual(os.environ.get(preflight.SKIP_GEN_CHECK_ENV), '1')

    def test_preflight_warns_for_missing_generated_artifacts_when_level_is_warning(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project = Path(temp_dir)
            _create_project(project)
            _create_manifest_dependency(project)
            os.environ[preflight.ASSIST_PREFLIGHT_ENV] = '1'
            output = io.StringIO()
            with redirect_stdout(output):
                preflight.run_preflight(
                    str(project),
                    {'project_dir': str(project)},
                    [types.SimpleNamespace(name='build', aliases=[])],
                )

            text = output.getvalue()
            self.assertIn('missing generated board artifacts', text)
            self.assertIn('[ESP_BMGR_ASSIST] Warning:', text)
            self.assertIn('ESP_BMGR_ASSIST_PREFLIGHT=0 idf.py build', text)
            self.assertEqual(os.environ.get(preflight.SKIP_GEN_CHECK_ENV), '1')
            self.assertIsNone(os.environ.get(preflight.SKIP_SDKCONFIG_CHECK_ENV))

    def test_preflight_respects_global_arg_preflight_level(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project = Path(temp_dir)
            _create_project(project)
            _create_manifest_dependency(project)
            output = io.StringIO()
            with redirect_stdout(output):
                preflight.run_preflight(
                    str(project),
                    {
                        'project_dir': str(project),
                        preflight.ASSIST_PREFLIGHT_LEVEL_ARG: 'warning',
                    },
                    [types.SimpleNamespace(name='build', aliases=[])],
                )

            text = output.getvalue()
            self.assertIn('missing generated board artifacts', text)
            self.assertIn('[ESP_BMGR_ASSIST] Warning:', text)

    def test_preflight_errors_for_target_mismatch_by_default(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project = Path(temp_dir)
            _create_project(project)
            _create_manifest_dependency(project)
            gen_dir = project / 'components' / 'gen_bmgr_codes'
            gen_dir.mkdir(parents=True, exist_ok=True)
            _write(gen_dir / 'board_manager.defaults', 'CONFIG_IDF_TARGET="esp32s3"\n')
            _write(project / 'sdkconfig', 'CONFIG_IDF_TARGET="esp32c5"\n')
            output = io.StringIO()
            with self.assertRaises(preflight.PreflightAbortError) as ctx:
                with redirect_stdout(output):
                    preflight.run_preflight(
                        str(project),
                        {'project_dir': str(project)},
                        [types.SimpleNamespace(name='build', aliases=[])],
                    )

            text = str(ctx.exception)
            self.assertEqual(output.getvalue(), '')
            self.assertIn('[ESP_BMGR_ASSIST] Error:', text)
            self.assertIn('CONFIG_IDF_TARGET mismatch', text)
            self.assertIn('ESP_BMGR_ASSIST_PREFLIGHT=0 idf.py build', text)

    def test_preflight_skips_non_build_commands(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            project = Path(temp_dir)
            _create_project(project)
            _create_manifest_dependency(project)
            output = io.StringIO()
            with redirect_stdout(output):
                preflight.run_preflight(
                    str(project),
                    {'project_dir': str(project)},
                    [types.SimpleNamespace(name='set-target', aliases=[])],
                )

            self.assertEqual(output.getvalue(), '')

    def test_resolve_preflight_level_supports_numeric_and_named_values(self) -> None:
        self.assertEqual(
            preflight.resolve_preflight_level({preflight.ASSIST_PREFLIGHT_LEVEL_ARG: 'warning'}),
            preflight.PREFLIGHT_LEVEL_WARNING,
        )

        os.environ[preflight.ASSIST_PREFLIGHT_ENV] = 'silent'
        self.assertEqual(
            preflight.resolve_preflight_level(),
            preflight.PREFLIGHT_LEVEL_SILENT,
        )

        os.environ[preflight.ASSIST_PREFLIGHT_ENV] = '2'
        self.assertEqual(
            preflight.resolve_preflight_level(),
            preflight.PREFLIGHT_LEVEL_ERROR,
        )
