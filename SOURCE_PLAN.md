# Source and release publication plan

Status: the paired [Android MP1 code 6](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v6) and [iOS MP1 build 2](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/ios-v2-unsigned) test prereleases contain the signed Android APK, unsigned iOS IPA, and identical complete corresponding-source ZIP with published SHA-256 checksums. Full anonymous downloads and actual platform-specific update parsers were verified. Android v3 remains Latest for the old-v2 updater bridge; earlier releases are retained.

The iOS build compiled and packaged on Xcode 27, and its downloaded game resources/audio linkage were checked. It still requires signing for installation. Current physical-device gameplay, audio, performance, crash behavior and paired co-op acceptance remain pending. Multiplayer is limited to same-Wi-Fi LAN testing; internet services and Quest work are unfinished.

The repository contains documentation, an earlier iOS baseline, and build/verification workflows. The current paired workflow fetches the hash-pinned full MP1 archive rather than mixing the candidate with older game content.

## Publication checks

1. Review the exact source/game inventory, retaining original notices, component license texts and attribution.
2. Include complete corresponding source: frontends, native/game changes, assets, build scripts, pinned inputs and build instructions.
3. Verify each exact binary against its source, version, package and signing status. Publish byte counts and SHA-256 checksums.
4. Exclude private signing material, credentials, local toolchains/caches and unrelated code.
5. State which compile, packaging and physical-device checks passed or remain unverified.
6. Attach only the reviewed files, label test builds clearly, and verify public anonymous downloads.
7. Test actual old/current update parsers, cross-platform release isolation and the current-build play gate. Require a valid published same-platform artifact before treating an older build as outdated.

## Device acceptance

Test installation, first launch, names, menus, simultaneous movement/aim/fire, stance, multiple rounds, doors/purchases, sound, death/restart, pause/background/return and settings persistence on each platform. Validate the iPhone signing/install route separately.

Co-op requires matching Android/iOS builds on real devices, host reversal, discovery/permission tests, actual player sign-on, several rounds, disconnect/reconnect and full-room behavior. A build, host list entry or solo session does not establish multiplayer readiness. No phone-FPS or broad device-compatibility guarantee follows from the automated checks.
