// SPDX-License-Identifier: GPL-3.0-or-later
#include "nzp_touch.h"
#include <float.h>
#include <math.h>
#include <string.h>

static bool finite_xy(float x, float y) { return isfinite(x) && isfinite(y); }
static double distance_squared(float x, float y, float origin_x, float origin_y) {
    double dx = (double)x - origin_x, dy = (double)y - origin_y;
    return dx * dx + dy * dy;
}
static void cancel_actions(NZPWorldInput *i) { if (i->sink.cancel_actions) i->sink.cancel_actions(i->sink.context); }
static void auto_fire(NZPWorldInput *i, bool enabled) {
    if (i->sink.auto_fire) i->sink.auto_fire(i->sink.context, enabled, i->weapon_at_down);
}
static bool send_look(NZPWorldInput *i, float x, float y, float origin_x, float origin_y) {
    double dx = (double)x - origin_x, dy = (double)y - origin_y;
    if (fabs(dx) > FLT_MAX || fabs(dy) > FLT_MAX) { nzp_world_cancel_all(i); return false; }
    if (i->sink.look) i->sink.look(i->sink.context, (float)dx, (float)dy);
    return true;
}
static bool world_time(NZPWorldInput *i, int64_t now) {
    if (now < 0 || now < i->last_time_ms) { nzp_world_cancel_all(i); return false; }
    i->last_time_ms = now;
    return true;
}
static bool manual_allowed(const NZPWorldInput *i, int64_t now) {
    return !(i->state.flags & NZP_STATE_VALID) || nzp_game_state_live(&i->state, now);
}
static void expire(NZPWorldInput *i, int64_t now) {
    if (i->known_at_down && i->pointer != -1 && !nzp_game_state_live(&i->state, now) && i->phase != NZP_WORLD_BLOCKED)
        nzp_world_cancel(i);
    else if (i->phase == NZP_WORLD_AUTO && (!nzp_game_state_automatic(&i->state, now) || i->state.weapon_id != i->weapon_at_down))
        nzp_world_cancel(i);
    if (i->aim_latched && !nzp_game_state_can_ads(&i->state, now)) { cancel_actions(i); nzp_world_clear_ads(i); }
}
void nzp_world_init(NZPWorldInput *i, NZPWorldSink sink) {
    memset(i, 0, sizeof(*i));
    i->sink = sink; i->state = nzp_game_state_unknown();
    i->pointer = -1; i->weapon_at_down = -1; i->aim_weapon = -1; i->last_time_ms = -1;
}
void nzp_world_set_active(NZPWorldInput *i, bool active) {
    if (active != i->active) { nzp_world_cancel_all(i); i->active = active; }
}
void nzp_world_update(NZPWorldInput *i, const NZPGameState *next, int64_t now) {
    NZPGameState unknown;
    bool weapon_changed, mode_changed, lost_known;
    if (!world_time(i, now)) return;
    if (!next) { unknown = nzp_game_state_unknown(); next = &unknown; }
    weapon_changed = i->state.weapon_id != next->weapon_id;
    mode_changed = (i->state.flags & NZP_STATE_AUTOMATIC) != (next->flags & NZP_STATE_AUTOMATIC);
    lost_known = (i->state.flags & NZP_STATE_VALID) && !nzp_game_state_live(next, now);
    if (weapon_changed || mode_changed || lost_known) nzp_world_cancel(i);
    if (weapon_changed || !nzp_game_state_can_ads(next, now)) nzp_world_clear_ads(i);
    i->state = *next;
}
bool nzp_world_down(NZPWorldInput *i, int64_t pointer, float x, float y, int64_t now) {
    if (!i->active || i->pointer != -1 || pointer < 0) return false;
    if (!world_time(i, now) || !finite_xy(x, y)) { nzp_world_cancel_all(i); return false; }
    expire(i, now);
    i->pointer = pointer; i->start_x = i->last_x = x; i->start_y = i->last_y = y; i->down_at_ms = now;
    i->weapon_at_down = i->state.weapon_id; i->known_at_down = nzp_game_state_live(&i->state, now);
    i->auto_at_down = nzp_game_state_automatic(&i->state, now);
    i->phase = manual_allowed(i, now) ? NZP_WORLD_PENDING : NZP_WORLD_LOOK;
    return true;
}
void nzp_world_tick(NZPWorldInput *i, int64_t now) {
    if (!world_time(i, now)) return;
    expire(i, now);
    if (i->pointer == -1 || i->phase != NZP_WORLD_PENDING) return;
    if (now - i->down_at_ms >= NZP_AUTO_HOLD_MS && i->auto_at_down
        && nzp_game_state_automatic(&i->state, now) && i->state.weapon_id == i->weapon_at_down) {
        i->phase = NZP_WORLD_AUTO; auto_fire(i, true);
    } else if (now - i->down_at_ms > NZP_MAX_TAP_MS) i->phase = NZP_WORLD_HELD;
}
void nzp_world_move(NZPWorldInput *i, int64_t pointer, float x, float y, int64_t now) {
    if (i->pointer == -1 || pointer != i->pointer) return;
    if (!finite_xy(x, y) || !world_time(i, now)) { nzp_world_cancel_all(i); return; }
    /* Match Android: physical hold can mature before a delayed timer callback. */
    nzp_world_tick(i, now);
    if ((i->phase == NZP_WORLD_PENDING || i->phase == NZP_WORLD_HELD)
        && distance_squared(x, y, i->start_x, i->start_y) > 64) {
        i->phase = NZP_WORLD_LOOK;
        if (!send_look(i, x, y, i->start_x, i->start_y)) return;
    } else if (i->phase == NZP_WORLD_LOOK || i->phase == NZP_WORLD_AUTO || i->phase == NZP_WORLD_BLOCKED) {
        if (!send_look(i, x, y, i->last_x, i->last_y)) return;
    }
    i->last_x = x; i->last_y = y;
}
void nzp_world_up(NZPWorldInput *i, int64_t pointer, float x, float y, int64_t now) {
    if (i->pointer == -1 || pointer != i->pointer) return;
    if (!finite_xy(x, y) || !world_time(i, now)) { nzp_world_cancel_all(i); return; }
    /* UP-only displacement must not retroactively earn automatic fire. */
    if ((i->phase == NZP_WORLD_PENDING || i->phase == NZP_WORLD_HELD)
        && distance_squared(x, y, i->start_x, i->start_y) > 64) {
        i->phase = NZP_WORLD_LOOK;
        if (!send_look(i, x, y, i->last_x, i->last_y)) return;
    } else nzp_world_move(i, pointer, x, y, now);
    if (i->phase == NZP_WORLD_PENDING && now - i->down_at_ms <= NZP_MAX_TAP_MS
        && manual_allowed(i, now) && i->state.weapon_id == i->weapon_at_down && i->sink.tap_fire)
        i->sink.tap_fire(i->sink.context);
    if (i->phase == NZP_WORLD_AUTO) auto_fire(i, false);
    i->pointer = -1; i->phase = NZP_WORLD_NONE;
}
void nzp_world_toggle_ads(NZPWorldInput *i, int64_t now) {
    if (!world_time(i, now)) return;
    expire(i, now);
    if (!i->active || !nzp_game_state_can_ads(&i->state, now)) { nzp_world_clear_ads(i); return; }
    i->aim_latched = !i->aim_latched; i->aim_weapon = i->state.weapon_id;
    if (i->sink.ads) i->sink.ads(i->sink.context, i->aim_latched, i->aim_weapon);
}
bool nzp_world_aim_latched(const NZPWorldInput *i) { return i->aim_latched; }
bool nzp_world_auto_firing(const NZPWorldInput *i) { return i->phase == NZP_WORLD_AUTO; }
bool nzp_world_has_contact(const NZPWorldInput *i) { return i->pointer != -1; }
void nzp_world_clear_ads(NZPWorldInput *i) {
    if (i->aim_latched) {
        i->aim_latched = false;
        if (i->sink.ads) i->sink.ads(i->sink.context, false, i->aim_weapon);
    }
}
void nzp_world_cancel(NZPWorldInput *i) {
    cancel_actions(i);
    if (i->phase == NZP_WORLD_AUTO) auto_fire(i, false);
    if (i->pointer != -1) i->phase = NZP_WORLD_BLOCKED;
    nzp_world_clear_ads(i);
}
void nzp_world_end_contacts(NZPWorldInput *i) {
    if (i->phase != NZP_WORLD_NONE && i->phase != NZP_WORLD_BLOCKED) nzp_world_cancel(i);
    i->pointer = -1; i->phase = NZP_WORLD_NONE;
}
void nzp_world_cancel_all(NZPWorldInput *i) {
    cancel_actions(i); nzp_world_end_contacts(i); nzp_world_clear_ads(i);
}

void nzp_weapon_begin(NZPWeaponGesture *g, float x, float y, int64_t now) {
    memset(g, 0, sizeof(*g)); g->x = x; g->y = y; g->down_at_ms = now;
    g->cancelled = now < 0 || !finite_xy(x, y);
}
NZPWeaponAction nzp_weapon_move(NZPWeaponGesture *g, float x, float y) {
    double dx, dy;
    if (g->cancelled || g->committed) return NZP_WEAPON_NONE;
    if (!finite_xy(x, y)) { nzp_weapon_cancel(g); return NZP_WEAPON_NONE; }
    dx = (double)x - g->x; dy = (double)y - g->y;
    if (dx * dx + dy * dy > 64) g->moved = true;
    if (fabs(dx) >= NZP_WEAPON_SWITCH_SLOP_POINTS && fabs(dx) > fabs(dy) * 1.25) {
        g->committed = true; return NZP_WEAPON_SWITCH;
    }
    return NZP_WEAPON_NONE;
}
NZPWeaponAction nzp_weapon_up(NZPWeaponGesture *g, float x, float y, int64_t now, bool inside) {
    NZPWeaponAction action;
    /* A reversed clock must not allow even a displaced UP-only switch. */
    if (now < 0 || now < g->down_at_ms) { nzp_weapon_cancel(g); return NZP_WEAPON_NONE; }
    action = nzp_weapon_move(g, x, y);
    if (action == NZP_WEAPON_SWITCH) return action;
    if (g->cancelled || g->committed) return NZP_WEAPON_NONE;
    g->committed = true;
    return !g->moved && inside && now - g->down_at_ms <= NZP_MAX_TAP_MS ? NZP_WEAPON_RELOAD : NZP_WEAPON_NONE;
}
void nzp_weapon_cancel(NZPWeaponGesture *g) { g->cancelled = true; }

static void release_sprint(NZPMovementGesture *g) {
    if (g->sprinting) { g->sprinting = false; if (g->sink.sprint) g->sink.sprint(g->sink.context, false); }
}
static bool movement_time(NZPMovementGesture *g, int64_t now) {
    if (now < 0 || now < g->last_time_ms) { nzp_movement_cancel(g); return false; }
    g->last_time_ms = now;
    return true;
}
void nzp_movement_init(NZPMovementGesture *g, NZPMovementSink sink) {
    memset(g, 0, sizeof(*g)); g->sink = sink; g->owner = -1; g->last_time_ms = -1;
}
bool nzp_movement_down(NZPMovementGesture *g, int64_t pointer, float x, float y, int64_t now) {
    if (g->owner != -1 || pointer < 0) return false;
    if (!finite_xy(x, y) || !movement_time(g, now)) { nzp_movement_cancel(g); return false; }
    g->owner = pointer; g->down_x = x; g->down_y = y; g->down_at_ms = now;
    g->moved = false; g->outer_armed = false; g->ever_sprint = false;
    g->armed = g->candidate && now >= g->last_tap_up_ms && now - g->last_tap_up_ms <= 280
        && distance_squared(x, y, g->last_tap_x, g->last_tap_y) <= 48 * 48;
    g->candidate = false;
    return true;
}
void nzp_movement_move(NZPMovementGesture *g, int64_t pointer, float x, float y, int64_t now) {
    if (g->owner == -1 || pointer != g->owner) return;
    if (!finite_xy(x, y) || !movement_time(g, now)) { nzp_movement_cancel(g); return; }
    if (distance_squared(x, y, g->down_x, g->down_y) > 12 * 12) g->moved = true;
}
void nzp_movement_analog(NZPMovementGesture *g, int64_t pointer, float forward, float raw_forward) {
    if (g->owner == -1 || pointer != g->owner) return;
    if (!finite_xy(forward, raw_forward)) { nzp_movement_cancel(g); return; }
    if (raw_forward > 1.0f && forward >= .25f) g->outer_armed = true;
    else if (raw_forward < .85f || forward < .20f) g->outer_armed = false;
    if (g->sprinting && !g->armed && !g->outer_armed) release_sprint(g);
    if ((g->armed || g->outer_armed) && !g->sprinting && forward >= .25f) {
        g->sprinting = true; g->ever_sprint = true;
        if (g->sink.sprint) g->sink.sprint(g->sink.context, true);
    }
}
void nzp_movement_up(NZPMovementGesture *g, int64_t pointer, float x, float y, int64_t now) {
    bool tapped;
    if (g->owner == -1 || pointer != g->owner) return;
    nzp_movement_move(g, pointer, x, y, now);
    if (g->owner == -1) return; /* invalid event cancelled the owner */
    tapped = !g->armed && !g->ever_sprint && !g->outer_armed && !g->sprinting
        && !g->moved && now - g->down_at_ms <= 220;
    release_sprint(g); g->owner = -1; g->armed = false; g->outer_armed = false;
    g->candidate = tapped;
    if (tapped) { g->last_tap_x = x; g->last_tap_y = y; g->last_tap_up_ms = now; }
}
void nzp_movement_cancel(NZPMovementGesture *g) {
    release_sprint(g); g->owner = -1; g->candidate = false;
    g->armed = false; g->outer_armed = false; g->moved = false;
}
bool nzp_movement_has_contact(const NZPMovementGesture *g) { return g->owner != -1; }
bool nzp_movement_sprinting(const NZPMovementGesture *g) { return g->sprinting; }
bool nzp_analog_normalize(float x, float y, float *out_x, float *out_y) {
    double length, scale;
    if (!out_x || !out_y) return false;
    *out_x = 0; *out_y = 0;
    if (!finite_xy(x, y)) return false;
    length = hypot((double)x, (double)y);
    if (length <= (double).13f) return true;
    scale = (fmin(length, 1.0) - (double).13f) / (1.0 - (double).13f) / length;
    *out_x = (float)(x * scale); *out_y = (float)(y * scale);
    return true;
}
