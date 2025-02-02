#ifndef GAMEPLAY_STATE
#define GAMEPLAY_STATE

#include <stdbool.h>

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
    ST_TARGETING,
    ST_LEVEL_UP,
    ST_SHOW_CHARACTER,
} rg_game_state;

typedef struct rg_player_level
{
    int current_level;
    int current_xp;
    int level_up_base;
    int level_up_factor;
} rg_player_level;

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
    bool target_selected;
    int target_x;
    int target_y;
    rg_item* targeting_item;
    rg_console console;
    rg_console panel;
    rg_console menu;
    rg_console character_screen;
    rg_entity_array entities;
    rg_items items;
    rg_entity_id player;
    rg_player_level player_level;
    rg_map game_map;
    rg_fov_map fov_map;
    bool recompute_fov;
    rg_game_state game_state;
    rg_game_state prev_state;
    rg_inventory inventory;
    rg_turn_logs logs;
    SDL_Point mouse_position;
} rg_game_state_data;

bool game_state_load_game(rg_game_state_data* data, struct rg_app* app);
void game_state_create_game(rg_game_state_data* data, struct rg_app* app);
void game_state_destroy(rg_game_state_data* data);

void game_state_update(struct rg_app* app,
                       SDL_Event* event,
                       rg_game_state_data* data);
void game_state_draw(struct rg_app* app, rg_game_state_data* data);

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

void state_targeting_turn(const SDL_Event* event,
                          rg_action* action,
                          rg_game_state_data* data);
void state_level_up_turn(const SDL_Event* event,
                          rg_action* action,
                          rg_game_state_data* data);
void state_show_character_turn(const SDL_Event* event,
                          rg_action* action,
                          rg_game_state_data* data);
#endif