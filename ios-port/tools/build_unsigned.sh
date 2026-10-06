#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SOURCE="${NZP_ANDROID_SOURCE:-$(dirname "$ROOT")}"
OUT="${NZP_IOS_BUILD_DIR:-$ROOT/build/apple-device}"
if [[ "$(uname -s)" != Darwin ]]; then
  echo 'Apple compilation requires macOS with Xcode, including a GitHub macOS runner.' >&2
  exit 2
fi
python3 "$ROOT/tools/prepare_engine.py" --android-root "$SOURCE"
xcodebuild -version
xcrun --sdk iphoneos --show-sdk-version
cmake -S "$ROOT" -B "$OUT" -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DNZP_ANDROID_SOURCE="$SOURCE" \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED=NO
cmake --build "$OUT" --config Release --target NZPMobile --parallel 3 -- \
  CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO
APP="$OUT/Release-iphoneos/NZPMobile.app"
test -f "$APP/NZPMobile"
xcrun lipo -info "$APP/NZPMobile"
python3 "$ROOT/tools/package_unsigned.py" "$APP" "$OUT/NZP-Mobile-iOS-unsigned.ipa"
shasum -a 256 "$OUT/NZP-Mobile-iOS-unsigned.ipa"
echo 'Unsigned arm64 IPA packaged. Installation and real-device gameplay remain unverified.'
