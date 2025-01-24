#include "gameplay_state.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astar.h"
#include "color.h"

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
        int distance = (int)entity_get_distance(e, target);
        if (distance >= 2)
        {
            entity_move_astar(e, target, game_map, entities);
        }
        else if (target->fighter.hp > 0)
            entity_attack(e, target, logs, dead_entity);
    }
}

void handle_player_movement(const rg_action* action, rg_game_state_data* data)
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

void handle_player_pickup(const rg_action* action, rg_game_state_data* data)
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

void handle_inventory_input(const SDL_Event* event,
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
            action->type = ACTION_INVENTORY_SELECT;
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

void handle_player_input(const SDL_Event* event,
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
        }
        break;
    }
    }
}

void handle_player_dead_input(const SDL_Event* event,
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

void handle_targeting_input(const SDL_Event* event,
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
    case ACTION_INVENTORY_SELECT:
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
        data->prev_state = data->game_state;
    data->game_state = ST_SHOW_INVENTORY;

    handle_inventory_input(event, action, data);

    switch (action->type)
    {
    case ACTION_INVENTORY_SELECT:
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