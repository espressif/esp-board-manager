# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
#
# SPDX-License-Identifier: Apache-2.0

import re

import pytest
from pytest_embedded import Dut


@pytest.mark.esp32s3
@pytest.mark.timeout(60000)
@pytest.mark.parametrize(
    'config',
    [
        'default',
    ],
    indirect=True,
)
def test_esp_board_manager(dut: Dut) -> None:
    # The interactive test app boots into a console REPL instead of running
    # every test automatically. Drive the console the same way an operator or
    # CI would: wait for the prompt, initialize the board, then run all the
    # automatic cases and check the summary.
    #
    # The board config in the build matrix/CI is generated without --debug, so
    # the generated debug init code is not available. Use the normal init path,
    # which is always available; `bmgr init --debug` stays for interactive use
    # when the board is generated with `idf.py bmgr -b <board> --debug`.
    dut.expect('Starting ESP Board Manager interactive test application', timeout=30)
    dut.expect('bmgr-test>', timeout=30)

    dut.write('bmgr init')
    dut.expect(re.compile(rb'Board manager initialized via normal path'), timeout=60)

    dut.write('case run-all')
    summary = dut.expect(
        re.compile(rb'\[CASE\] SUMMARY total=(\d+) pass=(\d+) fail=(\d+) skip=(\d+)'),
        timeout=3000,
    )
    total = int(summary.group(1))
    passed = int(summary.group(2))
    failed = int(summary.group(3))
    skipped = int(summary.group(4))

    assert failed == 0, (
        'case run-all reported failures: '
        f'total={total} pass={passed} fail={failed} skip={skipped}'
    )
    assert passed > 0, (
        'case run-all did not execute any successful automatic case: '
        f'total={total} pass={passed} fail={failed} skip={skipped}'
    )
