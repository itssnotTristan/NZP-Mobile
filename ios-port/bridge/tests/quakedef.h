/* Test-only fake FTE surface. Never include this directory in production. */
#ifndef NZP_TEST_QUAKEDEF_H
#define NZP_TEST_QUAKEDEF_H
#include <stdbool.h>
typedef int qboolean;
enum { K_MOUSE1 = 178, K_MOUSE2 = 179, K_UPARROW = 132, K_RIGHTARROW = 135,
       K_F11 = 155, K_F12 = 156, ca_active = 4 };
enum { kdm_menu = 1, kdm_console = 2, kdm_message = 4, kdm_cwindows = 8 };
typedef struct { int local; } netadr_t;
typedef struct { netadr_t remote_address; } netchan_t;
typedef struct { int state, demoplayback; netchan_t netchan; } test_cls_t;
typedef struct { int playernum; } test_view_t;
typedef struct { int paused; test_view_t playerview[1]; } test_cl_t;
extern test_cls_t cls;
extern test_cl_t cl;
extern int host_initialized;
double Sys_DoubleTime(void);
int Key_Dest_Has(int mask);
int NET_IsLoopBackAddress(const netadr_t *address);
void IN_KeyEvent(unsigned device, int down, int key, unsigned unicode);
void IN_MouseMove(unsigned device, int absolute, float x, float y, float z, float size);
#endif
