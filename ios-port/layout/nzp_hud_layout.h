/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef NZP_HUD_LAYOUT_H
#define NZP_HUD_LAYOUT_H
#include <stddef.h>

/* UIKit points, never framebuffer pixels. Origin includes the safe-area inset. */
typedef struct { float x, y; } nzp_point;
typedef struct { float x, y, width, height; } nzp_rect;
typedef struct { float top, left, bottom, right; } nzp_insets;
typedef enum {
    NZP_HUD_PAUSE, NZP_HUD_STANCE, NZP_HUD_WEAPON, NZP_HUD_GRENADE,
    NZP_HUD_BETTY, NZP_HUD_ADS, NZP_HUD_KNIFE, NZP_HUD_JUMP,
    NZP_HUD_USE, NZP_HUD_CONTROL_COUNT
} nzp_hud_control;
typedef struct {
    int valid, has_use;
    float unit, stick_radius, stick_capture_radius;
    nzp_rect safe_bounds, controls[NZP_HUD_CONTROL_COUNT];
    nzp_point stick, points, round;
    nzp_rect powerups[2];
    unsigned powerup_count;
} nzp_hud_layout;

/* Invalid/portrait surfaces fail closed until landscape layout is established. */
int nzp_hud_make_layout(nzp_hud_layout *out, float width, float height,
                       nzp_insets safe, int has_use, unsigned powerup_mask);
int nzp_hud_contains(nzp_rect rect, float x, float y);
/* Buttons take priority over the movement capture circle, then world touch. */
int nzp_hud_hit_control(const nzp_hud_layout *layout, float x, float y);
int nzp_hud_hit_stick(const nzp_hud_layout *layout, float x, float y);
#endif
