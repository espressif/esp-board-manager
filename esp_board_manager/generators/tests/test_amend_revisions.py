# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
#
# See LICENSE file for details.

import sys
import subprocess
import shutil
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from generators.amend import (
    AmendError,
    AmendPlan,
    AmendFragment,
    KIND_YAML,
    apply_amend_plan_to_yaml,
    build_amend_plan,
    build_revision_overlay_plan,
)
import generators.amend as amend_module
import gen_bmgr_config_codes as bmgr_module
from gen_bmgr_config_codes import BoardConfigGenerator


class TestAmendRevisions(unittest.TestCase):
    @unittest.skipUnless(shutil.which('cc'), 'host C compiler is required')
    def test_revision_append_uses_configured_capacity_and_can_retry(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = '''#include <stddef.h>
#include <string.h>
#include <assert.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_INVALID_STATE 1
#define CONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM 4
#define ESP_BOARD_DEVICE_TABLE_MAX_NUM 2
typedef struct desc {
    const struct desc *next;
    const char *name;
    int value;
} esp_board_device_desc_t;
static esp_board_device_desc_t base_tail = {.name = "base_tail"};
static esp_board_device_desc_t g_esp_board_devices[] = {
    {.next = &base_tail, .name = "base_head"}
};
'''
            source += BoardConfigGenerator._revision_table_helpers(
                'device', 'esp_board_device_desc_t', 'g_esp_board_devices'
            )
            source += '''int main(void) {
    esp_board_device_desc_t added_a = {.name = "added_a", .value = 3};
    esp_board_device_desc_t added_b = {.name = "added_b", .value = 4};
    esp_board_device_desc_t too_many = {.name = "too_many"};
    assert(revision_device_replace_or_append(&added_a) == ESP_OK);
    assert(revision_device_replace_or_append(&added_b) == ESP_OK);
    assert(base_tail.next == &added_a && added_a.next == &added_b);
    assert(revision_device_replace_or_append(&too_many) == ESP_ERR_INVALID_STATE);
    assert(revision_device_replace_or_append(&added_b) == ESP_OK);
    assert(added_b.next == NULL);
    assert(revision_device_unlink("base_tail") == ESP_OK);
    assert(revision_device_replace_or_append(&too_many) == ESP_OK);
    assert(added_b.next == &too_many);
    assert(revision_device_unlink("base_head") == ESP_OK);
    assert(strcmp(g_esp_board_devices[0].name, "added_a") == 0);
    assert(revision_device_unlink("added_a") == ESP_OK);
    assert(revision_device_unlink("added_b") == ESP_OK);
    assert(revision_device_unlink("too_many") == ESP_OK);
    assert(g_esp_board_devices[0].name == NULL);
    assert(revision_device_replace_or_append(&base_tail) == ESP_OK);
    assert(strcmp(g_esp_board_devices[0].name, "base_tail") == 0);
    assert(g_esp_board_devices[0].next == NULL);
    return 0;
}
'''
            c_path = root / 'revision_test.c'
            c_path.write_text(source, encoding='utf-8')
            executable = root / 'revision_test'
            subprocess.run(['cc', '-std=c17', '-Wall', '-Wextra', '-Werror',
                            str(c_path), '-o', str(executable)], check=True, capture_output=True)
            subprocess.run([str(executable)], check=True, capture_output=True)

    def make_manifest(self, manifest, files):
        root = Path(tempfile.mkdtemp())
        (root / 'board_amend.yaml').write_text(manifest, encoding='utf-8')
        for name, content in files.items():
            (root / name).write_text(content, encoding='utf-8')
        self.addCleanup(lambda: __import__('shutil').rmtree(root, ignore_errors=True))
        return root

    def test_revision_only_manifest_resolves_yaml_fragments(self):
        root = self.make_manifest(
            'board_revisions:\n  v1_2:\n    - v1_2.yaml\n',
            {'v1_2.yaml': 'peripherals:\n  - name: i2c_subboard\n'},
        )

        plan = build_amend_plan(root, allow_board_revisions=True, board_manifest=True)

        self.assertEqual(list(plan.board_revisions), ['v1_2'])
        self.assertEqual(plan.board_revisions['v1_2'][0].yaml_data['peripherals'][0]['name'], 'i2c_subboard')

    def test_multiple_revisions_with_shared_fragment_merge_in_order(self):
        self._check_multiple_revision_views(inherit=True)

    def test_multiple_revisions_without_shared_fragment_are_independent(self):
        self._check_multiple_revision_views(inherit=False)

    def test_revision_unused_filters_devices_and_peripherals(self):
        root = self.make_manifest(
            'board_revisions:\n  removed:\n    - unused.yaml\n'
            '  restored:\n    - unused.yaml\n    - restored.yaml\n',
            {
                'unused.yaml': 'devices:\n  - name: camera\n    unused: true\n'
                               'peripherals:\n  - name: bus\n    unused: true\n',
                'restored.yaml': 'devices:\n  - name: camera\n    unused: false\n'
                                 'peripherals:\n  - name: bus\n    unused: false\n',
            },
        )
        board_plan = build_amend_plan(root, allow_board_revisions=True, board_manifest=True)
        for revision, skipped in [('removed', True), ('restored', False)]:
            plan = build_revision_overlay_plan(revision, board_plan, None)
            for kind, name in [('devices', 'camera'), ('peripherals', 'bus')]:
                view = {kind: [{'name': name, 'type': 'test'}]}
                apply_amend_plan_to_yaml(view, plan)
                self.assertEqual(len(view[kind]), 0 if skipped else 1)
                if not skipped:
                    self.assertNotIn('unused', view[kind][0])
                    self.assertNotIn('gen_skip', view[kind][0])

    def test_revision_unused_requires_boolean(self):
        root = self.make_manifest(
            'board_revisions:\n  v1_2:\n    - revision.yaml\n',
            {'revision.yaml': 'devices:\n  - name: camera\n    unused: "true"\n'},
        )
        with self.assertRaisesRegex(AmendError, 'unused.*boolean'):
            build_amend_plan(root, allow_board_revisions=True, board_manifest=True)

    def test_application_amend_cannot_set_unused(self):
        root = self.make_manifest(
            'apply:\n  - patch.yaml\n',
            {'patch.yaml': 'devices:\n  - name: camera\n    unused: false\n'},
        )
        with self.assertRaisesRegex(AmendError, 'unused is supported only in board revisions'):
            build_amend_plan(root)

    def test_application_amend_does_not_restore_unused_resources(self):
        root = self.make_manifest(
            'board_revisions:\n  removed:\n    - patch.yaml\n',
            {'patch.yaml': 'devices:\n  - name: camera\n    unused: true\n'
                           'peripherals:\n  - name: bus\n    unused: true\n'},
        )
        board = build_amend_plan(root, allow_board_revisions=True, board_manifest=True)
        for skip in (False, True):
            fragment = AmendFragment(
                kind=KIND_YAML, resolved_path=root / 'app.yaml', raw_item='app.yaml', item_index=0,
                yaml_data={kind: [{'name': name, 'gen_skip': skip, 'config': {'value': 42}}]
                           for kind, name in [('devices', 'camera'), ('peripherals', 'bus')]},
            )
            app = AmendPlan(root, root / 'app.yaml', fragments=[fragment])
            plan = build_revision_overlay_plan('removed', board, app)
            for kind, name in [('devices', 'camera'), ('peripherals', 'bus')]:
                view = {kind: [{'name': name, 'type': 'test'}]}
                apply_amend_plan_to_yaml(view, plan)
                self.assertEqual(view[kind], [])

    def test_revision_rejects_gen_skip(self):
        for section in ('devices', 'peripherals'):
            for value in ('true', 'false'):
                with self.subTest(section=section, value=value):
                    root = self.make_manifest(
                        'board_revisions:\n  v1_2:\n    - revision.yaml\n',
                        {'revision.yaml': f'{section}:\n  - name: resource\n    gen_skip: {value}\n'},
                    )
                    with self.assertRaisesRegex(AmendError, 'gen_skip is not supported; use unused'):
                        build_amend_plan(root, allow_board_revisions=True, board_manifest=True)

    def _check_multiple_revision_views(self, inherit):
        shared_fragment = '    - board_revision_v1_2.yaml\n' if inherit else ''
        root = self.make_manifest(
            'board_revisions:\n'
            '  v1_2:\n    - board_revision_v1_2.yaml\n'
            '  v1_3:\n' + shared_fragment + '    - board_revision_v1_3.yaml\n',
            {
                'board_revision_v1_2.yaml':
                    'devices:\n  - name: camera\n    config: {width: 800, height: 600}\n'
                    '  - name: revision_12_device\n    type: custom\n'
                    'peripherals:\n  - name: i2c_master\n    config: {sda: 12, scl: 13}\n',
                'board_revision_v1_3.yaml':
                    'devices:\n  - name: camera\n    config: {width: 1280}\n'
                    '  - name: revision_13_device\n    type: custom\n'
                    'peripherals:\n  - name: i2c_master\n    config: {sda: 20}\n',
            },
        )
        board_plan = build_amend_plan(root, allow_board_revisions=True, board_manifest=True)
        self.assertEqual(list(board_plan.board_revisions), ['v1_2', 'v1_3'])

        def revision_view(name):
            plan = build_revision_overlay_plan(name, board_plan, None)
            devices = {'devices': [
                {'name': 'camera', 'type': 'camera',
                 'config': {'width': 640, 'height': 480, 'mirror': False}},
            ]}
            peripherals = {'peripherals': [
                {'name': 'i2c_master', 'type': 'i2c',
                 'config': {'sda': 1, 'scl': 2, 'frequency': 100000}},
            ]}
            apply_amend_plan_to_yaml(devices, plan)
            apply_amend_plan_to_yaml(peripherals, plan)
            return devices, peripherals

        v12 = revision_view('v1_2')
        v13 = revision_view('v1_3')
        self.assertEqual(v12[0]['devices'][0]['config'],
                         {'width': 800, 'height': 600, 'mirror': False})
        self.assertEqual(v12[1]['peripherals'][0]['config'],
                         {'sda': 12, 'scl': 13, 'frequency': 100000})
        self.assertEqual(v13[0]['devices'][0]['config'],
                         {'width': 1280, 'height': 600 if inherit else 480, 'mirror': False})
        self.assertEqual(v13[1]['peripherals'][0]['config'],
                         {'sda': 20, 'scl': 13 if inherit else 2, 'frequency': 100000})
        self.assertEqual([d['name'] for d in v12[0]['devices']],
                         ['camera', 'revision_12_device'])
        self.assertEqual([d['name'] for d in v13[0]['devices']],
                         ['camera', 'revision_12_device', 'revision_13_device'] if inherit
                         else ['camera', 'revision_13_device'])
        # Regeneration in reverse order must not mutate shared fragment data.
        self.assertEqual(revision_view('v1_3'), v13)
        self.assertEqual(revision_view('v1_2'), v12)

    @unittest.skipUnless(shutil.which('cc'), 'host C compiler is required')
    def test_multiple_revision_views_generate_and_apply(self):
        for inherit in (False, True):
            for app_skip in (None, False, True):
                with self.subTest(inherit=inherit, app_skip=app_skip):
                    self._compile_revision_views(inherit, app_skip)

    def _compile_revision_views(self, inherit, app_skip):
        root = self.make_manifest(
            'board_revisions:\n  v1_2:\n    - board_revision_v1_2.yaml\n'
            '  v1_3:\n' + ('    - board_revision_v1_2.yaml\n' if inherit else '') +
            '    - board_revision_v1_3.yaml\n',
            {
                'board_info.yaml': 'board: fixture\nchip: esp32s3\n',
                'board_devices.yaml': 'devices:\n  - name: controller\n    type: power_ctrl\n    sub_type: custom\n'
                                      '  - name: retired\n    type: power_ctrl\n    sub_type: custom\n',
                'board_peripherals.yaml': 'peripherals: []\n',
                'board_revision_v1_2.yaml':
                    'devices:\n  - name: controller\n    init_skip: true\n'
                    '  - name: retired\n    unused: true\n'
                    '  - name: added12\n    type: power_ctrl\n    sub_type: custom\n',
                'board_revision_v1_3.yaml':
                    'devices:\n  - name: added13\n    type: power_ctrl\n    sub_type: custom\n',
            },
        )
        # A real CLI invocation covers parsing, revision views, config emission,
        # capacity generation, and apply emission together.
        script = Path(__file__).parent.parent.parent / 'gen_bmgr_config_codes.py'
        command = [sys.executable, str(script), '-b', str(root), '--project-dir', str(root)]
        if app_skip is not None:
            app = root / 'app_overlay'
            app.mkdir()
            (app / 'board_amend.yaml').write_text('apply:\n  - patch.yaml\n')
            (app / 'patch.yaml').write_text(
                'devices:\n  - name: controller\n    init_skip: false\n'
                f'  - name: retired\n    init_skip: true\n    gen_skip: {str(app_skip).lower()}\n'
                '  - name: common_added\n    type: power_ctrl\n    sub_type: custom\n'
            )
            command.extend(['-a', str(app)])
        result = subprocess.run(
            command,
            capture_output=True, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        generated = root / 'components' / 'gen_bmgr_codes'
        kconfig = (generated / 'Kconfig.projbuild').read_text()
        capacity = 4 if app_skip is None or app_skip else 5
        self.assertIn(f'range {capacity} 4096\n    default {capacity}', kconfig)
        # Use the actual public descriptor declarations, with host substitutes
        # only for ESP-IDF dependencies; no hand-written revision arrays.
        include = root / 'host_include'
        include.mkdir()
        for filename, struct_name in [('esp_board_device.h', 'esp_board_device_desc'),
                                      ('esp_board_periph.h', 'esp_board_periph_desc')]:
            public = (script.parent / 'include' / filename).read_text()
            begin = public.index('typedef struct ' + struct_name + ' {')
            end = public.index('} ' + struct_name + '_t;', begin) + len('} ' + struct_name + '_t;')
            (include / filename).write_text(
                '#pragma once\n#include <stdint.h>\n#include <stdbool.h>\n'
                '#include "esp_board_manager_defs.h"\n' + public[begin:end] + '\n'
            )
        (include / 'esp_board_manager_defs.h').write_text(
            '#pragma once\n#define ESP_BOARD_TABLE_CONST\n'
            'typedef enum {ESP_BOARD_PERIPH_ROLE_NONE} esp_board_periph_role_t;\n'
        )
        (include / 'sdkconfig.h').write_text(
            f'#define CONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM {capacity}\n'
            '#define CONFIG_ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM 1\n'
        )
        (include / 'esp_err.h').write_text(
            '#pragma once\ntypedef int esp_err_t;\n#define ESP_OK 0\n#define ESP_ERR_INVALID_STATE 1\n'
        )
        (include / 'dev_power_ctrl.h').write_text(
            '#include <stdint.h>\n'
            'typedef struct {const char *name; const char *sub_type; union {struct {'
            'const char *const *periph_names; uint8_t periph_count; const void *user_cfg;'
            'uint16_t user_cfg_size;} custom;} sub_cfg;} dev_power_ctrl_config_t;\n'
        )
        harness = r'''#include <assert.h>
#include <string.h>
#include "gen_board_revisions.h"
#include "dev_power_ctrl.h"
extern esp_board_device_desc_t g_esp_board_devices[];
static const esp_board_device_desc_t *find(const char *name) {
    const esp_board_device_desc_t *entry = g_esp_board_devices;
    int count = 0;
    for (; entry && entry->name; entry = entry->next) {
        assert(++count <= CAPACITY);
        if (!strcmp(entry->name, name)) return entry;
    }
    return NULL;
}
int main(int argc, char **argv) {
    assert(argc == 2);
    assert(find("controller") && !find("controller")->init_skip);
    assert(!find("added12") && !find("added13"));
    assert((find("retired") != NULL) == RETIRED_BASE);
    assert((find("common_added") != NULL) == APP);
    if (!strcmp(argv[1], "v1_2")) {
        assert(gen_board_revision_apply_v1_2() == ESP_OK);
        assert(find("controller")->init_skip == !APP);
        assert(find("added12") && !find("added13"));
        assert(!find("retired"));
    } else {
        assert(gen_board_revision_apply_v1_3() == ESP_OK);
        assert(find("controller")->init_skip == (INHERIT && !APP));
        assert((find("added12") != NULL) == INHERIT);
        assert(find("added13"));
        assert((find("retired") == NULL) == (INHERIT || !RETIRED_BASE));
    }
    assert(find("controller")->cfg && find("controller")->cfg_size > 0);
    assert(!strcmp(((const dev_power_ctrl_config_t *)find("controller")->cfg)->name, "controller"));
    return 0;
}
'''
        (root / 'harness.c').write_text(harness)
        sources = [root / 'harness.c', generated / 'gen_board_device_config.c',
                   generated / 'gen_board_periph_config.c']
        sources.extend(generated.glob('gen_board_device_config_v*.c'))
        sources.extend(generated.glob('gen_board_periph_config_v*.c'))
        sources.extend(generated.glob('gen_board_revision_v*.c'))
        compiled = subprocess.run(
            ['cc', '-std=c17', '-Wall', '-Wextra', '-Werror', '-Wno-type-limits', '-Wno-old-style-declaration',
             '-DINHERIT=' + str(int(inherit)), '-DAPP=' + str(int(app_skip is not None)),
             '-DRETIRED_BASE=' + str(int(app_skip is not True)), '-DCAPACITY=' + str(capacity), '-I', str(include), '-I', str(generated),
             *map(str, sources), '-o', str(root / 'revision_views')],
            capture_output=True, text=True,
        )
        self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
        # Separate processes ensure each apply starts from generated base state.
        for revision in ('v1_2', 'v1_3'):
            executed = subprocess.run([str(root / 'revision_views'), revision],
                                      capture_output=True, text=True)
            self.assertEqual(executed.returncode, 0, executed.stdout + executed.stderr)

    def test_board_manifest_does_not_warn_about_board_owned_special_files(self):
        root = self.make_manifest(
            'board_revisions:\n  v1_2:\n    - v1_2.yaml\n',
            {
                'v1_2.yaml': 'peripherals:\n  - name: i2c_subboard\n',
                'sdkconfig.defaults.board': 'CONFIG_TEST=y\n',
                'Kconfig.projbuild': 'menu "Board"\nendmenu\n',
            },
        )

        with mock.patch.object(amend_module, '_warn_unreferenced_special_files') as warn:
            build_amend_plan(root, allow_board_revisions=True, board_manifest=True)

        warn.assert_not_called()

    def test_application_amend_cannot_declare_board_revisions(self):
        root = self.make_manifest(
            'board_revisions:\n  v1_2:\n    - v1_2.yaml\n',
            {'v1_2.yaml': 'devices:\n  - name: camera\n'},
        )

        with self.assertRaisesRegex(AmendError, 'only by the selected board manifest'):
            build_amend_plan(root)

    def test_board_manifest_cannot_declare_apply(self):
        root = self.make_manifest(
            'apply:\n  - patch.yaml\nboard_revisions:\n  v1_2:\n    - v1_2.yaml\n',
            {
                'patch.yaml': 'devices:\n  - name: camera\n',
                'v1_2.yaml': 'devices:\n  - name: camera\n',
            },
        )

        with self.assertRaisesRegex(AmendError, "cannot declare 'apply'"):
            build_amend_plan(root, allow_board_revisions=True, board_manifest=True)

    def test_revision_yaml_applies_before_application_amend(self):
        root = self.make_manifest(
            'board_revisions:\n  v1_2:\n    - revision.yaml\n',
            {'revision.yaml': 'devices:\n  - name: camera\n    config:\n      bus: i2c_subboard\n'},
        )
        board_plan = build_amend_plan(root, allow_board_revisions=True, board_manifest=True)
        app_fragment = AmendFragment(
            kind=KIND_YAML,
            resolved_path=root / 'app.yaml',
            raw_item='app.yaml',
            item_index=0,
            yaml_data={'devices': [{'name': 'camera', 'config': {'mirror': True}}]},
        )
        app_plan = AmendPlan(
            amend_dir=root,
            manifest_path=root / 'app-board_amend.yaml',
            fragments=[app_fragment],
        )

        view = {'devices': [{'name': 'camera', 'config': {'bus': 'i2c_master'}}]}
        combined_plan = build_revision_overlay_plan('v1_2', board_plan, app_plan)
        apply_amend_plan_to_yaml(view, combined_plan)

        self.assertEqual(view['devices'][0]['config'], {
            'bus': 'i2c_subboard',
            'mirror': True,
        })

    def test_revision_rejects_non_yaml_file(self):
        root = self.make_manifest(
            'board_revisions:\n  v1_2:\n    - overlay.c\n',
            {'overlay.c': 'int value;\n'},
        )

        with self.assertRaisesRegex(AmendError, 'existing YAML file'):
            build_amend_plan(root, allow_board_revisions=True, board_manifest=True)

    def test_revision_name_must_be_lowercase_identifier(self):
        root = self.make_manifest(
            'board_revisions:\n  V1.2:\n    - v1_2.yaml\n',
            {'v1_2.yaml': 'devices:\n  - name: camera\n'},
        )

        with self.assertRaisesRegex(AmendError, 'invalid revision name'):
            build_amend_plan(root, allow_board_revisions=True, board_manifest=True)

    def test_application_amend_rejects_revision_only_names(self):
        root = self.make_manifest(
            'board_revisions:\n  v1_2:\n    - revision.yaml\n',
            {
                'revision.yaml': 'peripherals:\n  - name: i2c_subboard\n',
                'board_peripherals.yaml': 'peripherals:\n  - name: i2c_master\n',
                'board_devices.yaml': 'devices: []\n',
            },
        )
        board_plan = build_amend_plan(root, allow_board_revisions=True, board_manifest=True)
        app_fragment = AmendFragment(
            kind=KIND_YAML,
            resolved_path=root / 'application.yaml',
            raw_item='application.yaml',
            item_index=0,
            yaml_data={'peripherals': [{'name': 'i2c_subboard', 'config': {'scl': 3}}]},
        )
        app_plan = AmendPlan(root, root / 'app-board_amend.yaml', fragments=[app_fragment])

        with self.assertRaisesRegex(ValueError, 'revision-only names'):
            BoardConfigGenerator._reject_application_amend_revision_only_names(
                root / 'board_peripherals.yaml', root / 'board_devices.yaml', board_plan, app_plan
            )

    def test_application_amend_can_modify_base_names(self):
        root = self.make_manifest(
            'board_revisions:\n  v1_2:\n    - revision.yaml\n',
            {
                'revision.yaml': 'peripherals:\n  - name: i2c_master\n    config:\n      scl: 3\n',
                'board_peripherals.yaml': 'peripherals:\n  - name: i2c_master\n',
                'board_devices.yaml': 'devices: []\n',
            },
        )
        board_plan = build_amend_plan(root, allow_board_revisions=True, board_manifest=True)
        app_fragment = AmendFragment(
            kind=KIND_YAML,
            resolved_path=root / 'application.yaml',
            raw_item='application.yaml',
            item_index=0,
            yaml_data={'peripherals': [{'name': 'i2c_master', 'config': {'sda': 1}}]},
        )
        app_plan = AmendPlan(root, root / 'app-board_amend.yaml', fragments=[app_fragment])

        BoardConfigGenerator._reject_application_amend_revision_only_names(
            root / 'board_peripherals.yaml', root / 'board_devices.yaml', board_plan, app_plan
        )

    def test_generated_revision_apply_is_bounded_and_retryable(self):
        root = Path(tempfile.mkdtemp())
        self.addCleanup(lambda: __import__('shutil').rmtree(root, ignore_errors=True))
        generator = BoardConfigGenerator(Path(__file__).parent.parent.parent)
        generator._write_revision_apply_files(
            root,
            [{
                'name': 'v1_2',
                'device_names': {'camera'},
                'peripheral_names': {'i2c_subboard'},
                'device_delta_num': 1,
                'peripheral_delta_num': 1,
            }],
            {'camera'},
            {'i2c_master'},
            {'camera'},
            {'i2c_master', 'i2c_subboard'},
        )
        source = (root / 'gen_board_revision_v1_2.c').read_text(encoding='utf-8')

        self.assertIn('i < CONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM', source)
        self.assertIn('i < CONFIG_ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM', source)
        self.assertIn('esp_err_t revision_device_replace_or_append', source)
        self.assertIn('esp_err_t revision_periph_replace_or_append', source)
        self.assertNotIn('static bool applied', source)
        self.assertNotIn('ESP_ERR_NOT_FOUND', source)

    def test_revision_handles_include_base_and_overridden_type_role_pairs(self):
        generator = BoardConfigGenerator(Path(__file__).parent.parent.parent)
        base_periph = SimpleNamespace(name='bus', type='i2c', role='master')
        revision_periph = SimpleNamespace(name='bus', type='spi', role='master')
        base_device = SimpleNamespace(name='device', type='test')
        captured = {}

        def process_peripherals(*args, **kwargs):
            generator._last_peripherals = [revision_periph]
            generator._last_peripheral_parsers = {}
            return {}, {}, {'spi'}

        def process_devices(*args, **kwargs):
            generator._last_devices = [base_device]
            generator._last_device_parsers = {}
            return {'test'}, {}

        with mock.patch.object(bmgr_module, 'build_revision_overlay_plan', return_value=None), \
                mock.patch.object(generator, '_reject_application_amend_revision_only_names'), \
                mock.patch.object(generator, 'process_peripherals', side_effect=process_peripherals), \
                mock.patch.object(generator, 'process_devices', side_effect=process_devices), \
                mock.patch.object(generator, 'write_periph_handles',
                                  side_effect=lambda items, *args: captured.setdefault('peripherals', items)), \
                mock.patch.object(generator, 'write_device_handles'), \
                mock.patch.object(generator, '_write_revision_apply_files'):
            generator.board_amend_plan = SimpleNamespace(board_revisions={'rev_a': []})
            generator.amend_plan = None
            generator.generation_mode = 'full'
            generator._generate_board_revisions(
                None, None, None, [base_periph], [base_device], {}, [], set(), {}, set()
            )

        type_roles = {(item.type, item.role) for item in captured['peripherals']}
        self.assertEqual(type_roles, {('i2c', 'master'), ('spi', 'master')})

    def test_pure_writable_kconfig_allows_capacity_for_c_nodes(self):
        root = Path(tempfile.mkdtemp())
        self.addCleanup(lambda: __import__('shutil').rmtree(root, ignore_errors=True))
        generator = BoardConfigGenerator(Path(__file__).parent.parent.parent)
        generator.writable_tables_enabled = True
        generator.descriptor_device_table_max_num = 2
        generator.descriptor_periph_table_max_num = 3

        self.assertTrue(generator.generate_selected_board_kconfig_projbuild('fixture', root))
        kconfig = (root / 'components' / 'gen_bmgr_codes' / 'Kconfig.projbuild').read_text(encoding='utf-8')
        self.assertIn('config ESP_BOARD_MANAGER_WRITABLE_TABLES\n    bool\n    default y', kconfig)
        self.assertIn('int "Maximum device table entries"\n    range 2 4096\n    default 2', kconfig)
        self.assertIn('int "Maximum peripheral table entries"\n    range 3 4096\n    default 3', kconfig)


if __name__ == '__main__':
    unittest.main()
