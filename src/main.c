#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <SDL.h>
#include <SDL_image.h>

#include "tileset.h"
#include "panic.h"
#include "console.h"
#include "app.h"
#include "terminal.h"
#include "events.h"
#include "array.h"
#include "color.h"
#include "entity.h"

#define DESIRED_FPS 20.0
#define DESIRED_FRAME_RATE ((1.0 / DESIRED_FPS) * 1000.0)

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
#endif

    rg_app app;
    app_create(&app);

    int screen_width = 80;
    int screen_height = 50;

    rg_tileset tileset;
    tileset_create(
        &tileset,
        app.renderer,
        "res/dejavu10x10_gs_tc.png",
        32,
        8);

    rg_terminal terminal;
    terminal_create(
        &terminal,
        &app,
        screen_width,
        screen_height,
        &tileset,
        "roguelike",
        true);

    rg_console console;
    console_create(
        &console,
        app.renderer,
        terminal.width,
        terminal.height,
        terminal.tileset);

    rg_entity_array entities = {
        .capacity = 2,
        .data = malloc(sizeof(rg_entity) * 2),
        .len = 0,
    };
    ARRAY_PUSH(&entities, ((rg_entity){screen_width / 2, screen_height / 2, '@', WHITE}));
    ARRAY_PUSH(&entities, ((rg_entity){(screen_width / 2) - 5, screen_height / 2, '@', YELLOW}));
    rg_entity *player = &entities.data[0];
    rg_entity *npc = &entities.data[1];

    // first draw before waitevent
    
    SDL_RenderClear(app.renderer);
    for (int i = 0; i < entities.len; i++)
    {
        entity_draw(&entities.data[i], &console);
    }
    SDL_RenderPresent(app.renderer);

    bool running = true;
    while (running)
    {

        SDL_Event event = {0};
        SDL_WaitEvent(&event);

        Uint64 frame_start = SDL_GetPerformanceCounter();

        rg_action action;
        event_dispatch(&event, &action);

        if (action.type == ACTION_NONE)
            continue;

        if (action.type == ACTION_MOVEMENT)
        {
            entity_move(player, action.dx, action.dy);
        }
        else if (action.type == ACTION_ESCAPE || action.type == ACTION_QUIT)
            running = false;

        SDL_RenderClear(app.renderer);

        for (int i = 0; i < entities.len; i++)
        {
            entity_draw(&entities.data[i], &console);
        }

        SDL_RenderPresent(app.renderer);

        Uint64 frame_end = SDL_GetPerformanceCounter();
        double dt = (frame_end - frame_start) / (double)SDL_GetPerformanceFrequency() * 1000.0;

        Uint32 sleep_time = (Uint32)floor(DESIRED_FRAME_RATE - dt);
        const Uint32 max_sleep_time = 1000;
        if (sleep_time > max_sleep_time)
            sleep_time = max_sleep_time;
        // SLEEP
        SDL_Delay(sleep_time);
    }

    free(entities.data);
    console_destroy(&console);
    terminal_destroy(&terminal);
    tileset_destroy(&tileset);

    app_destroy(&app);
    return 0;
}