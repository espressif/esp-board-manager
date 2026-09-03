"""Host regression tests for the examples/common WAV and SD file helpers."""

from __future__ import annotations

import subprocess
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
COMMON_DIR = REPO_ROOT / 'esp_board_manager' / 'examples' / 'common'


def _write_esp_stubs(stub_dir: Path) -> None:
    (stub_dir / 'esp_err.h').write_text(
        '#pragma once\n'
        'typedef int esp_err_t;\n'
        '#define ESP_OK 0\n'
        '#define ESP_FAIL -1\n'
        '#define ESP_ERR_INVALID_ARG 0x102\n'
        '#define ESP_ERR_INVALID_SIZE 0x103\n'
        '#define ESP_ERR_NO_MEM 0x105\n',
        encoding='utf-8',
    )
    (stub_dir / 'esp_log.h').write_text(
        '#pragma once\n'
        '#define ESP_LOGE(tag, ...) do { (void)(tag); } while (0)\n'
        '#define ESP_LOGI(tag, ...) do { (void)(tag); } while (0)\n'
        '#define ESP_LOGW(tag, ...) do { (void)(tag); } while (0)\n',
        encoding='utf-8',
    )
    (stub_dir / 'esp_heap_caps.h').write_text(
        '#pragma once\n'
        '#include <stddef.h>\n'
        '#include <stdlib.h>\n'
        '#define MALLOC_CAP_SPIRAM 0\n'
        '#define MALLOC_CAP_8BIT 0\n'
        '#define MALLOC_CAP_INTERNAL 0\n'
        '#define MALLOC_CAP_DMA 0\n'
        '#define MALLOC_CAP_CACHE_ALIGNED 0\n'
        'static inline void *heap_caps_aligned_alloc(size_t alignment, size_t size, unsigned caps)\n'
        '{\n'
        '    void *ptr = NULL;\n'
        '    (void)caps;\n'
        '    return posix_memalign(&ptr, alignment, size) == 0 ? ptr : NULL;\n'
        '}\n'
        'static inline void heap_caps_free(void *ptr) { free(ptr); }\n',
        encoding='utf-8',
    )
    (stub_dir / 'soc').mkdir()
    (stub_dir / 'soc' / 'soc_caps.h').write_text(
        '#pragma once\n#define SOC_SDMMC_PSRAM_DMA_CAPABLE 0\n',
        encoding='utf-8',
    )


def _compile_and_run(tmp_path: Path, name: str, source: str, sources: list[Path]) -> None:
    stub_dir = tmp_path / 'stubs'
    stub_dir.mkdir()
    _write_esp_stubs(stub_dir)

    runner = tmp_path / f'{name}.c'
    executable = tmp_path / name
    runner.write_text(source, encoding='utf-8')

    subprocess.run(
        [
            'cc', '-std=c11', '-D_GNU_SOURCE', '-Wall', '-Werror',
            '-I', str(stub_dir), '-I', str(COMMON_DIR / 'include'),
            *map(str, sources), str(runner), '-o', str(executable),
        ],
        check=True,
        cwd=REPO_ROOT,
    )
    subprocess.run([str(executable)], check=True)


def test_wav_info_scans_metadata_chunks_and_reports_data_bounds(tmp_path: Path) -> None:
    _compile_and_run(
        tmp_path,
        'wav_info',
        r'''
        #include <assert.h>
        #include <stdint.h>
        #include <stdio.h>
        #include "wav_header.h"

        int main(void)
        {
            static const uint8_t wav[] = {
                'R', 'I', 'F', 'F', 0x34, 0x00, 0x00, 0x00, 'W', 'A', 'V', 'E',
                'J', 'U', 'N', 'K', 0x03, 0x00, 0x00, 0x00, 0xaa, 0xbb, 0xcc, 0x00,
                'f', 'm', 't', ' ', 0x10, 0x00, 0x00, 0x00,
                0x01, 0x00, 0x02, 0x00, 0x80, 0xbb, 0x00, 0x00,
                0x00, 0xee, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00,
                'd', 'a', 't', 'a', 0x04, 0x00, 0x00, 0x00, 1, 2, 3, 4,
            };
            wav_info_t info = {0};
            FILE *stream = fmemopen((void *)wav, sizeof(wav), "rb");

            assert(stream != NULL);
            assert(read_wav_info(stream, &info) == ESP_OK);
            assert(info.sample_rate == 48000);
            assert(info.channels == 2);
            assert(info.bits_per_sample == 16);
            assert(info.block_align == 4);
            assert(info.data_size == 4);
            assert(fgetc(stream) == 1);
            assert(fclose(stream) == 0);
            return 0;
        }
        ''',
        [COMMON_DIR / 'wav_header.c'],
    )


def test_sd_file_cache_releases_the_buffer_only_after_closing_the_stream(tmp_path: Path) -> None:
    _compile_and_run(
        tmp_path,
        'sd_file_cache',
        r'''
        #include <assert.h>
        #include <stdio.h>
        #include "sd_file_cache.h"

        int main(void)
        {
            sd_file_cache_t cache = {0};
            FILE *stream = tmpfile();

            assert(stream != NULL);
            assert(sd_file_cache_attach(stream, &cache, 1024, 256) == ESP_OK);
            assert(cache.buffer != NULL);
            assert(fputs("pcm", stream) >= 0);
            assert(sd_file_cache_close(&stream, &cache) == ESP_OK);
            assert(stream == NULL);
            assert(cache.buffer == NULL);
            assert(cache.size == 0);
            return 0;
        }
        ''',
        [COMMON_DIR / 'sd_file_cache.c'],
    )
