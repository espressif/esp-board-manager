#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$(cd "$(dirname "$0")" && pwd)"
STUBS="$HERE/stubs"
HDR="$ROOT/private_inc"
OUT="$HERE/host_runtime_override_test"
WEAK_OUT="$HERE/host_weak_pre_init_test"

COMMON_FLAGS=(-std=c17 -Wall -Wextra -Werror -D_DEFAULT_SOURCE -DCONFIG_ESP_BOARD_MANAGER_WRITABLE_TABLES=1 -DCONFIG_ESP_BOARD_MANAGER_DEVICE_TABLE_MAX_NUM=3 -DCONFIG_ESP_BOARD_MANAGER_PERIPH_TABLE_MAX_NUM=3 -I"$STUBS" -I"$HDR")

cc "${COMMON_FLAGS[@]}" \
  -o "$OUT" \
  "$HERE/host_main.c" \
  "$STUBS/freertos_task_stub.c" \
  "$ROOT/src/esp_board_runtime_state.c" \
  "$ROOT/src/esp_board_runtime_pre_init.c" \
  "$ROOT/src/esp_board_manager.c" \
  "$ROOT/src/esp_board_periph.c" \
  "$ROOT/src/esp_board_device.c"

"$OUT"
rm -f "$OUT"

cc "${COMMON_FLAGS[@]}" \
  -o "$WEAK_OUT" \
  "$HERE/weak_hook_main.c" \
  "$STUBS/freertos_task_stub.c" \
  "$ROOT/src/esp_board_runtime_state.c" \
  "$ROOT/src/esp_board_runtime_pre_init.c"

"$WEAK_OUT"
rm -f "$WEAK_OUT"
