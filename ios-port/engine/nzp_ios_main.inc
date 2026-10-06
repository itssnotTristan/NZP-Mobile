/* SPDX-License-Identifier: GPL-3.0-or-later
 * Included by the isolated sys_sdl.c translation unit. SDL owns UIApplication.
 * All FTE state and UIKit callbacks execute on the main thread. */
#include "nzp_ios_platform.h"
#include "nzp_ios_engine.h"
#include "nzp_ios_mobile_config.h"

static double nzp_ios_last_time;
static int nzp_ios_active = 1;
static int nzp_ios_ready;
static char *nzp_ios_home;
static char *nzp_ios_base;
static const char *nzp_ios_argv[32];

static void NZP_IOS_Animate(void *unused)
{
    double now, elapsed;
    (void)unused;
    if (!nzp_ios_active || !host_initialized) return;
    now = Sys_DoubleTime();
    elapsed = now - nzp_ios_last_time;
    nzp_ios_last_time = now;
    if (elapsed < 0) elapsed = 0;
    if (elapsed > 0.1) elapsed = 0.1;
    NZP_IOS_PrepareDrawable();
    (void)Host_Frame(elapsed);
    NZP_IOS_FrameReady();
}

void NZP_IOS_AttachWindow(SDL_Window *window)
{
    if (!window) return;
    nzp_ios_last_time = Sys_DoubleTime();
    if (SDL_iPhoneSetAnimationCallback(window, 1, NZP_IOS_Animate, NULL) != 0)
        Sys_Error("Unable to attach iOS display callback: %s", SDL_GetError());
    NZP_IOS_InitializeUI(window);
    nzp_ios_ready = 1;
}

static void NZP_IOS_SetLifecycle(int active)
{
    soundcardinfo_t *sc;
    if (nzp_ios_active == active) return;
    nzp_ios_active = active;
    NZP_IOS_SetActive(active);
    if (host_initialized) {
        Key_ClearStates();
        if (!active) Host_WriteConfiguration();
        /* The iOS configuration compiles only SDL output. Keep device IDs and
         * queued mixer state intact, rather than recreating sound resources. */
        for (sc = sndcardinfo; sc; sc = sc->next)
            if (sc->audio_fd > 0) SDL_PauseAudioDevice(sc->audio_fd, !active);
        vid.activeapp = active;
    }
    nzp_ios_last_time = Sys_DoubleTime();
    if (nzp_ios_ready) NZP_IOS_HandleLifecycle(active);
}

static int SDLCALL NZP_IOS_AppEvent(void *unused, SDL_Event *event)
{
    (void)unused;
    switch (event->type) {
    case SDL_APP_WILLENTERBACKGROUND:
    case SDL_APP_DIDENTERBACKGROUND:
        NZP_IOS_SetLifecycle(0);
        return 0;
    case SDL_APP_DIDENTERFOREGROUND:
        NZP_IOS_SetLifecycle(1);
        return 0;
    case SDL_APP_TERMINATING:
        NZP_IOS_SetLifecycle(0);
        /* UIKit may deliver this while SDL is pumping within Host_Frame.
         * Do not destroy engine state reentrantly; state is already saved and
         * native input/audio are stopped. The OS reclaims the process. */
        return 0;
    case SDL_APP_LOWMEMORY:
        /* Do not mutate renderer/VM resources inside the event callback.
         * The app must handle memory pressure through conservative budgets. */
        return 0;
    default:
        return 1;
    }
}

static void NZP_IOS_WriteConfig(const char *path, const char *text, int first_run)
{
    int fd = open(path, O_WRONLY | O_CREAT | (first_run ? O_EXCL : O_TRUNC), 0600);
    const char *cursor = text;
    size_t left = strlen(text);
    if (fd < 0) {
        if (first_run && errno == EEXIST) return;
        Sys_Error("Cannot open private iOS settings: %s", strerror(errno));
    }
    while (left) {
        ssize_t written = write(fd, cursor, left);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) { close(fd); Sys_Error("Cannot write private iOS settings"); }
        cursor += written; left -= written;
    }
    if (fsync(fd) || close(fd)) Sys_Error("Cannot synchronize private iOS settings");
}
static void NZP_IOS_PrepareSettings(void)
{
    char path[MAX_OSPATH];
    if (strlen(nzp_ios_home) + sizeof("nzp/data/user_settings.cfg") >= sizeof(path))
        Sys_Error("Private iOS settings path exceeds engine path limit");
    snprintf(path, sizeof(path), "%snzp", nzp_ios_home);
    if (mkdir(path, 0700) && errno != EEXIST) Sys_Error("Cannot create private game directory");
    snprintf(path, sizeof(path), "%snzp/data", nzp_ios_home);
    if (mkdir(path, 0700) && errno != EEXIST) Sys_Error("Cannot create private saved settings directory");
    snprintf(path, sizeof(path), "%snzp/data/user_settings.cfg", nzp_ios_home);
    NZP_IOS_WriteConfig(path, NZP_IOS_FIRST_RUN_CONFIG, 1);
    snprintf(path, sizeof(path), "%snzp/mobile.cfg", nzp_ios_home);
    NZP_IOS_WriteConfig(path, NZP_IOS_MOBILE_CONFIG, 0);
}

int QDECL main(int argc, char **argv)
{
    quakeparms_t parms;
    char *bundle;
    int n = 0;
    (void)argc; (void)argv;
    memset(&parms, 0, sizeof(parms));
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetEventFilter(NZP_IOS_AppEvent, NULL);
    bundle = SDL_GetBasePath();
    nzp_ios_home = SDL_GetPrefPath("org.nzp", "NZPortable");
    if (!bundle || !nzp_ios_home) Sys_Error("Cannot resolve iOS application storage");
    NZP_IOS_PrepareSettings();
    nzp_ios_base = SDL_malloc(strlen(bundle) + sizeof("game/"));
    if (!nzp_ios_base) Sys_Error("Cannot allocate iOS game path");
    strcpy(nzp_ios_base, bundle);
    strcat(nzp_ios_base, "game/");
    nzp_ios_argv[n++] = "nzportable";
    nzp_ios_argv[n++] = "-basedir"; nzp_ios_argv[n++] = nzp_ios_base;
    nzp_ios_argv[n++] = "-homedir"; nzp_ios_argv[n++] = nzp_ios_home;
    nzp_ios_argv[n++] = "-game"; nzp_ios_argv[n++] = "nzp";
    nzp_ios_argv[n++] = "+set"; nzp_ios_argv[n++] = "vid_renderer"; nzp_ios_argv[n++] = "gl";
    nzp_ios_argv[n++] = "+set"; nzp_ios_argv[n++] = "vid_fullscreen"; nzp_ios_argv[n++] = "1";
    nzp_ios_argv[n++] = "+set"; nzp_ios_argv[n++] = "vid_vsync"; nzp_ios_argv[n++] = "1";
    nzp_ios_argv[n++] = "+set"; nzp_ios_argv[n++] = "cl_maxfps"; nzp_ios_argv[n++] = "60";
    nzp_ios_argv[n++] = "+set"; nzp_ios_argv[n++] = "sys_osk"; nzp_ios_argv[n++] = "0";
    nzp_ios_argv[n++] = "-usehome";
    nzp_ios_argv[n++] = "+exec"; nzp_ios_argv[n++] = "mobile.cfg";
    nzp_ios_argv[n] = NULL;
    parms.argc = n; parms.argv = nzp_ios_argv;
    parms.basedir = nzp_ios_base; parms.binarydir = bundle;
#ifdef CONFIG_MANIFEST_TEXT
    parms.manifest = CONFIG_MANIFEST_TEXT;
#endif
    COM_InitArgv(parms.argc, parms.argv);
    TL_InitLanguages(parms.basedir);
    Host_Init(&parms);
    NZP_IOS_Initialize();
    /* Assets are bundled: there is no asynchronous content downloader. Finish
     * startup before returning to UIKit's display callback. */
    (void)Host_Frame(0.01);
    if (!nzp_ios_ready) Sys_Error("iOS renderer did not create a drawable window");
    NZP_IOS_SetActive(nzp_ios_active);
    return 0;
}
