# Source and release publication plan

Status: the [android-v1 phone-test prerelease](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v1) includes the reviewed signed APK and matching complete source archive, with published SHA-256 checksums. The repository tree remains a documentation scaffold. Android phone validation is pending.

Publication checklist for each APK:

1. Review the exact source and game-data inventory, retaining original copyright notices, component license texts and asset attribution.
2. Publish the complete corresponding source needed for the distributed build, including Android frontend and native changes, game-code changes, build scripts, pinned upstream revisions and build instructions.
3. Review the build outputs and publish SHA-256 checksums tying the APK and source archive to the same release.
4. Keep signing keys, passwords, access tokens, local machine configuration, downloaded toolchains and unrelated private code out of this repository and its release assets.
5. Clearly identify the Android requirements, known limitations and which phone tests have and have not been completed. Do not call an untested build validated.
6. Publish only the specifically reviewed signed APK and matching source package. Label the initial phone-test build as a preview/prerelease and provide a direct download until mobile validation and the stable update channel are ready. Public GitHub release downloads should be usable without a player sign-in.

## Phone-first validation

Check installation, first launch, menus, simultaneous movement/aim/fire, other touch actions, multiple rounds, doors and purchases, sound, death/restart, pause/background/return, surface recreation, content repair and settings persistence on an Android phone. Co-op requires separate multi-device verification. No performance, broad device-compatibility or Quest claim is made by this scaffold.

The initial preview passed packaging and fresh-source build checks. Those checks do not establish phone installation, gameplay, performance or interruption behavior.
