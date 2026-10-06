// SPDX-License-Identifier: GPL-3.0-or-later
#include "nzp_mobile_state.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Preserve UTF-8 without ever splitting a scalar; malformed sequences become
 * '?'. The native adapter supplies NUL-terminated strings to the constructor. */
static size_t utf8_size(const unsigned char *s, unsigned *units) {
    size_t n, i;
    unsigned code;
    *units = 1;
    if (s[0] < 0x80) return 1;
    if (s[0] >= 0xc2 && s[0] <= 0xdf) { n = 2; code = s[0] & 0x1f; }
    else if (s[0] >= 0xe0 && s[0] <= 0xef) { n = 3; code = s[0] & 0x0f; }
    else if (s[0] >= 0xf0 && s[0] <= 0xf4) { n = 4; code = s[0] & 0x07; }
    else return 0;
    for (i = 1; i < n; ++i) {
        if (s[i] == 0 || (s[i] & 0xc0) != 0x80) return 0;
        code = (code << 6) | (s[i] & 0x3f);
    }
    if ((n == 3 && code < 0x800) || (n == 4 && code < 0x10000)
        || (code >= 0xd800 && code <= 0xdfff) || code > 0x10ffff) return 0;
    if (n == 4) *units = 2;
    return n;
}

static void clean_text(char *out, size_t capacity, const char *text, unsigned limit) {
    const unsigned char *s = (const unsigned char *)(text ? text : "");
    size_t written = 0, last_nonspace = 0;
    unsigned used = 0;
    bool started = false, full = false, nonspace_after_full = false;
    while (*s) {
        char replacement;
        const unsigned char *bytes = s;
        unsigned units;
        size_t n;
        bool space;
        if (s[0] == '^' && s[1] >= '0' && s[1] <= '9') { s += 2; continue; }
        n = utf8_size(s, &units);
        if (n == 0) { n = 1; replacement = '?'; bytes = (const unsigned char *)&replacement; }
        else if (*s <= 31 || *s == 127) { replacement = ' '; bytes = (const unsigned char *)&replacement; }
        space = n == 1 && *bytes <= ' ';
        s += n;
        if (!started && space) continue;
        started = true;
        if (used + units > limit || written + n >= capacity) full = true;
        if (!full) {
            memcpy(out + written, bytes, n);
            written += n;
            used += units;
            if (!space) last_nonspace = written;
        } else if (!space) nonspace_after_full = true;
    }
    out[nonspace_after_full ? written : last_nonspace] = '\0';
}

static bool fresh(int64_t received, int64_t now) {
    /* Unlike Java's max(0, age), future timestamps are not authoritative. */
    return received >= 0 && now >= received && now - received <= NZP_SNAPSHOT_MAX_AGE_MS;
}

NZPGameState nzp_game_state_make(int flags, int stance, int weapon_id,
    int magazine, int reserve, int grenades, int tactical,
    const char *weapon_name, const char *use_prompt, int64_t received_at_ms) {
    NZPGameState s = {0};
    s.flags = flags; s.stance = stance; s.weapon_id = weapon_id;
    s.magazine = magazine > 0 ? magazine : 0;
    s.reserve = reserve > 0 ? reserve : 0;
    s.grenades = grenades > 0 ? grenades : 0;
    s.tactical = tactical > 0 ? tactical : 0;
    s.received_at_ms = received_at_ms;
    clean_text(s.weapon_name, sizeof(s.weapon_name), weapon_name, 48);
    clean_text(s.use_prompt, sizeof(s.use_prompt), use_prompt, 120);
    (void)snprintf(s.ammo_text, sizeof(s.ammo_text), "%d / %d", s.magazine, s.reserve);
    (void)snprintf(s.grenade_text, sizeof(s.grenade_text), "%d", s.grenades);
    (void)snprintf(s.tactical_text, sizeof(s.tactical_text), "%d", s.tactical);
    return s;
}
NZPGameState nzp_game_state_unknown(void) {
    return nzp_game_state_make(0, -1, -1, 0, 0, 0, 0, "", "", -1);
}
bool nzp_game_state_valid(const NZPGameState *s, int64_t now) {
    return s && (s->flags & NZP_STATE_VALID) && fresh(s->received_at_ms, now);
}
bool nzp_game_state_live(const NZPGameState *s, int64_t now) {
    return nzp_game_state_valid(s, now) && (s->flags & NZP_STATE_ALIVE);
}
bool nzp_game_state_automatic(const NZPGameState *s, int64_t now) {
    return nzp_game_state_live(s, now) && (s->flags & NZP_STATE_AUTOMATIC);
}
bool nzp_game_state_can_ads(const NZPGameState *s, int64_t now) {
    return nzp_game_state_live(s, now) && (s->flags & NZP_STATE_CAN_ADS) && !(s->flags & NZP_STATE_DUAL);
}
bool nzp_game_state_can_use(const NZPGameState *s, int64_t now) {
    return nzp_game_state_live(s, now) && (s->flags & NZP_STATE_CAN_USE) && s->use_prompt[0];
}
bool nzp_game_state_can_tactical(const NZPGameState *s, int64_t now) {
    return nzp_game_state_live(s, now) && (s->flags & NZP_STATE_CAN_TACTICAL);
}
bool nzp_game_state_ads(const NZPGameState *s, int64_t now) {
    return nzp_game_state_live(s, now) && (s->flags & NZP_STATE_ADS);
}
const char *nzp_game_state_stance_label(const NZPGameState *s, int64_t now) {
    if (!nzp_game_state_live(s, now)) return "Stance";
    switch (s->stance) { case 2: return "Standing"; case 1: return "Crouched"; case 0: return "Prone"; default: return "Stance"; }
}
bool nzp_game_state_same_display(const NZPGameState *a, const NZPGameState *b) {
    return a && b && a->flags == b->flags && a->stance == b->stance && a->weapon_id == b->weapon_id
        && a->magazine == b->magazine && a->reserve == b->reserve && a->grenades == b->grenades
        && a->tactical == b->tactical && !strcmp(a->weapon_name, b->weapon_name) && !strcmp(a->use_prompt, b->use_prompt);
}
NZPHudState nzp_hud_state_make(int points, int round, int powerups, int64_t received) {
    NZPHudState s = {0};
    s.points = points; s.round = round;
    s.powerups = powerups < 0 ? 0 : powerups & (NZP_HUD_DOUBLE_POINTS | NZP_HUD_INSTA_KILL);
    s.received_at_ms = received;
    if (points >= 0) (void)snprintf(s.points_text, sizeof(s.points_text), "%d", points);
    if (round >= 0) (void)snprintf(s.round_text, sizeof(s.round_text), "ROUND %d", round);
    return s;
}
NZPHudState nzp_hud_state_unknown(void) { return nzp_hud_state_make(-1, -1, 0, -1); }
bool nzp_hud_state_valid(const NZPHudState *s, int64_t now) {
    return s && (s->points >= 0 || s->round >= 0 || s->powerups != 0) && fresh(s->received_at_ms, now);
}
bool nzp_hud_state_same_display(const NZPHudState *a, const NZPHudState *b) {
    return a && b && a->points == b->points && a->round == b->round && a->powerups == b->powerups;
}
