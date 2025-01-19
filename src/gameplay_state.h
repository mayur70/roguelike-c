#ifndef GAMEPLAY_STATE
#define GAMEPLAY_STATE

#include <SDL.h>

#include "console.h"
#include "entity.h"
#include "events.h"
#include "fov.h"
#include "game_map.h"
#include "inventory.h"
#include "terminal.h"
#include "tileset.h"
#include "turn_log.h"

typedef enum rg_game_state
{
    ST_TURN_PLAYER,
    ST_TURN_ENEMY,
    ST_TURN_PLAYER_DEAD,
    ST_SHOW_INVENTORY,
    ST_DROP_INVENTORY,
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
    int max_items_per_room;
    bool fov_light_walls;
    int fov_radius;
    rg_tileset tileset;
    rg_terminal terminal;
    rg_console console;
    rg_console panel;
    rg_console menu;
    rg_entity_array entities;
    rg_items items;
    rg_entity_id player;
    rg_map game_map;
    rg_fov_map fov_map;
    bool recompute_fov;
    rg_game_state game_state;
    rg_game_state prev_state;
    rg_inventory inventory;
    rg_turn_logs logs;
    SDL_Point mouse_position;
} rg_game_state_data;

void state_player_turn(const SDL_Event* event,
                       rg_action* action,
                       rg_game_state_data* data);
void state_player_dead_turn(const SDL_Event* event,
                            rg_action* action,
                            rg_game_state_data* data);
void state_enemy_turn(const SDL_Event* event,
                      rg_action* action,
                      rg_game_state_data* data);
void state_inventory_turn(const SDL_Event* event,
                          rg_action* action,
                          rg_game_state_data* data);

void state_inventory_drop_turn(const SDL_Event* event,
                               rg_action* action,
                               rg_game_state_data* data);

#endif