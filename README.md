# NZP Mobilized

Unofficial, work-in-progress Android and iOS ports of Nazi Zombies: Portable (NZ:P).

## Paired direct-P2P test downloads

**Online room browsing and direct P2P connection attempts are included. No relay is configured, so some carrier, symmetric-NAT or UDP-blocking networks may fail to connect. Cross-network phone gameplay remains unverified.**

- **Android 0.7.0-p2p-test, code 7:** [signed ARM64 APK](https://github.com/itssnotTristan/NZP-Mobile/releases/download/android-v7/nzp-mobile-arm64.apk) · [release notes](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v7)
- **iOS 0.3.0, build 3:** [unsigned IPA](https://github.com/itssnotTristan/NZP-Mobile/releases/download/ios-v3-unsigned/NZP-Mobile-iOS-unsigned.ipa) · [release notes](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/ios-v3-unsigned)
- **Both platforms:** [complete paired corresponding source](https://github.com/itssnotTristan/NZP-Mobile/releases/download/android-v7/NZP-Mobilized-P2P-paired-source-final.zip), also attached to the iOS release.

Public downloads do not require a GitHub account. The iOS IPA needs your own signing/install process; it cannot install as delivered. No Apple credentials or signing keys are included.

The paired update adds direct-P2P browsing/transport, puts iOS menu controls in a bottom dock with safe-area menu fitting, and repairs zombie/dog movement against foreign limb/corpse hitboxes. Stance below rounds, names, aim slowdown, prior gameplay fixes and the real iOS audio driver carry forward. Phone gameplay, audible playback, menu layout, frame pacing and multiplayer acceptance still need testing.

## Host and join

1. Install the matching Android 7/iOS 3 P2P builds on both phones.
2. Choose Multiplayer → Host Game to start a four-player Nacht host and list it online. Keep the host app open and foregrounded.
3. On the other phone, choose Join Game and select the room. Nearby LAN remains available separately.
4. Test both hosting directions, player names, full sign-on, several rounds, death/restart and reconnects.

The [public directory/signaling service](https://nzp-mobilized-connect.nogofortniteclan.chatgpt.site) is already configured. No service URL or PC setup is required in the game. It lists room names/player counts and exchanges private, short-lived ICE descriptions over HTTPS. Gameplay uses direct FTE ICE/UDP with mutually pinned DTLS; the service does not relay it.

If a network cannot establish a direct route, there is no relay fallback. Guest controls retain manual tap-fire fallback. Host migration and background hosting are not provided.

## Updating

Preview-aware Android builds can find code 7 through the in-game Update button. Install over the existing app to retain its data; package `org.nzp.mobile.preview` and the signing identity are unchanged. GitHub Latest deliberately remains [android-v3](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v3), so older v2 can reach the preview-aware updater and then check again.

iOS build 2 can identify build 3 and open its release page; signing/install remain manual. The original iOS build 1 has no updater. Platform-specific update checks recognize the newly published versions and allow current Android 7/iOS 3 to play. A failed/offline check does not establish an outdated app.

Android requires ARM64, API 23+ and OpenGL ES 3.0; 16 KB-page compatibility is unverified. iOS is ARM64 with deployment target iOS 15. No Quest work is included.

## Source and verification

Use the explicitly named complete paired ZIP, not GitHub's automatic source archives. It contains `android-port-p2p` and `ios-port`, matching game content, pinned OpenSSL and other dependencies, build instructions, tests and component license/credit files. The frozen source documents record pre-Apple-build status; the evidence below is newer.

[The Xcode 27 build](https://github.com/itssnotTristan/NZP-Mobile/actions/runs/37574457786) passed compilation, linking and unsigned IPA packaging. Independent downloaded-IPA checks matched all 1,175 game files, both HUD images and all 12 notices to the frozen source, and verified strong real audio, FTE ICE/P2P, DTLS and OpenSSL linkage. The local transport proof checks real encrypted FTE challenge packets between two processes; it does not establish full sign-on or physical cross-network play.

[Actual iOS parser and update-state verification](https://github.com/itssnotTristan/NZP-Mobile/actions/runs/37576531442) passed by replaying the exact verified anonymous public-feed response on the Apple runner. The runner's first direct API request was rate-limited before parsing; the recorded-response check does not claim that request succeeded. Actual Android parser/state checks and the old-v2 Latest bridge also passed. Both APK/IPA and each attached source ZIP passed full anonymous download size/SHA-256 verification.

- Android APK: 147,458,739 bytes; SHA-256 `8f7b78ba154cdc93ee15e412fe86aab421cda880043d984d4baa3082499bbc3d`
- Unsigned iOS IPA: 113,893,871 bytes; SHA-256 `4dcfbf9815216e4a1bed9e2a1fd2f98e70ada9a829c3ad3d06b0353551b41ac6`
- Paired source ZIP: 198,422,204 bytes; SHA-256 `e53a12207f9254cd15110ead50efd6d33acee1e6afcb71eec5fe78b8744f953f`

The repository's `ios-port/` directory preserves the earlier baseline. The current P2P workflow fetches and checksum-verifies the complete paired source archive, without older-content fallback. See [SOURCE_PLAN.md](SOURCE_PLAN.md) for release checks.

Earlier releases remain available: [Android 6 / MP1](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v6), [v4](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v4), [v3](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v3), [v2](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v2), [v1](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v1), [iOS 2 / MP1](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/ios-v2-unsigned), and [iOS 1](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/ios-v1-unsigned).

## Credits and licensing

Credit belongs to the original NZ:P Team, id Software's Quake/id Tech foundations, FTE QuakeWorld contributors, the Q3E/Android port contributors and the original asset creators. This unofficial project is not endorsed by those teams or authors.

Components and assets have different licenses. Original copyright notices and component-specific terms are retained; no single license is asserted for every file or asset. See [LICENSES.md](LICENSES.md) and the complete source archive.
