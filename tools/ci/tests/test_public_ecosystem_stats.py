# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET


CI_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CI_ROOT))
from public_ecosystem_stats import public_files


BANNERS = {'ecosystem-banner-{}-{}.svg'.format(language, theme)
           for language in ('en', 'zh') for theme in ('light', 'dark')}
SVG = {'s': 'http://www.w3.org/2000/svg'}


class PublicEcosystemBannerTests(unittest.TestCase):
    def snapshot(self):
        return {'updated_at': '2026-11-09T00:00:00Z',
                'boards': {'count': 1042, 'approved_count': 23},
                'projects': {'count': 17}}

    def banners(self, snapshot=None):
        files = public_files(self.snapshot() if snapshot is None else snapshot)
        self.assertTrue(BANNERS <= set(files), 'All four localized banner variants must be generated')
        return {name: ET.fromstring(files[name]) for name in BANNERS}

    def test_eight_public_files_keep_existing_badge_urls_and_json_schema(self):
        files = public_files(self.snapshot())
        self.assertEqual(set(files), BANNERS | {
            'stats.json', 'supported-boards.svg', 'dependent-projects.svg', 'README.md'})
        self.assertEqual(json.loads(files['stats.json']), {
            'schema_version': 1, 'updated_at': '2026-11-09T00:00:00Z',
            'supported_boards': 23, 'public_dependent_projects': 17})

    def test_all_variants_use_approved_counts_and_localized_accessible_labels(self):
        labels = {'en': ('Supported boards', 'Projects using Board Manager'),
                  'zh': ('适配的板卡', '支持的项目')}
        for name, root in self.banners().items():
            language = name.split('-')[2]
            with self.subTest(name=name):
                self.assertEqual(root.find('.//s:text[@id="supported-boards-count"]', SVG).text, '23')
                self.assertEqual(root.find('.//s:text[@id="projects-count"]', SVG).text, '17')
                self.assertEqual(root.find('.//s:text[@id="supported-boards-label"]', SVG).text,
                                 labels[language][0])
                self.assertEqual(root.find('.//s:text[@id="projects-label"]', SVG).text,
                                 labels[language][1])
                self.assertEqual(root.get('{http://www.w3.org/XML/1998/namespace}lang'),
                                 'zh-CN' if language == 'zh' else 'en')
                for identifier in root.get('aria-labelledby').split():
                    self.assertTrue(root.find('.//*[@id="{}"]'.format(identifier)).text)
                description = root.find('s:desc', SVG).text
                self.assertIn('23', description)
                self.assertIn('17', description)
        readme = public_files(self.snapshot())['README.md']
        self.assertIn('适配的板卡', readme)
        self.assertIn('支持的项目', readme)

    def test_unreviewed_count_fallback_and_zero_values_are_generated(self):
        snapshot = self.snapshot()
        del snapshot['boards']['approved_count']
        snapshot['boards']['count'] = 0
        snapshot['projects']['count'] = 0
        for root in self.banners(snapshot).values():
            for identifier in ('supported-boards-count', 'projects-count'):
                self.assertEqual(root.find('.//s:text[@id="{}"]'.format(identifier), SVG).text, '0')

    def test_template_resolution_does_not_depend_on_current_directory(self):
        previous = Path.cwd()
        try:
            with tempfile.TemporaryDirectory() as temporary:
                os.chdir(temporary)
                self.banners()
        finally:
            os.chdir(previous)

    def test_banners_are_self_contained_static_svg_and_keep_original_brand_paths(self):
        original = ET.parse(CI_ROOT.parents[1] / 'docs/_static/bmgr_banner.svg').getroot()
        original_paths = [node.attrib for node in original.findall('s:path', SVG)]
        forbidden = {'script', 'image', 'foreignObject', 'animate', 'animateTransform',
                     'animateMotion', 'set', 'linearGradient', 'radialGradient'}
        for name, root in self.banners().items():
            with self.subTest(name=name):
                brand = root.find('s:g[@id="brand-artwork"]', SVG)
                self.assertEqual([node.attrib for node in brand.findall('s:path', SVG)], original_paths)
                for node in root.iter():
                    self.assertNotIn(node.tag.rsplit('}', 1)[-1], forbidden)
                    for key, value in node.attrib.items():
                        self.assertFalse(key.rsplit('}', 1)[-1].lower().startswith('on'))
                        self.assertNotEqual(key.rsplit('}', 1)[-1], 'href')
                        self.assertNotIn('url(', value)

    def test_private_snapshot_fields_cannot_enter_any_public_artifact(self):
        snapshot = self.snapshot()
        marker = 'PRIVATE-CANDIDATE-PATH-DO-NOT-PUBLISH'
        snapshot['decision_summary'] = {'candidates': [marker]}
        snapshot['boards']['paths'] = [marker]
        snapshot['projects']['repositories'] = [{'repository': marker}]
        self.banners(snapshot)
        self.assertNotIn(marker, ''.join(public_files(snapshot).values()))

    def test_dark_variants_preserve_layout_and_brand_geometry(self):
        roots = self.banners()
        palette_attributes = {'fill', 'stroke', 'fill-opacity', 'stroke-opacity'}
        for language in ('en', 'zh'):
            light = roots['ecosystem-banner-{}-light.svg'.format(language)]
            dark = roots['ecosystem-banner-{}-dark.svg'.format(language)]
            self.assertEqual(len(list(light.iter())), len(list(dark.iter())))
            for left, right in zip(light.iter(), dark.iter()):
                self.assertEqual(left.tag, right.tag)
                self.assertEqual(left.text, right.text)
                self.assertEqual({k: v for k, v in left.attrib.items() if k not in palette_attributes},
                                 {k: v for k, v in right.attrib.items() if k not in palette_attributes})
            self.assertNotEqual(light.find('.//*[@id="statistics-panel"]').get('fill'),
                                dark.find('.//*[@id="statistics-panel"]').get('fill'))


if __name__ == '__main__':
    unittest.main()
