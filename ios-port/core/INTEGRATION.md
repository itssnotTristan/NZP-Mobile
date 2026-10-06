# Portable iOS touch foundation

This directory is an allocation-free C99 translation of the Android
`v4-revised` input state machines. It has no UIKit, engine, command parser,
network, signing, or account dependency. It is source groundwork, not a
complete playable iOS port.

## Adapter contract

- Include `nzp_touch.h` and compile `nzp_touch.c` plus `nzp_mobile_state.c`.
  On hosts that require it, link the standard math library (`-lm`).
- Initialize each struct using its initializer before any other calls. The
  exposed struct fields support allocation and FFI; do not mutate them directly.
- Serialize UI events, display-link ticks, telemetry updates, and cancellations
  on one queue. Callbacks are synchronous and must not reenter the state machine.
  Keep callback contexts alive until all input is cancelled and the state machine
  is no longer used. A null callback is supported.
- Use UIKit **points**, not physical pixels. Map each UITouch to a stable,
  nonnegative `int64_t` ID until it ends. `-1` is the unowned sentinel. Different
  contacts may not steal world or movement ownership.
- Use one monotonic millisecond clock for events and snapshot receipt times.
  A UI receipt timestamp is not a server wall-clock timestamp. Call
  `nzp_world_tick` regularly, including stationary touches, to earn automatic
  hold at 320 ms and expire state after 1500 ms. Tick also expires latched ADS
  when no finger remains down.
- Call `nzp_world_update` for every authoritative gameplay snapshot. A display
  heartbeat can be visually deduplicated, but must still refresh receipt time.
  Use constructors to copy and sanitize telemetry into bounded value objects.
- Treat snapshot names and prompts as **display text only**. Their contents are
  never a source for gameplay commands. HUD points, round, and powerups confer
  no firing, aiming, movement, or interaction permission.
- Native `auto_fire` and `ads` adapters must recheck `expected_weapon` and current
  authority on the engine thread before applying the action. Preserve the
  distinction between a normal release and `cancel_actions`: cancellation drops
  pending native action pulses before a release; an ordinary lift preserves an
  already earned automatic pulse. Do not implement native calls by concatenating
  strings into the engine command buffer.
- Normal world contact: `down`, zero or more `move` calls, then `up`.
  `nzp_world_end_contacts` is only normal all-fingers-up cleanup. It preserves
  toggled ADS when no world contact needs cancellation.
- A weapon-widget contact starts with `nzp_weapon_begin` and should call
  `nzp_world_cancel` immediately, as the Android UI does. Route `RELOAD` to one
  typed reload action and `SWITCH` to one typed weapon-switch action. Cancel
  world actions again before dispatching either. A tap reloads; a predominantly
  horizontal swipe of at least 24 points switches in either direction.
- Menu, app inactive/background, scene interruption, controller replacement,
  teardown, or cancelled UIKit touches require `nzp_world_cancel_all`,
  `nzp_movement_cancel`, cancellation of any weapon gesture, and the adapter's
  own release of held buttons and analog input. Use `nzp_world_set_active(false)`
  during menu/inactive states. These state machines do not own unrelated buttons.
- For the movement ring, compute raw displacement divided by the ring radius,
  then circular-clamp the vector. Send that clamped forward component, **before
  deadzone adjustment**, and raw forward displacement to `nzp_movement_analog`.
  Send engine analog movement through `nzp_analog_normalize` separately. This
  matches `TouchControls.java`: feeding deadzone-adjusted forward into the sprint
  recognizer would shift the intended 0.25/0.20 thresholds.

## Retained behavior

- Ordinary tap: no early drag, no cancellation, at most 500 ms, inclusive.
- Automatic hold: only an authoritative live automatic weapon that was already
  automatic at contact-down, unchanged weapon identity, at least 320 ms.
- Stationary slop: 8 points inclusive. Earlier drag disarms firing for the whole
  contact. A hold that has already earned auto-fire can continue aiming while
  firing. A delayed move processes the physical hold threshold first, just like
  Android; a displaced UP-only event never invents that hold.
- ADS is toggled, survives ordinary lifts, and is cleared on weapon, capability,
  stale-state, death, or lifecycle invalidation. Dual weapons cannot claim ADS.
- Unknown/unmodified remote servers receive ordinary tap/drag behavior only;
  weapon names never imply automatic fire or ADS support.
- Weapon widget dispatches exactly one reload or switch, never both.
- Movement supports a 220 ms first tap, a 280 ms gap, and a 48-point second-tap
  radius; sprint begins at forward >=0.25. The outer ring enters above 1.0,
  retains sprint down to 0.85, and uses normalized 0.25/0.20 hysteresis.
  Doubletap-owned sprint survives returning inside the ring until finger lift.
- All authoritative gameplay and display-only HUD snapshots expire after
  1500 ms, inclusive at the boundary.

## Deliberate defensive additions

Valid monotonic input is checked against the unchanged canonical Java with a
50,000-event differential trace. Malformed-input behavior is intentionally
stricter than Java:

- Nonfinite touch/analog coordinates cancel safely; overflowed look deltas are
  not forwarded. Unrelated pointers cannot inject cancellations into an owner.
- Negative IDs/times are rejected. A timestamp earlier than the last accepted
  timestamp cancels active gestures and cannot produce a new action. A clock
  reset requires fresh initialization after cancellation, or waiting until the
  monotonic clock catches up. Use one clock to avoid this condition.
- Future-dated telemetry fails closed instead of treating negative age as zero.
- Display text keeps valid UTF-8; malformed sequences become `?`, and truncation
  never splits a Unicode scalar. Limits follow 48/120 UTF-16 units, except that
  a supplementary scalar crossing the last unit is omitted intact rather than
  leaving half a surrogate as Java substring could.

No dynamic memory allocation occurs in this core. UTF-8 input strings must be
NUL-terminated; the native transport adapter must validate/copy raw packet data
before constructing a state.

## Tests

Run `sh tests/run_tests.sh` from the iOS directory (or invoke it from elsewhere).
It compiles the production C sources directly with strict C99 warnings,
AddressSanitizer and UndefinedBehaviorSanitizer, then checks 162 assertions.
LeakSanitizer is disabled in this ptrace-managed environment; AddressSanitizer
and UBSan are enabled. No leak-coverage claim is made.

Optional baseline/differential verification:

`JAVA_HOME=/path/to/existing/jdk sh tests/run_java_parity.sh /path/to/android-port-v4-revised`

The script compiles the unchanged canonical Java sources, runs their 49 original
world-input assertions, and compares a 50,000-event callback/state trace with
this C implementation. Its local default finds the existing audited JDK and the
sibling Android source checkout. Those external prerequisites are not part of
this core. Both scripts clean their temporary build directories.

## Canonical source provenance

All source paths below are relative to
`android-port-v4-revised/app/src/main/java/org/nzp/mobile/preview/`.
They are GPL-3.0-or-later; the translation retains that license.

| Source | SHA-256 |
|---|---|
| GameplayInput.java | `9bae81edf7e8244312654c6bae0cab5618a3c27012597c0eead9005632b5f175` |
| MobileGameState.java | `e53338b64a4fbbdbadd6adc6d3446208791fe5fc7c7eb3771338fb79b628899c` |
| MobileHudState.java | `d69f41b5ec4e9ae358e03f309e8a6bc3bbc985da6bcfda4822be1543ac953417` |
| WeaponGesture.java | `fffce002e64d6967d9a1deb07de74751649098ab1cd24f234518980d48da28c8` |
| MovementGesture.java | `f692457e8798efd55e7acfa9603b2874d9a4a6b61c24eea6ad41a9e0da96b461` |
| InputState.java (analog math) | `3883f5f87f544e42ecbd0d007d35344b329c490c7b53fefd202705dcade0ff29` |
