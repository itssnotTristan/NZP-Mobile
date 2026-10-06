/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef NZP_IOS_ENGINE_H
#define NZP_IOS_ENGINE_H

#include "nzp_mobile_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Fixed Quake key ABI. Generic calls accept only this list, never function
 * keys used by guarded auto/ADS, console keys, commands, or VM names.
 * UIKit owns pointer reference counts before sending held key edges. */
enum {
    NZP_IOS_KEY_ACCEPT = 13, NZP_IOS_KEY_PAUSE = 27,
    NZP_IOS_KEY_JUMP = 32, NZP_IOS_KEY_BETTY = '4',
    NZP_IOS_KEY_USE = 'e', NZP_IOS_KEY_GRENADE = 'g',
    NZP_IOS_KEY_WEAPON_NEXT = 'q', NZP_IOS_KEY_RELOAD = 'r',
    NZP_IOS_KEY_KNIFE = 'v', NZP_IOS_KEY_STANCE = 'z',
    NZP_IOS_KEY_UP = 132, NZP_IOS_KEY_DOWN = 133,
    NZP_IOS_KEY_LEFT = 134, NZP_IOS_KEY_RIGHT = 135,
    NZP_IOS_KEY_FIRE = 178, NZP_IOS_KEY_SECONDARY = 179
};

/* Thread-safe UI requests. The engine thread alone touches FTE/VM state.
 * Taps survive a UI down/up between polls and have a paired next-poll release.
 * Requests arriving while inactive are discarded. SetActive(0) invalidates
 * published snapshots immediately and clears every pending input.
 * CancelActions clears auto/ADS/sprint and queued fire/secondary pulses;
 * ReleaseAll additionally clears normal keys, analog movement and look. */
void NZP_IOS_Key(int key, int down);
void NZP_IOS_TapKey(int key);
void NZP_IOS_SetMove(float side, float forward);
void NZP_IOS_AddLook(float dx, float dy);
void NZP_IOS_AutoFire(int expected_weapon, int enabled);
void NZP_IOS_ADS(int expected_weapon, int enabled);
void NZP_IOS_Sprint(int enabled);
void NZP_IOS_CancelActions(void);
void NZP_IOS_ReleaseAll(void);
void NZP_IOS_SetActive(int active);
/* Set only after the overlay attaches successfully. Defaults to false. */
void NZP_IOS_SetHUDReady(int ready);
void NZP_IOS_ReadState(NZPGameState *game, NZPHudState *hud, int *in_game);
int64_t NZP_IOS_Milliseconds(void);

/* Engine-thread hooks. Initialize after Host_Init (or first BeginFrame).
 * BeginFrame runs after SDL events, before engine input processing.
 * PublishState runs after Host_Frame and before UIKit reads its snapshot.
 * SetMove accepts raw [-1,1] stick displacement: this adapter applies the
 * canonical 0.13 radial deadzone and circular normalization exactly once.
 * AnalogMove is called from CL_BaseMove for seat zero, before speed scaling. */
void NZP_IOS_Initialize(void);
void NZP_IOS_BeginFrame(void);
void NZP_IOS_PublishState(void);
void NZP_IOS_AnalogMove(float moves[3], float forwardspeed,
                       float backspeed, float sidespeed);

#ifdef __cplusplus
}
#endif
#endif
