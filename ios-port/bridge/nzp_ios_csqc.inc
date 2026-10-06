/* SPDX-License-Identifier: GPL-3.0-or-later
 * Extracted from canonical v4-revised pr_csqc.c. Include only inside that TU,
 * after csqc VM declarations; do not compile this file on its own.
 */
#include "nzp_ios_qc_state.h"
/* NZP Mobile 2026-10-06: exact, typed, read-only game presentation globals.
 * References are renewed with each VM load and never used after shutdown. */
static struct {
    float *numbers[11];
    float *hud_ready;
    string_t *strings[2];
    double updated;
} nzp_mobile_qc;
static void NZP_MobileFindGlobals(void)
{
    static const char *names[] = {
        "nzp_mobile_version", "nzp_mobile_flags", "nzp_mobile_stance",
        "nzp_mobile_weapon", "nzp_mobile_magazine", "nzp_mobile_reserve",
        "nzp_mobile_grenades", "nzp_mobile_tactical",
        "nzp_mobile_points", "nzp_mobile_round", "nzp_mobile_powerups"
    };
    static const char *strings[] = { "nzp_mobile_weapon_name", "nzp_mobile_use_prompt" };
    int i;
    etype_t type;
    memset(&nzp_mobile_qc, 0, sizeof(nzp_mobile_qc));
    for (i = 0; i < 11; ++i) {
        float *p = (float *)PR_FindGlobal(csqcprogs, names[i], 0, &type);
        if (p && (type & 0xfff) == ev_float) nzp_mobile_qc.numbers[i] = p;
    }
    {
        float *p = (float *)PR_FindGlobal(csqcprogs, "nzp_mobile_hud_ready", 0, &type);
        if (p && (type & 0xfff) == ev_float) { nzp_mobile_qc.hud_ready = p; *p = 0; }
    }
    for (i = 0; i < 2; ++i) {
        string_t *p = (string_t *)PR_FindGlobal(csqcprogs, strings[i], 0, &type);
        if (p && (type & 0xfff) == ev_string) nzp_mobile_qc.strings[i] = p;
    }
}
/* Engine-owned readiness: resets with each VM, never a console command. */
void CSQC_NZPSetMobileHudReady(qboolean ready)
{
    if (csqcprogs && nzp_mobile_qc.hud_ready)
        *nzp_mobile_qc.hud_ready = ready ? 1 : 0;
}
qboolean CSQC_NZPReadMobileState(nzp_mobile_state_t *out)
{
    int i, version;
    int *values[] = { &out->flags, &out->stance, &out->weapon, &out->magazine,
                     &out->reserve, &out->grenades, &out->tactical,
                     &out->points, &out->round, &out->powerups };
    const int low[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    const int high[] = { 255, 2, 1024, 1000000, 1000000, 10000, 10000, 16777216, 255, 3 };
    NZP_MobileClear(out);
    if (!csqcprogs || csqc_nogameaccess || cls.state != ca_active || !cl.worldmodel ||
        !nzp_mobile_qc.updated || Sys_DoubleTime() < nzp_mobile_qc.updated ||
        Sys_DoubleTime() - nzp_mobile_qc.updated > 0.25)
        return false;
    for (i = 0; i < 8; ++i) if (!nzp_mobile_qc.numbers[i]) return false;
    for (i = 0; i < 2; ++i) if (!nzp_mobile_qc.strings[i]) return false;
    if (!NZP_MobileInteger(*nzp_mobile_qc.numbers[0], 1, 1, &version)) return false;
    for (i = 0; i < 7; ++i) {
        if (!NZP_MobileInteger(*nzp_mobile_qc.numbers[i+1], low[i], high[i], values[i])) {
            NZP_MobileClear(out); return false;
        }
    }
    if (!(out->flags & NZP_MOBILE_VALID)) { NZP_MobileClear(out); return false; }
    /* HUD hiding (scoreboard/intro/cinematic/hardcore) must not remove input
     * capabilities. Missing or malformed additive HUD fields fall back. */
    if (nzp_mobile_qc.numbers[8] && nzp_mobile_qc.numbers[9] && nzp_mobile_qc.numbers[10] &&
        NZP_MobileInteger(*nzp_mobile_qc.numbers[8], -1, 16777216, &out->points) &&
        NZP_MobileInteger(*nzp_mobile_qc.numbers[9], -1, 255, &out->round) &&
        NZP_MobileInteger(*nzp_mobile_qc.numbers[10], 0, 3, &out->powerups)) {
        if (out->points < 0) { out->round=-1;out->powerups=0; }
        if (out->round < 0) out->powerups=0;
    } else { out->points=out->round=-1;out->powerups=0; }
    NZP_MobileText(out->weapon_name, sizeof(out->weapon_name),
        PR_GetString(csqcprogs, *nzp_mobile_qc.strings[0]));
    NZP_MobileText(out->use_prompt, sizeof(out->use_prompt),
        PR_GetString(csqcprogs, *nzp_mobile_qc.strings[1]));
    if (!(out->flags & NZP_MOBILE_ALIVE))
        out->flags &= ~(NZP_MOBILE_AUTOMATIC | NZP_MOBILE_CAN_USE | NZP_MOBILE_CAN_TACTICAL | NZP_MOBILE_CAN_ADS);
    if (!out->weapon) out->flags &= ~(NZP_MOBILE_AUTOMATIC | NZP_MOBILE_CAN_ADS);
    if (out->flags & NZP_MOBILE_DUAL) out->flags &= ~NZP_MOBILE_CAN_ADS;
    if (!out->use_prompt[0]) out->flags &= ~NZP_MOBILE_CAN_USE;
    if (!(out->flags & NZP_MOBILE_CAN_USE)) out->use_prompt[0] = 0;
    return true;
}
