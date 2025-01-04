#ifndef GAME_MAP_H
#define GAME_MAP_H

#include <stdbool.h>
#include <stddef.h>

#include <SDL.h>

#include "entity.h"

typedef struct rg_tile
{
    bool blocked;
    bool block_sight;
    bool explored;
} rg_tile;

typedef struct rg_tile_array
{
    size_t len;
    size_t capacity;
    rg_tile *data;
} rg_tile_array;

typedef struct rg_map
{
    int width;
    int height;
    rg_tile_array tiles;
} rg_map;

void map_create(rg_map *m,
                int w,
                int h,
                int room_min_size,
                int room_max_size,
                int max_rooms,
                int max_monsters_per_room,
                rg_entity_array* entities,
                rg_entity *player);
void map_destroy(rg_map *m);

void map_set_tile(rg_map *m, int x, int y, rg_tile tile);
rg_tile *map_get_tile(rg_map *m, int x, int y);
bool map_is_blocked(rg_map *m, int x, int y);

void map_create_room(rg_map *m, SDL_Rect room);
void map_create_h_tunnel(rg_map *m, int x1, int x2, int y);
void map_create_v_tunnel(rg_map *m, int y1, int y2, int x);
void map_place_entities(rg_map *m,
                        SDL_Rect *room,
                        rg_entity_array *entities,
                        int max_monsters_per_room);

void room_get_center(SDL_Rect *r, int *x, int *y);
bool room_intersects(SDL_Rect *lhs, SDL_Rect *rhs);

#endif