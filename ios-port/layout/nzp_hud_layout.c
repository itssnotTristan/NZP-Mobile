/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "nzp_hud_layout.h"
#include <math.h>
#include <string.h>

static nzp_rect centered(float x, float y, float w, float h) {
    nzp_rect r = {x-w/2, y-h/2, w, h}; return r;
}
static int number(float n) { return isfinite(n) && n >= 0 && n <= 100000; }
int nzp_hud_make_layout(nzp_hud_layout *o, float width, float height,
                       nzp_insets safe, int has_use, unsigned mask) {
    float w,h,d,x,y,size,gap;
    unsigned i;
    if (!o) return 0;
    memset(o,0,sizeof(*o));
    if (!number(width)||!number(height)||!number(safe.top)||!number(safe.left)||
        !number(safe.bottom)||!number(safe.right)) return 0;
    w=width-safe.left-safe.right; h=height-safe.top-safe.bottom;
    if (w<=0||h<=0||w<h) return 0;
    d=fminf(1.0f,fminf(w/560.0f,h/320.0f));
    if (d<=0) return 0;
    o->valid=1; o->unit=d;
    o->safe_bounds=(nzp_rect){safe.left,safe.top,w,h};
    x=safe.left; y=safe.top;
    o->stick=(nzp_point){x+78*d,y+h-116*d};
    o->stick_radius=52*d; o->stick_capture_radius=52*d*sqrtf(1.35f);
    o->points=(nzp_point){x+12*d,y+69*d};
    o->round=(nzp_point){x+12*d,y+92*d};
    o->controls[NZP_HUD_PAUSE]=centered(x+34*d,y+32*d,52*d,52*d);
    /* Latest requested change: stance sits directly below the round counter. */
    o->controls[NZP_HUD_STANCE]=centered(x+34*d,y+122*d,44*d,44*d);
    o->controls[NZP_HUD_WEAPON]=centered(x+w-90*d,y+34*d,164*d,56*d);
    o->controls[NZP_HUD_GRENADE]=centered(x+w-94*d,y+106*d,54*d,56*d);
    o->controls[NZP_HUD_BETTY]=centered(x+w-34*d,y+106*d,54*d,56*d);
    o->controls[NZP_HUD_ADS]=centered(x+w-44*d,y+174*d,64*d,64*d);
    o->controls[NZP_HUD_KNIFE]=centered(x+w/2,y+h-31*d,90*d,52*d);
    o->controls[NZP_HUD_JUMP]=centered(x+w-38*d,y+h-34*d,56*d,56*d);
    o->has_use=!!has_use;
    if (o->has_use) o->controls[NZP_HUD_USE]=centered(x+w/2,y+h*.61f,fminf(340*d,w-320*d),56*d);
    mask&=3; o->powerup_count=(mask&1)+((mask>>1)&1);
    size=38*d; gap=6*d;
    x+=w/2-(o->powerup_count*size+(o->powerup_count?o->powerup_count-1:0)*gap)/2;
    for (i=0;i<o->powerup_count;i++) o->powerups[i]=(nzp_rect){x+i*(size+gap),y+8*d,size,size};
    return 1;
}
int nzp_hud_contains(nzp_rect r,float x,float y) {
    return isfinite(x)&&isfinite(y)&&r.width>0&&r.height>0&&
        x>=r.x&&y>=r.y&&x<r.x+r.width&&y<r.y+r.height;
}
int nzp_hud_hit_control(const nzp_hud_layout *o,float x,float y) {
    int i;
    if (!o||!o->valid) return -1;
    for(i=0;i<NZP_HUD_CONTROL_COUNT;i++) {
        if(i==NZP_HUD_USE&&!o->has_use) continue;
        if(nzp_hud_contains(o->controls[i],x,y)) return i;
    }
    return -1;
}
int nzp_hud_hit_stick(const nzp_hud_layout *o,float x,float y) {
    float dx,dy;
    if(!o||!o->valid||!isfinite(x)||!isfinite(y)) return 0;
    dx=x-o->stick.x;dy=y-o->stick.y;
    return dx*dx+dy*dy<o->stick_capture_radius*o->stick_capture_radius;
}
