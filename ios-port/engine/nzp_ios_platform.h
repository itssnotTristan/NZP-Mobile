/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef NZP_IOS_PLATFORM_H
#define NZP_IOS_PLATFORM_H
#include <SDL.h>
#ifdef __cplusplus
extern "C" {
#endif
void NZP_IOS_AttachWindow(SDL_Window *window);
void NZP_IOS_PrepareDrawable(void);
void NZP_IOS_InitializeUI(SDL_Window *window);
void NZP_IOS_FrameReady(void);
void NZP_IOS_HandleLifecycle(int active);
#ifdef __cplusplus
}
#endif
#endif
