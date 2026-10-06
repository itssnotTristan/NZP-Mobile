/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "quakedef.h"
#include "nzp_ios_engine.h"
#include "nzp_ios_qc_state.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

test_cls_t cls;
test_cl_t cl;
int host_initialized = 1;
static double now = 1;
static int destinations, valid, guard_available = 1, hud_ready;
static int key_down[256], downs[256], ups[256], auto_guard, ads_guard, sprint_value;
static int auto_owner, ads_owner, sprint_owner;
static float look_x, look_y;
static nzp_mobile_state_t state;
double Sys_DoubleTime(void) { return now; }
int Key_Dest_Has(int mask) { return destinations & mask; }
int NET_IsLoopBackAddress(const netadr_t *address) { return address->local; }
void IN_KeyEvent(unsigned device, int down, int key, unsigned unicode)
{
    (void)device; (void)unicode;
    assert(key > 0 && key < 256);
    assert(key_down[key] != down); /* no duplicate down/unbalanced up */
    key_down[key] = down;
    if (down) ++downs[key]; else ++ups[key];
}
void IN_MouseMove(unsigned device, int absolute, float x, float y, float z, float size)
{ (void)device; (void)absolute; (void)z; (void)size; look_x += x; look_y += y; }
qboolean CSQC_NZPReadMobileState(nzp_mobile_state_t *out)
{ *out = state; if (!valid) { NZP_MobileClear(out); return false; } return true; }
void CSQC_NZPSetMobileHudReady(qboolean ready) { hud_ready = ready; }
qboolean SV_NZPMobileAutoGuard(int weapon, int entity, qboolean apply)
{
    if (!guard_available || !cls.netchan.remote_address.local) return false;
    if (apply) { auto_guard = weapon; auto_owner = entity; }
    return true;
}
qboolean SV_NZPMobileAdsGuard(int weapon, int entity, qboolean apply)
{
    if (!guard_available || !cls.netchan.remote_address.local) return false;
    if (apply) { ads_guard = weapon; ads_owner = entity; }
    return true;
}
qboolean SV_NZPMobileSprintIntent(int entity, int enabled)
{
    if (!guard_available || !cls.netchan.remote_address.local) return false;
    sprint_owner = entity; sprint_value = enabled; return true;
}
static void Frame(void) { now += 0.016; NZP_IOS_BeginFrame(); NZP_IOS_PublishState(); }
static void Reset(void)
{
    NZP_IOS_ReleaseAll(); Frame();
    memset(downs, 0, sizeof(downs)); memset(ups, 0, sizeof(ups));
    memset(key_down, 0, sizeof(key_down));
    cls.state = ca_active; cls.demoplayback = 0; cls.netchan.remote_address.local = 1;
    cl.paused = 0; cl.playerview[0].playernum = 0; destinations = 0;
    valid = guard_available = 1;
    NZP_MobileClear(&state);
    state.flags = NZP_MOBILE_VALID | NZP_MOBILE_ALIVE | NZP_MOBILE_AUTOMATIC | NZP_MOBILE_CAN_ADS;
    state.weapon = 7; state.stance = 2; state.points = 500; state.round = 3;
    state.magazine = 12; strcpy(state.weapon_name, "Test rifle");
    NZP_IOS_SetActive(1); NZP_IOS_SetHUDReady(1); Frame();
}
static void TestNormalKeys(void)
{
    Reset();
    NZP_IOS_Key(NZP_IOS_KEY_FIRE, 1); NZP_IOS_Key(NZP_IOS_KEY_FIRE, 0);
    Frame(); assert(key_down[NZP_IOS_KEY_FIRE]);
    Frame(); assert(!key_down[NZP_IOS_KEY_FIRE]);
    assert(downs[NZP_IOS_KEY_FIRE] == 1 && ups[NZP_IOS_KEY_FIRE] == 1);
    NZP_IOS_TapKey(NZP_IOS_KEY_RELOAD); NZP_IOS_TapKey(NZP_IOS_KEY_RELOAD);
    Frame(); Frame(); Frame(); Frame();
    assert(downs['r'] == 2 && ups['r'] == 2);
    NZP_IOS_Key('e', 1); Frame(); Frame(); assert(downs['e'] == 1);
    NZP_IOS_Key('e', 0); Frame(); assert(ups['e'] == 1);
    NZP_IOS_Key(K_F11, 1); NZP_IOS_TapKey(K_F12); NZP_IOS_TapKey('`'); Frame();
    assert(!downs[K_F11] && !downs[K_F12] && !downs['`']);
}
static void TestAutoFire(void)
{
    Reset();
    NZP_IOS_AutoFire(7, 1); NZP_IOS_AutoFire(7, 0);
    Frame(); assert(key_down[K_F11] && auto_guard == 7 && auto_owner == 1);
    Frame(); assert(!key_down[K_F11] && auto_guard == 0);
    NZP_IOS_AutoFire(7, 1); NZP_IOS_CancelActions(); Frame(); assert(downs[K_F11] == 1);
    NZP_IOS_AutoFire(7, 1); state.weapon = 8; Frame(); assert(!key_down[K_F11]);
    state.weapon = 7; Frame(); assert(!key_down[K_F11]);
    NZP_IOS_AutoFire(7, 1); Frame(); assert(key_down[K_F11]);
    cl.playerview[0].playernum = 1; Frame(); assert(!key_down[K_F11] && auto_owner == 1);
    NZP_IOS_AutoFire(7, 1); valid = 0; Frame(); assert(!key_down[K_F11]);
    valid = 1; Frame(); assert(!key_down[K_F11]);
    cls.netchan.remote_address.local = 0; NZP_IOS_AutoFire(7, 1); Frame(); assert(!key_down[K_F11]);
    cls.netchan.remote_address.local = 1; guard_available = 0;
    NZP_IOS_AutoFire(7, 1); Frame(); assert(!key_down[K_F11]);
}
static void TestAdsSprint(void)
{
    Reset();
    NZP_IOS_ADS(7, 1); NZP_IOS_Sprint(1); Frame();
    assert(key_down[K_F12] && ads_guard == 7 && ads_owner == 1);
    assert(sprint_value == 1 && sprint_owner == 1);
    state.flags |= NZP_MOBILE_DUAL; Frame(); assert(!key_down[K_F12]);
    state.flags &= ~NZP_MOBILE_DUAL; Frame(); assert(!key_down[K_F12]);
    NZP_IOS_ADS(7, 1); Frame(); assert(key_down[K_F12]);
    state.flags &= ~NZP_MOBILE_ALIVE; Frame();
    assert(!key_down[K_F12] && !sprint_value);
}
static void TestMenuLifecycle(void)
{
    NZPGameState game; NZPHudState hud; int active;
    Reset();
    NZP_IOS_AutoFire(7, 1); NZP_IOS_ADS(7, 1); NZP_IOS_Sprint(1);
    NZP_IOS_Key('e', 1); NZP_IOS_SetMove(1, 1); Frame();
    destinations = kdm_menu; Frame();
    assert(!key_down['e'] && !key_down[K_F11] && !key_down[K_F12] && !sprint_value);
    NZP_IOS_TapKey(NZP_IOS_KEY_DOWN); Frame(); assert(key_down[NZP_IOS_KEY_DOWN]);
    Frame(); assert(!key_down[NZP_IOS_KEY_DOWN]);
    destinations = 0; Frame(); assert(!key_down['e'] && !key_down[K_F11]);
    NZP_IOS_TapKey(NZP_IOS_KEY_FIRE); NZP_IOS_AutoFire(7, 1);
    NZP_IOS_SetActive(0); NZP_IOS_ReadState(&game, &hud, &active);
    assert(!active && !(game.flags & NZP_STATE_VALID) && hud.received_at_ms == -1);
    NZP_IOS_TapKey('e'); NZP_IOS_SetActive(1); Frame();
    assert(!key_down[NZP_IOS_KEY_FIRE] && !key_down[K_F11] && !key_down['e']);
}
static void TestAnalogAndPublication(void)
{
    NZPGameState game; NZPHudState hud; int active;
    float move[3] = {0};
    Reset();
    NZP_IOS_SetMove(.1f, 0); Frame(); NZP_IOS_AnalogMove(move, 200, 100, 150); assert(move[1] == 0);
    NZP_IOS_SetMove(0, -1); NZP_IOS_AddLook(5, -3); Frame();
    NZP_IOS_AnalogMove(move, 200, 100, 150); assert(move[0] == -100);
    assert(look_x == 5 && look_y == -3);
    Frame(); assert(look_x == 5 && look_y == -3);
    NZP_IOS_SetMove(NAN, INFINITY); Frame(); memset(move, 0, sizeof(move));
    NZP_IOS_AnalogMove(move, 200, 100, 150); assert(move[0] == 0 && move[1] == 0);
    NZP_IOS_ReadState(&game, &hud, &active);
    assert(active && nzp_game_state_automatic(&game, NZP_IOS_Milliseconds()));
    assert(game.weapon_id == 7 && hud.points == 500 && hud_ready);
    NZP_IOS_SetHUDReady(0); Frame(); assert(!hud_ready);
    NZP_IOS_SetHUDReady(1); Frame(); assert(hud_ready);
    cls.netchan.remote_address.local = 0; Frame(); NZP_IOS_ReadState(&game, NULL, NULL);
    assert(!(game.flags & (NZP_STATE_AUTOMATIC | NZP_STATE_CAN_ADS)));
    valid = 0; Frame(); NZP_IOS_ReadState(&game, &hud, NULL);
    assert(!(game.flags & NZP_STATE_VALID) && !hud_ready && hud.points == -1);
}
int main(void)
{
    NZP_IOS_Initialize();
    TestNormalKeys(); TestAutoFire(); TestAdsSprint(); TestMenuLifecycle(); TestAnalogAndPublication();
    puts("engine bridge: normal keys, pulse pairing, guard refusal, weapon/entity changes, death, menus, lifecycle, movement, snapshots passed");
    return 0;
}
