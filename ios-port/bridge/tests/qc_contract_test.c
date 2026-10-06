/* SPDX-License-Identifier: GPL-3.0-or-later
 * Compile extracted CSQC/SSQC adapters against a deliberately small fake VM.
 */
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
typedef int qboolean;
typedef int string_t;
typedef int etype_t;
typedef union { float _float; } eval_t;
typedef struct { eval_t sprint; } edict_t;
typedef struct prog_s pubprogfuncs_t;
struct prog_s {
    eval_t *(*GetEdictFieldValue)(pubprogfuncs_t*, edict_t*, const char*, etype_t, void*);
};
enum { ev_float = 2, ev_string = 1, ca_active = 4, ss_active = 3, cs_spawned = 2 };
typedef struct { int local; } netadr_t;
typedef struct { int state; edict_t *edict; struct { netadr_t remote_address; } netchan; } client_t;
static struct { int state, allocated_client_slots; } sv;
static struct { client_t clients[2]; } svs;
static struct { int state; } cls;
static struct { void *worldmodel; } cl;
static pubprogfuncs_t client_vm, server_vm;
static pubprogfuncs_t *csqcprogs = &client_vm, *svprogfuncs = &server_vm;
static int csqc_nogameaccess;
static double now = 1;
static double Sys_DoubleTime(void) { return now; }
static int NET_IsLoopBackAddress(const netadr_t *a) { return a->local; }
static const char *client_names[] = {
 "nzp_mobile_version", "nzp_mobile_flags", "nzp_mobile_stance", "nzp_mobile_weapon",
 "nzp_mobile_magazine", "nzp_mobile_reserve", "nzp_mobile_grenades", "nzp_mobile_tactical",
 "nzp_mobile_points", "nzp_mobile_round", "nzp_mobile_powerups", "nzp_mobile_hud_ready"
};
static float client_values[12];
static string_t client_strings[2] = {1, 2};
static const char *texts[3] = {"", "^1Test rifle", "\nUse ^2item\t"};
static const char *server_names[] = {
 "nzp_mobile_guard_version", "nzp_mobile_auto_weapon", "nzp_mobile_auto_entity",
 "nzp_mobile_auto_release", "nzp_mobile_ads_weapon", "nzp_mobile_ads_entity",
 "nzp_mobile_ads_release", "nzp_mobile_sprint_version"
};
static float server_values[8];
static const char *missing, *wrong_type;
static void *PR_FindGlobal(pubprogfuncs_t *vm, const char *name, int unused, etype_t *type)
{
    size_t i; (void)unused;
    if (missing && !strcmp(name, missing)) return NULL;
    *type = wrong_type && !strcmp(name, wrong_type) ? ev_string : ev_float;
    if (vm == &client_vm) {
        for (i = 0; i < 12; ++i) if (!strcmp(name, client_names[i])) return &client_values[i];
        if (!strcmp(name, "nzp_mobile_weapon_name")) { *type = ev_string; return &client_strings[0]; }
        if (!strcmp(name, "nzp_mobile_use_prompt")) { *type = ev_string; return &client_strings[1]; }
    } else if (vm == &server_vm) {
        for (i = 0; i < 8; ++i) if (!strcmp(name, server_names[i])) return &server_values[i];
    }
    return NULL;
}
static const char *PR_GetString(pubprogfuncs_t *vm, string_t value)
{ (void)vm; return value >= 0 && value < 3 ? texts[value] : ""; }
static eval_t *GetField(pubprogfuncs_t *vm, edict_t *edict, const char *name, etype_t type, void *unused)
{
    (void)vm; (void)unused;
    assert(type == ev_float && !strcmp(name, "nzp_mobile_sprint_intent"));
    return edict ? &edict->sprint : NULL;
}
#include "nzp_ios_csqc.inc"
#include "nzp_ios_ssqc.inc"

static void ResetClient(void)
{
    float values[12] = {1, 255, 2, 7, 10, 100, 2, 1, 500, 3, 3, 0};
    memcpy(client_values, values, sizeof(values));
    missing = wrong_type = NULL; csqc_nogameaccess = 0;
    csqcprogs = &client_vm; cls.state = ca_active; cl.worldmodel = &client_vm;
    now = 1; NZP_MobileFindGlobals(); nzp_mobile_qc.updated = now;
}
static void ClientTests(void)
{
    nzp_mobile_state_t out;
    ResetClient(); assert(CSQC_NZPReadMobileState(&out));
    assert(out.weapon == 7 && out.points == 500 && !strcmp(out.weapon_name, "Test rifle"));
    assert(!(out.flags & NZP_MOBILE_CAN_ADS)); /* dual flag clears ADS */
    CSQC_NZPSetMobileHudReady(true); assert(client_values[11] == 1);
    NZP_MobileFindGlobals(); assert(client_values[11] == 0);
    nzp_mobile_qc.updated = 1; now = 1.251; assert(!CSQC_NZPReadMobileState(&out));
    now = .9; assert(!CSQC_NZPReadMobileState(&out));
    ResetClient(); client_values[3] = NAN; assert(!CSQC_NZPReadMobileState(&out));
    ResetClient(); client_values[3] = 3.5; assert(!CSQC_NZPReadMobileState(&out));
    ResetClient(); client_values[3] = 1025; assert(!CSQC_NZPReadMobileState(&out));
    ResetClient(); wrong_type = client_names[3]; NZP_MobileFindGlobals(); nzp_mobile_qc.updated = 1;
    assert(!CSQC_NZPReadMobileState(&out));
    ResetClient(); missing = client_names[3]; NZP_MobileFindGlobals(); nzp_mobile_qc.updated = 1;
    assert(!CSQC_NZPReadMobileState(&out));
    ResetClient(); missing = client_names[8]; NZP_MobileFindGlobals(); nzp_mobile_qc.updated = 1;
    assert(CSQC_NZPReadMobileState(&out) && out.points == -1 && out.round == -1 && !out.powerups);
    ResetClient(); csqc_nogameaccess = 1; assert(!CSQC_NZPReadMobileState(&out));
    ResetClient(); client_values[1] = NZP_MOBILE_VALID | NZP_MOBILE_AUTOMATIC | NZP_MOBILE_CAN_ADS;
    assert(CSQC_NZPReadMobileState(&out) && out.flags == NZP_MOBILE_VALID);
    ResetClient(); client_values[8] = -1;
    assert(CSQC_NZPReadMobileState(&out) && out.points == -1 && out.round == -1 && !out.powerups);
}
static void ServerTests(void)
{
    edict_t entities[2] = {0};
    missing = wrong_type = NULL; memset(server_values, 0, sizeof(server_values));
    sv.state = ss_active; sv.allocated_client_slots = 2;
    server_vm.GetEdictFieldValue = GetField;
    for (int i = 0; i < 2; ++i) {
        svs.clients[i].state = cs_spawned; svs.clients[i].edict = &entities[i];
        svs.clients[i].netchan.remote_address.local = 1;
    }
    server_values[0] = server_values[7] = 1;
    assert(SV_NZPMobileAutoGuard(7, 1, true));
    assert(server_values[1] == 7 && server_values[2] == 1 && server_values[3] == 0);
    assert(!SV_NZPMobileAutoGuard(0, 2, true) && server_values[3] == 0);
    assert(SV_NZPMobileAutoGuard(0, 1, true) && server_values[3] == 1);
    assert(!SV_NZPMobileAutoGuard(1025, 1, true));
    assert(!SV_NZPMobileAutoGuard(1, 3, true));
    assert(SV_NZPMobileAdsGuard(7, 1, true) && server_values[4] == 7 && server_values[5] == 1);
    assert(!SV_NZPMobileAdsGuard(0, 2, true) && server_values[6] == 0);
    assert(SV_NZPMobileAdsGuard(0, 1, true) && server_values[6] == 1);
    assert(SV_NZPMobileSprintIntent(1, 1) && entities[0].sprint._float == 1);
    assert(SV_NZPMobileSprintIntent(1, 0) && entities[0].sprint._float == 0);
    assert(!SV_NZPMobileSprintIntent(1, 2));
    svs.clients[0].netchan.remote_address.local = 0;
    assert(!SV_NZPMobileAutoGuard(7, 1, true));
    assert(!SV_NZPMobileAdsGuard(7, 1, true));
    assert(!SV_NZPMobileSprintIntent(1, 1));
    svs.clients[0].netchan.remote_address.local = 1;
    svs.clients[0].state = 0; assert(!SV_NZPMobileAutoGuard(7, 1, true));
    svs.clients[0].state = cs_spawned;
    server_values[0] = 2; assert(!SV_NZPMobileAutoGuard(7, 1, true));
    server_values[0] = 1;
    wrong_type = server_names[1]; assert(!SV_NZPMobileAutoGuard(7, 1, true));
    wrong_type = NULL; missing = server_names[4]; assert(!SV_NZPMobileAdsGuard(7, 1, true));
}
int main(void)
{
    ClientTests(); ServerTests();
    puts("QC contract: typed bounded globals, stale/future timestamps, optional HUD fallback, spawned loopback ownership and exact entity release passed");
    return 0;
}
