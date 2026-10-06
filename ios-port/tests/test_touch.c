// SPDX-License-Identifier: GPL-3.0-or-later
#include "nzp_touch.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(expr, description) do { ++checks; if (!(expr)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, description); exit(1); } } while (0)
#define LIVE (NZP_STATE_VALID | NZP_STATE_ALIVE)
typedef struct {
    int taps, auto_down, auto_up, ads_down, ads_up, cancels, expected, sprint_on, sprint_off;
    float dx, dy;
    char events[1024];
    size_t event_count;
} Fake;
typedef struct { NZPWorldInput input; Fake sink; } Fixture;
static void event(Fake *f, char value) { if (f->event_count + 1 < sizeof(f->events)) { f->events[f->event_count++] = value; f->events[f->event_count] = 0; } }
static void tap_fire(void *p) { Fake *f = p; ++f->taps; event(f, 't'); }
static void auto_fire(void *p, bool on, int expected) { Fake *f = p; if (on) ++f->auto_down; else ++f->auto_up; f->expected = expected; event(f, on ? 'A' : 'a'); }
static void ads(void *p, bool on, int expected) { Fake *f = p; if (on) ++f->ads_down; else ++f->ads_up; f->expected = expected; event(f, on ? 'D' : 'd'); }
static void look(void *p, float x, float y) { Fake *f = p; CHECK(isfinite(x) && isfinite(y), "look callback receives finite deltas"); f->dx += x; f->dy += y; }
static void cancel(void *p) { Fake *f = p; ++f->cancels; event(f, 'c'); }
static void sprint(void *p, bool on) { Fake *f = p; if (on) ++f->sprint_on; else ++f->sprint_off; }
static NZPGameState state(int flags, int weapon, int64_t time) { return nzp_game_state_make(flags, 2, weapon, 10, 50, 2, 1, "Weapon", "Hold to repair", time); }
static void setup(Fixture *f, int flags) {
    NZPWorldSink sink = {&f->sink, tap_fire, auto_fire, look, ads, cancel};
    NZPGameState s = state(flags, 7, 0);
    memset(f, 0, sizeof(*f)); nzp_world_init(&f->input, sink);
    nzp_world_set_active(&f->input, true); nzp_world_update(&f->input, &s, 0);
    f->sink.cancels = 0; f->sink.event_count = 0; f->sink.events[0] = 0;
}
#define D(id,x,y,t) nzp_world_down(&f.input,id,x,y,t)
#define M(id,x,y,t) nzp_world_move(&f.input,id,x,y,t)
#define U(id,x,y,t) nzp_world_up(&f.input,id,x,y,t)
#define T(t) nzp_world_tick(&f.input,t)
static void test_world(void) {
    Fixture f;
    NZPGameState s;
    int cancels;
    setup(&f, LIVE); D(77,0,0,0); U(77,0,0,100);
    CHECK(f.sink.taps==1 && f.sink.auto_down==0,"Semi tap is one ordinary trigger");
    setup(&f, LIVE); D(3,0,0,0); T(320); U(3,0,0,400);
    CHECK(f.sink.taps==1 && f.sink.auto_down==0,"Semi 320-500ms tap grace");
    setup(&f, LIVE); D(3,0,0,0); U(3,0,0,500); CHECK(f.sink.taps==1,"Inclusive 500ms tap");
    setup(&f, LIVE); D(3,0,0,0); T(501); U(3,0,0,600); CHECK(f.sink.taps==0 && f.sink.auto_down==0,"Long semi hold does nothing");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(3,0,0,0); U(3,0,0,319); CHECK(f.sink.taps==1 && f.sink.auto_down==0,"Automatic quick tap");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(999,0,0,0); T(319); CHECK(f.sink.auto_down==0,"No automatic before 320ms");
    T(320); CHECK(f.sink.auto_down==1 && f.sink.expected==7,"320ms hold carries expected weapon");
    M(999,25,8,400); CHECK(f.sink.auto_down==1 && f.sink.auto_up==0 && f.sink.dx==25,"Auto plus drag keeps firing and aims");
    U(999,30,8,500); cancels=f.sink.cancels; nzp_world_end_contacts(&f.input);
    CHECK(f.sink.auto_up==1 && f.sink.taps==0 && f.sink.cancels==cancels,"Normal auto lift preserves earned native pulse");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(4,0,0,0); M(4,9,0,100); T(800); U(4,9,0,900);
    CHECK(f.sink.taps==0 && f.sink.auto_down==0 && f.sink.dx==9,"Early drag disarms fire permanently");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(4,0,0,0); M(4,8,0,100); T(320); CHECK(f.sink.auto_down==1,"Exactly 8 points remains stationary");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(4,0,0,0); M(4,0,8.01f,100); T(320); CHECK(f.sink.auto_down==0,"Above 8 points is drag");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(4,0,0,0); M(4,25,0,400); CHECK(f.sink.auto_down==1 && f.sink.dx==25,"Delayed timer retains earned stationary hold");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(4,0,0,0); U(4,40,0,400); CHECK(f.sink.auto_down==0 && f.sink.taps==0,"Displaced UP-only never earns auto or tap");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(4,0,0,0); nzp_world_cancel_all(&f.input); T(400); U(4,0,0,500);
    CHECK(f.sink.auto_down==0 && f.sink.taps==0,"Delayed timer after cancel cannot fire");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(4,0,0,0); T(320); nzp_world_cancel(&f.input);
    CHECK(f.sink.auto_up==1 && f.sink.cancels>0,"Cancellation drops pending pulse then releases");
    CHECK(!strcmp(f.sink.events,"Aca"),"Native cancellation precedes auto release");
    T(600); U(4,0,0,700); CHECK(f.sink.auto_down==1 && f.sink.taps==0,"Cancelled contact cannot rearm");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(4,0,0,0); T(320); s=state(LIVE,8,400); nzp_world_update(&f.input,&s,400);
    CHECK(f.sink.auto_up==1,"Weapon change releases guarded fire"); M(4,30,0,420); CHECK(f.sink.dx==30,"Invalidated fire still aims");
    U(4,30,0,450); CHECK(f.sink.taps==0,"Weapon switch cannot become late semi shot"); D(9,0,0,500); U(9,0,0,600); CHECK(f.sink.taps==1,"Fresh semi contact restores tap");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(4,0,0,0); s=state(LIVE,7,200); nzp_world_update(&f.input,&s,200); T(400); CHECK(f.sink.auto_down==0,"Mode change requires fresh contact");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(4,0,0,0); T(320); T(1501); CHECK(f.sink.auto_up==1 && !nzp_world_auto_firing(&f.input),"Stale snapshot releases auto");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(4,0,0,0); T(320); s=state(NZP_STATE_VALID,7,400); nzp_world_update(&f.input,&s,400); CHECK(f.sink.auto_up==1,"Death releases auto");
    setup(&f, LIVE|NZP_STATE_AUTOMATIC); D(4,0,0,0); CHECK(!D(5,20,20,10),"Second world pointer cannot steal owner"); U(5,20,20,100); T(320); CHECK(f.sink.auto_down==1,"Wrong pointer cannot release owner");
    setup(&f, LIVE|NZP_STATE_CAN_ADS); nzp_world_toggle_ads(&f.input,0); nzp_world_end_contacts(&f.input); CHECK(nzp_world_aim_latched(&f.input) && f.sink.ads_down==1 && f.sink.expected==7,"Toggled ADS survives ordinary lifts");
    nzp_world_toggle_ads(&f.input,10); CHECK(!nzp_world_aim_latched(&f.input) && f.sink.ads_up==1,"Second ADS tap toggles off");
    setup(&f, LIVE|NZP_STATE_CAN_ADS); nzp_world_toggle_ads(&f.input,0); s=state(LIVE|NZP_STATE_DUAL,8,50); nzp_world_update(&f.input,&s,50);
    CHECK(!nzp_world_aim_latched(&f.input) && f.sink.ads_up==1 && f.sink.cancels>0,"Dual switch cancels ADS"); nzp_world_toggle_ads(&f.input,60); CHECK(f.sink.ads_down==1,"Dual weapon never latches second trigger as ADS");
    setup(&f, LIVE|NZP_STATE_CAN_ADS); nzp_world_toggle_ads(&f.input,0); T(1501); CHECK(f.sink.ads_up==1 && !nzp_world_aim_latched(&f.input),"Stale ADS unlatches");
    setup(&f,0); D(1,0,0,0); U(1,0,0,100); CHECK(f.sink.taps==1,"Unknown remote permits ordinary tap"); D(2,0,0,200); T(600); U(2,0,0,800); CHECK(f.sink.auto_down==0 && f.sink.taps==1,"Unknown remote grants no hold"); nzp_world_toggle_ads(&f.input,900); CHECK(f.sink.ads_down==0,"Unknown remote grants no ADS");
    setup(&f,LIVE|NZP_STATE_CAN_ADS|NZP_STATE_AUTOMATIC); nzp_world_toggle_ads(&f.input,0); D(3,0,0,0); T(320); nzp_world_set_active(&f.input,false);
    CHECK(f.sink.auto_up==1 && f.sink.ads_up==1,"Menu or lifecycle releases both held actions"); CHECK(!D(3,0,0,400),"Inactive surface rejects contacts");
    setup(&f,NZP_STATE_VALID); D(3,0,0,0); U(3,0,0,100); CHECK(f.sink.taps==0,"Known dead state cannot tap");
    setup(&f,LIVE); T(1501); D(3,0,0,1501); U(3,0,0,1550); CHECK(f.sink.taps==0,"Known stale state cannot tap");
    setup(&f,LIVE); D(3,0,0,0); M(3,20,0,50); M(3,0,0,60); U(3,0,0,100); CHECK(f.sink.taps==0,"Returning drag to origin is not tap");
}
static void test_weapon(void) {
    NZPWeaponGesture w;
    nzp_weapon_begin(&w,0,0,0); CHECK(nzp_weapon_up(&w,0,0,100,true)==NZP_WEAPON_RELOAD,"Weapon tap reloads"); CHECK(nzp_weapon_up(&w,0,0,200,true)==NZP_WEAPON_NONE,"Weapon action only once");
    nzp_weapon_begin(&w,0,0,0); CHECK(nzp_weapon_up(&w,30,0,100,true)==NZP_WEAPON_SWITCH,"UP-only horizontal swipe switches");
    nzp_weapon_begin(&w,0,0,0); CHECK(nzp_weapon_move(&w,-24,0)==NZP_WEAPON_SWITCH,"Both directions at 24 point boundary"); CHECK(nzp_weapon_move(&w,-60,0)==NZP_WEAPON_NONE && nzp_weapon_up(&w,-60,0,200,false)==NZP_WEAPON_NONE,"Swipe switches once with no reload");
    nzp_weapon_begin(&w,0,0,0); CHECK(nzp_weapon_up(&w,0,30,100,true)==NZP_WEAPON_NONE,"Vertical gun drag does nothing");
    nzp_weapon_begin(&w,0,0,0); nzp_weapon_cancel(&w); CHECK(nzp_weapon_up(&w,0,0,100,true)==NZP_WEAPON_NONE && nzp_weapon_move(&w,50,0)==NZP_WEAPON_NONE,"Cancelled widget cannot act");
    nzp_weapon_begin(&w,0,0,0); CHECK(nzp_weapon_up(&w,8,0,500,true)==NZP_WEAPON_RELOAD,"Reload includes 8 points / 500ms");
    nzp_weapon_begin(&w,0,0,0); CHECK(nzp_weapon_up(&w,8.1f,0,100,true)==NZP_WEAPON_NONE,"Over-slop reload disarmed");
    nzp_weapon_begin(&w,0,0,0); CHECK(nzp_weapon_up(&w,0,0,501,true)==NZP_WEAPON_NONE,"Long gun hold does not reload");
    nzp_weapon_begin(&w,0,0,0); CHECK(nzp_weapon_up(&w,0,0,100,false)==NZP_WEAPON_NONE,"Outside release does not reload");
    nzp_weapon_begin(&w,0,0,0); CHECK(nzp_weapon_up(&w,25,20,100,true)==NZP_WEAPON_NONE,"Exactly 1.25 directional ratio is excluded");
    nzp_weapon_begin(&w,0,0,0); CHECK(nzp_weapon_up(&w,25.01f,20,100,true)==NZP_WEAPON_SWITCH,"Above directional ratio switches");
}
static void movement_setup(NZPMovementGesture *m, Fake *f) { NZPMovementSink sink={f,sprint}; memset(f,0,sizeof(*f)); nzp_movement_init(m,sink); }
static void movement_tap(NZPMovementGesture *m,int64_t id,float x,float y,int64_t down,int64_t up) { nzp_movement_down(m,id,x,y,down); nzp_movement_up(m,id,x,y,up); }
static void test_movement(void) {
    Fake f; NZPMovementGesture m;
    movement_setup(&m,&f); movement_tap(&m,1,0,0,0,100); nzp_movement_down(&m,9,0,0,200); CHECK(f.sprint_on==0,"Neutral doubletap only arms");
    nzp_movement_analog(&m,9,0,0); nzp_movement_analog(&m,9,-1,-1); CHECK(f.sprint_on==0,"Neutral and backwards never start sprint"); nzp_movement_analog(&m,9,.249f,.249f); CHECK(f.sprint_on==0,"Forward threshold not reached");
    nzp_movement_analog(&m,9,.25f,.25f); CHECK(f.sprint_on==1,"Inclusive forward .25 starts doubletap sprint"); nzp_movement_analog(&m,9,1,1); CHECK(f.sprint_on==1,"Held sprint sends no repeats"); nzp_movement_up(&m,9,0,-50,400); CHECK(f.sprint_off==1,"Owner lift releases sprint");
    movement_setup(&m,&f); movement_tap(&m,1,0,0,0,220); nzp_movement_down(&m,2,48,0,500); nzp_movement_analog(&m,2,1,1); CHECK(f.sprint_on==1,"220ms first tap,280ms gap,48 point boundaries"); nzp_movement_cancel(&m); CHECK(f.sprint_off==1,"Cancel releases sprint"); nzp_movement_down(&m,3,48,0,550); nzp_movement_analog(&m,3,1,1); CHECK(f.sprint_on==1,"Cancel clears doubletap history");
    movement_setup(&m,&f); movement_tap(&m,1,0,0,0,221); nzp_movement_down(&m,2,0,0,300); nzp_movement_analog(&m,2,1,1); CHECK(f.sprint_on==0,"Long first tap cannot arm");
    movement_setup(&m,&f); movement_tap(&m,1,0,0,0,100); nzp_movement_down(&m,2,0,0,381); nzp_movement_analog(&m,2,1,1); CHECK(f.sprint_on==0,"Expired gap cannot arm");
    movement_setup(&m,&f); movement_tap(&m,1,0,0,0,100); nzp_movement_down(&m,2,48.1f,0,200); nzp_movement_analog(&m,2,1,1); CHECK(f.sprint_on==0,"Distant second tap cannot arm");
    movement_setup(&m,&f); nzp_movement_down(&m,1,0,0,0); nzp_movement_move(&m,1,12.1f,0,20); nzp_movement_up(&m,1,0,0,50); nzp_movement_down(&m,2,0,0,100); nzp_movement_analog(&m,2,1,1); CHECK(f.sprint_on==0,"First drag returning to origin is no tap");
    movement_setup(&m,&f); nzp_movement_down(&m,1,0,0,0); CHECK(!nzp_movement_down(&m,2,0,0,50),"Overlapping movement pointer rejected"); nzp_movement_up(&m,2,0,0,100); nzp_movement_analog(&m,2,1,2); CHECK(f.sprint_on==0 && nzp_movement_has_contact(&m),"Wrong pointer cannot release or sprint");
    nzp_movement_up(&m,1,0,0,150); nzp_movement_down(&m,3,0,0,200); nzp_movement_analog(&m,3,1,1); CHECK(f.sprint_on==1,"Fresh second pointer can use first tap"); nzp_movement_cancel(&m); nzp_movement_cancel(&m); CHECK(f.sprint_off==1,"Cancellation releases only once");
    movement_setup(&m,&f); movement_tap(&m,9999,0,0,0,100); nzp_movement_down(&m,INT64_MAX,0,0,200); nzp_movement_analog(&m,INT64_MAX,1,1); CHECK(f.sprint_on==1,"Sparse large IDs work"); nzp_movement_up(&m,9999,0,0,300); CHECK(f.sprint_off==0,"Old pointer cannot lift new owner"); nzp_movement_cancel(&m); CHECK(f.sprint_off==1,"Lifecycle releases sparse owner"); CHECK(!nzp_movement_down(&m,-1,0,0,400),"Negative pointer rejected");
    movement_setup(&m,&f); nzp_movement_down(&m,5,0,0,0); nzp_movement_analog(&m,5,1,1); CHECK(f.sprint_on==0,"Exactly on ring walks"); nzp_movement_analog(&m,5,1,1.001f); CHECK(f.sprint_on==1,"Outside ring starts sprint");
    nzp_movement_analog(&m,5,.95f,.95f); CHECK(f.sprint_off==0,"Ring jitter does not flicker"); nzp_movement_analog(&m,5,.85f,.85f); CHECK(f.sprint_off==0,"Inclusive .85 exit boundary"); nzp_movement_analog(&m,5,.849f,.849f); CHECK(f.sprint_off==1,"Below .85 releases ring sprint");
    nzp_movement_analog(&m,5,1,1.2f); CHECK(f.sprint_on==2,"Fresh outward crossing re-arms"); nzp_movement_up(&m,5,0,0,300); CHECK(f.sprint_off==2,"Lift releases ring sprint");
    movement_setup(&m,&f); nzp_movement_down(&m,1,0,0,0); nzp_movement_analog(&m,1,.249f,1.2f); CHECK(f.sprint_on==0,"Mostly strafe cannot enter ring sprint"); nzp_movement_analog(&m,1,.25f,1.2f); CHECK(f.sprint_on==1,"Ring forward entry .25 inclusive");
    nzp_movement_analog(&m,1,.20f,1.2f); CHECK(f.sprint_off==0,"Forward exit .20 inclusive"); nzp_movement_analog(&m,1,.199f,1.2f); CHECK(f.sprint_off==1,"Mostly strafe releases ring sprint"); nzp_movement_analog(&m,1,-1,-1.5f); CHECK(f.sprint_on==1,"Backwards never ring sprints");
    movement_setup(&m,&f); movement_tap(&m,1,0,0,0,100); nzp_movement_down(&m,2,0,0,200); nzp_movement_analog(&m,2,.25f,.25f); nzp_movement_analog(&m,2,1,1.2f); nzp_movement_analog(&m,2,.3f,.3f); CHECK(f.sprint_off==0,"Returning inside preserves doubletap sprint"); nzp_movement_up(&m,2,0,0,400); CHECK(f.sprint_off==1,"Doubletap sprint ends on lift");
    movement_setup(&m,&f); nzp_movement_down(&m,1,0,0,0); nzp_movement_analog(&m,1,1,1.1f); nzp_movement_analog(&m,1,.8f,.8f); nzp_movement_up(&m,1,0,0,100); nzp_movement_down(&m,2,0,0,200); nzp_movement_analog(&m,2,.5f,.5f); CHECK(f.sprint_on==1,"Ring sprint cannot seed doubletap");
    movement_setup(&m,&f); nzp_movement_down(&m,1,0,0,0); nzp_movement_analog(&m,1,1,NAN); CHECK(f.sprint_on==0 && !nzp_movement_has_contact(&m),"Nonfinite raw direction cancels"); nzp_movement_down(&m,1,0,0,100); nzp_movement_analog(&m,1,1,1.1f); nzp_movement_analog(&m,1,INFINITY,1.1f); CHECK(f.sprint_off==1,"Nonfinite normalized direction releases active sprint");
}
static void test_snapshots(void) {
    NZPGameState s=state(LIVE|NZP_STATE_CAN_USE,7,0), other;
    NZPHudState h, h2;
    char long_text[300];
    CHECK(nzp_game_state_can_use(&s,1500) && !nzp_game_state_can_use(&s,1501),"Use prompt expires at 1500ms");
    CHECK(!strcmp(nzp_game_state_stance_label(&s,0),"Standing"),"Stance 2 is standing"); s.stance=1; CHECK(!strcmp(nzp_game_state_stance_label(&s,0),"Crouched"),"Stance 1 is crouched"); s.stance=0; CHECK(!strcmp(nzp_game_state_stance_label(&s,0),"Prone"),"Stance 0 is prone"); CHECK(!strcmp(nzp_game_state_stance_label(&s,1501),"Stance"),"Stale stance not displayed");
    s=nzp_game_state_make(LIVE|NZP_STATE_CAN_USE,2,7,-1,-2,-3,-4," ^1A\nB\177 ^9 ","^2  ",-1);
    CHECK(s.magazine==0 && s.reserve==0 && s.grenades==0 && s.tactical==0,"Negative inventory clamped to zero"); CHECK(!strcmp(s.weapon_name,"A B") && !s.use_prompt[0],"Quake color and control cleanup"); CHECK(!strcmp(s.ammo_text,"0 / 0"),"Ammo display text"); CHECK(!nzp_game_state_valid(&s,0),"Negative received time invalid");
    memset(long_text,'x',sizeof(long_text)-1); long_text[sizeof(long_text)-1]=0;
    s=nzp_game_state_make(LIVE,2,7,INT_MAX,INT_MAX,INT_MAX,INT_MAX,long_text,long_text,0);
    CHECK(strlen(s.weapon_name)==48 && strlen(s.use_prompt)==120,"Display strings bounded"); CHECK(!strcmp(s.ammo_text,"2147483647 / 2147483647"),"Maximum integer ammo text fits");
    s=nzp_game_state_make(LIVE,2,7,1,2,3,4,"\xf0\x9f\xa7\x9f Caf\xc3\xa9","\xe2\x82",0); CHECK(!strcmp(s.weapon_name,"\xf0\x9f\xa7\x9f Caf\xc3\xa9"),"UTF-8 names preserved"); CHECK(!strcmp(s.use_prompt,"??"),"Truncated UTF-8 made safe");
    other=s; other.received_at_ms=50; CHECK(nzp_game_state_same_display(&s,&other),"Game heartbeat avoids redraw"); other.magazine++; CHECK(!nzp_game_state_same_display(&s,&other),"Ammo change redraws");
    s=state(LIVE|NZP_STATE_CAN_ADS|NZP_STATE_DUAL,7,0); CHECK(!nzp_game_state_can_ads(&s,0),"Dual vetoes CAN_ADS flag");
    s=state(LIVE|NZP_STATE_CAN_TACTICAL|NZP_STATE_ADS,7,0); CHECK(nzp_game_state_can_tactical(&s,0) && nzp_game_state_ads(&s,0),"Live tactical and ADS flags");
    h=nzp_hud_state_make(940,3,3,100); CHECK(nzp_hud_state_valid(&h,1600) && !nzp_hud_state_valid(&h,1601),"HUD 1500ms expiry"); CHECK(!strcmp(h.points_text,"940") && !strcmp(h.round_text,"ROUND 3"),"HUD display text");
    h2=nzp_hud_state_unknown(); CHECK(!nzp_hud_state_valid(&h2,0),"Unknown HUD hidden"); h2=nzp_hud_state_make(10,-1,0,0); CHECK(nzp_hud_state_valid(&h2,0),"Points-only HUD"); h2=nzp_hud_state_make(-1,5,0,0); CHECK(nzp_hud_state_valid(&h2,0),"Round-only HUD"); h2=nzp_hud_state_make(-1,-1,2,0); CHECK(nzp_hud_state_valid(&h2,0),"Powerup-only HUD"); h2=nzp_hud_state_make(-1,-1,0,0); CHECK(!nzp_hud_state_valid(&h2,0),"Explicit hide all HUD");
    h2=nzp_hud_state_make(1,1,127,0); CHECK(h2.powerups==3,"Unknown positive bits ignored"); h2=nzp_hud_state_make(1,1,-1,0); CHECK(h2.powerups==0,"Negative mask does not invent powerups"); h2=nzp_hud_state_make(940,3,3,500); CHECK(nzp_hud_state_same_display(&h,&h2),"HUD heartbeat avoids redraw"); h2.points=950; CHECK(!nzp_hud_state_same_display(&h,&h2),"Points change redraws");
}
static void test_invalid_events(void) {
    Fixture f; Fake fake; NZPMovementGesture m; NZPWeaponGesture w; NZPGameState s; NZPHudState h;
    float x,y;
    setup(&f,LIVE|NZP_STATE_AUTOMATIC|NZP_STATE_CAN_ADS); CHECK(!D(-1,0,0,0),"Negative world pointer rejected"); CHECK(!D(1,NAN,0,0),"NaN down rejected"); CHECK(!D(1,0,0,-1),"Negative time rejected");
    D(1,0,0,0); T(320); M(1,NAN,0,400); CHECK(f.sink.auto_up==1 && !nzp_world_has_contact(&f.input),"Invalid motion releases fire and owner");
    setup(&f,LIVE|NZP_STATE_AUTOMATIC|NZP_STATE_CAN_ADS); nzp_world_toggle_ads(&f.input,0); D(1,0,0,0); T(320); U(1,INFINITY,0,400); CHECK(f.sink.auto_up==1 && f.sink.ads_up==1 && !nzp_world_has_contact(&f.input),"Invalid UP releases every held action");
    setup(&f,LIVE|NZP_STATE_AUTOMATIC); D(1,0,0,100); U(1,0,0,99); CHECK(f.sink.taps==0 && !nzp_world_has_contact(&f.input),"Backward UP cannot become tap");
    setup(&f,LIVE|NZP_STATE_AUTOMATIC); D(1,0,0,0); T(320); T(319); CHECK(f.sink.auto_up==1 && !nzp_world_has_contact(&f.input),"Backward tick releases auto"); T(INT64_MIN); CHECK(f.sink.auto_up==1,"INT64_MIN time cannot overflow");
    setup(&f,0); D(INT64_MAX,0,0,INT64_MAX-500); U(INT64_MAX,0,0,INT64_MAX); CHECK(f.sink.taps==1,"Maximum timestamp and pointer remain safe");
    setup(&f,LIVE); D(1,FLT_MAX,0,0); M(1,-FLT_MAX,0,100); CHECK(!nzp_world_has_contact(&f.input),"Overflowing delta cancels instead of emitting infinity");
    setup(&f,LIVE); D(1,0,0,10); M(2,NAN,INFINITY,-1); U(1,0,0,100); CHECK(f.sink.taps==1,"Malformed unrelated pointer cannot cancel owner");
    s=state(LIVE|NZP_STATE_AUTOMATIC,7,100); CHECK(!nzp_game_state_valid(&s,99),"Future gameplay snapshot fails closed"); CHECK(!nzp_game_state_valid(&s,INT64_MIN),"Negative extreme snapshot age safe"); s.received_at_ms=0; CHECK(!nzp_game_state_valid(&s,INT64_MAX),"Huge snapshot age expires safely");
    h=nzp_hud_state_make(1,1,3,100); CHECK(!nzp_hud_state_valid(&h,99),"Future HUD snapshot hidden");
    nzp_weapon_begin(&w,NAN,0,0); CHECK(nzp_weapon_up(&w,0,0,100,true)==NZP_WEAPON_NONE,"Invalid weapon start cannot act"); nzp_weapon_begin(&w,0,0,100); CHECK(nzp_weapon_up(&w,40,0,99,true)==NZP_WEAPON_NONE,"Backward weapon time cannot swipe"); nzp_weapon_begin(&w,0,0,0); CHECK(nzp_weapon_move(&w,INFINITY,0)==NZP_WEAPON_NONE && nzp_weapon_up(&w,0,0,100,true)==NZP_WEAPON_NONE,"Invalid widget motion permanently cancels");
    movement_setup(&m,&fake); CHECK(!nzp_movement_down(&m,1,NAN,0,0),"Invalid movement down rejected"); nzp_movement_down(&m,1,0,0,100); nzp_movement_analog(&m,1,1,1.1f); nzp_movement_move(&m,1,0,0,99); CHECK(fake.sprint_off==1 && !nzp_movement_has_contact(&m),"Backward movement time cancels sprint");
    movement_setup(&m,&fake); nzp_movement_down(&m,1,0,0,0); nzp_movement_analog(&m,1,1,1.1f); nzp_movement_up(&m,1,NAN,0,100); CHECK(fake.sprint_off==1 && !nzp_movement_has_contact(&m),"Invalid movement UP cancels sprint"); nzp_movement_analog(&m,-1,1,2); CHECK(fake.sprint_on==1,"Sentinel cannot sprint without owner");
    CHECK(nzp_analog_normalize(0,.13f,&x,&y) && x==0 && y==0,"Inclusive analog deadzone"); CHECK(nzp_analog_normalize(0,1,&x,&y) && x==0 && y==1,"Full stick normalization"); CHECK(nzp_analog_normalize(1,1,&x,&y) && fabsf(x-.70710678f)<.00001f && x==y,"Diagonal circular normalization");
    CHECK(nzp_analog_normalize(FLT_MAX,FLT_MAX,&x,&y) && isfinite(x) && isfinite(y),"Extreme finite analog cannot overflow"); CHECK(!nzp_analog_normalize(NAN,0,&x,&y) && x==0 && y==0,"Invalid analog is zeroed");
    { NZPWorldSink empty={0}; NZPMovementSink em={0}; nzp_world_init(&f.input,empty); nzp_world_set_active(&f.input,true); D(1,0,0,0); U(1,0,0,100); nzp_world_cancel_all(&f.input); nzp_movement_init(&m,em); nzp_movement_down(&m,1,0,0,0); nzp_movement_analog(&m,1,1,2); nzp_movement_cancel(&m); CHECK(true,"Null callback sinks are supported"); }
}
int main(void) { test_world(); test_weapon(); test_movement(); test_snapshots(); test_invalid_events(); printf("PASS: %u C99 touch, ADS, weapon, sprint, telemetry and malformed-input assertions\n",checks); return 0; }
