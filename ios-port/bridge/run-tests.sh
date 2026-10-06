#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT=$(mktemp -d "${TMPDIR:-/tmp}/nzp-ios-bridge.XXXXXX")
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
CC=${CC:-cc}
FLAGS="-std=c11 -Wall -Wextra -Werror -pedantic -g"
if [ "${NZP_SANITIZERS:-0}" = 1 ]; then
  FLAGS="$FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer"
  # This executor runs under ptrace, where LeakSanitizer cannot inspect threads.
  export ASAN_OPTIONS="detect_leaks=0${ASAN_OPTIONS:+:$ASAN_OPTIONS}"
fi
# Intentional word splitting for compiler flags; paths remain quoted.
# shellcheck disable=SC2086
"$CC" $FLAGS -I"$ROOT/bridge/tests" -I"$ROOT/bridge" -I"$ROOT/core" \
  "$ROOT/bridge/tests/engine_bridge_test.c" "$ROOT/bridge/nzp_ios_engine.c" \
  "$ROOT/core/nzp_touch.c" "$ROOT/core/nzp_mobile_state.c" \
  -lm -lpthread -o "$OUT/engine_bridge_test"
# shellcheck disable=SC2086
"$CC" $FLAGS -I"$ROOT/bridge" "$ROOT/bridge/tests/qc_contract_test.c" \
  -lm -o "$OUT/qc_contract_test"
"$OUT/engine_bridge_test"
"$OUT/qc_contract_test"
