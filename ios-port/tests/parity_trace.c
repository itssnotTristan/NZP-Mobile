// SPDX-License-Identifier: GPL-3.0-or-later
/* Deterministic valid-input trace; compare with CanonicalParity.java. */
#include "nzp_touch.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
static uint32_t seed=UINT32_C(0x17ab3491);
static uint64_t trace=UINT64_C(14695981039346656037);
static uint32_t random_below(uint32_t n) { seed=seed*UINT32_C(1664525)+UINT32_C(1013904223); return seed%n; }
static void hash(uint32_t n) { trace^=n; trace*=UINT64_C(1099511628211); }
static void tap(void *c) { (void)c; hash(1); }
static void automatic(void *c,bool on,int weapon) { (void)c; hash(2); hash(on); hash((uint32_t)weapon); }
static void look(void *c,float x,float y) { uint32_t a,b; (void)c; memcpy(&a,&x,sizeof(a)); memcpy(&b,&y,sizeof(b)); hash(3); hash(a); hash(b); }
static void ads(void *c,bool on,int weapon) { (void)c; hash(4); hash(on); hash((uint32_t)weapon); }
static void cancel(void *c) { (void)c; hash(5); }
static void sprint(void *c,bool on) { (void)c; hash(6); hash(on); }
int main(void) {
    NZPWorldInput world; NZPMovementGesture movement; NZPWeaponGesture weapon;
    NZPWorldSink ws={NULL,tap,automatic,look,ads,cancel}; NZPMovementSink ms={NULL,sprint};
    NZPGameState s=nzp_game_state_make(71,2,7,10,50,2,1,"Weapon","Use",0);
    int64_t now=0;
    unsigned i;
    nzp_world_init(&world,ws); nzp_world_set_active(&world,true); nzp_world_update(&world,&s,0);
    nzp_movement_init(&movement,ms); nzp_weapon_begin(&weapon,0,0,0);
    for (i=0;i<50000;++i) {
        int op,id,flags,weapon_id,age,on;
        float x,y,forward,raw;
        now+=random_below(101); op=(int)random_below(20); id=(int)random_below(4);
        x=(float)((int)random_below(81)-40); y=(float)((int)random_below(81)-40);
        flags=(int)random_below(256); weapon_id=(int)random_below(10); age=(int)random_below(2001); on=(int)random_below(2);
        forward=(float)((int)random_below(15)-7)*.25f; raw=(float)((int)random_below(15)-7)*.25f;
        switch(op) {
        case 0: nzp_world_set_active(&world,on!=0); break;
        case 1: s=nzp_game_state_make(flags,2,weapon_id,10,50,2,1,"Weapon","Use",now-age); nzp_world_update(&world,&s,now); break;
        case 2: hash(nzp_world_down(&world,id,x,y,now)); break;
        case 3: nzp_world_move(&world,id,x,y,now); break;
        case 4: nzp_world_up(&world,id,x,y,now); break;
        case 5: nzp_world_tick(&world,now); break;
        case 6: nzp_world_toggle_ads(&world,now); break;
        case 7: nzp_world_cancel(&world); break;
        case 8: nzp_world_end_contacts(&world); break;
        case 9: nzp_world_cancel_all(&world); break;
        case 10: hash(nzp_movement_down(&movement,id,x,y,now)); break;
        case 11: nzp_movement_move(&movement,id,x,y,now); break;
        case 12: nzp_movement_up(&movement,id,x,y,now); break;
        case 13: nzp_movement_analog(&movement,id,forward,raw); break;
        case 14: nzp_movement_cancel(&movement); break;
        case 15: nzp_weapon_begin(&weapon,x,y,now); break;
        case 16: hash((uint32_t)nzp_weapon_move(&weapon,x,y)); break;
        case 17: hash((uint32_t)nzp_weapon_up(&weapon,x,y,now,on!=0)); break;
        case 18: nzp_weapon_cancel(&weapon); break;
        case 19: nzp_world_tick(&world,now); break;
        }
        hash(nzp_world_aim_latched(&world)); hash(nzp_world_auto_firing(&world)); hash(nzp_world_has_contact(&world));
        hash(nzp_movement_sprinting(&movement)); hash(nzp_movement_has_contact(&movement));
    }
    nzp_world_cancel_all(&world); nzp_movement_cancel(&movement);
    printf("%016" PRIx64 "\n",trace);
    return 0;
}
