# Isolated FTE iOS engine target

This is a native source integration, not a prebuilt or device-validated iOS release.
The app target owns its UIKit overlay, assets, identifiers and signing. The engine
uses SDL2's UIKit/CoreAudio platform layer and OpenGL ES 3, falling back to ES 2.

## Build contract

1. Run `python3 tools/prepare_engine.py --android-root /path/to/canonical/source`.
2. Configure the parent project with `CMAKE_SYSTEM_NAME=iOS`, an Apple SDK, arm64
   architecture, and `NZP_ANDROID_SOURCE` pointing to that same source root.
3. Link `nzp_fte_engine` and the portable core; compile the exported
   `NZP_IOS_ENTRY_SOURCE` directly into the app executable.

Preparation copies engine and codec sources into `build/`, checks each exact
source patch against its before/after SHA-256, verifies the pinned SDL archive and
its member hashes, and removes only SDL's Android Q3E build prologue. It never
runs the Android FTE top-level CMake or compiles JNI, Q3E, Oboe, Vulkan, JIT,
plugins, package downloaders, an embedded compiler or a dedicated-server binary.
Local Quake game/server execution remains part of the app.

The generated source list is extracted from the canonical source's explicit
FTE source groups. No source-globbing is used for engine compilation. JPEG,
PNG, Ogg, Vorbis and SDL are static; zlib is supplied by the Apple SDK.

## Runtime integration

- SDL owns UIApplication. The entry point calls `SDL_UIKitRunApp`.
- The FTE startup initializes packaged `game/nzp` assets, creates writable
  `nzp/data`, preserves existing `user_settings.cfg`, and writes `mobile.cfg`
  from the exact trusted Java `MobileConfig.CONFIG` literals in the canonical
  Android source. It executes that config using `-usehome +exec mobile.cfg`.
- FTE runs in the UIKit display callback, with a maximum 100 ms resumed delta.
- `NZP_IOS_InitializeUI` is called when the renderer creates a window.
- `NZP_IOS_BeginFrame` runs after SDL events, before the engine processes input.
- `NZP_IOS_FrameReady` runs after `Host_Frame`; the overlay publishes/reads the
  native snapshot there. `NZP_IOS_HandleLifecycle` receives background/resume.
- Background entry invalidates typed input, clears FTE keys, saves configuration,
  pauses SDL audio and stops engine frames. It does not render while suspended.
- UIKit's framebuffer and color renderbuffer are restored for rendering and
  presentation. Framebuffer zero is not treated as the screen on iOS.
- The QC state readers and guarded local-server actions are separate from
  Android telemetry and headers. Internet broker/ICE support is disabled in
  this initial configuration. The no-TLS scheme rejection is also preserved
  independently of telemetry.

## Validation boundary

Source preparation and Linux C syntax/link checks can run without Apple tools.
They do not validate Apple SDK declarations, Objective-C integration, iOS linker
framework selection, signing, simulator/device startup, visible rendering,
audio, saved settings or sustained gameplay. Those are separate required checks.
Do not label this target playable or distributable solely because host checks
or an unsigned archive succeed. OpenGL ES is deprecated on Apple platforms; a
future Metal-backed renderer requires additional integration and validation.
