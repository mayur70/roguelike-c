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
#include "panic.h"
#include "terminal.h"
#include "tileset.h"
#include "turn_log.h"
#include "types.h"

#define DESIRED_FPS 20.0
#define DESIRED_FRAME_RATE ((1.0 / DESIRED_FPS) * 1000.0)

int entity_sort_by_render_order(const rg_entity* lhs, const rg_entity* rhs)
{
    return lhs->render_order - rhs->render_order;
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
    for (int i = 0; i < data->items.len; i++)
    {
        const rg_item* e = &data->items.data[i];
        rg_fov_map* fov_map = &data->fov_map;
        if (e->x == x && e->y == y && e->visible_on_map &&
            fov_map_is_in_fov(fov_map, x, y))
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
    event_dispatch(app,
                   event,
                   &action,
                   &data->mouse_position,
                   data->tileset.tile_size,
                   data->tileset.tile_size);

    if (action.type == ACTION_QUIT)
    {
        // App Quit signal
        app->running = false;
        return;
    }

    switch (data->game_state)
    {
    case ST_TURN_PLAYER:
        state_player_turn(event, &action, data);
        break;
    case ST_SHOW_INVENTORY:
        state_inventory_turn(event, &action, data);
        break;
    case ST_DROP_INVENTORY:
        state_inventory_drop_turn(event, &action, data);
        break;
    case ST_TURN_ENEMY:
        state_enemy_turn(event, &action, data);
        break;
    case ST_TURN_PLAYER_DEAD:
        state_player_dead_turn(event, &action, data);
        break;
    case ST_TARGETING:
        state_targeting_turn(event, &action, data);
        break;
    default:
        break;
    }

    if (action.type == ACTION_QUIT)
    {
        // App Quit signal
        app->running = false;
        return;
    }

    if (data->recompute_fov) game_recompute_fov(data);
}

void draw(rg_app* app, rg_game_state_data* data)
{
    rg_map* game_map = &data->game_map;
    rg_fov_map* fov_map = &data->fov_map;
    rg_console* console = &data->console;
    rg_entity_array* entities = &data->entities;
    rg_items* items = &data->items;

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

    for (int i = 0; i < items->len; i++)
    {
        const rg_item* e = &items->data[i];
        if (!e->visible_on_map) continue;
        if (fov_map_is_in_fov(fov_map, e->x, e->y))
            console_print(console, e->x, e->y, e->ch, e->color);
    }

    for (int i = 0; i < entities->len; i++)
    {
        const rg_entity* e = &entities->data[i];
        if (fov_map_is_in_fov(fov_map, e->x, e->y))
            console_print(console, e->x, e->y, e->ch, e->color);
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

    //-----Menu----------------
    int menu_height;
    int menu_width = 50;
    if (data->game_state == ST_SHOW_INVENTORY)
    {
        console_begin(&data->menu);
        console_clear(&data->menu, ((SDL_Color){ 0, 0, 0, 0 }));
        const char* header =
          "Press the key next to an item to use it, or Esc to cancel.";
        inventory_draw(&data->menu,
                       header,
                       &data->items,
                       &data->inventory,
                       menu_width,
                       &menu_height);
        console_end(&data->menu);
    }
    else if (data->game_state == ST_DROP_INVENTORY)
    {
        console_begin(&data->menu);
        console_clear(&data->menu, ((SDL_Color){ 0, 0, 0, 0 }));
        const char* header =
          "Press the key next to an item to drop it, or Esc to cancel.";
        inventory_draw(&data->menu,
                       header,
                       &data->items,
                       &data->inventory,
                       menu_width,
                       &menu_height);
        console_end(&data->menu);
    }

    ///-----Screen/Window---------------
    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    SDL_RenderClear(app->renderer);
    console_flush(&data->console, 0, 0);
    console_flush(&data->panel, 0, data->panel_y);

    if (data->game_state == ST_SHOW_INVENTORY ||
        data->game_state == ST_DROP_INVENTORY)
    {
        int x =
          (int)((data->screen_width / (double)2) - (menu_width / (double)2));
        int y =
          (int)((data->screen_height / (double)2) - (menu_height / (double)2));
        SDL_Rect r = { x * data->menu.tileset->tile_size,
                       y * data->menu.tileset->tile_size,
                       menu_width * data->menu.tileset->tile_size,
                       menu_height * data->menu.tileset->tile_size };

        SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 178);
        SDL_RenderFillRect(app->renderer, &r);
        console_flush(&data->menu, x, y);
    }
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
    data.max_items_per_room = 2;
    data.fov_light_walls = true;
    data.fov_radius = 10;
    data.game_state = ST_TURN_PLAYER;
    data.prev_state = ST_TURN_PLAYER;
    data.target_selected = false;
    data.targeting_item = NULL;
    data.target_x = -1;
    data.target_y = -1;

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
    console_create(&data.menu,
                   app.renderer,
                   data.screen_width,
                   data.screen_height,
                   &data.tileset);
    data.entities.capacity = data.max_monsters_per_room;
    data.entities.data =
      malloc(sizeof(*data.entities.data) * data.entities.capacity);
    ASSERT_M(data.entities.data != NULL);
    data.entities.len = 0;
    data.items.capacity = data.max_items_per_room;
    data.items.data = malloc(sizeof(*data.items.data) * data.items.capacity);
    ASSERT_M(data.items.data != NULL);
    data.items.len = 0;
    rg_fighter fighter = { .hp = 30, .defence = 2, .power = 5, .max_hp = 30 };
    ARRAY_PUSH(&data.entities,
               ((rg_entity){
                 .x = 0,
                 .y = 0,
                 .ch = '@',
                 .color = WHITE,
                 .name = "Player",
                 .blocks = true,
                 .type = ENTITY_PLAYER,
                 .fighter = fighter,
                 .render_order = RENDER_ORDER_ACTOR,
               }));
    data.player = data.entities.len - 1;

    inventory_create(&data.inventory, 26);

    turn_logs_create(&data.logs, data.message_width, data.message_height);

    map_create(&data.game_map,
               data.map_width,
               data.map_height,
               data.room_min_size,
               data.room_max_size,
               data.max_rooms,
               data.max_monsters_per_room,
               data.max_items_per_room,
               &data.entities,
               &data.items,
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
    data.mouse_position.x = 0;
    data.mouse_position.y = 0;
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

    inventory_destroy(&data.inventory);
    turn_logs_destroy(&data.logs);
    map_destroy(&data.game_map);
    free(data.entities.data);
    console_destroy(&data.menu);
    console_destroy(&data.console);
    console_destroy(&data.panel);
    terminal_destroy(&data.terminal);
    tileset_destroy(&data.tileset);

    app_destroy(&app);
    return 0;
}