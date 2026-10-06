#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
base=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cc=${CC:-cc}
build=$(mktemp -d "$base/tests/.core-build.XXXXXX")
trap 'rm -rf "$build"' EXIT HUP INT TERM
flags='-std=c99 -Wall -Wextra -Werror -Wpedantic -g -O1 -fno-omit-frame-pointer'
# Sanitizers apply to production sources and tests, not prebuilt objects.
# LeakSanitizer is disabled because the sandbox runs under ptrace. The core
# does not allocate memory; AddressSanitizer and UBSan remain enabled.
"$cc" $flags -fsanitize=address,undefined -I"$base/core" \
  "$base/core/nzp_mobile_state.c" "$base/core/nzp_touch.c" \
  "$base/tests/test_touch.c" -lm -o "$build/test_touch"
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$build/test_touch"
printf '%s\n' 'PASS: AddressSanitizer + UndefinedBehaviorSanitizer host build'
