/* SPDX-License-Identifier: GPL-3.0-or-later
 * Extracted from canonical v4-revised pr_cmds.c. Include in that TU only.
 * Local ownership is checked both here and in the client input gate.
 */
/* NZP Mobile 2026-10-06: fixed typed globals for local held-input safety.
 * No arbitrary VM names, command text, file paths or client permissions.
 * Keep the expected-weapon guard after key-up until QC observes button0 up.
 */
static qboolean SV_NZPMobileInputGuard(int weapon, int entity, qboolean apply, qboolean ads)
{
    static const char *names[] = { "nzp_mobile_guard_version", "nzp_mobile_auto_weapon",
                                  "nzp_mobile_auto_entity", "nzp_mobile_auto_release" };
    static const char *ads_names[] = { "nzp_mobile_guard_version", "nzp_mobile_ads_weapon",
                                      "nzp_mobile_ads_entity", "nzp_mobile_ads_release" };
    float *values[4];
    etype_t type;
    int i;
    if (!svprogfuncs || sv.state != ss_active || entity < 1 ||
        entity > sv.allocated_client_slots || weapon < 0 || weapon > 1024)
        return false;
    /* Same spawned local-client ownership check used by sprint. */
    if (svs.clients[entity-1].state != cs_spawned || !svs.clients[entity-1].edict ||
        !NET_IsLoopBackAddress(&svs.clients[entity-1].netchan.remote_address))
        return false;
    for (i = 0; i < 4; ++i) {
        values[i] = (float *)PR_FindGlobal(svprogfuncs, ads ? ads_names[i] : names[i], 0, &type);
        if (!values[i] || (type & 0xfff) != ev_float) return false;
    }
    if (*values[0] != 1) return false;
    if (apply) {
        if (weapon) {
            *values[1] = weapon;
            *values[2] = entity;
            *values[3] = 0;
        } else {
            if (*values[2] != entity) return false;
            *values[3] = 1;
        }
    }
    return true;
}
qboolean SV_NZPMobileAutoGuard(int weapon, int entity, qboolean apply)
{
    return SV_NZPMobileInputGuard(weapon, entity, apply, false);
}
qboolean SV_NZPMobileAdsGuard(int weapon, int entity, qboolean apply)
{
    return SV_NZPMobileInputGuard(weapon, entity, apply, true);
}
qboolean SV_NZPMobileSprintIntent(int entity, int enabled)
{
    etype_t type;
    float *version;
    eval_t *intent;
    client_t *client;
    if (!svprogfuncs || sv.state != ss_active || entity<1 ||
        entity>sv.allocated_client_slots || (enabled!=0 && enabled!=1)) return false;
    client=&svs.clients[entity-1];
    if (client->state != cs_spawned || !client->edict ||
        !NET_IsLoopBackAddress(&client->netchan.remote_address)) return false;
    version=(float*)PR_FindGlobal(svprogfuncs,"nzp_mobile_sprint_version",0,&type);
    if (!version || (type&0xfff)!=ev_float || *version!=1) return false;
    intent=svprogfuncs->GetEdictFieldValue(svprogfuncs,(edict_t*)client->edict,
                                         "nzp_mobile_sprint_intent",ev_float,NULL);
    if (!intent) return false;
    intent->_float=enabled;
    return true;
}
