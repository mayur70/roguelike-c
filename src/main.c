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

#define DESIRED_FPS 20.0
#define DESIRED_FRAME_RATE ((1.0 / DESIRED_FPS) * 1000.0)

typedef struct rg_line_data
{
    int step_x, step_y;
    int e;
    int delta_x, delta_y;
    int orig_x, orig_y;
    int dest_x, dest_y;
} rg_line_data;

typedef struct rg_fov_map_cell
{
    bool transparent;
    bool walkable;
    bool fov;
} rg_fov_map_cell;

typedef struct rg_fov_map
{
    int width;
    int height;
    rg_fov_map_cell *cells;
} rg_fov_map;

typedef struct rg_game_state
{
    int screen_width;
    int screen_height;
    int map_width;
    int map_height;
    int room_max_size;
    int room_min_size;
    int max_rooms;
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

void line_init(int x0, int y0, int x1, int y1, rg_line_data *data)
{
    data->orig_x = x0;
    data->orig_y = y0;
    data->dest_x = x1;
    data->dest_y = y1;
    data->delta_x = x1 - x0;
    data->delta_y = y1 - y0;

    if (data->delta_x > 0)
        data->step_x = 1;
    else if (data->delta_x < 0)
        data->step_x = -1;
    else
        data->step_x = 0;

    if (data->delta_y > 0)
        data->step_y = 1;
    else if (data->delta_y < 0)
        data->step_y = -1;
    else
        data->step_y = 0;

    if (data->step_x * data->delta_x > data->step_y * data->delta_y)
        data->e = data->step_x * data->delta_x;
    else
        data->e = data->step_y * data->delta_y;
    data->delta_x *= 2;
    data->delta_y *= 2;
}

bool line_step(int *x, int *y, rg_line_data *data)
{
    if (data->step_x * data->delta_x > data->step_y * data->delta_y)
    {
        if (data->orig_x == data->dest_x)
            return true;
        data->orig_x += data->step_x;
        data->e -= data->step_y * data->delta_y;
        if (data->e < 0)
        {
            data->orig_y += data->step_y;
            data->e += data->step_x * data->delta_x;
        }
    }
    else
    {
        if (data->orig_y == data->dest_y)
            return true;
        data->orig_y += data->step_y;
        data->e -= data->step_x * data->delta_x;
        if (data->e < 0)
        {
            data->orig_x += data->step_x;
            data->e += data->step_y * data->delta_y;
        }
    }
    *x = data->orig_x;
    *y = data->orig_y;
    return false;
}

void fov_map_create(rg_fov_map *m, int w, int h)
{
    memset(m, 0, sizeof(*m));
    m->width = w;
    m->height = h;
    m->cells = calloc(m->width * m->height, sizeof(*m->cells));
}

void fov_map_destroy(rg_fov_map *m)
{
    if (m == NULL)
        return;
    free(m->cells);
}

bool fov_map_in_bounds(rg_fov_map *m, int x, int y)
{
    if (m == NULL)
        return false;
    return x >= 0 && x < m->width && y >= 0 && y < m->height;
}

bool fov_map_is_in_fov(rg_fov_map *m, int x, int y)
{
    if (!fov_map_in_bounds(m, x, y))
        return false;
    return m->cells[x + y * m->width].fov;
}

void fov_map_set_props(rg_fov_map *m,
                       int x,
                       int y,
                       bool transparent,
                       bool walkable)
{
    if (!fov_map_in_bounds(m, x, y))
        return;
    size_t idx = x + y * m->width;
    m->cells[idx].transparent = transparent;
    m->cells[idx].walkable = walkable;
}

void fov_map_cast_ray(rg_fov_map *m,
                      int orig_x,
                      int orig_y,
                      int dest_x,
                      int dest_y,
                      int radius_squared,
                      bool light_walls)
{
    rg_line_data data;
    int current_x;
    int current_y;
    line_init(orig_x, orig_y, dest_x, dest_y, &data);
    while (!line_step(&current_x, &current_y, &data))
    {
        if (!fov_map_in_bounds(m, current_x, current_y))
        {
            return; // Out of bounds.
        }
        if (radius_squared > 0)
        {
            const int current_radius =
                (current_x - orig_x) * (current_x - orig_x) + (current_y - orig_y) * (current_y - orig_y);
            if (current_radius > radius_squared)
            {
                return; // Outside of radius.
            }
        }
        const int map_index = current_x + current_y * m->width;
        if (!m->cells[map_index].transparent)
        {
            if (light_walls)
            {
                m->cells[map_index].fov = true;
            }
            return; // Blocked by wall.
        }
        // Tile is transparent.
        m->cells[map_index].fov = true;
    }
}

void fov_map_postprocess_quad(rg_fov_map *m,
                              int x0,
                              int y0,
                              int x1,
                              int y1,
                              int dx,
                              int dy)
{
    if (abs(dx) != 1 || abs(dy) != 1)
    {
        return; // Bad parameters.
    }
    for (int cx = x0; cx <= x1; cx++)
    {
        for (int cy = y0; cy <= y1; cy++)
        {
            const int x2 = cx + dx;
            const int y2 = cy + dy;
            const int offset = cx + cy * m->width;
            if (offset < (m->width * m->height) && m->cells[offset].fov == 1 && m->cells[offset].transparent)
            {
                if (x2 >= x0 && x2 <= x1)
                {
                    const int offset2 = x2 + cy * m->width;
                    if (offset2 < (m->width * m->height) && !m->cells[offset2].transparent)
                    {
                        m->cells[offset2].fov = true;
                    }
                }
                if (y2 >= y0 && y2 <= y1)
                {
                    const int offset2 = cx + y2 * m->width;
                    if (offset2 < (m->width * m->height) && !m->cells[offset2].transparent)
                    {
                        m->cells[offset2].fov = true;
                    }
                }
                if (x2 >= x0 && x2 <= x1 && y2 >= y0 && y2 <= y1)
                {
                    const int offset2 = x2 + y2 * m->width;
                    if (offset2 < (m->width * m->height) && !m->cells[offset2].transparent)
                    {
                        m->cells[offset2].fov = true;
                    }
                }
            }
        }
    }
}

void fov_map_postprocess(rg_fov_map *m,
                         int pov_x,
                         int pov_y,
                         int radius)
{
    int x_min = 0;
    int y_min = 0;
    int x_max = m->width;
    int y_max = m->height;
    if (radius > 0)
    {
        x_min = MAX(x_min, pov_x - radius);
        y_min = MAX(y_min, pov_y - radius);
        x_max = MIN(x_max, pov_x + radius + 1);
        y_max = MIN(y_max, pov_y + radius + 1);
    }
    fov_map_postprocess_quad(m, x_min, y_min, pov_x, pov_y, -1, -1);
    fov_map_postprocess_quad(m, pov_x, y_min, x_max - 1, pov_y, 1, -1);
    fov_map_postprocess_quad(m, x_min, pov_y, pov_x, y_max - 1, -1, 1);
    fov_map_postprocess_quad(m, pov_x, pov_y, x_max - 1, y_max - 1, 1, 1);
}

void fov_map_compute(rg_fov_map *m,
                     int pov_x,
                     int pov_y,
                     int max_radius,
                     bool light_walls)
{
    if (m == NULL)
        return;
    if (!fov_map_in_bounds(m, pov_x, pov_y))
        return;
    for (int i = 0; i < m->width * m->height; i++)
        m->cells[i].fov = false;

    int x_min = 0;
    int y_min = 0;
    int x_max = m->width;
    int y_max = m->height;
    if (max_radius > 0)
    {
        x_min = MAX(x_min, pov_x - max_radius);
        y_min = MAX(y_min, pov_y - max_radius);
        x_max = MIN(x_max, pov_x + max_radius + 1);
        y_max = MIN(y_max, pov_y + max_radius + 1);
    }

    int idx = pov_x + pov_y * m->width;

    m->cells[idx].fov = true;
    // Cast rays along the perimeter.
    const int radius_squared = max_radius * max_radius;
    for (int x = x_min; x < x_max; ++x)
    {
        fov_map_cast_ray(m, pov_x, pov_y, x, y_min, radius_squared, light_walls);
    }
    for (int y = y_min + 1; y < y_max; ++y)
    {
        fov_map_cast_ray(m, pov_x, pov_y, x_max - 1, y, radius_squared, light_walls);
    }
    for (int x = x_max - 2; x >= x_min; --x)
    {
        fov_map_cast_ray(m, pov_x, pov_y, x, y_max - 1, radius_squared, light_walls);
    }
    for (int y = y_max - 2; y > y_min; --y)
    {
        fov_map_cast_ray(m, pov_x, pov_y, x_min, y, radius_squared, light_walls);
    }
    if (light_walls)
    {
        fov_map_postprocess(m, pov_x, pov_y, max_radius);
    }
}

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
            tile->explored = true;
            bool is_wall = tile->block_sight;
            if (visible)
            {
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
        rg_entity *e = &entities->data[i];
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

    state.entities.capacity = 2;
    state.entities.data = malloc(sizeof(rg_entity) * 2);
    state.entities.len = 0;
    ARRAY_PUSH(&state.entities, ((rg_entity){state.screen_width / 2, state.screen_height / 2, '@', WHITE}));
    ARRAY_PUSH(&state.entities, ((rg_entity){(state.screen_width / 2) - 5, state.screen_height / 2, '@', YELLOW}));
    state.player = &state.entities.data[0];
    rg_entity *npc = &state.entities.data[1];

    map_create(&state.game_map,
               state.map_width,
               state.map_height,
               state.room_min_size,
               state.room_max_size,
               state.max_rooms,
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
    draw(&app, &state);

    while (app.running)
    {
        SDL_Event event = {0};
        SDL_WaitEvent(&event);

        Uint64 frame_start = SDL_GetPerformanceCounter();

        update(&app, &event, &state);
        draw(&app, &state);

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