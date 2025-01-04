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
#include "types.h"
#include "fov.h"

#define DESIRED_FPS 20.0
#define DESIRED_FRAME_RATE ((1.0 / DESIRED_FPS) * 1000.0)

typedef struct rg_game_state
{
    int screen_width;
    int screen_height;
    int map_width;
    int map_height;
    int room_max_size;
    int room_min_size;
    int max_rooms;
    int max_monsters_per_room;
    bool fov_light_walls;
    int fov_radius;
    rg_tileset tileset;
    rg_terminal terminal;
    rg_console console;
    rg_entity_array entities;
    rg_entity *player;
    rg_map game_map;
    rg_fov_map fov_map;
    bool recompute_fov;
} rg_game_state;

void game_recompute_fov(rg_game_state *state)
{
    fov_map_compute(&state->fov_map,
                    state->player->x,
                    state->player->y,
                    state->fov_radius,
                    state->fov_light_walls);
}

void update(rg_app *app,
            SDL_Event *event,
            rg_game_state *state)
{
    rg_map *game_map = &state->game_map;
    rg_fov_map *fov_map = &state->fov_map;
    rg_entity *player = state->player;
    rg_action action;
    event_dispatch(event, &action);

    if (action.type == ACTION_NONE)
        return;

    if (action.type == ACTION_MOVEMENT)
    {

        if (!map_is_blocked(game_map,
                            player->x + action.dx,
                            player->y + action.dy))
        {
            entity_move(player, action.dx, action.dy);
            state->recompute_fov = true;
        }
    }

    if (state->recompute_fov)
        game_recompute_fov(state);

    if (action.type == ACTION_ESCAPE || action.type == ACTION_QUIT)
        app->running = false;
}

void draw(rg_app *app,
          rg_game_state *state)
{
    rg_map *game_map = &state->game_map;
    rg_fov_map *fov_map = &state->fov_map;
    rg_console *console = &state->console;
    rg_entity_array *entities = &state->entities;

    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    SDL_RenderClear(app->renderer);

    for (int y = 0; y < game_map->height; y++)
    {
        for (int x = 0; x < game_map->width; x++)
        {
            bool visible = fov_map_is_in_fov(fov_map, x, y);
            rg_tile *tile = map_get_tile(game_map, x, y);
            bool is_wall = tile->block_sight;
            if (visible)
            {
                tile->explored = true;
                if (is_wall)
                    console_fill(console,
                                 x,
                                 y,
                                 LIGHT_WALL);
                else
                    console_fill(console,
                                 x,
                                 y,
                                 LIGHT_GROUND);
            }
            else if (tile->explored)
            {
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
    }
    for (int i = 0; i < entities->len; i++)
    {
        const rg_entity *e = &entities->data[i];
        if (fov_map_is_in_fov(fov_map, e->x, e->y))
            entity_draw(e, console);
    }
    SDL_RenderPresent(app->renderer);

    state->recompute_fov = false;
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
#endif

    srand((unsigned int)time(NULL));

    rg_app app;
    app_create(&app);

    rg_game_state state;
    state.screen_width = 80;
    state.screen_height = 50;
    state.map_width = 80;
    state.map_height = 45;
    state.room_max_size = 10;
    state.room_min_size = 6;
    state.max_rooms = 30;
    state.max_monsters_per_room = 3;
    state.fov_light_walls = true;
    state.fov_radius = 10;

    tileset_create(
        &state.tileset,
        app.renderer,
        "res/dejavu10x10_gs_tc.png",
        32,
        8);

    terminal_create(
        &state.terminal,
        &app,
        state.screen_width,
        state.screen_height,
        &state.tileset,
        "roguelike",
        true);

    console_create(
        &state.console,
        app.renderer,
        state.terminal.width,
        state.terminal.height,
        state.terminal.tileset);

    state.entities.capacity = state.max_monsters_per_room;
    state.entities.data = malloc(sizeof(*state.entities.data) * state.entities.capacity);
    state.entities.len = 0;
    ARRAY_PUSH(&state.entities, ((rg_entity){0, 0, '@', WHITE}));
    state.player = &state.entities.data[0];

    map_create(&state.game_map,
               state.map_width,
               state.map_height,
               state.room_min_size,
               state.room_max_size,
               state.max_rooms,
               state.max_monsters_per_room,
               &state.entities,
               state.player);

    fov_map_create(&state.fov_map, state.map_width, state.map_height);
    for (int y = 0; y < state.map_height; y++)
    {
        for (int x = 0; x < state.map_width; x++)
        {
            rg_tile *tile = map_get_tile(&state.game_map, x, y);
            fov_map_set_props(&state.fov_map,
                              x,
                              y,
                              !tile->block_sight,
                              !tile->blocked);
        }
    }

    // first draw before waitevent
    game_recompute_fov(&state);
    draw(&app, &state);
    state.player = &state.entities.data[0];//FIXME: draw method changinge state.player data

    while (app.running)
    {
        SDL_Event event = {0};
        SDL_WaitEvent(&event);

        Uint64 frame_start = SDL_GetPerformanceCounter();

        update(&app, &event, &state);
        draw(&app, &state);
        state.player = &state.entities.data[0];

        Uint64 frame_end = SDL_GetPerformanceCounter();
        double dt = (frame_end - frame_start) / (double)SDL_GetPerformanceFrequency() * 1000.0;

        Uint32 sleep_time = (Uint32)floor(DESIRED_FRAME_RATE - dt);
        const Uint32 max_sleep_time = 1000;
        if (sleep_time > max_sleep_time)
            sleep_time = max_sleep_time;
        // SLEEP
        SDL_Delay(sleep_time);
    }

    map_destroy(&state.game_map);
    free(state.entities.data);
    console_destroy(&state.console);
    terminal_destroy(&state.terminal);
    tileset_destroy(&state.tileset);

    app_destroy(&app);
    return 0;
}