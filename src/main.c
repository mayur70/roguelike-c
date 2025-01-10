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
#include "color.h"
#include "console.h"
#include "entity.h"
#include "events.h"
#include "fov.h"
#include "game_map.h"
#include "panic.h"
#include "terminal.h"
#include "tileset.h"
#include "types.h"
#include "astar.h"

#define DESIRED_FPS 20.0
#define DESIRED_FRAME_RATE ((1.0 / DESIRED_FPS) * 1000.0)

typedef enum rg_game_state
{
    ST_TURN_PLAYER,
    ST_TURN_ENEMY
} rg_game_state;

typedef struct rg_game_state_data
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
    rg_entity_id player;
    rg_map game_map;
    rg_fov_map fov_map;
    bool recompute_fov;
    rg_game_state game_state;
} rg_game_state_data;

float distance_between(rg_entity* a, rg_entity* b)
{
    int dx = b->x - a->x;
    int dy = b->y - a->y;
    return (float)sqrt(dx * dx + dy * dy);
}

void entity_move_towards(rg_entity* e,
                         int x,
                         int y,
                         rg_map* map,
                         rg_entity_array* entities)
{
    int dx = x - e->x;
    int dy = y - e->y;
    float distance = (float)sqrt(dx * dx + dy * dy);
    dx = (int)floor(dx / distance);
    dy = (int)floor(dy / distance);

    if (!map_is_blocked(map, e->x + dx, e->y + dy))
    {
        rg_entity* other = NULL;
        entity_get_at_loc(entities, e->x + dx, e->y + dy, &other);
        if (other == NULL) entity_move(e, dx, dy);
    }
}

void entity_move_astar(rg_entity* e,
                       rg_entity* target,
                       rg_map* game_map,
                       rg_entity_array* entities)
{
    rg_fov_map map;
    fov_map_create(&map, game_map->width, game_map->height);
    for (int y = 0; y < game_map->height; y++)
    {
        for (int x = 0; x < game_map->width; x++)
        {
            rg_tile* t = map_get_tile(game_map, x, y);
            fov_map_set_props(&map, x, y, !t->block_sight, !t->blocked);
        }
    }
    for (int i = 0; i < entities->len; i++)
    {
        rg_entity* t = &entities->data[i];
        if (t->blocks && t != e && t != target)
        {
            fov_map_set_props(&map, t->x, t->y, true, false);
        }
    }

    astar_path* path= astar_path_new_using_map(&map, (float)1.41);
    astar_path_compute(path, e->x, e->y, target->x, target->y);

    if (!astar_path_is_empty(path) && astar_path_size(path) < 25)
    {
        int dx, dy;
        bool res = astar_path_walk(path, &dx, &dy, true);
        if (res)
        {
            e->x = dx;
            e->y = dy;
        }
    }
    else
    {
        entity_move_towards(e, target->x, target->y, game_map, entities);
    }
    astar_path_delete(path);
}

void basic_monster_update(rg_entity* e,
                          rg_entity* target,
                          rg_fov_map* fov_map,
                          rg_map* game_map,
                          rg_entity_array* entities)
{
    if (fov_map_is_in_fov(fov_map, e->x, e->y))
    {
        int distance = (int)distance_between(e, target);
        if (distance >= 2)
        {
            //entity_move_towards(e, target->x, target->y, game_map, entities);
            entity_move_astar(e, target, game_map, entities);
        }
        else if (target->fighter.hp > 0)
            SDL_Log("The '%s' insults you! your ego is damaged!", e->name);
    }
}

void game_recompute_fov(rg_game_state_data* data)
{
    fov_map_compute(&data->fov_map,
                    data->entities.data[data->player].x,
                    data->entities.data[data->player].y,
                    data->fov_radius,
                    data->fov_light_walls);
}

void update(rg_app* app, SDL_Event* event, rg_game_state_data* data)
{
    rg_map* game_map = &data->game_map;
    rg_fov_map* fov_map = &data->fov_map;
    rg_action action;
    event_dispatch(event, &action);

    if (action.type == ACTION_NONE) return;

    if (action.type == ACTION_MOVEMENT && data->game_state == ST_TURN_PLAYER)
    {
        int destination_x = data->entities.data[data->player].x + action.dx;
        int destination_y = data->entities.data[data->player].y + action.dy;
        if (!map_is_blocked(game_map, destination_x, destination_y))
        {
            rg_entity* target = NULL;
            entity_get_at_loc(
              &data->entities, destination_x, destination_y, &target);

            if (target == NULL)
            {
                entity_move(
                  &data->entities.data[data->player], action.dx, action.dy);
                data->recompute_fov = true;
            }
            else
            {
                SDL_Log("You kick the '%s' in shins, much to its annoyance!",
                        target->name);
            }
            data->game_state = ST_TURN_ENEMY;
        }
    }

    if (data->recompute_fov) game_recompute_fov(data);

    if (action.type == ACTION_ESCAPE || action.type == ACTION_QUIT)
        app->running = false;

    if (data->game_state == ST_TURN_ENEMY)
    {
        for (int i = 0; i < data->entities.len; i++)
        {
            if (i != data->player)
            {
                rg_entity* e = &data->entities.data[i];
                rg_entity* player = &data->entities.data[data->player];
                if (e->type == ENTITY_BASIC_MONSTER)
                    basic_monster_update(e,
                                         player,
                                         &data->fov_map,
                                         &data->game_map,
                                         &data->entities);
            }
        }
        data->game_state = ST_TURN_PLAYER;
    }
}

void draw(rg_app* app, rg_game_state_data* data)
{
    rg_map* game_map = &data->game_map;
    rg_fov_map* fov_map = &data->fov_map;
    rg_console* console = &data->console;
    rg_entity_array* entities = &data->entities;

    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    SDL_RenderClear(app->renderer);

    for (int y = 0; y < game_map->height; y++)
    {
        for (int x = 0; x < game_map->width; x++)
        {
            bool visible = fov_map_is_in_fov(fov_map, x, y);
            rg_tile* tile = map_get_tile(game_map, x, y);
            bool is_wall = tile->block_sight;
            if (visible)
            {
                tile->explored = true;
                if (is_wall) console_fill(console, x, y, LIGHT_WALL);
                else
                    console_fill(console, x, y, LIGHT_GROUND);
            }
            else if (tile->explored)
            {
                if (is_wall) console_fill(console, x, y, DARK_WALL);
                else
                    console_fill(console, x, y, DARK_GROUND);
            }
        }
    }
    for (int i = 0; i < entities->len; i++)
    {
        const rg_entity* e = &entities->data[i];
        if (fov_map_is_in_fov(fov_map, e->x, e->y)) entity_draw(e, console);
    }
    SDL_RenderPresent(app->renderer);

    data->recompute_fov = false;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
#endif

    srand((unsigned int)time(NULL));

    rg_app app;
    app_create(&app);

    rg_game_state_data data;
    data.screen_width = 80;
    data.screen_height = 50;
    data.map_width = 80;
    data.map_height = 45;
    data.room_max_size = 10;
    data.room_min_size = 6;
    data.max_rooms = 30;
    data.max_monsters_per_room = 3;
    data.fov_light_walls = true;
    data.fov_radius = 10;
    data.game_state = ST_TURN_PLAYER;

    tileset_create(
      &data.tileset, app.renderer, "res/dejavu10x10_gs_tc.png", 32, 8);

    terminal_create(&data.terminal,
                    &app,
                    data.screen_width,
                    data.screen_height,
                    &data.tileset,
                    "roguelike",
                    true);

    console_create(&data.console,
                   app.renderer,
                   data.terminal.width,
                   data.terminal.height,
                   data.terminal.tileset);

    data.entities.capacity = data.max_monsters_per_room;
    data.entities.data =
      malloc(sizeof(*data.entities.data) * data.entities.capacity);
    ASSERT_M(data.entities.data != NULL);
    data.entities.len = 0;
    rg_fighter fighter = { .hp = 30, .defence = 2, .power = 5 };
    ARRAY_PUSH(&data.entities,
               ((rg_entity){ .x = 0,
                             .y = 0,
                             .ch = '@',
                             .color = WHITE,
                             .name = "player",
                             .blocks = true,
                             .type = ENTITY_PLAYER,
                             .fighter = fighter }));
    data.player = data.entities.len - 1;

    map_create(&data.game_map,
               data.map_width,
               data.map_height,
               data.room_min_size,
               data.room_max_size,
               data.max_rooms,
               data.max_monsters_per_room,
               &data.entities,
               data.player);

    fov_map_create(&data.fov_map, data.map_width, data.map_height);
    for (int y = 0; y < data.map_height; y++)
    {
        for (int x = 0; x < data.map_width; x++)
        {
            rg_tile* tile = map_get_tile(&data.game_map, x, y);
            fov_map_set_props(
              &data.fov_map, x, y, !tile->block_sight, !tile->blocked);
        }
    }

    // first draw before waitevent
    game_recompute_fov(&data);
    draw(&app, &data);

    while (app.running)
    {
        SDL_Event event = { 0 };
        SDL_WaitEvent(&event);

        Uint64 frame_start = SDL_GetPerformanceCounter();

        update(&app, &event, &data);
        draw(&app, &data);

        Uint64 frame_end = SDL_GetPerformanceCounter();
        double dt = (frame_end - frame_start) /
                    (double)SDL_GetPerformanceFrequency() * 1000.0;

        Uint32 sleep_time = (Uint32)floor(DESIRED_FRAME_RATE - dt);
        const Uint32 max_sleep_time = 1000;
        if (sleep_time > max_sleep_time) sleep_time = max_sleep_time;
        // SLEEP
        SDL_Delay(sleep_time);
    }

    map_destroy(&data.game_map);
    free(data.entities.data);
    console_destroy(&data.console);
    terminal_destroy(&data.terminal);
    tileset_destroy(&data.tileset);

    app_destroy(&app);
    return 0;
}