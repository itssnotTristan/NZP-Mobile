/* SPDX-License-Identifier: GPL-3.0-or-later
 * Native UIKit multi-touch overlay. All callbacks run on the SDL/UIKit main
 * thread. The C engine bridge is the sole owner of engine/QC commands.
 */
#import <UIKit/UIKit.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "NZPTouchOverlay.h"
#include "../../core/nzp_touch.h"
#include "../../layout/nzp_hud_layout.h"
#include "../../bridge/nzp_ios_engine.h"

enum { CONTACT_INERT, CONTACT_MOVE, CONTACT_WORLD, CONTACT_BUTTON };
enum { KEY_ENTER=13, KEY_ESCAPE=27, KEY_SPACE=32, KEY_UP=132,
       KEY_DOWN=133, KEY_LEFT=134, KEY_RIGHT=135, KEY_FIRE=178 };
enum { MENU_UP=NZP_HUD_CONTROL_COUNT, MENU_DOWN, MENU_LEFT, MENU_RIGHT,
       MENU_ACCEPT, MENU_BACK, MENU_TYPE, CONTROL_COUNT };

@interface NZPContact : NSObject {
@public
    int64_t identifier, beganAt, nextRepeat;
    NSInteger role, control;
    CGPoint began, last;
    BOOL moved, earlyWorldDrag;
    NSTimeInterval beganTouchTime;
    int weapon;
    NSString *usePrompt;
    NZPWeaponGesture weaponGesture;
}
@end
@implementation NZPContact
@end

static void Fire(void *context) { (void)context; NZP_IOS_TapKey(KEY_FIRE); }
static void Auto(void *context,bool enabled,int weapon) { (void)context; NZP_IOS_AutoFire(weapon,enabled); }
static void Look(void *context,float dx,float dy) { (void)context; NZP_IOS_AddLook(dx*6.0f,dy*6.0f); }
static void Aim(void *context,bool enabled,int weapon) { (void)context; NZP_IOS_ADS(weapon,enabled); }
static void Cancel(void *context) { (void)context; NZP_IOS_CancelActions(); }
static void Sprint(void *context,bool enabled) { (void)context; NZP_IOS_Sprint(enabled); }
static NSString *Text(const char *value) { return value ? [NSString stringWithUTF8String:value] ?: @"" : @""; }
static CGRect NZPUIKitRect(nzp_rect r) { return CGRectMake(r.x,r.y,r.width,r.height); }
static BOOL ValidPoint(CGPoint p) {
    /* CGFloat is double on iOS; the portable core deliberately uses floats. */
    return isfinite(p.x)&&isfinite(p.y)&&fabs(p.x)<=FLT_MAX&&fabs(p.y)<=FLT_MAX;
}

@interface NZPTouchOverlay : UIView {
    NZPWorldInput _world;
    NZPMovementGesture _movement;
    NZPGameState _state;
    NZPHudState _hud;
    nzp_hud_layout _layout;
    CGRect _buttons[CONTROL_COUNT];
    unsigned _heldKeys[256];
    NSMapTable<UITouch *,NZPContact *> *_contacts;
    int64_t _nextID, _moveID;
    BOOL _playing, _active, _awaitingFrame;
    float _stickDX, _stickDY;
    UIImage *_doublePoints, *_instaKill;
}
- (void)frameReady;
- (void)setApplicationActive:(BOOL)active;
- (void)cancelAll;
- (void)updateHUDReadiness;
@end

@implementation NZPTouchOverlay
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])) {
        self.backgroundColor=UIColor.clearColor; self.opaque=NO;
        self.multipleTouchEnabled=YES;
        self.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        self.accessibilityLabel=@"NZP game controls";
        _contacts=[NSMapTable strongToStrongObjectsMapTable]; _moveID=-1; _active=YES; _awaitingFrame=YES;
        _state=nzp_game_state_unknown(); _hud=nzp_hud_state_unknown();
        nzp_world_init(&_world,(NZPWorldSink){NULL,Fire,Auto,Look,Aim,Cancel});
        nzp_movement_init(&_movement,(NZPMovementSink){NULL,Sprint});
        _doublePoints=[UIImage imageNamed:@"double-points.png"];
        _instaKill=[UIImage imageNamed:@"insta-kill.png"];
    }
    return self;
}
- (void)updateHUDReadiness {
    NZP_IOS_SetHUDReady(_layout.valid&&_active&&!_awaitingFrame&&self.window!=nil&&self.superview!=nil&&!self.hidden&&self.alpha>0.01f);
}
- (void)setApplicationActive:(BOOL)active {
    if(!active) { [self cancelAll]; _awaitingFrame=YES; }
    _active=active;
    /* Resume must receive an engine publication before accepting fresh input. */
    nzp_world_set_active(&_world,_active&&!_awaitingFrame&&_playing);
    [self updateHUDReadiness];
    [self setNeedsDisplay];
}
- (void)layoutSubviews {
    [super layoutSubviews]; [self cancelAll]; [self rebuildGeometry];
}
- (void)safeAreaInsetsDidChange {
    [super safeAreaInsetsDidChange]; [self cancelAll]; [self rebuildGeometry];
}
- (void)rebuildGeometry {
    UIEdgeInsets s=self.safeAreaInsets;
    nzp_hud_make_layout(&_layout,self.bounds.size.width,self.bounds.size.height,
        (nzp_insets){s.top,s.left,s.bottom,s.right},
        _playing&&nzp_game_state_can_use(&_state,NZP_IOS_Milliseconds()),
        (unsigned)_hud.powerups);
    memset(_buttons,0,sizeof(_buttons));
    [self updateHUDReadiness];
    if(!_layout.valid) { [self cancelAll]; [self setNeedsDisplay]; return; }
    if(_playing) {
        for(int i=0;i<NZP_HUD_CONTROL_COUNT;i++) _buttons[i]=NZPUIKitRect(_layout.controls[i]);
    } else {
        float d=_layout.unit,x=_layout.safe_bounds.x,y=_layout.safe_bounds.y;
        float w=_layout.safe_bounds.width,h=_layout.safe_bounds.height;
#define MB(i,cx,cy,ww,hh) _buttons[i]=CGRectMake(x+(cx)-(ww)/2,y+(cy)-(hh)/2,ww,hh)
        MB(MENU_UP,90*d,h-149*d,52*d,52*d); MB(MENU_DOWN,90*d,h-35*d,52*d,52*d);
        MB(MENU_LEFT,33*d,h-92*d,52*d,52*d); MB(MENU_RIGHT,147*d,h-92*d,52*d,52*d);
        MB(MENU_ACCEPT,w-66*d,h-98*d,68*d,68*d); MB(MENU_BACK,w-146*d,h-39*d,58*d,58*d);
        MB(MENU_TYPE,35*d,34*d,52*d,52*d);
#undef MB
    }
    [self setNeedsDisplay];
}
- (void)frameReady {
    int inGame=0; NZPGameState next; NZPHudState hud;
    NZP_IOS_ReadState(&next,&hud,&inGame);
    int64_t now=NZP_IOS_Milliseconds();
    _awaitingFrame=!_active;
    BOOL geometryChanged=(_playing!=!!inGame);
    BOOL nextUse=nzp_game_state_can_use(&next,now);
    if(inGame && !nzp_game_state_live(&next,now)&&((next.flags|_state.flags)&NZP_STATE_VALID)) [self cancelAll];
    if(_playing!=!!inGame) { [self cancelAll]; _playing=!!inGame; }
    if(!nextUse||strcmp(_state.use_prompt,next.use_prompt)) {
        for(NZPContact *c in _contacts.objectEnumerator) if(c->control==NZP_HUD_USE&&c->role==CONTACT_BUTTON) {
            [self holdKey:[self keyForControl:c->control] down:NO]; c->role=CONTACT_INERT;
        }
    }
    if(_layout.has_use!=(_playing&&nextUse)||_hud.powerups!=hud.powerups) geometryChanged=YES;
    _state=next; _hud=hud;
    nzp_world_set_active(&_world,_playing&&_active&&!_awaitingFrame);
    nzp_world_update(&_world,&_state,now); nzp_world_tick(&_world,now);
    if(geometryChanged) [self rebuildGeometry];
    [self updateHUDReadiness];
    if(!_playing) for(NZPContact *c in _contacts.objectEnumerator) {
        if(c->role==CONTACT_BUTTON&&c->control>=MENU_UP&&c->control<=MENU_RIGHT&&now>=c->nextRepeat) {
            NZP_IOS_TapKey([self keyForControl:c->control]); c->nextRepeat=now+170;
        }
    }
    [self setNeedsDisplay];
}
- (int)keyForControl:(NSInteger)i {
    switch(i) {
        case NZP_HUD_PAUSE: case MENU_BACK:return KEY_ESCAPE;
        case NZP_HUD_STANCE:return 'z'; case NZP_HUD_WEAPON:return 'r';
        case NZP_HUD_GRENADE:return 'g';case NZP_HUD_BETTY:return '4';
        case NZP_HUD_KNIFE:return 'v';case NZP_HUD_JUMP:return KEY_SPACE;
        case NZP_HUD_USE:return 'e';case MENU_UP:return KEY_UP;case MENU_DOWN:return KEY_DOWN;
        case MENU_LEFT:return KEY_LEFT;case MENU_RIGHT:return KEY_RIGHT;case MENU_ACCEPT:return KEY_ENTER;
        default:return 0;
    }
}
- (BOOL)enabled:(NSInteger)i at:(int64_t)now {
    if(!_active||_awaitingFrame) return NO;
    if(!_playing||i==NZP_HUD_PAUSE) return YES;
    if((_state.flags&NZP_STATE_VALID)&&!nzp_game_state_live(&_state,now)) return NO;
    if(i==NZP_HUD_ADS) return nzp_game_state_can_ads(&_state,now);
    if(i==NZP_HUD_USE) return nzp_game_state_can_use(&_state,now);
    if(i==NZP_HUD_BETTY) return !(_state.flags&NZP_STATE_VALID)||(nzp_game_state_can_tactical(&_state,now)&&_state.tactical>0);
    if(i==NZP_HUD_GRENADE) return !(_state.flags&NZP_STATE_VALID)||_state.grenades>0;
    return YES;
}
- (BOOL)heldControl:(NSInteger)i { return i==NZP_HUD_GRENADE||i==NZP_HUD_USE; }
- (void)holdKey:(int)key down:(BOOL)down {
    if(key<=0||key>=256)return;
    if(down){if(_heldKeys[key]++==0)NZP_IOS_Key(key,1);}
    else if(_heldKeys[key]&&--_heldKeys[key]==0)NZP_IOS_Key(key,0);
}
- (void)updateStick:(CGPoint)p {
    if(_moveID<0||!nzp_movement_has_contact(&_movement)) return;
    if(!ValidPoint(p)||!isfinite(_layout.stick_radius)||_layout.stick_radius<=0) { [self cancelAll]; return; }
    float x=(p.x-_layout.stick.x)/_layout.stick_radius;
    float y=(_layout.stick.y-p.y)/_layout.stick_radius, raw=y;
    float len=hypotf(x,y); if(len>1){x/=len;y/=len;}
    if(!isfinite(x)||!isfinite(y)||!isfinite(raw)) { [self cancelAll]; return; }
    _stickDX=x;_stickDY=y;
    NZP_IOS_SetMove(x,y); nzp_movement_analog(&_movement,_moveID,y,raw);
}
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    (void)event; if(!_active||_awaitingFrame||!_layout.valid) return;
    if(_contacts.count==0) nzp_world_end_contacts(&_world);
    for(UITouch *touch in touches) {
        if(_contacts.count>=16) continue;
        CGPoint p=[touch locationInView:self]; int64_t now=NZP_IOS_Milliseconds();
        if(!ValidPoint(p)||now<0||!isfinite(touch.timestamp)||touch.timestamp<0) { [self cancelAll]; return; }
        if(_nextID==INT64_MAX) { [self cancelAll]; _nextID=0; }
        NZPContact *c=[NZPContact new]; c->identifier=++_nextID;c->control=-1;
        c->began=c->last=p;c->beganAt=now;c->beganTouchTime=touch.timestamp;
        c->weapon=_state.weapon_id;c->usePrompt=Text(_state.use_prompt);
        for(NSInteger i=0;i<CONTROL_COUNT;i++) if(!CGRectIsEmpty(_buttons[i])&&CGRectContainsPoint(_buttons[i],p)) {
            c->control=i; c->role=[self enabled:i at:now]?CONTACT_BUTTON:CONTACT_INERT;
            if(c->role==CONTACT_BUTTON) {
                if([self heldControl:i]) [self holdKey:[self keyForControl:i] down:YES];
                else if(i==NZP_HUD_WEAPON) { nzp_weapon_begin(&c->weaponGesture,p.x,p.y,now);nzp_world_cancel(&_world); }
                else if(i>=MENU_UP&&i<=MENU_RIGHT) {NZP_IOS_TapKey([self keyForControl:i]);c->nextRepeat=now+380;}
                else if(i==MENU_ACCEPT||i==MENU_BACK) NZP_IOS_TapKey([self keyForControl:i]);
            }
            break;
        }
        if(c->control<0&&_playing) {
            if(_moveID<0&&nzp_hud_hit_stick(&_layout,p.x,p.y)) {
                if(nzp_movement_down(&_movement,c->identifier,p.x,p.y,now)) {
                    c->role=CONTACT_MOVE;_moveID=c->identifier;[self updateStick:p];
                }
            } else if(nzp_world_down(&_world,c->identifier,p.x,p.y,now)) c->role=CONTACT_WORLD;
        }
        [_contacts setObject:c forKey:touch];
    }
    [self setNeedsDisplay];
}
- (BOOL)moveTouch:(UITouch *)touch contact:(NZPContact *)c {
    CGPoint p=[touch locationInView:self];
    if(!ValidPoint(p)) { [self cancelAll]; return NO; }
    /* The engine clock owns expiry. UIKit timestamps use a different epoch.
     * Coalesced coordinates are applied in order at the current engine time. */
    int64_t now=NZP_IOS_Milliseconds();
    float dx=p.x-c->began.x,dy=p.y-c->began.y;
    if(dx*dx+dy*dy>64) c->moved=YES;
    if(c->role==CONTACT_MOVE) {
        if(c->identifier!=_moveID) { c->role=CONTACT_INERT; return YES; }
        nzp_movement_move(&_movement,c->identifier,p.x,p.y,now);
        if(!nzp_movement_has_contact(&_movement)) {
            c->role=CONTACT_INERT;_moveID=-1;_stickDX=_stickDY=0;NZP_IOS_SetMove(0,0);return NO;
        }
        [self updateStick:p];
    }
    else if(c->role==CONTACT_WORLD) {
        /* UIKit timestamps are used only as same-clock duration differences,
         * never as absolute core times. An early physical drag delivered late
         * must not acquire automatic fire from the engine's later receipt time. */
        NSTimeInterval elapsed=touch.timestamp-c->beganTouchTime;
        if(!c->earlyWorldDrag&&dx*dx+dy*dy>64&&isfinite(elapsed)&&elapsed>=0&&elapsed<.320) {
            c->earlyWorldDrag=YES;
            if(now>=c->beganAt&&now-c->beganAt>=NZP_AUTO_HOLD_MS) {
                BOOL keepAim=nzp_world_aim_latched(&_world);
                BOOL keepSprint=nzp_movement_sprinting(&_movement)&&nzp_movement_has_contact(&_movement);
                nzp_world_cancel(&_world);
                /* cancel_actions also clears ADS/sprint. Preserve these separate
                 * already-authorized gestures; the world contact stays BLOCKED. */
                if(keepAim) nzp_world_toggle_ads(&_world,now);
                if(keepSprint) NZP_IOS_Sprint(1);
            }
        }
        nzp_world_move(&_world,c->identifier,p.x,p.y,now);
    }
    else if(c->role==CONTACT_BUTTON&&c->control==NZP_HUD_WEAPON) {
        if(nzp_weapon_move(&c->weaponGesture,p.x,p.y)==NZP_WEAPON_SWITCH&&
           [self enabled:NZP_HUD_WEAPON at:now]&&c->weapon==_state.weapon_id) {
            nzp_world_cancel(&_world);NZP_IOS_TapKey('q');
        }
    }
    c->last=p;
    return YES;
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for(UITouch *touch in touches) {
        NZPContact *c=[_contacts objectForKey:touch]; if(!c) continue;
        NSArray<UITouch *> *samples=[event coalescedTouchesForTouch:touch];
        if(samples.count) {
            for(UITouch *sample in samples) if(![self moveTouch:sample contact:c]) break;
        } else [self moveTouch:touch contact:c];
    }
    [self setNeedsDisplay];
}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    (void)event;
    for(UITouch *touch in touches) {
        NZPContact *c=[_contacts objectForKey:touch];if(!c)continue;
        CGPoint p=[touch locationInView:self];int64_t now=NZP_IOS_Milliseconds();
        if(!ValidPoint(p)||now<0) { [self cancelAll]; return; }
        if(c->role==CONTACT_MOVE) {
            if(c->identifier==_moveID) {
                nzp_movement_up(&_movement,c->identifier,p.x,p.y,now);_moveID=-1;_stickDX=_stickDY=0;NZP_IOS_SetMove(0,0);
            }
        } else if(c->role==CONTACT_WORLD) nzp_world_up(&_world,c->identifier,p.x,p.y,now);
        else if(c->role==CONTACT_BUTTON) {
            NSInteger i=c->control;BOOL inside=CGRectContainsPoint(_buttons[i],p);
            float dx=p.x-c->began.x,dy=p.y-c->began.y;if(dx*dx+dy*dy>64)c->moved=YES;
            if([self heldControl:i]) [self holdKey:[self keyForControl:i] down:NO];
            else if(i==NZP_HUD_WEAPON) {
                NZPWeaponAction action=nzp_weapon_up(&c->weaponGesture,p.x,p.y,now,inside);
                if([self enabled:i at:now]&&c->weapon==_state.weapon_id&&action!=NZP_WEAPON_NONE) {
                    nzp_world_cancel(&_world);NZP_IOS_TapKey(action==NZP_WEAPON_SWITCH?'q':'r');
                }
            } else if(i==MENU_TYPE&&inside) {
                [self cancelAll];SDL_StartTextInput();
            } else if(inside&&!c->moved&&now>=c->beganAt&&now-c->beganAt<=500&&[self enabled:i at:now]) {
                if(i==NZP_HUD_ADS) {if(c->weapon==_state.weapon_id)nzp_world_toggle_ads(&_world,now);}
                else if(i<MENU_UP) NZP_IOS_TapKey([self keyForControl:i]);
            }
        }
        [_contacts removeObjectForKey:touch];
    }
    if(!_contacts.count) nzp_world_end_contacts(&_world);
    [self setNeedsDisplay];
}
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { (void)touches;(void)event;[self cancelAll]; }
- (void)cancelAll {
    nzp_movement_cancel(&_movement);nzp_world_cancel_all(&_world);NZP_IOS_ReleaseAll();
    [_contacts removeAllObjects];memset(_heldKeys,0,sizeof(_heldKeys));_moveID=-1;_stickDX=_stickDY=0;[self setNeedsDisplay];
}
- (void)didMoveToWindow {
    [super didMoveToWindow];
    if(!self.window) { [self cancelAll]; _awaitingFrame=YES; }
    [self updateHUDReadiness];
}
- (void)label:(NSString *)text x:(CGFloat)x y:(CGFloat)y size:(CGFloat)size width:(CGFloat)width color:(UIColor *)color {
    UIFont *font=[UIFont systemFontOfSize:size weight:UIFontWeightMedium];
    CGSize s=[text sizeWithAttributes:@{NSFontAttributeName:font}];
    if(s.width>width&&s.width>0) font=[font fontWithSize:MAX(8*_layout.unit,size*width/s.width)];
    NSMutableParagraphStyle *p=[NSMutableParagraphStyle new];p.alignment=NSTextAlignmentCenter;p.lineBreakMode=NSLineBreakByTruncatingTail;
    CGRect r=CGRectMake(x-width/2,y-font.lineHeight/2,width,font.lineHeight);
    [text drawInRect:r withAttributes:@{NSFontAttributeName:font,NSForegroundColorAttributeName:color,NSParagraphStyleAttributeName:p}];
}
- (void)leftLabel:(NSString *)text x:(CGFloat)x y:(CGFloat)y size:(CGFloat)size width:(CGFloat)width color:(UIColor *)color {
    UIFont *font=[UIFont systemFontOfSize:size weight:UIFontWeightMedium];
    CGSize measured=[text sizeWithAttributes:@{NSFontAttributeName:font}];
    if(measured.width>width&&measured.width>0) font=[font fontWithSize:size*width/measured.width];
    NSMutableParagraphStyle *style=[NSMutableParagraphStyle new];style.alignment=NSTextAlignmentLeft;style.lineBreakMode=NSLineBreakByTruncatingTail;
    [text drawInRect:CGRectMake(x,y-font.lineHeight/2,width,font.lineHeight)
        withAttributes:@{NSFontAttributeName:font,NSForegroundColorAttributeName:color,NSParagraphStyleAttributeName:style}];
}
static void Line(CGContextRef c,CGFloat x1,CGFloat y1,CGFloat x2,CGFloat y2) {
    CGContextMoveToPoint(c,x1,y1);CGContextAddLineToPoint(c,x2,y2);CGContextStrokePath(c);
}
- (void)icon:(NSInteger)i stance:(int)stance context:(CGContextRef)c {
    switch(i) {
        case NZP_HUD_PAUSE:Line(c,-5,-9,-5,9);Line(c,5,-9,5,9);break;
        case NZP_HUD_STANCE:
            if(stance==0){CGContextStrokeEllipseInRect(c,CGRectMake(-17,-3,6,6));Line(c,-10,0,4,0);Line(c,4,0,14,5);Line(c,4,0,15,-4);Line(c,-6,0,-9,7);}
            else if(stance==1){CGContextStrokeEllipseInRect(c,CGRectMake(-3,-16,6,6));Line(c,0,-9,-2,0);Line(c,-2,0,8,5);Line(c,8,5,2,13);Line(c,2,13,12,13);Line(c,-1,-5,10,-5);}
            else{CGContextStrokeEllipseInRect(c,CGRectMake(-3,-19,6,6));Line(c,0,-12,0,1);Line(c,0,1,-7,14);Line(c,0,1,7,14);Line(c,0,-7,-9,1);Line(c,0,-7,9,1);}break;
        case NZP_HUD_WEAPON:
            Line(c,-71,-3,-36,-3);Line(c,-36,-3,-32,0);Line(c,-32,0,-70,0);
            Line(c,-65,0,-60,8);Line(c,-60,8,-54,8);Line(c,-54,8,-55,0);
            Line(c,-45,0,-44,7);Line(c,-44,7,-39,7);Line(c,-39,7,-40,0);break;
        case NZP_HUD_GRENADE:
            CGContextStrokeEllipseInRect(c,CGRectMake(-9,-9,18,21));Line(c,-4,-13,4,-13);Line(c,3,-12,9,-17);
            CGContextStrokeEllipseInRect(c,CGRectMake(8,-20,6,6));Line(c,-7,-3,7,-3);Line(c,-8,3,8,3);break;
        case NZP_HUD_BETTY:
            CGContextStrokeEllipseInRect(c,CGRectMake(-14,-5,28,10));Line(c,-10,4,-15,13);Line(c,0,5,0,14);Line(c,10,4,15,13);Line(c,0,-5,0,-13);break;
        case NZP_HUD_ADS:
            CGContextStrokeEllipseInRect(c,CGRectMake(-13,-13,26,26));Line(c,-21,0,-7,0);Line(c,7,0,21,0);Line(c,0,-21,0,-7);Line(c,0,7,0,21);break;
        case NZP_HUD_KNIFE:
            Line(c,-32,3,12,-5);Line(c,12,-5,12,5);Line(c,12,5,-32,3);Line(c,15,-10,15,11);Line(c,16,-3,32,-3);Line(c,32,-3,32,4);Line(c,32,4,16,4);break;
        case NZP_HUD_JUMP:Line(c,0,13,0,-14);Line(c,-9,-5,0,-14);Line(c,0,-14,9,-5);break;
        default:break;
    }
}
- (void)drawRect:(CGRect)rect {
    (void)rect;if(!_layout.valid)return;
    CGContextRef c=UIGraphicsGetCurrentContext();float d=_layout.unit;
    int64_t now=NZP_IOS_Milliseconds();BOOL live=nzp_game_state_live(&_state,now);
    UIColor *white=[UIColor colorWithRed:.94 green:.96 blue:.97 alpha:.87];
    CGContextSetLineCap(c,kCGLineCapRound);CGContextSetLineJoin(c,kCGLineJoinRound);
    if(_playing) {
        if(nzp_game_state_valid(&_state,now)&&nzp_hud_state_valid(&_hud,now)) {
            if(_hud.points>=0)[self leftLabel:Text(_hud.points_text) x:_layout.points.x y:_layout.points.y size:19*d width:100*d color:white];
            if(_hud.round>=0)[self leftLabel:Text(_hud.round_text) x:_layout.round.x y:_layout.round.y size:11*d width:100*d color:white];
            unsigned pi=0;
            if(_hud.powerups&1){CGRect p=NZPUIKitRect(_layout.powerups[pi++]);if(_doublePoints)[_doublePoints drawInRect:p];else[self label:@"2×" x:CGRectGetMidX(p) y:CGRectGetMidY(p) size:14*d width:p.size.width color:white];}
            if(_hud.powerups&2){CGRect p=NZPUIKitRect(_layout.powerups[pi]);if(_instaKill)[_instaKill drawInRect:p];else[self label:@"KILL" x:CGRectGetMidX(p) y:CGRectGetMidY(p) size:14*d width:p.size.width color:white];}
        }
        CGRect stick=CGRectMake(_layout.stick.x-_layout.stick_radius,_layout.stick.y-_layout.stick_radius,2*_layout.stick_radius,2*_layout.stick_radius);
        CGContextSetRGBFillColor(c,.19,.22,.25,.13);CGContextFillEllipseInRect(c,stick);
        CGContextSetRGBStrokeColor(c,.87,.90,.93,.44);CGContextSetLineWidth(c,1.5*d);CGContextStrokeEllipseInRect(c,stick);
        CGContextSetRGBFillColor(c,.75,.81,.84,_moveID<0?.44:.69);
        CGContextFillEllipseInRect(c,CGRectMake(_layout.stick.x+_stickDX*_layout.stick_radius*.68f-18*d,_layout.stick.y-_stickDY*_layout.stick_radius*.68f-18*d,36*d,36*d));
    }
    NSArray<NSString *> *menu=@[@"↑",@"↓",@"←",@"→",@"Select",@"Back",@"Type"];
    for(NSInteger i=0;i<CONTROL_COUNT;i++) {
        CGRect b=_buttons[i];if(CGRectIsEmpty(b))continue;
        CGFloat x=CGRectGetMidX(b),y=CGRectGetMidY(b);BOOL available=[self enabled:i at:now];
        UIColor *color=available?white:[UIColor colorWithWhite:.5 alpha:.38];
        if(i>=MENU_UP) {
            [[UIColor colorWithRed:.19 green:.22 blue:.25 alpha:.53]setFill];
            UIBezierPath *path=[UIBezierPath bezierPathWithRoundedRect:b cornerRadius:16*d];[path fill];[color setStroke];path.lineWidth=d;[path stroke];
            [self label:menu[i-MENU_UP] x:x y:y size:12*d width:b.size.width-4*d color:color];continue;
        }
        BOOL pressed=(i==NZP_HUD_ADS)&&nzp_game_state_ads(&_state,now);
        if(pressed){[[UIColor colorWithWhite:.3 alpha:.4]setFill];[[UIBezierPath bezierPathWithRoundedRect:b cornerRadius:16*d]fill];}
        CGContextSaveGState(c);CGContextTranslateCTM(c,x,y);CGContextScaleCTM(c,d,d);
        CGContextSetStrokeColorWithColor(c,color.CGColor);CGContextSetLineWidth(c,2);CGContextSetShadowWithColor(c,CGSizeMake(0,1),2,UIColor.blackColor.CGColor);
        [self icon:i stance:live?_state.stance:-1 context:c];
        if(i==NZP_HUD_ADS&&pressed){CGContextSetRGBStrokeColor(c,.93,.45,.36,1);CGContextStrokeEllipseInRect(c,CGRectMake(-4,-4,8,8));}
        else if(i==NZP_HUD_ADS&&nzp_world_aim_latched(&_world)){CGContextSetRGBFillColor(c,.85,.69,.42,.8);CGContextFillEllipseInRect(c,CGRectMake(16,16,4,4));}
        CGContextRestoreGState(c);
        if(i==NZP_HUD_WEAPON){[self label:live?Text(_state.weapon_name):@"Weapon" x:x+12*d y:y-11*d size:10*d width:b.size.width-10*d color:color];[self label:live?Text(_state.ammo_text):@"—" x:x+20*d y:y+12*d size:15*d width:b.size.width-10*d color:color];}
        else if(i==NZP_HUD_STANCE)[self label:Text(nzp_game_state_stance_label(&_state,now)) x:x y:y+24*d size:9*d width:44*d color:color];
        else if(i==NZP_HUD_GRENADE)[self label:live?Text(_state.grenade_text):@"—" x:x+19*d y:y+19*d size:12*d width:32*d color:color];
        else if(i==NZP_HUD_BETTY)[self label:live?Text(_state.tactical_text):@"—" x:x+19*d y:y+19*d size:12*d width:32*d color:color];
        else if(i==NZP_HUD_USE)[self label:Text(_state.use_prompt) x:x y:y size:13*d width:b.size.width-10*d color:color];
    }
}
@end

static NZPTouchOverlay *overlay;
void NZP_IOS_InitializeUI(SDL_Window *window) {
    NZP_IOS_SetHUDReady(0);
    if(!window)return;SDL_SysWMinfo info;SDL_VERSION(&info.version);
    if(!SDL_GetWindowWMInfo(window,&info)||info.subsystem!=SDL_SYSWM_UIKIT)return;
    UIWindow *native=info.info.uikit.window;
    UIView *host=native.rootViewController.view;
    if(!host)return;
    if(overlay.superview!=host){[overlay removeFromSuperview];overlay=[[NZPTouchOverlay alloc]initWithFrame:host.bounds];[host addSubview:overlay];}
    if(!overlay||overlay.superview!=host)return;
    [host bringSubviewToFront:overlay];[overlay setNeedsLayout];[overlay layoutIfNeeded];
    [overlay updateHUDReadiness];NZP_IOS_SetActive(1);
}
void NZP_IOS_FrameReady(void) { NZP_IOS_PublishState();[overlay frameReady]; }
void NZP_IOS_HandleLifecycle(int active) { NZP_IOS_SetActive(active);[overlay setApplicationActive:!!active]; }
