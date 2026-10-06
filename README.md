# NZP-Mobile

Unofficial, work-in-progress Android mobile port of Nazi Zombies: Portable (NZ:P).

## Latest mobile test preview

[NZP Mobile 0.3.0 - Mobile test preview](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v3) is available.

- [Download the v3 signed ARM64 APK](https://github.com/itssnotTristan/NZP-Mobile/releases/download/android-v3/nzp-mobile-arm64.apk)
- [Download the v3 complete corresponding source](https://github.com/itssnotTristan/NZP-Mobile/releases/download/android-v3/nzp-mobile-0.3.0-preview-source.zip)
- Public downloads do not require a GitHub account.
- Earlier previews and their original assets remain available: [v2](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v2), [v1](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v1).

GitHub lists v3 as Latest/non-prerelease so the update button in installed v2 can discover it through the stable endpoint. This is an updater-compatibility setting. **v3 remains a mobile test preview; phone acceptance is pending.**

## Changes to try

- Raised movement pad and double-tap sprint
- Player-name prompt before first Play, plus rename support
- Updater support for subsequent preview releases
- LAN/direct multiplayer groundwork

Internet-ready multiplayer is not complete. LAN/direct play still needs multi-device testing. Movement, sprint, name persistence, update installation and v3 gameplay need phone validation. No Quest work is included.

## Updating and compatibility

On installed v2, use the Update button to check for v3. A direct APK download is also available above. Install over the existing app rather than uninstalling it to retain app data. v3 keeps package `org.nzp.mobile.preview` and the original signing identity, with versionCode 3. The on-phone Android installation prompt and upgrade behavior still need validation.

Requirements: ARM64, Android API 23 or newer, OpenGL ES 3.0. The preview targets API 30 and has a 4 KB memory-page baseline; support for 16 KB-page devices is not established.

## Source and checksums

The repository tree contains project documentation. The specifically named complete-source ZIP in each release's assets contains corresponding source, build instructions, changes, component license texts and attribution. GitHub's automatically generated source archives contain only this documentation scaffold.

v3 checksums:

- APK: 144,808,464 bytes; SHA-256 `db7a09c4b4e5eb4032a74a8cb7865a8a56c34bba00f3a1d1fca5610d9651a06b`
- Complete source: 140,320,651 bytes; SHA-256 `3ce99ba7611d299893e771a7dc9805e7d857636955d25cfe3c0a6d5ac1acaa6e`

See the [v3 release notes](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v3) for signing details, build checks and validation limits, and [SOURCE_PLAN.md](SOURCE_PLAN.md) for the publication checklist.

## Credits and licensing

Credit belongs to the original NZ:P Team, id Software's Quake/id Tech foundations, FTE QuakeWorld contributors, the Q3E/Android port contributors and the original asset creators. This unofficial project is not endorsed by those teams or authors.

The project combines components with different licenses. Original copyright notices and component-specific terms are retained; no single license is asserted for every file or asset. See [LICENSES.md](LICENSES.md) and the complete source archive.
