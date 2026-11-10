# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""Generate aggregate-only public files from a validated private snapshot."""

from datetime import datetime
from html import escape
import json
from pathlib import Path
import xml.etree.ElementTree as ET


SVG_NAMESPACE = 'http://www.w3.org/2000/svg'
ET.register_namespace('', SVG_NAMESPACE)
BANNER_LANGUAGES = {
    'en': {
        'lang': 'en', 'title': 'ESP Board Manager ecosystem statistics',
        'labels': ('Supported boards', 'Projects using Board Manager'),
        'description': '{supported} supported boards. {projects} projects using Board Manager.',
        'font': 'Arial, Helvetica, sans-serif',
        'label_size': '18', 'label_y': '268',
    },
    'zh': {
        'lang': 'zh-CN', 'title': 'ESP Board Manager 生态统计',
        'labels': ('适配的板卡', '支持的项目'),
        'description': '适配的板卡：{supported}；支持的项目：{projects}。',
        'font': 'Noto Sans CJK SC, Microsoft YaHei, PingFang SC, Arial, sans-serif',
        'label_size': '26', 'label_y': '271',
    },
}
BANNER_DARK_STYLES = {
    'statistics-shadow': {'fill': '#17191E', 'fill-opacity': '0.10'},
    'statistics-panel': {'fill': '#24272D', 'stroke': '#353940'},
    'projects-shadow': {'fill': '#17191E', 'fill-opacity': '0.10'},
    'projects-panel': {'fill': '#24272D', 'stroke': '#353940'},
    'supported-boards-icon-background': {'fill': '#3C2C30'},
    'projects-icon-background': {'fill': '#3C2C30'},
    'supported-boards-icon': {'stroke': '#FF7870'},
    'projects-icon': {'stroke': '#FF7870'},
    'supported-boards-count': {'fill': '#F7F8FA'},
    'projects-count': {'fill': '#F7F8FA'},
    'supported-boards-label': {'fill': '#BEC2CA'},
    'projects-label': {'fill': '#BEC2CA'},
}


def _banner(supported, projects, language, theme):
    """Fill only aggregate counts, localized text and the statistics card palettes."""
    template = Path(__file__).resolve().parent / 'templates' / 'ecosystem-banner.svg'
    root = ET.parse(template).getroot()
    nodes = {node.get('id'): node for node in root.iter() if node.get('id')}
    localized = BANNER_LANGUAGES[language]
    root.set('{http://www.w3.org/XML/1998/namespace}lang', localized['lang'])
    nodes['banner-title'].text = localized['title']
    nodes['banner-description'].text = localized['description'].format(supported=supported, projects=projects)
    nodes['supported-boards-count'].text = str(supported)
    nodes['projects-count'].text = str(projects)
    for identifier, label in zip(('supported-boards-label', 'projects-label'), localized['labels']):
        nodes[identifier].text = label
        nodes[identifier].set('font-family', localized['font'])
        nodes[identifier].set('font-size', localized['label_size'])
        nodes[identifier].set('y', localized['label_y'])
    if theme == 'dark':
        for identifier, attributes in BANNER_DARK_STYLES.items():
            nodes[identifier].attrib.update(attributes)
    return ET.tostring(root, encoding='unicode') + '\n'


def badge(label, value, color):
    left = max(128, len(label) * 8 + 32)
    right = max(56, len(value) * 10 + 32)
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="32" viewBox="0 0 {width} 32" '
            'role="img" aria-label="{title}">'
            '<title>{title}</title><rect width="{width}" height="32" rx="4" fill="#555"/>'
            '<path fill="{color}" d="M{left} 0h{right}v32H{left}z"/>'
            '<g fill="#fff" text-anchor="middle" font-family="Verdana,DejaVu Sans,sans-serif">'
            '<text x="{lx}" y="21" font-size="14">{label}</text>'
            '<text x="{rx}" y="22" font-size="16" font-weight="700">{value}</text></g></svg>\n').format(
                width=left + right, title=escape(label + ': ' + value, quote=True), color=color,
                left=left, right=right, lx=left / 2, rx=left + right / 2, label=escape(label), value=escape(value))


def public_files(snapshot):
    """Return eight freshly generated files, with no private metadata copied."""
    updated_at = snapshot['updated_at']
    timestamp_format = '%Y-%m-%dT%H:%M:%SZ'
    if (not isinstance(updated_at, str)
            or datetime.strptime(updated_at, timestamp_format).strftime(timestamp_format) != updated_at):
        raise ValueError('Statistics update time must be a UTC timestamp')
    boards = snapshot['boards']
    supported = boards.get('approved_count', boards['count'])
    projects = snapshot['projects']['count']
    if any(type(value) is not int or value < 0 for value in (supported, projects)):
        raise ValueError('Public statistics counts must be nonnegative integers')
    public = {'schema_version': 1, 'updated_at': updated_at,
              'supported_boards': supported, 'public_dependent_projects': projects}
    readme = '\n'.join([
        '# ESP Board Manager ecosystem / 生态统计', '',
        'Updated at / 更新时间: **{}**'.format(updated_at), '',
        '| Metric / 指标 | Count / 数量 |',
        '| --- | ---: |',
        '| Supported boards / 适配的板卡 | {} |'.format(supported),
        '| Projects using Board Manager / 支持的项目 | {} |'.format(projects), '',
        'Board totals include the configured board components and manually approved outside models. '
        'Reviewed copies and configuration variants count once; pending candidates are excluded. '
        'The total does not imply hardware validation.', '',
        '板卡总数包含已列板级组件和人工确认纳入的范围外型号。'
        '已确认的副本和配置变体只计一次，待审候选不计入；该数量不代表已完成硬件验证。', '',
        'Projects using Board Manager are counted once per public, non-fork, non-template repository when its '
        'default-branch component manifest or dependency lock records an ESP Board Manager dependency, '
        'including resolved transitive dependencies. Configured cache and embedded upstream paths are excluded.', '',
        '支持的项目按公开、非 fork、非模板仓库去重，依据默认分支组件清单或依赖锁文件中的 '
        'ESP Board Manager 依赖记录统计，包含已解析的传递依赖，并排除配置指定的缓存和内嵌上游路径。', '',
    ])
    files = {'stats.json': json.dumps(public, ensure_ascii=False, indent=2) + '\n',
             'supported-boards.svg': badge('Supported boards', str(supported), '#007ec6'),
             'dependent-projects.svg': badge('Projects using Board Manager', str(projects), '#2e7d32'),
             'README.md': readme}
    for language in BANNER_LANGUAGES:
        for theme in ('light', 'dark'):
            files['ecosystem-banner-{}-{}.svg'.format(language, theme)] = _banner(
                supported, projects, language, theme)
    return files
