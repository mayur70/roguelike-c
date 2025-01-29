#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <SDL.h>
#include <SDL_image.h>

#include "app.h"
#include "array.h"
#include "astar.h"
#include "color.h"
#include "console.h"
#include "entity.h"
#include "events.h"
#include "fov.h"
#include "game_map.h"
#include "gameplay_state.h"
#include "mainmenu_state.h"
#include "panic.h"
#include "savefile.h"
#include "terminal.h"
#include "tileset.h"
#include "turn_log.h"
#include "types.h"

#define DESIRED_FPS 20.0
#define DESIRED_FRAME_RATE ((1.0 / DESIRED_FPS) * 1000.0)

static void update(rg_app* app, SDL_Event* event)
{
    switch (app->screen)
    {
    case APP_SCREEN_MENU:
        mainmenu_state_update(app, event, &app->menu_state_data);
        break;
    case APP_SCREEN_PLAY:
        game_state_update(app, event, &app->game_state_data);
        break;
    default:
        ASSERT_M(false);
        break;
    }
}

static void draw(rg_app* app)
{
    switch (app->screen)
    {
    case APP_SCREEN_MENU:
        mainmenu_state_draw(app, &app->menu_state_data);
        break;
    case APP_SCREEN_PLAY:
        game_state_draw(app, &app->game_state_data);
        break;
    default:
        ASSERT_M(false);
        break;
    }
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    // SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
#endif

    srand((unsigned int)time(NULL));

    rg_app app;
    app_create(&app,
               80,
               50,
               "res/dejavu10x10_gs_tc.png",
               32,
               8,
               "res/menu_background.png");

    mainmenu_state_create(&app.menu_state_data, &app);
    SDL_Event event = { 0 };

    update(&app, &event);
    draw(&app);

    while (app.running)
    {
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT)
        {
            app.running = false;
            break;
        }

        Uint64 frame_start = SDL_GetPerformanceCounter();

        update(&app, &event);
        draw(&app);

        Uint64 frame_end = SDL_GetPerformanceCounter();
        double dt = (frame_end - frame_start) /
                    (double)SDL_GetPerformanceFrequency() * 1000.0;

        Uint32 sleep_time = (Uint32)floor(DESIRED_FRAME_RATE - dt);
        const Uint32 max_sleep_time = 1000;
        if (sleep_time > max_sleep_time) sleep_time = max_sleep_time;
        // SLEEP
        SDL_Delay(sleep_time);
    }
    mainmenu_state_destroy(&app.menu_state_data);
    app_destroy(&app);
    return 0;
}