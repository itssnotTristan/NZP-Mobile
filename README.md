# NZP-Mobile

Unofficial, work-in-progress Android mobile port of Nazi Zombies: Portable (NZ:P).

## Latest Android phone-test preview

[NZP Mobile 0.2.0 Preview - Touch controls redesign](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v2) is available as a prerelease.

- [Download the v2 signed ARM64 APK](https://github.com/itssnotTristan/NZP-Mobile/releases/download/android-v2/nzp-mobile-arm64.apk)
- [Download the v2 complete corresponding source](https://github.com/itssnotTristan/NZP-Mobile/releases/download/android-v2/nzp-mobile-0.2.0-preview-source.zip)
- Public downloads do not require a GitHub account.
- [The previous v1 preview and its original assets remain available](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v1).

v1 was reported working well on a Galaxy S23+, with clunky controls. v2 introduces a compact HUD, tap-fire/drag-look input, stationary-hold firing, toggle ADS, a fixed stance control, contextual use, and gun-control tap-to-reload/swipe-to-switch. Existing menu interaction is preserved.

Stationary-hold firing and toggle ADS are restricted to local solo play. Remote/legacy co-op uses manual taps and drag-look fallback. v2 installation, upgrade behavior, controls, gameplay, sound, interruptions and sustained performance still need phone validation. Multiplayer behavior is not device-validated. No Quest work is included.

## Updating and compatibility

Download the APK and install it over v1. v2 keeps package `org.nzp.mobile.preview` and the same signing identity, with versionCode 2. Do not uninstall v1 if you want to retain its app data. Actual upgrade behavior still needs phone validation.

The in-app updater uses the stable release feed and excludes these previews. Use the direct GitHub APK download for this update; no stable release is published yet.

Requirements: ARM64, Android API 23 or newer, OpenGL ES 3.0. The preview targets API 30 and has a 4 KB memory-page baseline; support for 16 KB-page devices is not established.

## Source and checksums

The repository tree contains project documentation. The specifically named complete-source ZIP in each release's assets contains corresponding source, build instructions, changes, component license texts and attribution. GitHub's automatically generated source archives contain only this documentation scaffold.

v2 checksums:

- APK: 144,796,176 bytes; SHA-256 `752fb176c9225c3d64591785dcdfb1a0fd42106948407a12fac9ddc8e8857e32`
- Complete source: 140,281,029 bytes; SHA-256 `9dc5a5da8bd4a5259aacdeeb04803f12014e07303c05859c82803a83cfcce646`

See the [v2 release notes](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v2) for signing details, build checks and validation limits, and [SOURCE_PLAN.md](SOURCE_PLAN.md) for the publication checklist.

## Credits and licensing

Credit belongs to the original NZ:P Team, id Software's Quake/id Tech foundations, FTE QuakeWorld contributors, the Q3E/Android port contributors and the original asset creators. This unofficial project is not endorsed by those teams or authors.

The project combines components with different licenses. Original copyright notices and component-specific terms are retained; no single license is asserted for every file or asset. See [LICENSES.md](LICENSES.md) and the complete source archive.
