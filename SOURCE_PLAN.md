# Source and release publication plan

Status: documentation scaffold only. No APK or playable release is available. Android phone validation is pending.

Before distributing an APK:

1. Review the exact source and game-data inventory, retaining original copyright notices, component license texts and asset attribution.
2. Publish the complete corresponding source needed for the distributed build, including Android frontend and native changes, game-code changes, build scripts, pinned upstream revisions and build instructions.
3. Review the build outputs and publish SHA-256 checksums tying the APK and source archive to the same release.
4. Keep signing keys, passwords, access tokens, local machine configuration, downloaded toolchains and unrelated private code out of this repository and its release assets.
5. Clearly identify the Android requirements, known limitations and which phone tests have and have not been completed. Do not call an untested build validated.
6. Publish only the specifically reviewed signed APK and matching source package. Label the initial phone-test build as a preview/prerelease and provide a direct download until mobile validation and the stable update channel are ready. Public GitHub release downloads should be usable without a player sign-in.

## Phone-first validation

Check installation, first launch, menus, simultaneous movement/aim/fire, other touch actions, multiple rounds, doors and purchases, sound, death/restart, pause/background/return, surface recreation, content repair and settings persistence on an Android phone. Co-op requires separate multi-device verification. No performance, broad device-compatibility or Quest claim is made by this scaffold.

No APK, tag or release is created by this publication plan itself.
