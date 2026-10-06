// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NZP_MOBILE_STATE_H
#define NZP_MOBILE_STATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The gameplay flags are the Android v4-revised/native-QC contract, not guesses
 * based on a weapon name. All timestamps use one monotonic millisecond clock. */
enum {
    NZP_STATE_VALID = 1, NZP_STATE_ALIVE = 2, NZP_STATE_AUTOMATIC = 4,
    NZP_STATE_CAN_USE = 8, NZP_STATE_ADS = 16, NZP_STATE_CAN_TACTICAL = 32,
    NZP_STATE_CAN_ADS = 64, NZP_STATE_DUAL = 128,
    NZP_SNAPSHOT_MAX_AGE_MS = 1500,
    NZP_HUD_DOUBLE_POINTS = 1, NZP_HUD_INSTA_KILL = 2
};

/* UTF-8 display text only. Never pass these fields to a command interpreter. */
typedef struct {
    int flags, stance, weapon_id, magazine, reserve, grenades, tactical;
    char weapon_name[193]; /* at most 48 UTF-16 units, like the Java source */
    char use_prompt[481];  /* at most 120 UTF-16 units */
    char ammo_text[32], grenade_text[16], tactical_text[16];
    int64_t received_at_ms;
} NZPGameState;

typedef struct {
    int points, round, powerups;
    int64_t received_at_ms;
    char points_text[16], round_text[32];
} NZPHudState;

NZPGameState nzp_game_state_unknown(void);
NZPGameState nzp_game_state_make(int flags, int stance, int weapon_id,
    int magazine, int reserve, int grenades, int tactical,
    const char *weapon_name, const char *use_prompt, int64_t received_at_ms);
bool nzp_game_state_valid(const NZPGameState *state, int64_t now_ms);
bool nzp_game_state_live(const NZPGameState *state, int64_t now_ms);
bool nzp_game_state_automatic(const NZPGameState *state, int64_t now_ms);
bool nzp_game_state_can_ads(const NZPGameState *state, int64_t now_ms);
bool nzp_game_state_can_use(const NZPGameState *state, int64_t now_ms);
bool nzp_game_state_can_tactical(const NZPGameState *state, int64_t now_ms);
bool nzp_game_state_ads(const NZPGameState *state, int64_t now_ms);
const char *nzp_game_state_stance_label(const NZPGameState *state, int64_t now_ms);
bool nzp_game_state_same_display(const NZPGameState *a, const NZPGameState *b);

/* Display-only HUD: these values confer no gameplay authority. */
NZPHudState nzp_hud_state_unknown(void);
NZPHudState nzp_hud_state_make(int points, int round, int powerups, int64_t received_at_ms);
bool nzp_hud_state_valid(const NZPHudState *state, int64_t now_ms);
bool nzp_hud_state_same_display(const NZPHudState *a, const NZPHudState *b);

#ifdef __cplusplus
}
#endif
#endif
