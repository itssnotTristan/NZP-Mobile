/* SPDX-License-Identifier: GPL-3.0-or-later
 * NZP Mobile, 2026-10-06: bounded, read-only gameplay presentation snapshot.
 * Canonical Android v4-revised snapshot ABI, used only inside the FTE adapter.
 */
#ifndef NZP_IOS_QC_STATE_H
#define NZP_IOS_QC_STATE_H

#include <stddef.h>
#include <string.h>
#include <math.h>

enum {
    NZP_MOBILE_VALID = 1, NZP_MOBILE_ALIVE = 2,
    NZP_MOBILE_AUTOMATIC = 4, NZP_MOBILE_CAN_USE = 8,
    NZP_MOBILE_ADS = 16, NZP_MOBILE_CAN_TACTICAL = 32,
    NZP_MOBILE_CAN_ADS = 64, NZP_MOBILE_DUAL = 128
};
typedef struct {
    int flags, stance, weapon, magazine, reserve, grenades, tactical;
    int points, round, powerups;
    char weapon_name[96], use_prompt[192];
} nzp_mobile_state_t;

static inline void NZP_MobileClear(nzp_mobile_state_t *out)
{
    memset(out, 0, sizeof(*out));
    out->stance = -1;
    out->points = out->round = -1;
}

/* Validate before float-to-int conversion, including hostile VM values. */
static inline int NZP_MobileInteger(float value, int low, int high, int *out)
{
    if (!isfinite(value) || value < low || value > high || value != floorf(value))
        return 0;
    *out = (int)value;
    return 1;
}

/* Game strings are legacy byte strings: retain printable ASCII only and
 * strip Quake color escapes, matching the canonical source snapshot.
 * Display text never becomes a command, URL, path, or format string.
 */
static inline void NZP_MobileText(char *out, size_t capacity, const char *text)
{
    size_t used = 0, scanned = 0;
    if (!capacity) return;
    while (text && *text && used + 1 < capacity && scanned++ < 1024) {
        unsigned char c = (unsigned char)*text++;
        if (c == '^' && *text >= '0' && *text <= '9') { ++text; continue; }
        if (c >= 32 && c <= 126) out[used++] = (char)c;
        else if (c == '\n' || c == '\r' || c == '\t') out[used++] = ' ';
        else if (c >= 128) out[used++] = '?';
    }
    out[used] = 0;
}
#endif
