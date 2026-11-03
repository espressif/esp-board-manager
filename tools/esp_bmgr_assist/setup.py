from pathlib import Path
import re

from setuptools import setup
from setuptools.command.build_py import build_py


ROOT = Path(__file__).parent
PTH_FILE = ROOT / 'esp_bmgr_py.pth'
README = (ROOT / 'README.md').read_text(encoding='utf-8')
INIT_PY = (ROOT / 'esp_bmgr_py' / '__init__.py').read_text(encoding='utf-8')
VERSION = re.search(r'''__version__\s*=\s*['"]([^'"]+)['"]''', INIT_PY).group(1)


class BuildPyWithPth(build_py):
    """Place the bootstrap .pth file at the wheel root (purelib/site-packages)."""

    def run(self):
        super().run()
        self.copy_file(str(PTH_FILE), str(Path(self.build_lib) / PTH_FILE.name), preserve_mode=False)

setup(
    name='esp-bmgr-assist',
    version=VERSION,
    description='ESP Board Manager Python package for idf.py extensions',
    long_description=README,
    long_description_content_type='text/markdown',
    url='https://github.com/espressif/esp-board-manager/tree/main/tools/esp_bmgr_assist',
    project_urls={'Homepage': 'https://github.com/espressif/esp-board-manager/tree/main/tools/esp_bmgr_assist'},
    license='Apache-2.0',
    python_requires='>=3.8',
    packages=['esp_bmgr_py'],
    include_package_data=True,
    package_data={'esp_bmgr_py': ['*.py']},
    install_requires=[
        'filelock>=3.12',
        'pyyaml>=5.1',
    ],
    keywords=['esp-idf', 'board-manager', 'idf.py', 'extensions'],
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
    ],
    cmdclass={'build_py': BuildPyWithPth},
)
