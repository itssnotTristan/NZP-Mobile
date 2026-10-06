/* SPDX-License-Identifier: GPL-3.0-or-later
 * Native UIKit -> FTE adapter. Guarded actions are adapted from canonical
 * v4-revised sys_android.c; no JNI, Android loader, or command text is used.
 */
#include "quakedef.h"
#include "nzp_ios_engine.h"
#include "nzp_ios_qc_state.h"
#include "nzp_touch.h"
#include <math.h>
#include <pthread.h>
#include <string.h>

extern qboolean CSQC_NZPReadMobileState(nzp_mobile_state_t *state);
extern void CSQC_NZPSetMobileHudReady(qboolean ready);
extern qboolean SV_NZPMobileAutoGuard(int weapon, int entity, qboolean apply);
extern qboolean SV_NZPMobileAdsGuard(int weapon, int entity, qboolean apply);
extern qboolean SV_NZPMobileSprintIntent(int entity, int enabled);

/* ABI assertions protect Objective-C clients from silent FTE key changes. */
_Static_assert((int)K_MOUSE1 == NZP_IOS_KEY_FIRE, "FTE mouse key ABI changed");
_Static_assert((int)K_MOUSE2 == NZP_IOS_KEY_SECONDARY, "FTE mouse key ABI changed");
_Static_assert((int)K_UPARROW == NZP_IOS_KEY_UP, "FTE arrow key ABI changed");
_Static_assert((int)K_RIGHTARROW == NZP_IOS_KEY_RIGHT, "FTE arrow key ABI changed");

static const int keys[] = {
    NZP_IOS_KEY_ACCEPT, NZP_IOS_KEY_PAUSE, NZP_IOS_KEY_JUMP,
    NZP_IOS_KEY_BETTY, NZP_IOS_KEY_USE, NZP_IOS_KEY_GRENADE,
    NZP_IOS_KEY_WEAPON_NEXT, NZP_IOS_KEY_RELOAD, NZP_IOS_KEY_KNIFE,
    NZP_IOS_KEY_STANCE, NZP_IOS_KEY_UP, NZP_IOS_KEY_DOWN,
    NZP_IOS_KEY_LEFT, NZP_IOS_KEY_RIGHT, NZP_IOS_KEY_FIRE,
    NZP_IOS_KEY_SECONDARY
};
enum { KEY_COUNT = sizeof(keys) / sizeof(keys[0]), MAX_TAPS = 8 };
typedef struct {
    int active, cancel_actions, release_all, hud_ready;
    int auto_weapon, auto_pulse, ads_weapon, sprint;
    unsigned char held[KEY_COUNT], taps[KEY_COUNT];
    float side, forward, look_x, look_y;
} pending_t;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pending_t pending; /* inactive until the initialized host enables UI */
static NZPGameState published_game;
static NZPHudState published_hud;
static int published_in_game, initialized;

/* The following fields are owned exclusively by the engine thread. */
static unsigned char pressed[KEY_COUNT];
static int auto_pressed, ads_pressed, auto_entity, ads_entity, sprint_entity;
static int cancelled_capabilities, previously_active;
static float analog_side, analog_forward;

static int KeyIndex(int key)
{
    int i;
    for (i = 0; i < KEY_COUNT; ++i) if (keys[i] == key) return i;
    return -1;
}
static int MenuKey(int key)
{
    return key == NZP_IOS_KEY_ACCEPT || key == NZP_IOS_KEY_PAUSE ||
        (key >= NZP_IOS_KEY_UP && key <= NZP_IOS_KEY_RIGHT);
}
static void ClearActionsLocked(void)
{
    int i;
    pending.auto_weapon = pending.auto_pulse = pending.ads_weapon = pending.sprint = 0;
    for (i = 0; i < KEY_COUNT; ++i) {
        if (keys[i] == NZP_IOS_KEY_FIRE || keys[i] == NZP_IOS_KEY_SECONDARY)
            pending.held[i] = pending.taps[i] = 0;
    }
    pending.cancel_actions = 1;
}
static void ClearAllLocked(void)
{
    ClearActionsLocked();
    memset(pending.held, 0, sizeof(pending.held));
    memset(pending.taps, 0, sizeof(pending.taps));
    pending.side = pending.forward = pending.look_x = pending.look_y = 0;
    pending.release_all = 1;
}
void NZP_IOS_Initialize(void)
{
    pthread_mutex_lock(&mutex);
    if (!initialized) {
        published_game = nzp_game_state_unknown();
        published_hud = nzp_hud_state_unknown();
        initialized = 1;
    }
    pthread_mutex_unlock(&mutex);
}
int64_t NZP_IOS_Milliseconds(void)
{
    /* Sys_DoubleTime is the CSQC freshness clock too. No wall-clock mixing. */
    return (int64_t)(Sys_DoubleTime() * 1000.0);
}
void NZP_IOS_Key(int key, int down)
{
    int i = KeyIndex(key);
    if (i < 0) return;
    pthread_mutex_lock(&mutex);
    if (pending.active) {
        if (down && !pending.held[i] && pending.taps[i] < MAX_TAPS)
            ++pending.taps[i];
        pending.held[i] = !!down;
    }
    pthread_mutex_unlock(&mutex);
}
void NZP_IOS_TapKey(int key)
{
    int i = KeyIndex(key);
    if (i < 0) return;
    pthread_mutex_lock(&mutex);
    if (pending.active && pending.taps[i] < MAX_TAPS) ++pending.taps[i];
    pthread_mutex_unlock(&mutex);
}
void NZP_IOS_SetMove(float side, float forward)
{
    float x, y;
    nzp_analog_normalize(side, forward, &x, &y);
    pthread_mutex_lock(&mutex);
    pending.side = pending.active ? x : 0;
    pending.forward = pending.active ? y : 0;
    pthread_mutex_unlock(&mutex);
}
static float BoundedDelta(float value)
{
    if (!isfinite(value)) return 0;
    return value < -4096 ? -4096 : value > 4096 ? 4096 : value;
}
void NZP_IOS_AddLook(float dx, float dy)
{
    if (!isfinite(dx) || !isfinite(dy)) return;
    pthread_mutex_lock(&mutex);
    if (pending.active) {
        pending.look_x = BoundedDelta(pending.look_x + BoundedDelta(dx));
        pending.look_y = BoundedDelta(pending.look_y + BoundedDelta(dy));
    }
    pthread_mutex_unlock(&mutex);
}
void NZP_IOS_AutoFire(int weapon, int enabled)
{
    int request = enabled && weapon > 0 && weapon <= 1024 ? weapon : 0;
    pthread_mutex_lock(&mutex);
    if (!pending.active) request = 0;
    /* Ordinary finger-up retains the first qualified hold pulse. Cancellation
     * clears it. Its matching release is owned by the next engine input poll. */
    if (request && !pending.auto_weapon) pending.auto_pulse = request;
    pending.auto_weapon = request;
    pthread_mutex_unlock(&mutex);
}
void NZP_IOS_ADS(int weapon, int enabled)
{
    pthread_mutex_lock(&mutex);
    pending.ads_weapon = pending.active && enabled && weapon > 0 && weapon <= 1024 ? weapon : 0;
    pthread_mutex_unlock(&mutex);
}
void NZP_IOS_Sprint(int enabled)
{
    pthread_mutex_lock(&mutex);
    pending.sprint = pending.active && enabled == 1;
    pthread_mutex_unlock(&mutex);
}
void NZP_IOS_CancelActions(void)
{
    pthread_mutex_lock(&mutex);
    ClearActionsLocked();
    pthread_mutex_unlock(&mutex);
}
void NZP_IOS_ReleaseAll(void)
{
    pthread_mutex_lock(&mutex);
    ClearAllLocked();
    pthread_mutex_unlock(&mutex);
}
void NZP_IOS_SetActive(int active)
{
    NZP_IOS_Initialize();
    pthread_mutex_lock(&mutex);
    pending.active = !!active;
    if (!active) {
        ClearAllLocked();
        published_game = nzp_game_state_unknown();
        published_hud = nzp_hud_state_unknown();
        published_in_game = 0;
    }
    pthread_mutex_unlock(&mutex);
}
void NZP_IOS_SetHUDReady(int ready)
{
    pthread_mutex_lock(&mutex);
    pending.hud_ready = !!ready;
    pthread_mutex_unlock(&mutex);
}
static qboolean GameplayActive(void)
{
    int active;
    pthread_mutex_lock(&mutex);
    active = pending.active;
    pthread_mutex_unlock(&mutex);
    return active && host_initialized && cls.state == ca_active &&
        !Key_Dest_Has(kdm_menu|kdm_console|kdm_message|kdm_cwindows) &&
        !cl.paused && !cls.demoplayback;
}
static void ReleaseAuto(void)
{
    if (auto_pressed) IN_KeyEvent(0, false, K_F11, 0);
    auto_pressed = 0;
    if (auto_entity) SV_NZPMobileAutoGuard(0, auto_entity, true);
    auto_entity = 0;
}
static void ReleaseADS(void)
{
    if (ads_pressed) IN_KeyEvent(0, false, K_F12, 0);
    ads_pressed = 0;
    if (ads_entity) SV_NZPMobileAdsGuard(0, ads_entity, true);
    ads_entity = 0;
}
static void ReleaseSprint(void)
{
    if (sprint_entity) SV_NZPMobileSprintIntent(sprint_entity, 0);
    sprint_entity = 0;
}
static void ApplyAuto(int request, qboolean active)
{
    nzp_mobile_state_t state;
    int entity = cl.playerview[0].playernum + 1;
    qboolean permitted = request && active && (!auto_entity || auto_entity == entity) &&
        NET_IsLoopBackAddress(&cls.netchan.remote_address) &&
        CSQC_NZPReadMobileState(&state) && state.weapon == request &&
        (state.flags & (NZP_MOBILE_VALID|NZP_MOBILE_ALIVE|NZP_MOBILE_AUTOMATIC)) ==
            (NZP_MOBILE_VALID|NZP_MOBILE_ALIVE|NZP_MOBILE_AUTOMATIC);
    /* The server's W_Fire rechecks actual weapon at firing time. */
    if (permitted) permitted = SV_NZPMobileAutoGuard(request, entity, true);
    if (!permitted) {
        if (request) cancelled_capabilities |= NZP_MOBILE_AUTOMATIC;
        pthread_mutex_lock(&mutex);
        if (pending.auto_weapon == request) pending.auto_weapon = 0;
        pthread_mutex_unlock(&mutex);
        ReleaseAuto();
    } else {
        auto_entity = entity;
        if (!auto_pressed) IN_KeyEvent(0, true, K_F11, 0);
        auto_pressed = 1;
    }
}
static void ApplyADS(int request, qboolean active)
{
    nzp_mobile_state_t state;
    int entity = cl.playerview[0].playernum + 1;
    qboolean permitted = request && active && (!ads_entity || ads_entity == entity) &&
        NET_IsLoopBackAddress(&cls.netchan.remote_address) &&
        CSQC_NZPReadMobileState(&state) && state.weapon == request &&
        (state.flags & (NZP_MOBILE_VALID|NZP_MOBILE_ALIVE|NZP_MOBILE_CAN_ADS)) ==
            (NZP_MOBILE_VALID|NZP_MOBILE_ALIVE|NZP_MOBILE_CAN_ADS) && !(state.flags & NZP_MOBILE_DUAL);
    if (permitted) permitted = SV_NZPMobileAdsGuard(request, entity, true);
    if (!permitted) {
        if (request) cancelled_capabilities |= NZP_MOBILE_CAN_ADS;
        pthread_mutex_lock(&mutex);
        if (pending.ads_weapon == request) pending.ads_weapon = 0;
        pthread_mutex_unlock(&mutex);
        ReleaseADS();
    } else {
        ads_entity = entity;
        if (!ads_pressed) IN_KeyEvent(0, true, K_F12, 0);
        ads_pressed = 1;
    }
}
static void ApplySprint(int request, qboolean active)
{
    nzp_mobile_state_t state;
    int entity = cl.playerview[0].playernum + 1;
    qboolean permitted = request && active && (!sprint_entity || sprint_entity == entity) &&
        NET_IsLoopBackAddress(&cls.netchan.remote_address) && CSQC_NZPReadMobileState(&state) &&
        (state.flags & (NZP_MOBILE_VALID|NZP_MOBILE_ALIVE)) == (NZP_MOBILE_VALID|NZP_MOBILE_ALIVE);
    if (permitted) permitted = SV_NZPMobileSprintIntent(entity, 1);
    if (permitted) sprint_entity = entity;
    else {
        pthread_mutex_lock(&mutex);
        pending.sprint = 0;
        pthread_mutex_unlock(&mutex);
        ReleaseSprint();
    }
}
void NZP_IOS_BeginFrame(void)
{
    pending_t input;
    qboolean active = GameplayActive();
    int i;
    NZP_IOS_Initialize();
    pthread_mutex_lock(&mutex);
    /* Loading, menu, disconnect and pause never preserve gameplay intent. */
    if (!active) {
        ClearActionsLocked();
        pending.side = pending.forward = pending.look_x = pending.look_y = 0;
        for (i = 0; i < KEY_COUNT; ++i)
            if (!MenuKey(keys[i])) pending.held[i] = pending.taps[i] = 0;
    }
    input = pending;
    pending.auto_pulse = 0;
    pending.look_x = pending.look_y = 0;
    pending.release_all = pending.cancel_actions = 0;
    for (i = 0; i < KEY_COUNT; ++i)
        if (!pressed[i] && pending.taps[i]) --pending.taps[i];
    pthread_mutex_unlock(&mutex);

    if (input.cancel_actions || input.release_all || (previously_active && !active)) {
        ReleaseAuto();
        ReleaseADS();
        ReleaseSprint();
    }
    for (i = 0; i < KEY_COUNT; ++i) {
        int permitted = input.active && (active || MenuKey(keys[i]));
        int reset = input.release_all || (!active && !MenuKey(keys[i])) ||
            (input.cancel_actions && (keys[i] == NZP_IOS_KEY_FIRE || keys[i] == NZP_IOS_KEY_SECONDARY));
        if (reset && pressed[i]) {
            IN_KeyEvent(0, false, keys[i], 0);
            pressed[i] = 0;
        }
        /* A queued tap occupies one poll, then one released poll. */
        if (permitted && (input.held[i] || (!pressed[i] && input.taps[i]))) {
            if (!pressed[i]) IN_KeyEvent(0, true, keys[i], 0);
            pressed[i] = 1;
        } else if (pressed[i]) {
            IN_KeyEvent(0, false, keys[i], 0);
            pressed[i] = 0;
        }
    }
    analog_side = active ? input.side : 0;
    analog_forward = active ? input.forward : 0;
    if (active && (input.look_x || input.look_y))
        IN_MouseMove(0, false, input.look_x, input.look_y, 0, 0);
    ApplyAuto(input.auto_weapon ? input.auto_weapon : input.auto_pulse, active);
    ApplyADS(input.ads_weapon, active);
    ApplySprint(input.sprint, active);
    previously_active = active;
}
void NZP_IOS_AnalogMove(float moves[3], float forwardspeed, float backspeed, float sidespeed)
{
    if (!GameplayActive()) return;
    moves[0] += (analog_forward > 0 ? forwardspeed : backspeed) * analog_forward;
    moves[1] += sidespeed * analog_side;
}
void NZP_IOS_PublishState(void)
{
    nzp_mobile_state_t state;
    NZPGameState game;
    NZPHudState hud;
    qboolean active = GameplayActive();
    int entity = cl.playerview[0].playernum + 1;
    int ui_ready;
    int64_t now = NZP_IOS_Milliseconds();
    NZP_IOS_Initialize();
    NZP_MobileClear(&state);
    if (active && CSQC_NZPReadMobileState(&state)) {
        if (!NET_IsLoopBackAddress(&cls.netchan.remote_address) ||
            !SV_NZPMobileAutoGuard(0, entity, false)) state.flags &= ~NZP_MOBILE_AUTOMATIC;
        if (!NET_IsLoopBackAddress(&cls.netchan.remote_address) ||
            !SV_NZPMobileAdsGuard(0, entity, false)) state.flags &= ~NZP_MOBILE_CAN_ADS;
    }
    /* Publish a rejected capability for one complete UI update even if QC
     * recovers in this frame, preventing a silently latched UI intent. */
    state.flags &= ~cancelled_capabilities;
    cancelled_capabilities = 0;
    game = (state.flags & NZP_MOBILE_VALID) ? nzp_game_state_make(state.flags, state.stance,
        state.weapon, state.magazine, state.reserve, state.grenades, state.tactical,
        state.weapon_name, state.use_prompt, now) : nzp_game_state_unknown();
    hud = (state.flags & NZP_MOBILE_VALID) ?
        nzp_hud_state_make(state.points, state.round, state.powerups, now) : nzp_hud_state_unknown();
    pthread_mutex_lock(&mutex);
    if (!pending.active) { game = nzp_game_state_unknown(); hud = nzp_hud_state_unknown(); active = false; }
    published_game = game;
    published_hud = hud;
    published_in_game = active;
    ui_ready = pending.hud_ready;
    pthread_mutex_unlock(&mutex);
    CSQC_NZPSetMobileHudReady(ui_ready && active && (game.flags & NZP_STATE_VALID) && hud.points >= 0);
}
void NZP_IOS_ReadState(NZPGameState *game, NZPHudState *hud, int *in_game)
{
    NZP_IOS_Initialize();
    pthread_mutex_lock(&mutex);
    if (game) *game = published_game;
    if (hud) *hud = published_hud;
    if (in_game) *in_game = published_in_game;
    pthread_mutex_unlock(&mutex);
}
