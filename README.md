# NZP Mobilized

Unofficial, work-in-progress Android and iOS ports of Nazi Zombies: Portable (NZ:P).

## Paired MP1 test downloads

**Multiplayer is a same-Wi-Fi LAN test. Internet matchmaking, NAT traversal and relay are not ready.** Use the matching MP1 builds on both phones.

- **Android 0.6.0-mp1-test, code 6:** [signed ARM64 APK](https://github.com/itssnotTristan/NZP-Mobile/releases/download/android-v6/nzp-mobile-arm64.apk) · [release notes](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v6)
- **iOS 0.2.0, build 2:** [unsigned IPA](https://github.com/itssnotTristan/NZP-Mobile/releases/download/ios-v2-unsigned/NZP-Mobile-iOS-unsigned.ipa) · [release notes](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/ios-v2-unsigned)
- **Both platforms:** [complete paired corresponding source](https://github.com/itssnotTristan/NZP-Mobile/releases/download/android-v6/NZP-Mobilized-MP1-paired-source-final.zip), also attached to the iOS release.

Public downloads do not require a GitHub account. The iOS IPA needs your own signing/install process; it cannot install as delivered. No Apple credentials or signing keys are included.

MP1 puts Stance below rounds on both platforms and includes saved names, nearby Host/Join browsing, optional touch aim slowdown, and fixes targeting death/game-over, pause, collision and weapon swaps. The iOS binary includes the real SDL audio driver. These builds still need physical-device gameplay, audio, performance and paired co-op testing.

## First LAN test

1. Install matching MP1 builds and connect both phones to the same ordinary Wi-Fi. Allow Local Network access on iOS.
2. Choose Multiplayer Test → Host Game on one phone and keep it foregrounded.
3. Choose Multiplayer Test → Join Game on the other, then select the discovered host.
4. Test both hosting directions, names, shared rounds, death/restart and reconnects.

Host Game provides four slots. Guest input uses manual tap fire; automatic hold-fire, toggle ADS and held sprint are host-only in this first protocol. Guest/client Wi-Fi isolation can prevent discovery. Host migration and background hosting are not supported guarantees.

## Updating

Android v3/v4 can find code 6 through the in-game Update button. Install over the existing app to retain its data; package `org.nzp.mobile.preview` and the signing identity are unchanged. GitHub Latest deliberately remains [android-v3](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v3), so older v2 can reach the preview-aware updater and then check again.

The original iOS v1 has no updater, so download build 2 directly. Build2 checks only iOS releases and opens the relevant release page; subsequent installation still requires signing. The current MP1 builds pass their update checks without an outdated-version lock. A failed/offline check does not establish an outdated app.

Android requires ARM64, Android API 23+ and OpenGL ES 3.0; its native binaries retain the 4 KB-page baseline, with 16 KB-page support unverified. iOS is ARM64 with deployment target iOS 15. No Quest work is included.

## Source and verification

Use the explicitly named complete paired ZIP, not GitHub's automatic source archives. It contains `android-port-mp1` and `ios-port`, matching game content, build instructions, pinned inputs, tests and component license/credit files. The frozen source documentation records the pre-build state; the evidence below is newer.

[MP1's Xcode 27 build](https://github.com/itssnotTristan/NZP-Mobile/actions/runs/37559756396) passed compilation, linking and unsigned IPA packaging. Downloaded artifact structure, all 1,175 game files and both HUD images matched the frozen source, and the linked audio-driver check passed. [The actual iOS parser and update-state check](https://github.com/itssnotTristan/NZP-Mobile/actions/runs/37562478508) passed against the public feed. Actual archived-v4/current-v6 Android parsers and the old-v2 Latest bridge also passed. All public APK/IPA/source downloads were verified anonymously by full byte count and SHA-256.

- Android APK: 144,845,408 bytes; SHA-256 `c94c64c43579f46bcd973402073cc540c37294a67803497f349f1ac54b0c2d50`
- Unsigned iOS IPA: 111,678,472 bytes; SHA-256 `dac2b175ee88405ab4eb7a174247eb15d7594974681494f3741773b892ab0e1b`
- Paired source ZIP: 145,032,955 bytes; SHA-256 `8acfd7761ed5445da198e753cc2d2ffc70b864c5ad81c6fba6e8d68a7685d02e`

The repository's `ios-port/` directory preserves the earlier baseline. The current MP1 workflow fetches and checksum-verifies the complete paired source archive; it never substitutes old v4 inputs. See [SOURCE_PLAN.md](SOURCE_PLAN.md) for release checks.

Earlier releases remain available: [Android v4](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v4), [v3](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v3), [v2](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v2), [v1](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v1), and [iOS v1](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/ios-v1-unsigned).

## Credits and licensing

Credit belongs to the original NZ:P Team, id Software's Quake/id Tech foundations, FTE QuakeWorld contributors, the Q3E/Android port contributors and the original asset creators. This unofficial project is not endorsed by those teams or authors.

Components and assets have different licenses. Original copyright notices and component-specific terms are retained; no single license is asserted for every file or asset. See [LICENSES.md](LICENSES.md) and the complete source archive.
