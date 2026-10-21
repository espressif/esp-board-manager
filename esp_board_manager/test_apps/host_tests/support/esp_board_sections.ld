/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

/* Host equivalent of the IDF linker sections, for GNU-compatible ELF linkers.
 * SUBALIGN keeps descriptors contiguous despite host compiler over-alignment.
 * Production registration macros and lookup functions are used unchanged.
 */
SECTIONS
{
    .esp_board_entries_desc : ALIGN(8) SUBALIGN(8)
    {
        _esp_board_entries_array_start = .;
        KEEP(*(.esp_board_entries_desc))
        _esp_board_entries_array_end = .;
    }
    .esp_board_extra_func : ALIGN(8) SUBALIGN(8)
    {
        _esp_board_extra_func_array_start = .;
        KEEP(*(.esp_board_extra_func))
        _esp_board_extra_func_array_end = .;
    }
}
INSERT AFTER .data;
