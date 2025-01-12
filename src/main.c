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
#include "panic.h"
#include "terminal.h"
#include "tileset.h"
#include "turn_log.h"
#include "types.h"

#define DESIRED_FPS 20.0
#define DESIRED_FRAME_RATE ((1.0 / DESIRED_FPS) * 1000.0)

typedef enum rg_game_state
{
    ST_TURN_PLAYER,
    ST_TURN_ENEMY,
    ST_TURN_PLAYER_DEAD
} rg_game_state;

typedef struct rg_game_state_data
{
    int screen_width;
    int screen_height;
    int bar_width;
    int panel_height;
    int panel_y;
    int message_x;
    int message_width;
    int message_height;
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
    rg_console panel;
    rg_entity_array entities;
    rg_entity_id player;
    rg_map game_map;
    rg_fov_map fov_map;
    bool recompute_fov;
    rg_game_state game_state;
    rg_turn_logs logs;
    SDL_Point mouse_position;
} rg_game_state_data;

float distance_between(rg_entity* a, rg_entity* b)
{
    int dx = b->x - a->x;
    int dy = b->y - a->y;
    return (float)sqrt(dx * dx + dy * dy);
}

int entity_sort_by_render_order(const rg_entity* lhs, const rg_entity* rhs)
{
    return lhs->render_order - rhs->render_order;
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

    astar_path* path = astar_path_new_using_map(&map, (float)1.41);
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
                          rg_entity_array* entities,
                          rg_turn_logs* logs,
                          rg_entity** dead_entity)
{
    ASSERT_M(e != NULL);
    ASSERT_M(target != NULL);
    ASSERT_M(fov_map != NULL);
    ASSERT_M(game_map != NULL);
    ASSERT_M(entities != NULL);
    ASSERT_M(logs != NULL);
    ASSERT_M(dead_entity != NULL);
    *dead_entity = NULL;
    if (fov_map_is_in_fov(fov_map, e->x, e->y))
    {
        int distance = (int)distance_between(e, target);
        if (distance >= 2)
        {
            entity_move_astar(e, target, game_map, entities);
        }
        else if (target->fighter.hp > 0)
            entity_attack(e, target, logs, dead_entity);
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

void render_bar(rg_console* c,
                int x,
                int y,
                int total_width,
                const char* name,
                int value,
                int maximum,
                SDL_Color bar_color,
                SDL_Color back_color)
{
    int bar_width = (int)((float)(value / (float)maximum) * (float)total_width);

    console_fill_rect(c, x, y, total_width, 1, back_color);
    if (bar_width > 0) console_fill_rect(c, x, y, bar_width, 1, bar_color);

    const char* fmt = "%s: %d/%d";
    int sz = snprintf(NULL, 0, fmt, name, value, maximum);
    char* buf = malloc(sizeof(char) * (sz + 1));
    snprintf(buf, sz + 1, fmt, name, value, maximum);
    console_print_txt(c, x + total_width / 2 - (sz / 2), y, buf, WHITE);
    free(buf);
}

char* get_names_under_mouse(rg_game_state_data* data)
{
    const int x = data->mouse_position.x;
    const int y = data->mouse_position.y;
    ASSERT_M(x >= 0 && x <= 800);
    char* buf = NULL;
    size_t buf_len = 0;
    for (int i = 0; i < data->entities.len; i++)
    {
        const rg_entity* e = &data->entities.data[i];
        rg_fov_map* fov_map = &data->fov_map;
        if (e->x == x && e->y == y && fov_map_is_in_fov(fov_map, x, y))
        {
            if (buf == NULL)
            {
                buf_len = strlen(e->name);
                buf = strdup(e->name);
            }
            else
            {
                size_t sz = strlen(e->name);
                size_t newsize = sizeof(char) * (buf_len + sz + 3);
                buf = realloc(buf, newsize);
                ASSERT_M(buf != NULL);
                buf[buf_len] = ',';
                buf[buf_len + 1] = ' ';
                buf_len += 2;
                memcpy(buf + buf_len, e->name, sz);
                buf_len += sz;
                buf[buf_len] = '\0';
            }
        }
    }
    return buf;
}

void update(rg_app* app, SDL_Event* event, rg_game_state_data* data)
{
    rg_map* game_map = &data->game_map;
    rg_fov_map* fov_map = &data->fov_map;
    rg_action action;
    event_dispatch(event, &action, &data->mouse_position);
    int wx, wy;
    SDL_GetMouseState(&wx, &wy);
    float lx, ly;
    SDL_RenderWindowToLogical(app->renderer, wx, wy, &lx, &ly);

    int rw, rh;
    SDL_RenderGetLogicalSize(app->renderer, &rw, &rh);
    if (lx < 0) data->mouse_position.x = 0;
    else if (lx > rw)
        data->mouse_position.x = rw;
    else
        data->mouse_position.x = (int)lx;
    if (ly < 0) data->mouse_position.y = 0;
    else if (ly > rh)
        data->mouse_position.y = rh;
    else
        data->mouse_position.y = (int)ly;

    data->mouse_position.x = data->mouse_position.x / data->tileset.tile_size;
    data->mouse_position.y = data->mouse_position.y / data->tileset.tile_size;

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
            rg_entity* player = &data->entities.data[data->player];
            if (target == NULL)
            {
                entity_move(player, action.dx, action.dy);
                data->recompute_fov = true;
            }
            else
            {
                rg_entity* dead_entity;
                entity_attack(player, target, &data->logs, &dead_entity);
                if (dead_entity != NULL)
                {
                    entity_kill(dead_entity, &data->logs);
                    if (dead_entity == player)
                    {
                        data->game_state = ST_TURN_PLAYER_DEAD;
                    }
                }
            }
            if (data->game_state != ST_TURN_PLAYER_DEAD)
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
            if (i == data->player) continue;

            rg_entity* e = &data->entities.data[i];
            if (e->fighter.hp <= 0) continue;

            rg_entity* player = &data->entities.data[data->player];
            if (e->type == ENTITY_BASIC_MONSTER)
            {
                rg_entity* dead_entity;
                basic_monster_update(e,
                                     player,
                                     &data->fov_map,
                                     &data->game_map,
                                     &data->entities,
                                     &data->logs,
                                     &dead_entity);

                if (dead_entity != NULL)
                {
                    entity_kill(dead_entity, &data->logs);
                    if (dead_entity == player)
                    {
                        data->game_state = ST_TURN_PLAYER_DEAD;
                    }
                }
                if (data->game_state == ST_TURN_PLAYER_DEAD) break;
            }
        }
        if (data->game_state != ST_TURN_PLAYER_DEAD)
            data->game_state = ST_TURN_PLAYER;
    }
}

void draw(rg_app* app, rg_game_state_data* data)
{
    rg_map* game_map = &data->game_map;
    rg_fov_map* fov_map = &data->fov_map;
    rg_console* console = &data->console;
    rg_entity_array* entities = &data->entities;

    qsort(entities->data,
          entities->len,
          sizeof(*entities->data),
          entity_sort_by_render_order);
    for (int i = 0; i < entities->len; i++)
        if (entities->data[i].type == ENTITY_PLAYER)
        {
            data->player = i;
            break;
        }

    ///-----GameWorld---------------
    console_begin(&data->console);
    console_clear(&data->console, BLACK);
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
    console_end(&data->console);

    ///-----UI---------------
    console_begin(&data->panel);
    console_clear(&data->panel, BLACK);

    const rg_entity* player = &entities->data[data->player];
    render_bar(&data->panel,
               1,
               1,
               data->bar_width,
               "HP",
               player->fighter.hp,
               player->fighter.max_hp,
               LIGHT_RED,
               DARK_RED);
    char* names = get_names_under_mouse(data);
    if (names != NULL)
    {
        console_print_txt(&data->panel, 1, 0, names, LIGHT_GREY);
        free(names);
    }
    for (int i = 0, y = 1; i < data->logs.len; i++, y++)
    {
        rg_turn_log_entry* entry = &data->logs.data[i];
        console_print_txt(
          &data->panel, data->message_x, y, entry->text, entry->color);
    }
    console_end(&data->panel);

    ///-----Screen/Window---------------
    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    SDL_RenderClear(app->renderer);
    console_flush(&data->console, 0, 0);
    console_flush(&data->panel, 0, data->panel_y);
    SDL_RenderPresent(app->renderer);

    data->recompute_fov = false;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    // SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
#endif

    srand((unsigned int)time(NULL));

    rg_app app;
    app_create(&app);

    rg_game_state_data data;
    data.screen_width = 80;
    data.screen_height = 50;
    data.bar_width = 20;
    data.panel_height = 7;
    data.panel_y = data.screen_height - data.panel_height;
    data.message_x = data.bar_width + 2;
    data.message_width = data.screen_width - data.bar_width - 2;
    data.message_height = data.panel_height - 1;
    data.map_width = 80;
    data.map_height = 43;
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
                   data.screen_width,
                   data.screen_height,
                   data.terminal.tileset);
    console_create(&data.panel,
                   app.renderer,
                   data.screen_width,
                   data.panel_height,
                   &data.tileset);

    data.entities.capacity = data.max_monsters_per_room;
    data.entities.data =
      malloc(sizeof(*data.entities.data) * data.entities.capacity);
    ASSERT_M(data.entities.data != NULL);
    data.entities.len = 0;
    rg_fighter fighter = { .hp = 30, .defence = 2, .power = 5, .max_hp = 30 };
    ARRAY_PUSH(&data.entities,
               ((rg_entity){ .x = 0,
                             .y = 0,
                             .ch = '@',
                             .color = WHITE,
                             .name = "Player",
                             .blocks = true,
                             .type = ENTITY_PLAYER,
                             .fighter = fighter,
                             .render_order = RENDER_ORDER_ACTOR }));
    data.player = data.entities.len - 1;

    turn_logs_create(&data.logs, data.message_width, data.message_height);

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
    SDL_Event event = { 0 };
    update(&app, &event, &data);
    draw(&app, &data);

    while (app.running)
    {
        SDL_WaitEvent(&event);

        Uint64 frame_start = SDL_GetPerformanceCounter();

        // turn_logs_clear(&data.logs);
        update(&app, &event, &data);
        draw(&app, &data);
        // turn_logs_print(&data.logs);

        Uint64 frame_end = SDL_GetPerformanceCounter();
        double dt = (frame_end - frame_start) /
                    (double)SDL_GetPerformanceFrequency() * 1000.0;

        Uint32 sleep_time = (Uint32)floor(DESIRED_FRAME_RATE - dt);
        const Uint32 max_sleep_time = 1000;
        if (sleep_time > max_sleep_time) sleep_time = max_sleep_time;
        // SLEEP
        SDL_Delay(sleep_time);
    }

    turn_logs_destroy(&data.logs);
    map_destroy(&data.game_map);
    free(data.entities.data);
    console_destroy(&data.console);
    console_destroy(&data.panel);
    terminal_destroy(&data.terminal);
    tileset_destroy(&data.tileset);

    app_destroy(&app);
    return 0;
}