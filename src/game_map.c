#include "game_map.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "types.h"
#include "array.h"
#include "color.h"

typedef struct room_array
{
    size_t len;
    size_t capacity;
    SDL_Rect *data;
} room_array;

void map_create(rg_map *m,
                int width,
                int height,
                int room_min_size,
                int room_max_size,
                int max_rooms,
                int max_monsters_per_room,
                rg_entity_array* entities,
                rg_entity_id player)
{
    memset(m, 0, sizeof(rg_map));
    m->width = width;
    m->height = height;
    m->tiles.capacity = m->width * m->height;
    m->tiles.data = malloc(sizeof(m->tiles.data) * m->tiles.capacity);
    ASSERT_M(m->tiles.data != NULL);

    for (int y = 0; y < m->height; y++)
    {
        for (int x = 0; x < m->width; x++)
        {
            map_set_tile(m, x, y, (rg_tile){true, true});
        }
    }

    room_array rooms = {
        .capacity = max_rooms,
        .len = 0,
        .data = malloc(sizeof(SDL_Rect) * max_rooms)};
    int num_rooms = 0;

    for (int r = 0; r < max_rooms; r++)
    {
        int w = RAND_INT(room_min_size, room_max_size);
        int h = RAND_INT(room_min_size, room_max_size);
        int x = RAND_INT(0, m->width - w - 1);
        int y = RAND_INT(0, m->height - h - 1);

        SDL_Rect new_room = {
            .x = x,
            .y = y,
            .w = w,
            .h = h};
        bool intersects = false;
        for (int i = 0; i < rooms.len; i++)
        {
            if (room_intersects(&new_room, &rooms.data[i]))
            {
                intersects = true;
                break;
            }
        }
        if (intersects)
            continue;

        map_create_room(m, new_room);
        int new_x, new_y;
        room_get_center(&new_room, &new_x, &new_y);
        if (rooms.len == 0)
        {
            entities->data[player].x = new_x;
            entities->data[player].y = new_y;
        }
        else
        {
            int prev_x, prev_y;
            room_get_center(&rooms.data[rooms.len - 1], &prev_x, &prev_y);
            if (RAND_INT(0, 1) == 1)
            {
                map_create_h_tunnel(m, prev_x, new_x, prev_y);
                map_create_v_tunnel(m, prev_y, new_y, new_x);
            }
            else
            {
                map_create_v_tunnel(m, prev_y, new_y, prev_x);
                map_create_h_tunnel(m, prev_x, new_x, new_y);
            }
            map_place_entities(m, &new_room, entities, max_monsters_per_room);
        }

        memcpy(&rooms.data[rooms.len], &new_room, sizeof(SDL_Rect));
        rooms.len++;
    }
    free(rooms.data);
}

void map_destroy(rg_map *m)
{
    free(m->tiles.data);
}

rg_tile *map_get_tile(rg_map *m, int x, int y)
{
    size_t idx = x + m->width * y;
    ASSERT_M(idx < m->tiles.capacity);
    return &m->tiles.data[idx];
}

void map_set_tile(rg_map *m, int x, int y, rg_tile tile)
{
    size_t idx = x + m->width * y;
    ASSERT_M(idx < m->tiles.capacity);
    m->tiles.data[idx] = tile;
}

bool map_is_blocked(rg_map *m, int x, int y)
{
    return x < 0 || x >= m->width || y < 0 || y >= m->height || map_get_tile(m, x, y)->blocked;
}

void map_create_room(rg_map *m, SDL_Rect room)
{
    int x1 = room.x;
    int x2 = room.x + room.w;
    int y1 = room.y;
    int y2 = room.y + room.h;
    for (int x = x1 + 1; x < x2; x++)
    {
        for (int y = y1 + 1; y < y2; y++)
        {
            map_get_tile(m, x, y)->blocked = false;
            map_get_tile(m, x, y)->block_sight = false;
        }
    }
}

void map_create_h_tunnel(rg_map *m, int x1, int x2, int y)
{
    for (int x = MIN(x1, x2); x < MAX(x1, x2) + 1; x++)
    {
        map_get_tile(m, x, y)->blocked = false;
        map_get_tile(m, x, y)->block_sight = false;
    }
}

void map_create_v_tunnel(rg_map *m, int y1, int y2, int x)
{
    for (int y = MIN(y1, y2); y < MAX(y1, y2) + 1; y++)
    {
        map_get_tile(m, x, y)->blocked = false;
        map_get_tile(m, x, y)->block_sight = false;
    }
}

void map_place_entities(rg_map *m,
                        SDL_Rect *room,
                        rg_entity_array *entities,
                        int max_monsters_per_room)
{
    int num_monsters = RAND_INT(0, max_monsters_per_room);
    for (int i = 0; i < num_monsters; i++)
    {
        //FIXME: bug in random max limit???
        int x = RAND_INT(room->x + 1, room->x + room->w - 3);
        int y = RAND_INT(room->y + 1, room->y + room->h - 3);

        ASSERT_M(x != room->x && x != room->x + room->w);
        ASSERT_M(y != room->y && y != room->y + room->h);

        bool valid = true;
        for (int m = 0; m < entities->len; m++)
        {
            rg_entity *e = &entities->data[m];
            if (e->x == x && e->y == y)
            {
                valid = false;
                break;
            }
        }

        if (!valid)
            continue;
        if (RAND_INT(0, 100) < 80)
        {
            ARRAY_PUSH(entities, ((rg_entity){
                                     .x = x,
                                     .y = y,
                                     .ch = 'o',
                                     .color = DESATURATED_GREEN}));
        }
        else
        {
            ARRAY_PUSH(entities, ((rg_entity){
                                     .x = x,
                                     .y = y,
                                     .ch = 'T',
                                     .color = DARKER_GREEN}));
        }
    }
}

void room_get_center(SDL_Rect *r, int *x, int *y)
{
    if (x != NULL)
    {
        *x = (r->x + r->x + r->w) / 2;
    }
    if (y != NULL)
    {
        *y = (r->y + r->y + r->h) / 2;
    }
}

bool room_intersects(SDL_Rect *lhs, SDL_Rect *rhs)
{
    return SDL_HasIntersection(lhs, rhs);
}