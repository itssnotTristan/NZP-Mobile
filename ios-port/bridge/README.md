# UIKit/FTE bridge

`nzp_ios_engine.h` is the C ABI for UIKit. It imports only the portable snapshot
header, never Android, JNI, SDL, or FTE headers. The engine target compiles
`nzp_ios_engine.c`; the final application links `core/nzp_touch.c` and
`core/nzp_mobile_state.c` once. Do not add `bridge/tests` to production include
paths or compile the test sources into the application.

## Engine hooks

1. After `Host_Init`, initialize the overlay and call `NZP_IOS_SetActive(1)`.
   Initialization is otherwise lazy and the bridge starts inactive.
2. Call `NZP_IOS_SetHUDReady(1)` only after a visible overlay successfully attaches
   to the SDL window. Set it to zero when the overlay is detached or cannot draw.
   Native QC HUD remains enabled until this handshake and a valid HUD snapshot.
3. Call `NZP_IOS_BeginFrame()` at the end of `Sys_SendKeyEvents`, after SDL events.
4. At the end of `Host_Frame`, the UI's `NZP_IOS_FrameReady` hook calls
   `NZP_IOS_PublishState()`, then `NZP_IOS_ReadState` and updates its controls.
5. In seat-zero `CL_BaseMove`, call `NZP_IOS_AnalogMove(moves, fwdspeed, backspeed,
   sidespeed)` before movement speed scaling.
6. On background/inactive transitions call `NZP_IOS_SetActive(0)` before stopping
   the frame callback. The SDL lifecycle adapter also clears FTE key states.
   On resume set active again; old pending contacts and pulses do not return.

These engine/VM hooks run on the SDL/engine thread. UI request methods use a mutex
and never enter FTE or the QC VM. Use `NZP_IOS_Milliseconds()` for every portable
state-machine timestamp, so UI freshness and native freshness share a clock.

`SetMove` applies the canonical 0.13 radial deadzone and circular normalization.
Pass raw stick displacement, with positive forward toward the top of the screen.
`AddLook` accepts finite mouse deltas and bounds accumulated deltas. UIKit may
apply sensitivity before this call. UIKit owns multi-pointer reference counts
before calling `Key(key, down)`.

## Safety contract

- There is no command-string, file-path, user-selected global, or text-entry API.
  Normal keys have a fixed allowlist. F11/F12 can only be emitted by guarded code.
- An automatic hold retains its first pulse across ordinary finger-up and pairs
  it with a later release. `CancelActions` discards pending pulses and cancels
  auto/ADS/sprint plus ordinary queued fire. `ReleaseAll` also flushes other keys,
  look, and movement. Menus, pause, loading, disconnect, and lifecycle loss flush
  gameplay intent. A denied capability is published once to unlatch the UI.
- Each automatic/ADS request names an exact bounded weapon ID. Every input poll
  requires fresh typed CSQC state, live gameplay, the same weapon and entity,
  a loopback connection, and the expected server guard contract. QC rechecks the
  actual weapon at the moment of firing/ADS. Unmodified and remote servers fail
  closed for guarded actions. Ordinary manual taps still use normal input.
- CSQC numeric fields must be finite bounded integers of the correct VM type.
  The state expires after 250 ms without `CSQC_UpdateView`, including clock
  rollback. Optional invalid HUD fields hide only native overlay data, without
  removing valid gameplay capabilities. Display strings are bounded, sanitized,
  and never interpreted as commands.
- Server writes are restricted to four named numeric auto/ADS globals and the
  versioned per-player sprint field. The player must be a spawned loopback client;
  release cannot change another entity's guard. VM pointers are renewed on load
  and cleared on shutdown by the prepared FTE integration.

The fixed mobile config must bind F11 to `+attack`, F12 to `+button8`, and supply
canonical normal-key mappings. The bridge itself does not alter binds or execute
setup commands. The prepare/build adapter owns that config and the SDL hooks.

## Provenance and integration fragments

The private snapshot ABI and sanitizers in `nzp_ios_qc_state.h` are extracted from
Android v4-revised `doom3/neo/sys/android/nzp_mobile_state.h`. The include guard is
renamed to avoid colliding with the portable core header. `nzp_ios_csqc.inc` is the
canonical `engine/client/pr_csqc.c` telemetry block with Android/performance
includes removed and future timestamp rejection added. `nzp_ios_ssqc.inc` is the
canonical `engine/server/pr_cmds.c` guard block with spawned-loopback ownership
and matching-entity release checks added. Include these fragments inside their
original translation units. They rely on that translation unit's private VM
state and must not be compiled separately.

Prepared FTE source uses `NZP_IOS`, not `NZP_MOBILE_TELEMETRY`; diagnostics stay
independent and disabled. `tools/prepare_engine.py` owns the bounded changes to
VM load/shutdown/update hooks and these fragment includes. Canonical Android
source remains untouched.

## Checks

Run from any working directory:

```sh
ios-port/bridge/run-tests.sh
NZP_SANITIZERS=1 ios-port/bridge/run-tests.sh
```

The fake-host runtime checks tap pairing, rapid taps, held keys, key allowlists,
weapon/entity transitions, remote/missing guards, death, menus, lifecycle,
movement/deadzone, native HUD readiness, and snapshot publication. The fake-VM
checks compile the actual extracted includes and cover typed numeric limits,
NaN/fractional values, stale/future samples, additive HUD fallback, exact local
entity ownership, server version checks, and bounded writes.

These are portable contract tests, not evidence of an iOS application build,
GPU/audio correctness, or on-device gesture behavior. The actual Apple SDK build
and simulator/device smoke tests remain separate requirements.
