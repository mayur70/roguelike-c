#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

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
#include "game_map.h"

#define DESIRED_FPS 20.0
#define DESIRED_FRAME_RATE ((1.0 / DESIRED_FPS) * 1000.0)

void update(rg_app *app,
            SDL_Event *event,
            rg_map *game_map,
            rg_entity *player)
{
    rg_action action;
    event_dispatch(event, &action);

    if (action.type == ACTION_NONE)
        return;

    if (action.type == ACTION_MOVEMENT)
        if (!map_is_blocked(game_map,
                            player->x + action.dx,
                            player->y + action.dy))
            entity_move(player, action.dx, action.dy);

    if (action.type == ACTION_ESCAPE || action.type == ACTION_QUIT)
        app->running = false;
}

void draw(rg_app *app,
          rg_console *console,
          rg_map *game_map,
          rg_entity_array *entities)
{
    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    SDL_RenderClear(app->renderer);

    for (int y = 0; y < game_map->height; y++)
    {
        for (int x = 0; x < game_map->width; x++)
        {
            bool is_wall = map_get_tile(game_map, x, y)->block_sight;
            if (is_wall)
                console_fill(console,
                             x,
                             y,
                             DARK_WALL);
            else
                console_fill(console,
                             x,
                             y,
                             DARK_GROUND);
        }
    }
    for (int i = 0; i < entities->len; i++)
    {
        entity_draw(&entities->data[i], console);
    }
    SDL_RenderPresent(app->renderer);
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
#endif

    srand((unsigned int)time(NULL));

    rg_app app;
    app_create(&app);

    int screen_width = 80;
    int screen_height = 50;
    int map_width = 80;
    int map_height = 45;
    int room_max_size = 10;
    int room_min_size = 6;
    int max_rooms = 30;

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

    rg_map game_map;
    map_create(&game_map,
               map_width,
               map_height,
               room_min_size,
               room_max_size,
               max_rooms,
               player);

    // first draw before waitevent
    draw(&app, &console, &game_map, &entities);

    while (app.running)
    {
        SDL_Event event = {0};
        SDL_WaitEvent(&event);

        Uint64 frame_start = SDL_GetPerformanceCounter();

        update(&app, &event, &game_map, player);
        draw(&app, &console, &game_map, &entities);

        Uint64 frame_end = SDL_GetPerformanceCounter();
        double dt = (frame_end - frame_start) / (double)SDL_GetPerformanceFrequency() * 1000.0;

        Uint32 sleep_time = (Uint32)floor(DESIRED_FRAME_RATE - dt);
        const Uint32 max_sleep_time = 1000;
        if (sleep_time > max_sleep_time)
            sleep_time = max_sleep_time;
        // SLEEP
        SDL_Delay(sleep_time);
    }

    map_destroy(&game_map);
    free(entities.data);
    console_destroy(&console);
    terminal_destroy(&terminal);
    tileset_destroy(&tileset);

    app_destroy(&app);
    return 0;
}