// SPDX-License-Identifier: GPL-3.0-or-later
package org.nzp.mobile.preview;
/** Test adapter only. The production Java state machines are compiled unchanged. */
public final class CanonicalParity {
    private static int seed=0x17ab3491;
    private static long trace=0xcbf29ce484222325L;
    private static int randomBelow(int n) { seed=seed*1664525+1013904223; return Integer.remainderUnsigned(seed,n); }
    private static void hash(int n) { trace^=Integer.toUnsignedLong(n); trace*=0x100000001b3L; }
    private static void hash(boolean value) { hash(value?1:0); }
    private static final class Sink implements GameplayInput.Sink,MovementGesture.Sink {
        public void tapFire(){hash(1);}
        public void autoFire(boolean on,int weapon){hash(2);hash(on);hash(weapon);}
        public void look(float x,float y){hash(3);hash(Float.floatToIntBits(x));hash(Float.floatToIntBits(y));}
        public void ads(boolean on,int weapon){hash(4);hash(on);hash(weapon);}
        public void cancelActions(){hash(5);}
        public void sprint(boolean on){hash(6);hash(on);}
    }
    public static void main(String[] args) {
        Sink sink=new Sink(); GameplayInput world=new GameplayInput(sink); MovementGesture movement=new MovementGesture(sink);
        WeaponGesture weapon=new WeaponGesture(0,0,0);
        world.setActive(true); world.update(new MobileGameState(71,2,7,10,50,2,1,"Weapon","Use",0),0);
        long now=0;
        for(int i=0;i<50000;++i) {
            now+=randomBelow(101); int op=randomBelow(20),id=randomBelow(4);
            float x=randomBelow(81)-40,y=randomBelow(81)-40;
            int flags=randomBelow(256),weaponId=randomBelow(10),age=randomBelow(2001),on=randomBelow(2);
            float forward=(randomBelow(15)-7)*.25f,raw=(randomBelow(15)-7)*.25f;
            switch(op) {
            case 0:world.setActive(on!=0);break;
            case 1:world.update(new MobileGameState(flags,2,weaponId,10,50,2,1,"Weapon","Use",now-age),now);break;
            case 2:hash(world.down(id,x,y,now));break;
            case 3:world.move(id,x,y,now);break;
            case 4:world.up(id,x,y,now);break;
            case 5:world.tick(now);break;
            case 6:world.toggleAds(now);break;
            case 7:world.cancelWorld();break;
            case 8:world.endContacts();break;
            case 9:world.cancelAll();break;
            case 10:hash(movement.down(id,x,y,now));break;
            case 11:movement.move(id,x,y,now);break;
            case 12:movement.up(id,x,y,now);break;
            case 13:movement.movement(id,forward,raw);break;
            case 14:movement.cancel();break;
            case 15:weapon=new WeaponGesture(x,y,now);break;
            case 16:hash(weapon.move(x,y));break;
            case 17:hash(weapon.up(x,y,now,on!=0));break;
            case 18:weapon.cancel();break;
            case 19:world.tick(now);break;
            default:throw new AssertionError();
            }
            hash(world.aimLatched());hash(world.autoFiring());hash(world.hasWorldContact());hash(movement.sprinting());hash(movement.hasContact());
        }
        world.cancelAll();movement.cancel();System.out.printf("%016x%n",trace);
    }
}
