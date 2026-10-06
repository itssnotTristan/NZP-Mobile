/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <SDL.h>
#ifdef main
#undef main
#endif
int main(int argc, char **argv)
{
    return SDL_UIKitRunApp(argc, argv, SDL_main);
}
