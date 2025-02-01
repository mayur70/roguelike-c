#include "gameplay_state.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "array.h"
#include "astar.h"
#include "color.h"
#include "savefile.h"

#define SAVEFILE_NAME "savefile.data"
//------- internal functions ------------//

static void entity_move_towards(rg_entity* e,
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

static void entity_move_astar(rg_entity* e,
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

static void basic_monster_update(rg_entity* e,
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
        int distance = (int)entity_get_distance(e, target);
        if (distance >= 2)
        {
            entity_move_astar(e, target, game_map, entities);
        }
        else if (target->fighter.hp > 0)
            entity_attack(e, target, logs, dead_entity);
    }
}

static void confused_monster_update(rg_entity* e,
                                    rg_entity* target,
                                    rg_fov_map* fov_map,
                                    rg_map* game_map,
                                    rg_entity_array* entities,
                                    rg_turn_logs* logs,
                                    rg_entity** dead_entity)
{}

static void handle_player_movement(const rg_action* action,
                                   rg_game_state_data* data)
{
    rg_map* game_map = &data->game_map;
    int destination_x = data->entities.data[data->player].x + action->dx;
    int destination_y = data->entities.data[data->player].y + action->dy;
    if (!map_is_blocked(game_map, destination_x, destination_y))
    {
        rg_entity* target = NULL;
        entity_get_at_loc(
          &data->entities, destination_x, destination_y, &target);
        rg_entity* player = &data->entities.data[data->player];
        if (target == NULL)
        {
            entity_move(player, action->dx, action->dy);
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

static void handle_player_pickup(const rg_action* action,
                                 rg_game_state_data* data)
{
    rg_entity* player = &data->entities.data[data->player];
    bool status = false;
    for (int i = 0; i < data->items.len; i++)
    {
        rg_item* e = &data->items.data[i];
        if (e->x == player->x && e->y == player->y && e->visible_on_map)
        {
            inventory_add_item(&data->inventory, &data->items, i, &data->logs);
            status = true;
            data->game_state = ST_TURN_ENEMY;
            break;
        }
    }
    if (!status)
    {
        const char* fmt = "There is nothing here to pickup";
        int len = snprintf(NULL, 0, fmt);
        char* buf = malloc(sizeof(char) * (len + 1));
        snprintf(buf, len, fmt);
        rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE,
                                    .text = buf,
                                    .color = YELLOW };
        turn_logs_push(&data->logs, &entry);
    }
}

static void handle_entity_idle()
{
    // no-op
}

static void handle_entity_follow_player(rg_entity* e,
                                        rg_entity* target,
                                        rg_fov_map* fov_map,
                                        rg_map* game_map,
                                        rg_entity_array* entities,
                                        rg_turn_logs* logs,
                                        rg_entity** dead_entity)
{
    basic_monster_update(
      e, target, fov_map, game_map, entities, logs, dead_entity);
}

static void handle_entity_attack() {}

static void handle_entity_confused(rg_entity* e,
                                   rg_entity* target,
                                   rg_fov_map* fov_map,
                                   rg_map* game_map,
                                   rg_entity_array* entities,
                                   rg_turn_logs* logs,
                                   rg_entity** dead_entity)
{
    if (e->state.data.confused.num_turns > 0)
    {
        int x = RAND_INT(0, 2) - 1;
        int y = RAND_INT(0, 2) - 1;

        if (x != e->x && y != e->y)
            entity_move_towards(e, x, y, game_map, entities);
        e->state.data.confused.num_turns--;
    }
    else
    {
        e->state.type = e->state.data.confused.prev_state;

        const char* fmt = "The %s is no longer confused!";
        int len = snprintf(NULL, 0, fmt, e->name);
        char* buf = malloc(sizeof(char) * (len + 1));
        snprintf(buf, len, fmt, e->name);
        rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE,
                                    .text = buf,
                                    .color = RED };
        turn_logs_push(logs, &entry);
    }
}

static void enemy_state_update(rg_entity* e,
                               rg_entity* target,
                               rg_fov_map* fov_map,
                               rg_map* game_map,
                               rg_entity_array* entities,
                               rg_turn_logs* logs,
                               rg_entity** dead_entity)
{
    switch (e->state.type)
    {
    case ENTITY_STATE_NONE:
        // handle_entity_idle();
        // no-op
        break;
    case ENTITY_STATE_FOLLOW_PLAYER:
        handle_entity_follow_player(
          e, target, fov_map, game_map, entities, logs, dead_entity);
        break;
    case ENTITY_STATE_ATTACK:
        // no-op
        // handle_entity_attack();
        break;
    case ENTITY_STATE_CONFUSED:
        handle_entity_confused(
          e, target, fov_map, game_map, entities, logs, dead_entity);
        break;
    }
}

static void handle_inventory_input(const SDL_Event* event,
                                   rg_action* action,
                                   rg_game_state_data* data)
{
    action->index = -1;
    switch (event->type)
    {
    case SDL_KEYDOWN:
    {
        int index = (event->key.keysym.sym) - 'a';
        if (index >= 0)
        {
            action->type = ACTION_SELECT;
            action->index = index;
            return;
        }
        switch (event->key.keysym.sym)
        {
        case SDLK_ESCAPE:
            action->type = ACTION_ESCAPE;
            break;
        }
        break;
    }
    }
}

static void handle_player_input(const SDL_Event* event,
                                rg_action* action,
                                rg_game_state_data* data)
{
    // Default value
    // action->type = ACTION_NONE;
    switch (event->type)
    {
    case SDL_KEYDOWN:
    {
        switch (event->key.keysym.sym)
        {
        case SDLK_UP:
        case SDLK_k:
            action->type = ACTION_MOVEMENT;
            action->dx = 0;
            action->dy = -1;
            break;
        case SDLK_DOWN:
        case SDLK_j:
            action->type = ACTION_MOVEMENT;
            action->dx = 0;
            action->dy = 1;
            break;
        case SDLK_LEFT:
        case SDLK_h:
            action->type = ACTION_MOVEMENT;
            action->dx = -1;
            action->dy = 0;
            break;
        case SDLK_RIGHT:
        case SDLK_l:
            action->type = ACTION_MOVEMENT;
            action->dx = 1;
            action->dy = 0;
            break;
        case SDLK_y:
            action->type = ACTION_MOVEMENT;
            action->dx = -1;
            action->dy = -1;
            break;
        case SDLK_u:
            action->type = ACTION_MOVEMENT;
            action->dx = 1;
            action->dy = -1;
            break;
        case SDLK_b:
            action->type = ACTION_MOVEMENT;
            action->dx = -1;
            action->dy = 1;
            break;
        case SDLK_n:
            action->type = ACTION_MOVEMENT;
            action->dx = 1;
            action->dy = 1;
            break;
        case SDLK_g:
            action->type = ACTION_PICKUP;
            break;
        case SDLK_i:
            action->type = ACTION_SHOW_INVENTORY;
            break;
        case SDLK_d:
            action->type = ACTION_DROP_INVENTORY;
            break;
        case SDLK_ESCAPE:
            action->type = ACTION_ESCAPE;
            break;
        case SDLK_RETURN:
            action->type = ACTION_SELECT;
            break;
        }
        break;
    }
    }
}

static void handle_player_dead_input(const SDL_Event* event,
                                     rg_action* action,
                                     rg_game_state_data* data)
{
    switch (event->type)
    {
    case SDL_KEYDOWN:
    {
        switch (event->key.keysym.sym)
        {
        case SDLK_i:
            action->type = ACTION_SHOW_INVENTORY;
            break;
        case SDLK_ESCAPE:
            action->type = ACTION_ESCAPE;
            break;
        }
        break;
    }
    }
}

static void handle_targeting_input(const SDL_Event* event,
                                   rg_action* action,
                                   rg_game_state_data* data)
{
    Uint32 btn = SDL_GetMouseState(NULL, NULL);
    if ((btn & SDL_BUTTON_LMASK) != 0)
    {
        data->target_x = data->mouse_position.x;
        data->target_y = data->mouse_position.y;
        action->type = ACTION_TARGET_SELECTED;
        data->target_selected = true;
        return;
    }
    else if ((btn & SDL_BUTTON_RMASK) != 0)
    {
        action->type = ACTION_ESCAPE;
        data->target_selected = false;
        return;
    }
    switch (event->type)
    {
    case SDL_KEYDOWN:
    {
        switch (event->key.keysym.sym)
        {
        case SDLK_ESCAPE:
            action->type = ACTION_ESCAPE;
            break;
        }
        break;
    }
    }
}

static int entity_sort_by_render_order(const rg_entity* lhs,
                                       const rg_entity* rhs)
{
    return lhs->render_order - rhs->render_order;
}

static void render_bar(rg_console* c,
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

static char* get_names_under_mouse(rg_game_state_data* data)
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

static void game_level_create(rg_game_state_data* data, int level)
{
    map_create(&data->game_map,
               data->map_width,
               data->map_height,
               level,
               data->room_min_size,
               data->room_max_size,
               data->max_rooms,
               data->max_monsters_per_room,
               data->max_items_per_room,
               &data->entities,
               &data->items,
               data->player);

    fov_map_create(&data->fov_map, data->map_width, data->map_height);
    for (int y = 0; y < data->map_height; y++)
    {
        for (int x = 0; x < data->map_width; x++)
        {
            rg_tile* tile = map_get_tile(&data->game_map, x, y);
            fov_map_set_props(
              &data->fov_map, x, y, !tile->block_sight, !tile->blocked);
        }
    }

    // first draw before waitevent
    fov_map_compute(&data->fov_map,
                    data->entities.data[data->player].x,
                    data->entities.data[data->player].y,
                    data->fov_radius,
                    data->fov_light_walls);
}

static void game_next_level(rg_game_state_data* data)
{
    int level = data->game_map.level;
    if (data->player != 0)
    {
        rg_entity* p = &data->entities.data[data->player];
        data->player = 0;
        memcpy(data->entities.data, p, sizeof(*p));
    }
    data->entities.len = 1;
    rg_entity* p = &data->entities.data[data->player];
    p->fighter.hp = (int)floor(p->fighter.max_hp / 2.0);
    map_destroy(&data->game_map);
    fov_map_destroy(&data->fov_map);

    game_level_create(data, level + 1);
}

static void with_defaults(rg_game_state_data* data, rg_app* app)
{
    data->screen_width = app->screen_width;
    data->screen_height = app->screen_height;

    data->bar_width = 20;
    data->panel_height = 7;
    data->panel_y = data->screen_height - data->panel_height;
    data->message_x = data->bar_width + 2;
    data->message_width = data->screen_width - data->bar_width - 2;
    data->message_height = data->panel_height - 1;
    data->map_width = 80;
    data->map_height = 43;
    data->room_max_size = 10;
    data->room_min_size = 6;
    data->max_rooms = 30;
    data->max_monsters_per_room = 3;
    data->max_items_per_room = 8;
    data->fov_light_walls = true;
    data->fov_radius = 10;
    data->game_state = ST_TURN_PLAYER;
    data->prev_state = ST_TURN_PLAYER;
    data->target_selected = false;
    data->targeting_item = NULL;
    data->target_x = -1;
    data->target_y = -1;

    console_create(&data->console,
                   app->renderer,
                   data->screen_width,
                   data->screen_height,
                   app->terminal.tileset);
    console_create(&data->panel,
                   app->renderer,
                   data->screen_width,
                   data->panel_height,
                   app->terminal.tileset);
    console_create(&data->menu,
                   app->renderer,
                   data->screen_width,
                   data->screen_height,
                   app->terminal.tileset);

    inventory_create(&data->inventory, 26);
}

bool game_state_load_game(rg_game_state_data* data, rg_app* app)
{
    if (!savefile_exists(SAVEFILE_NAME)) return false;

    with_defaults(data, app);

    savefile_load(data, SAVEFILE_NAME);

    fov_map_create(&data->fov_map, data->map_width, data->map_height);
    for (int y = 0; y < data->map_height; y++)
    {
        for (int x = 0; x < data->map_width; x++)
        {
            rg_tile* tile = map_get_tile(&data->game_map, x, y);
            fov_map_set_props(
              &data->fov_map, x, y, !tile->block_sight, !tile->blocked);
        }
    }

    // first draw before waitevent
    fov_map_compute(&data->fov_map,
                    data->entities.data[data->player].x,
                    data->entities.data[data->player].y,
                    data->fov_radius,
                    data->fov_light_walls);
    data->mouse_position.x = 0;
    data->mouse_position.y = 0;

    return true;
}

void game_state_create_game(rg_game_state_data* data, rg_app* app)
{
    with_defaults(data, app);

    data->entities.capacity = data->max_monsters_per_room;
    data->entities.data =
      malloc(sizeof(*data->entities.data) * data->entities.capacity);
    ASSERT_M(data->entities.data != NULL);
    data->entities.len = 0;
    data->items.capacity = data->max_items_per_room;
    data->items.data = malloc(sizeof(*data->items.data) * data->items.capacity);
    ASSERT_M(data->items.data != NULL);
    data->items.len = 0;
    rg_fighter fighter = { .hp = 30, .defence = 2, .power = 5, .max_hp = 30 };
    ARRAY_PUSH(&data->entities,
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
    data->player = data->entities.len - 1;

    turn_logs_create(&data->logs, data->message_width, data->message_height);

    game_level_create(data, 1);
    
    data->mouse_position.x = 0;
    data->mouse_position.y = 0;
}

void game_state_destroy(rg_game_state_data* data)
{
    inventory_destroy(&data->inventory);
    turn_logs_destroy(&data->logs);
    map_destroy(&data->game_map);
    free(data->entities.data);
    console_destroy(&data->menu);
    console_destroy(&data->console);
    console_destroy(&data->panel);
    // terminal_destroy(&data->terminal);
    // tileset_destroy(&data->tileset);
}

void game_state_update(rg_app* app, SDL_Event* event, rg_game_state_data* data)
{
    rg_map* game_map = &data->game_map;
    rg_fov_map* fov_map = &data->fov_map;
    rg_action action;
    event_dispatch(app,
                   event,
                   &action,
                   &data->mouse_position,
                   app->tileset.tile_size,
                   app->tileset.tile_size);

    if (action.type == ACTION_QUIT)
    {
        // App Quit signal
        savefile_save(data, SAVEFILE_NAME);
        app->screen = APP_SCREEN_MENU;
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
        savefile_save(data, SAVEFILE_NAME);
        app->screen = APP_SCREEN_MENU;
        return;
    }

    if (data->recompute_fov)
        fov_map_compute(&data->fov_map,
                        data->entities.data[data->player].x,
                        data->entities.data[data->player].y,
                        data->fov_radius,
                        data->fov_light_walls);
}

void game_state_draw(rg_app* app, rg_game_state_data* data)
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
                if (is_wall)
                {
                    // console_fill(console, x, y, LIGHT_WALL);
                    console_print(console, x, y, '#', WHITE);
                }
                else
                {
                    // console_fill(console, x, y, LIGHT_GROUND);
                    console_print(console, x, y, '.', WHITE);
                }
            }
            else if (tile->explored)
            {
                if (is_wall)
                {
                    console_print(console, x, y, '#', DARK_GREY);
                    // console_fill(console, x, y, DARK_WALL);
                }
                else
                {

                    // console_fill(console, x, y, DARK_GROUND);
                }
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
        {
            console_print(console, e->x, e->y, e->ch, e->color);
        }
        else if (e->type == ENTITY_STAIRS &&
                 map_get_tile(game_map, e->x, e->y)->explored)
        {
            console_fill(console, e->x, e->y, BLACK);
            console_print(console, e->x, e->y, e->ch, e->color);
        }
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
    {
        const char* fmt = "Dungeon level: %d";
        int len = snprintf(NULL, 0, fmt, data->game_map.level);
        char* buf = malloc(sizeof(char) * (len + 1));
        snprintf(buf, len + 1, fmt, data->game_map.level);   
        console_print_txt(&data->panel, 1, 3, buf, WHITE);
        free(buf);
    }
    char* names = get_names_under_mouse(data);
    if (names != NULL)
    {
        console_print_txt(&data->panel, 1, 0, names, WHITE);
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
    {

        SDL_Rect r = { .x = 0,
                       .y = data->panel_y * data->panel.tileset->tile_size,
                       .w = 0,
                       .h = 0 };
        SDL_QueryTexture(data->panel.texture, NULL, NULL, &r.w, &r.h);

        SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(app->renderer, &r);

        r.y -= 1;
        SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(app->renderer, &r);

        console_flush(&data->panel, 0, data->panel_y);
    }

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

        // SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(app->renderer, &r);
        r.x -= 1;
        r.y -= 1;
        r.w += 2;
        r.h += 2;
        SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(app->renderer, &r);

        console_flush(&data->menu, x, y);
    }
    SDL_RenderPresent(app->renderer);

    data->recompute_fov = false;
}

void state_enemy_turn(const SDL_Event* event,
                      rg_action* action,
                      rg_game_state_data* data)
{
    for (int i = 0; i < data->entities.len; i++)
    {
        if (i == data->player) continue;

        rg_entity* e = &data->entities.data[i];
        if (e->fighter.hp <= 0) continue;

        rg_entity* player = &data->entities.data[data->player];

        rg_entity* dead_entity;
        enemy_state_update(e,
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
    if (data->game_state != ST_TURN_PLAYER_DEAD)
        data->game_state = ST_TURN_PLAYER;
}

void state_player_turn(const SDL_Event* event,
                       rg_action* action,
                       rg_game_state_data* data)
{
    handle_player_input(event, action, data);

    switch (action->type)
    {
    case ACTION_MOVEMENT:
        handle_player_movement(action, data);
        break;
    case ACTION_PICKUP:
        handle_player_pickup(action, data);
        break;
    case ACTION_SHOW_INVENTORY:
        state_inventory_turn(event, action, data);
        break;
    case ACTION_DROP_INVENTORY:
        state_inventory_drop_turn(event, action, data);
        break;
    case ACTION_SELECT:
    {
        for (size_t i = 0; i < data->entities.len; i++)
        {
            rg_entity* player = &data->entities.data[data->player];
            rg_entity* e = &data->entities.data[i];
            if (e->type == ENTITY_STAIRS && e->x == player->x
                && e->y == player->y)
            {
                game_next_level(data);
                return;
            }
        }

        {
            // no stairs
            const char* fmt = "There are no stairs here.";
            int len = snprintf(NULL, 0, fmt);
            char* buf = malloc(sizeof(char) * (len + 1));
            snprintf(buf, len + 1, fmt);
            rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE,
                                        .text = buf,
                                        .color = YELLOW };
            turn_logs_push(&data->logs, &entry);
        }

        break;
    }
    case ACTION_ESCAPE:
        action->type = ACTION_QUIT;
        break;
    }
}

void state_inventory_drop_turn(const SDL_Event* event,
                               rg_action* action,
                               rg_game_state_data* data)
{
    if (data->game_state != ST_DROP_INVENTORY)
        data->prev_state = data->game_state;
    data->game_state = ST_DROP_INVENTORY;
    handle_inventory_input(event, action, data);
    switch (action->type)
    {
    case ACTION_SELECT:
        if (data->prev_state == ST_TURN_PLAYER_DEAD) return;
        if (data->inventory.len == 0) return;
        if (data->inventory.len < action->index) return;
        ASSERT_M(action->index >= 0);
        const size_t idx = data->inventory.data[action->index];
        rg_item* item = &data->items.data[idx];

        {
            const char* fmt = "You dropped the %s";
            int len = snprintf(NULL, 0, fmt, item->name);
            char* buf = malloc(sizeof(char) * (len + 1));
            snprintf(buf, len + 1, fmt, item->name);
            rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE,
                                        .text = buf,
                                        .color = YELLOW };
            turn_logs_push(&data->logs, &entry);
        }
        inventory_remove_item(&data->inventory, &data->items, item);
        break;
    case ACTION_ESCAPE:
        data->game_state = data->prev_state;
        break;
    }
}

void state_inventory_turn(const SDL_Event* event,
                          rg_action* action,
                          rg_game_state_data* data)
{
    if (data->game_state != ST_SHOW_INVENTORY)
    {
        data->prev_state = data->game_state;
        data->game_state = ST_SHOW_INVENTORY;
        return;
    }

    handle_inventory_input(event, action, data);

    switch (action->type)
    {
    case ACTION_SELECT:
        if (data->prev_state == ST_TURN_PLAYER_DEAD) return;
        if (data->inventory.len == 0) return;
        if (data->inventory.len < action->index) return;
        ASSERT_M(action->index >= 0);
        const size_t idx = data->inventory.data[action->index];
        rg_item* item = &data->items.data[idx];

        {
            rg_entity* player = &data->entities.data[data->player];
            bool consumed;
            item_use(item, player, data, &data->logs, &consumed);
            if (consumed)
            {
                inventory_remove_item(&data->inventory, &data->items, item);
                data->game_state = ST_TURN_ENEMY;
            }
        }
        break;
    case ACTION_ESCAPE:
        data->game_state = data->prev_state;
        break;
    }
}

void state_player_dead_turn(const SDL_Event* event,
                            rg_action* action,
                            rg_game_state_data* data)
{
    handle_player_dead_input(event, action, data);

    switch (action->type)
    {
    case ACTION_SHOW_INVENTORY:
        state_inventory_turn(event, action, data);
        break;
    case ACTION_ESCAPE:
        action->type = ACTION_QUIT;
        break;
    }
}

void state_targeting_turn(const SDL_Event* event,
                          rg_action* action,
                          rg_game_state_data* data)
{
    handle_targeting_input(event, action, data);

    switch (action->type)
    {
    case ACTION_TARGET_SELECTED: // TARGET_SELECTED
        ASSERT_M(data->targeting_item != NULL);
        rg_item* item = data->targeting_item;
        {
            bool consumed;
            item_use(item, NULL, data, &data->logs, &consumed);
            if (consumed)
            {
                inventory_remove_item(&data->inventory, &data->items, item);
                data->game_state = ST_TURN_ENEMY;
            }
        }
        break;
    case ACTION_ESCAPE:
        data->game_state = data->prev_state;
        break;
    }
}