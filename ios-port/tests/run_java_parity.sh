#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
base=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
canonical=${1:-"$base/../android-port-v4-revised"}
java_home=${JAVA_HOME:-"$base/../toolchains/jdk-11.0.32.1+1"}
cc=${CC:-cc}
if [ ! -x "$java_home/bin/javac" ]; then
  printf '%s\n' 'Set JAVA_HOME to an existing JDK to run optional canonical parity tests.' >&2
  exit 2
fi
build=$(mktemp -d "$base/tests/.parity-build.XXXXXX")
trap 'rm -rf "$build"' EXIT HUP INT TERM
src="$canonical/app/src/main/java/org/nzp/mobile/preview"
"$java_home/bin/javac" -Xlint:all -Werror -d "$build/java" \
  "$src/MobileGameState.java" "$src/GameplayInput.java" "$src/WeaponGesture.java" \
  "$src/MovementGesture.java" "$canonical/frontend-tests/GameplayInputTest.java" \
  "$base/tests/CanonicalParity.java"
"$java_home/bin/java" -cp "$build/java" org.nzp.mobile.preview.GameplayInputTest
"$cc" -std=c99 -Wall -Wextra -Werror -Wpedantic -O1 -g -fno-omit-frame-pointer \
  -fsanitize=address,undefined -I"$base/core" \
  "$base/core/nzp_mobile_state.c" "$base/core/nzp_touch.c" "$base/tests/parity_trace.c" -lm -o "$build/parity"
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$build/parity" > "$build/c.trace"
"$java_home/bin/java" -cp "$build/java" org.nzp.mobile.preview.CanonicalParity > "$build/java.trace"
if ! cmp "$build/c.trace" "$build/java.trace"; then
  printf '%s\n' 'FAIL: C and canonical Java traces differ' >&2
  cat "$build/c.trace" "$build/java.trace" >&2
  exit 1
fi
printf 'PASS: 50,000 deterministic C/Java world, weapon and sprint events; trace '
cat "$build/c.trace"
