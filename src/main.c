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

#define DESIRED_FPS 20.0
#define DESIRED_FRAME_RATE ((1.0 / DESIRED_FPS) *  1000.0)

typedef enum rg_action_type
{
    ACTION_NONE,
    ACTION_QUIT,
    ACTION_ESCAPE,
    ACTION_MOVEMENT,
} rg_action_type;

typedef struct rg_action
{
    rg_action_type type;
    union
    {
        struct
        {
            int dx, dy;
        };
    };
} rg_action;

void event_dispatch(SDL_Event *event, rg_action *action)
{
    // Default value
    action->type = ACTION_NONE;

    switch (event->type)
    {
    case SDL_QUIT:
        action->type = ACTION_QUIT;
        break;
    case SDL_KEYDOWN:
    {
        switch (event->key.keysym.sym)
        {
        case SDLK_UP:
        case SDLK_w:
            action->type = ACTION_MOVEMENT;
            action->dx = 0;
            action->dy = -1;
            break;
        case SDLK_DOWN:
        case SDLK_s:
            action->type = ACTION_MOVEMENT;
            action->dx = 0;
            action->dy = 1;
            break;
        case SDLK_LEFT:
        case SDLK_a:
            action->type = ACTION_MOVEMENT;
            action->dx = -1;
            action->dy = 0;
            break;
        case SDLK_RIGHT:
        case SDLK_d:
            action->type = ACTION_MOVEMENT;
            action->dx = 1;
            action->dy = 0;
            break;

        case SDLK_ESCAPE:
            action->type = ACTION_ESCAPE;
            break;
        }
        break;
    }
    default:
        // no-op
        break;
    }
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
#endif

    rg_app app;
    app_create(&app);

    int screen_width = 80;
    int screen_height = 50;

    int player_x = screen_width / 2;
    int player_y = screen_height / 2;

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
            player_x += action.dx;
            player_y += action.dy;
        }
        else if (action.type == ACTION_ESCAPE || action.type == ACTION_QUIT)
            running = false;

        SDL_RenderClear(app.renderer);
        console_print(&console, player_x, player_y, '@');

        SDL_RenderPresent(app.renderer);

        Uint64 frame_end = SDL_GetPerformanceCounter();
        double dt = (frame_end - frame_start) / (double)SDL_GetPerformanceFrequency() * 1000.0;

        Uint32 sleep_time = (Uint32)floor(DESIRED_FRAME_RATE - dt);
        // SLEEP
        SDL_Delay(sleep_time);
    }

    console_destroy(&console);
    terminal_destroy(&terminal);
    tileset_destroy(&tileset);

    app_destroy(&app);
    return 0;
}