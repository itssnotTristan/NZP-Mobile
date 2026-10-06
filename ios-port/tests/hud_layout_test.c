/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../layout/nzp_hud_layout.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static int checks;
#define CHECK(x) do { checks++; assert(x); } while(0)
static void surface(float w,float h,nzp_insets safe) {
    nzp_hud_layout l; int i; nzp_rect stance;
    CHECK(nzp_hud_make_layout(&l,w,h,safe,1,3));
    CHECK(l.powerup_count==2);
    CHECK(l.round.y<l.controls[NZP_HUD_STANCE].y);
    for(i=0;i<NZP_HUD_CONTROL_COUNT;i++) {
        nzp_rect r=l.controls[i];
        CHECK(r.x>=l.safe_bounds.x-.01f && r.y>=l.safe_bounds.y-.01f);
        CHECK(r.x+r.width<=l.safe_bounds.x+l.safe_bounds.width+.01f);
        CHECK(r.y+r.height<=l.safe_bounds.y+l.safe_bounds.height+.01f);
        CHECK(nzp_hud_hit_control(&l,r.x+r.width/2,r.y+r.height/2)==i);
    }
    stance=l.controls[NZP_HUD_STANCE];
    {
        float cx=fmaxf(stance.x,fminf(l.stick.x,stance.x+stance.width));
        float cy=fmaxf(stance.y,fminf(l.stick.y,stance.y+stance.height));
        float dx=cx-l.stick.x,dy=cy-l.stick.y;
        CHECK(dx*dx+dy*dy>l.stick_capture_radius*l.stick_capture_radius);
    }
    CHECK(nzp_hud_hit_stick(&l,l.stick.x,l.stick.y));
    CHECK(nzp_hud_hit_control(&l,l.stick.x,l.stick.y)==-1);
    CHECK(!nzp_hud_hit_stick(&l,NAN,0));
    CHECK(nzp_hud_hit_control(&l,NAN,0)==-1);
    CHECK(nzp_hud_make_layout(&l,w,h,safe,0,2));
    CHECK(!l.has_use&&l.powerup_count==1);
    CHECK(l.controls[NZP_HUD_USE].width==0);
}
int main(void) {
    nzp_hud_layout l;
    surface(560,320,(nzp_insets){0,0,0,0});
    surface(667,375,(nzp_insets){0,0,0,0});
    surface(844,390,(nzp_insets){0,47,21,47});
    surface(932,430,(nzp_insets){0,59,21,59});
    surface(1024,768,(nzp_insets){24,0,20,0});
    surface(1366,1024,(nzp_insets){24,0,20,0});
    CHECK(!nzp_hud_make_layout(&l,390,844,(nzp_insets){0},0,0));
    CHECK(!nzp_hud_make_layout(&l,NAN,390,(nzp_insets){0},0,0));
    CHECK(!nzp_hud_make_layout(&l,844,390,(nzp_insets){0,900,0,0},0,0));
    CHECK(!nzp_hud_make_layout(&l,844,390,(nzp_insets){0,-1,0,0},0,0));
    CHECK(!nzp_hud_make_layout(NULL,844,390,(nzp_insets){0},0,0));
    CHECK(!nzp_hud_hit_stick(&l,0,0));
    printf("PASS: %d safe-area HUD layout assertions\n",checks);
    return 0;
}
