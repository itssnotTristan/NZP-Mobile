// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NZP_TOUCH_H
#define NZP_TOUCH_H

#include "nzp_mobile_state.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { NZP_AUTO_HOLD_MS = 320, NZP_MAX_TAP_MS = 500 };
#define NZP_DRAG_SLOP_POINTS 8.0f
#define NZP_WEAPON_SWITCH_SLOP_POINTS 24.0f

/* Callbacks are synchronous and optional. Their context must outlive the state
 * machine. Serialize all calls on one queue and do not reenter from callbacks.
 * cancel_actions must invalidate native pending fire/ADS pulses BEFORE releases.
 * A native adapter must check expected_weapon against current authority again.
 * No text commands are constructed or accepted by this interface. */
typedef struct {
    void *context;
    void (*tap_fire)(void *context);
    void (*auto_fire)(void *context, bool enabled, int expected_weapon);
    void (*look)(void *context, float dx, float dy);
    void (*ads)(void *context, bool enabled, int expected_weapon);
    void (*cancel_actions)(void *context);
} NZPWorldSink;

typedef enum {
    NZP_WORLD_NONE, NZP_WORLD_PENDING, NZP_WORLD_LOOK,
    NZP_WORLD_AUTO, NZP_WORLD_HELD, NZP_WORLD_BLOCKED
} NZPWorldPhase;

/* Fields are exposed for allocation/FFI, but owned by these functions. */
typedef struct {
    NZPWorldSink sink;
    NZPGameState state;
    int64_t pointer, down_at_ms, last_time_ms;
    int weapon_at_down, aim_weapon;
    NZPWorldPhase phase;
    bool auto_at_down, known_at_down, aim_latched, active;
    float start_x, start_y, last_x, last_y;
} NZPWorldInput;

void nzp_world_init(NZPWorldInput *input, NZPWorldSink sink);
void nzp_world_set_active(NZPWorldInput *input, bool active);
void nzp_world_update(NZPWorldInput *input, const NZPGameState *state, int64_t now_ms);
bool nzp_world_down(NZPWorldInput *input, int64_t pointer, float x, float y, int64_t now_ms);
void nzp_world_move(NZPWorldInput *input, int64_t pointer, float x, float y, int64_t now_ms);
void nzp_world_tick(NZPWorldInput *input, int64_t now_ms);
void nzp_world_up(NZPWorldInput *input, int64_t pointer, float x, float y, int64_t now_ms);
void nzp_world_toggle_ads(NZPWorldInput *input, int64_t now_ms);
bool nzp_world_aim_latched(const NZPWorldInput *input);
bool nzp_world_auto_firing(const NZPWorldInput *input);
bool nzp_world_has_contact(const NZPWorldInput *input);
void nzp_world_cancel(NZPWorldInput *input);       /* blocks current contact; look can continue */
void nzp_world_end_contacts(NZPWorldInput *input); /* normal all-fingers-up; preserves latched ADS when idle */
void nzp_world_cancel_all(NZPWorldInput *input);   /* interruption/lifecycle/menu */
void nzp_world_clear_ads(NZPWorldInput *input);

/* A weapon-widget tap reloads; a horizontal swipe switches once. It never
 * generates both. The widget routes the returned enum to a typed native API. */
typedef enum { NZP_WEAPON_NONE, NZP_WEAPON_RELOAD, NZP_WEAPON_SWITCH } NZPWeaponAction;
typedef struct {
    float x, y;
    int64_t down_at_ms;
    bool moved, committed, cancelled;
} NZPWeaponGesture;
void nzp_weapon_begin(NZPWeaponGesture *gesture, float x, float y, int64_t now_ms);
NZPWeaponAction nzp_weapon_move(NZPWeaponGesture *gesture, float x, float y);
NZPWeaponAction nzp_weapon_up(NZPWeaponGesture *gesture, float x, float y, int64_t now_ms, bool inside);
void nzp_weapon_cancel(NZPWeaponGesture *gesture);

typedef struct {
    void *context;
    void (*sprint)(void *context, bool enabled);
} NZPMovementSink;
typedef struct {
    NZPMovementSink sink;
    int64_t owner, down_at_ms, last_tap_up_ms, last_time_ms;
    float down_x, down_y, last_tap_x, last_tap_y;
    bool moved, candidate, armed, outer_armed, sprinting, ever_sprint;
} NZPMovementGesture;
void nzp_movement_init(NZPMovementGesture *gesture, NZPMovementSink sink);
bool nzp_movement_down(NZPMovementGesture *gesture, int64_t pointer, float x, float y, int64_t now_ms);
void nzp_movement_move(NZPMovementGesture *gesture, int64_t pointer, float x, float y, int64_t now_ms);
/* forward is the circular-clamped forward component BEFORE deadzone remapping;
 * raw_forward is displacement/ring radius BEFORE clamping, so >1 means outside
 * the ring. Use nzp_analog_normalize separately for the engine analog sink. */
void nzp_movement_analog(NZPMovementGesture *gesture, int64_t pointer, float forward, float raw_forward);
void nzp_movement_up(NZPMovementGesture *gesture, int64_t pointer, float x, float y, int64_t now_ms);
void nzp_movement_cancel(NZPMovementGesture *gesture);
bool nzp_movement_has_contact(const NZPMovementGesture *gesture);
bool nzp_movement_sprinting(const NZPMovementGesture *gesture);

/* Matches the canonical 0.13 analog deadzone and circular normalization.
 * Returns false and writes zeroes for nonfinite input. */
bool nzp_analog_normalize(float x, float y, float *out_x, float *out_y);

#ifdef __cplusplus
}
#endif
#endif
