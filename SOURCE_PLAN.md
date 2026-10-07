# Source and release publication plan

Status: the paired [Android P2P code 7](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v7) and [iOS P2P build 3](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/ios-v3-unsigned) test prereleases contain the signed Android APK, unsigned iOS IPA, and identical complete corresponding-source ZIP with published SHA-256 checksums. Full anonymous downloads and actual platform-specific update parsers were verified. Android v3 remains Latest for the old-v2 updater bridge; earlier releases are retained.

The iOS build compiled and packaged on Xcode 27. Its downloaded content, notices, audio, ICE/P2P, DTLS and OpenSSL linkage were independently checked. It still requires signing for installation. Physical-device gameplay, audio, menu layout, performance, crash behavior and cross-network pairing remain pending. Direct P2P is implemented with a fixed directory/signaling service; no relay is configured, so some networks may fail to connect. No Quest work is included.

The repository contains documentation, an earlier iOS baseline, and build/verification workflows. The current paired workflow fetches the hash-pinned full P2P archive rather than mixing it with older game content.

## Publication checks

1. Review the exact source/game inventory, retaining original notices, component license texts and attribution.
2. Include complete corresponding source: frontends, native/game changes, assets, build scripts, pinned inputs and build instructions.
3. Verify each exact binary against its source, version, package and signing status. Publish byte counts and SHA-256 checksums.
4. Exclude private signing material, credentials, local toolchains/caches and unrelated code.
5. State which compile, packaging and physical-device checks passed or remain unverified.
6. Attach only reviewed files, label test builds clearly, and verify public anonymous downloads.
7. Test actual old/current update parsers, cross-platform release isolation and the current-build play gate. Require a valid published same-platform artifact before treating an older build as outdated.

## Device acceptance

Test installation, first launch, names, menus, simultaneous movement/aim/fire, stance, multiple rounds, doors/purchases, sound, death/restart, pause/background/return and settings persistence on each platform. Validate the iPhone signing/install route separately.

Co-op requires matching Android/iOS builds on real devices, host reversal, discovery/permission tests, full player sign-on, several rounds, disconnect/reconnect and full-room behavior. Test different routers/carriers and direct-route failure. A build, online room entry, local encrypted challenge or solo session does not establish complete multiplayer readiness. No phone-FPS or broad device-compatibility guarantee follows from automated checks.
