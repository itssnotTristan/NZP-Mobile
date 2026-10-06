/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef NZP_TOUCH_OVERLAY_H
#define NZP_TOUCH_OVERLAY_H
struct SDL_Window;
void NZP_IOS_InitializeUI(struct SDL_Window *window);
void NZP_IOS_FrameReady(void);
void NZP_IOS_HandleLifecycle(int active);
#endif
