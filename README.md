# NZP-Mobile

Unofficial, work-in-progress Android mobile port of Nazi Zombies: Portable (NZ:P).

## iOS development build

The [iOS source and build instructions](ios-port/README.md) are included in this repository. The [Xcode 27 arm64 build](https://github.com/itssnotTristan/NZP-Mobile/actions/runs/37549863968) compiled, linked and packaged an unsigned IPA; downloaded bytes and all bundled game assets were independently verified.

The IPA still requires signing for installation. Simulator startup, physical iPhone installation, gameplay, audio, touch behavior and performance have not been validated. This is a development build, not a confirmed playable iOS release.

## Current mobile test preview: v4

[NZP Mobile 0.4.0 Preview - HUD and performance test](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v4) is available as a prerelease.

- [Download the v4 signed ARM64 APK](https://github.com/itssnotTristan/NZP-Mobile/releases/download/android-v4/nzp-mobile-arm64.apk)
- [Download the v4 complete corresponding source](https://github.com/itssnotTristan/NZP-Mobile/releases/download/android-v4/nzp-mobile-0.4.0-preview-source.zip)
- Public downloads do not require a GitHub account.
- Earlier releases remain available: [v3](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v3), [v2](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v2), [v1](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v1).

## HUD and performance testing

v4 removes the original use prompt and duplicate ammo/tactical/grenade counters, places points and rounds at the top-left, enlarges power-up indicators at the top-center, and adds forward-ring sprint input.

The revised build corrects a finite initial limb-pool reference defect affecting 72 reserved entities in the audited setup. The 13-zombie test fixture also shows redundant limb-model setter calls reduced from 6,240 to 1,560. These are code-level results. No measured phone FPS improvement or resolution of the progressively worsening Nacht lag is claimed.

Optional local diagnostics are off by default and can record timing, memory and thermal information in a local 30-second report. v4 phone acceptance and the affected gameplay rounds still need testing. Internet multiplayer remains unfinished. No Quest work is included.

## Updating and compatibility

On v3, use Update to check the preview feed for v4. v3 intentionally remains GitHub's Latest/non-prerelease release so older v2 installations can update to v3 first, then check again for v4. The GitHub Latest badge is an older-updater compatibility setting, not a statement that mobile testing is complete.

A direct APK download is also available above. Install over the existing app rather than uninstalling it to retain app data. v4 keeps package `org.nzp.mobile.preview` and the original signing identity, with versionCode 4. The on-phone Android installation prompt and upgrade behavior still need validation.

Requirements: ARM64, Android API 23 or newer, OpenGL ES 3.0. The preview targets API 30 and has a 4 KB memory-page baseline; support for 16 KB-page devices is not established.

## Source and checksums

For Android releases, use the specifically named complete-source ZIP assets for corresponding source, build instructions, changes, component license texts and attribution, rather than GitHub's automatically generated archives. The repository also contains experimental iOS source. Its build uses the reviewed Android v4 source inputs identified in the iOS instructions; an iOS git archive alone does not include those downloaded inputs.

v4 checksums:

- APK: 144,820,752 bytes; SHA-256 `d9a9ce474a2ae0fbea5e2cb6e1d3014df20b36a2d1d76f3927f56fd6430984e6`
- Complete source: 140,426,654 bytes; SHA-256 `9b26376dc82d7ff796826b288d13902e9b1b67738ec15ba8274129b7d7992a4f`

See the [v4 release notes](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v4) for signing details, build checks and validation limits, and [SOURCE_PLAN.md](SOURCE_PLAN.md) for the publication checklist.

## Credits and licensing

Credit belongs to the original NZ:P Team, id Software's Quake/id Tech foundations, FTE QuakeWorld contributors, the Q3E/Android port contributors and the original asset creators. This unofficial project is not endorsed by those teams or authors.

The project combines components with different licenses. Original copyright notices and component-specific terms are retained; no single license is asserted for every file or asset. See [LICENSES.md](LICENSES.md) and the complete source archive.
