#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
HERE="$(cd "$(dirname "$0")" && pwd)"
STUBS="$HERE/stubs"
HDR="$ROOT/private_inc"
OUT="$HERE/host_runtime_override_test"
WEAK_OUT="$HERE/host_weak_pre_init_test"

# Production registration sections require an ELF host compiler/linker.
HOST_DEFINES="$(cc -dM -E - </dev/null)"
if [[ "$HOST_DEFINES" != *"#define __ELF__ "* ]]; then
  printf '%s\n' 'SKIP: BMGR host registration tests require a GNU-compatible ELF linker'
  exit 77
fi

COMMON_FLAGS=(-Wl,-T,"$ROOT/test_apps/host_tests/support/esp_board_sections.ld" -std=c17 -Wall -Wextra -Werror -D_DEFAULT_SOURCE -DCONFIG_ESP_BOARD_MANAGER_WRITABLE_TABLES=1 -DCONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM=3 -DCONFIG_ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM=3 -I"$STUBS" -I"$ROOT/include" -I"$HDR" -I"$ROOT/devices/dev_power_ctrl")

# The generator regression suite supplies real revision apply sources here.
REVISION_SOURCES=()
if [[ -n "${BMGR_HOST_REVISION_DIR:-}" ]]; then
  COMMON_FLAGS+=(-DHOST_TEST_BOARD_REVISION -I"$BMGR_HOST_REVISION_DIR")
  REVISION_SOURCES+=("$BMGR_HOST_REVISION_DIR/gen_board_revision_cleanup.c" "$BMGR_HOST_REVISION_DIR/revision_fixture.c")
fi

cc "${COMMON_FLAGS[@]}" \
  -o "$OUT" \
  "$HERE/host_main.c" \
  "$STUBS/freertos_task_stub.c" \
  "$ROOT/src/esp_board_runtime_state.c" \
  "$ROOT/src/esp_board_runtime_pre_init.c" \
  "$ROOT/src/esp_board_manager.c" \
  "$ROOT/src/esp_board_periph.c" \
  "$ROOT/src/esp_board_device.c" \
  "$ROOT/devices/dev_power_ctrl/dev_power_ctrl_sub_custom.c" \
  "${REVISION_SOURCES[@]}"

"$OUT"
# Unrecoverable cases run in fresh processes; production has no reset/recovery API.
for failure_case in failed-bulk-device-init failed-bulk-periph-init failed-device-init failed-periph-init failed-legacy-periph-init failed-hook-periph failed-hook-lookups failed-hook-device failed-hook-base failed-hook-replace failed-hook-add failed-hook-preflight failed-hook-handle; do
  "$OUT" "$failure_case"
done
if [[ -n "${BMGR_HOST_REVISION_DIR:-}" ]]; then
  "$OUT" failed-hook-revision
fi
rm -f "$OUT"

cc "${COMMON_FLAGS[@]}" \
  -o "$WEAK_OUT" \
  "$HERE/weak_hook_main.c" \
  "$STUBS/freertos_task_stub.c" \
  "$ROOT/src/esp_board_runtime_state.c" \
  "$ROOT/src/esp_board_runtime_pre_init.c"

"$WEAK_OUT"
rm -f "$WEAK_OUT"
