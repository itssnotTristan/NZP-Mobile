# NZP Mobile iOS port

Native iOS development port of the accepted Android v4 preview, targeting an
arm64 iPhone with iOS 27. The Xcode 27 device build has compiled, linked and
packaged successfully on GitHub Actions. Signing, simulator startup and
real-device gameplay remain unverified, so this is not yet a device-validated
iOS release. The unsigned IPA must be signed for the device before installation.

Verified Apple build: [run 37549863968](https://github.com/itssnotTristan/NZP-Mobile/actions/runs/37549863968),
source commit `ed2dcae3c279e3734486e9e774892dd3d664ba9e`. Build stages and
remaining checks are recorded in `tests/APPLE_BUILD_RESULTS.json`.
The downloaded IPA was independently checked: a real arm64 executable, no code
signature or provisioning profile, and all 1,174 packaged game files matching
the reviewed content manifest exactly.

## Implementation

- The same pinned FTE engine and revised game bytecode/content, with isolated
  SDL2/UIKit/GLES and CoreAudio-backed SDL output. No emulator or browser shell.
- UIKit touch controls, toggle ADS, guarded automatic hold, world tap/drag,
  reload/switch gesture, movement and forward-ring/double-tap sprint.
- Points and rounds below Pause; stance immediately below rounds. Safe-area
  coordinates are UIKit points, independent of framebuffer pixels.
- A typed native bridge preserves local-server weapon/entity guards and reads
  bounded QC presentation snapshots. Unknown remote capabilities remain closed.
- Read-only bundled content and separate sandbox preference storage. No
  unsolicited content downloads, external plugins, microphone or JIT.

## Source layout and reproduction

The Android source root must contain `app/src/main/assets` and `vendor/idtech4a`
from the corresponding v4 source release. The iOS preparation script only writes
within this directory's `build/` area; it does not modify the Android sources.

`python3 tools/prepare_engine.py --android-root /path/to/Android-source`

Preparation verifies pinned patch anchors and their resulting hashes, extracts
the included SDL2 source archive against its manifest, stages four source codecs,
and writes the explicit FTE source list. It does not run downloaded binaries.

On a Mac or GitHub macOS runner with Xcode and CMake:

`NZP_ANDROID_SOURCE=/path/to/Android-source bash tools/build_unsigned.sh`

The build disables signing and packages only a real Mach-O app. Output is
`build/apple-device/NZP-Mobile-iOS-unsigned.ipa`. No Apple account, private key,
provisioning profile or signing secret belongs in this repository.

## GitHub build

Copy `workflows/ios-unsigned.yml` to the repository's `.github/workflows/` and
invoke it manually. It selects the standard `xcode-27` arm64 runner, checks
Xcode/SDK availability, runs portable tests, downloads the existing public v4
corresponding-source ZIP, verifies its exact SHA-256 and safely extracts it to
runner temporary storage. It then builds and stores a seven-day unsigned artifact.
Android source does not need to be checked into the git tree. The workflow refuses private repositories to avoid assuming
permission for billable private-repository minutes. It does not publish a release
or install anything on a phone.

GitHub currently documents standard runners as free for public repositories and
lists `xcode-27` as public preview. Runner image contents can change; each build
logs the actual Xcode/SDK and available simulator list.

## Tests

- `bash tests/run_tests.sh`: C state-machine sanitizer tests.
- `NZP_SANITIZERS=1 bash bridge/run-tests.sh`: typed native-input and extracted
  QC contract checks under AddressSanitizer and UndefinedBehaviorSanitizer.
- `bash tests/run_java_parity.sh`: fresh Android Java and C deterministic trace
  parity, with Android source/JDK paths accepted by the script.
- `cc -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined layout/nzp_hud_layout.c tests/hud_layout_test.c -lm -o build/hud-layout-test`
- `ASAN_OPTIONS=detect_leaks=0 build/hud-layout-test`: safe-area layout checks.
- `python3 tools/verify_content.py /path/to/Android-source`: exact asset manifest.

Portable tests do not validate UIKit compilation, GPU rendering, audio,
installation, frame pacing, heat or actual iPhone gameplay.

## Device acceptance gates

1. Xcode arm64 device build links successfully and contains the complete content.
2. Simulator starts, shows the original menus, enters `ndu`, and renders/audio runs.
3. A user-signed installation launches on the target iPhone Air/iOS 27.
4. Move/look/fire simultaneously; verify every tap/hold/button and multi-touch
   ownership. ADS must survive ordinary lifts and release on menu/weapon change.
5. Background/foreground, interruptions, resize, low-memory behavior and relaunch
   release all held input. Saved settings persist in writable sandbox storage.
6. Multiple complete rounds, purchases, doors, downed/death/restart and weapon
   changes show no missing HUD, runaway input or progressively growing resources.
7. Co-op requires its own multi-device test; it is not established by a solo run.

## Installation options

An own Mac is not required when GitHub builds the app. A separate signing/install
step remains. A free Apple Account can provision personal test apps with a
seven-day expiry. Windows tools such as Sideloadly or AltServer are third-party
routes; the user enters their Apple credentials locally. Never send credentials
to this repository or its Actions secrets. Exact iOS 27 installation compatibility
must be confirmed by a real test, not inferred from vendor statements for iOS26+.

## Upstream and licensing

Engine base: idTech4A `ce49510f3b0c04969eab8ebe8238416e84e4f0bb` with the
corresponding Android v4 adaptation. Assets: NZP legacy source commit
`3785457eed74cc481838ca8005f5956f925aaef3`. The exact exported v4 source archive
was SHA-256 `9b26376dc82d7ff796826b288d13902e9b1b67738ec15ba8274129b7d7992a4f`.
SDL2 2.32.5 source is preserved with per-file provenance; Android-only build
alterations are removed only in the staged iOS copy.

Preserve the parent source LICENSE/NOTICE and individual dependency notices.
New adaptation code is GPL-3.0-or-later. Game assets keep their separate licenses
and exceptions. Full corresponding source must accompany a distributed binary.
An App Store release requires its own license/distribution review; no App Store
or TestFlight availability is promised by this development build.

## Primary references (checked 2026-10-06)

- Apple account and personal testing: https://developer.apple.com/help/account/basics/about-your-developer-account
- Apple Xcode/SDK table: https://developer.apple.com/xcode/system-requirements
- GitHub standard/public runners: https://docs.github.com/en/actions/reference/runners/github-hosted-runners
- Xcode 27 image: https://github.com/actions/runner-images/blob/main/images/macos/xcode-27-arm64-Readme.md
- SDL2 native iOS backend: https://wiki.libsdl.org/SDL2/README-ios
- Sideloadly vendor FAQ: https://sideloadly.io/faq
- AltServer Windows guide: https://faq.altstore.io/altstore-classic/how-to-install-altstore-windows
