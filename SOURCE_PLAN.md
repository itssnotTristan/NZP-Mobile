# Source and release publication plan

Status: [android-v3 mobile test preview](https://github.com/itssnotTristan/NZP-Mobile/releases/tag/android-v3) includes the reviewed signed APK and matching complete source archive, with published SHA-256 checksums. Its GitHub Latest/non-prerelease setting lets installed v2 discover the update; it does not establish mobile readiness. Phone acceptance and multi-device testing remain pending, and internet-ready multiplayer is not complete. Original v1/v2 releases remain available. The repository tree remains a documentation scaffold.

Publication checklist for each APK:

1. Review the exact source and game-data inventory, retaining original copyright notices, component license texts and asset attribution.
2. Publish the complete corresponding source needed for the distributed build, including Android frontend and native changes, game-code changes, build scripts, pinned upstream revisions and build instructions.
3. Review the build outputs and publish SHA-256 checksums tying the APK and source archive to the same release.
4. Keep signing keys, passwords, access tokens, local machine configuration, downloaded toolchains and unrelated private code out of this repository and its release assets.
5. Clearly identify the Android requirements, known limitations and which phone tests have and have not been completed. Do not call an untested build validated.
6. Publish only the specifically reviewed signed APK and matching source package. Clearly label test builds as mobile previews in their title and notes. If a GitHub release flag is chosen for older-updater compatibility, explain that purpose without claiming device validation. Verify the intended update endpoint and provide public direct downloads without player sign-in.

## Phone-first validation

Check installation, first launch, menus, simultaneous movement/aim/fire, other touch actions, multiple rounds, doors and purchases, sound, death/restart, pause/background/return, surface recreation, content repair and settings persistence on an Android phone. Co-op requires separate multi-device verification. No performance, broad device-compatibility or Quest claim is made by this scaffold.

The initial preview passed packaging and fresh-source build checks. Those checks do not establish phone installation, gameplay, performance or interruption behavior.
