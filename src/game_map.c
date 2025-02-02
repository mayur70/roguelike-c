#include "game_map.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "array.h"
#include "color.h"
#include "types.h"

typedef struct room_array
{
    size_t len;
    size_t capacity;
    SDL_Rect *data;
} room_array;

void map_create(rg_map *m,
                int width,
                int height,
                int level,
                int room_min_size,
                int room_max_size,
                int max_rooms,
                int max_monsters_per_room,
                int max_items_per_room,
                rg_entity_array *entities,
                rg_items *items,
                rg_entity_id player)
{
    memset(m, 0, sizeof(rg_map));
    m->width = width;
    m->height = height;
    m->level = level;
    m->tiles.capacity = m->width * m->height;
    m->tiles.data = malloc(sizeof(m->tiles.data) * m->tiles.capacity);
    m->tiles.len = m->tiles.capacity;
    ASSERT_M(m->tiles.data != NULL);

    for (int y = 0; y < m->height; y++)
    {
        for (int x = 0; x < m->width; x++)
        {
            map_set_tile(m, x, y, (rg_tile){ true, true });
        }
    }

    room_array rooms = { .capacity = max_rooms,
                         .len = 0,
                         .data = malloc(sizeof(SDL_Rect) * max_rooms) };
    int num_rooms = 0;

    for (int r = 0; r < max_rooms; r++)
    {
        int w = RAND_INT(room_min_size, room_max_size);
        int h = RAND_INT(room_min_size, room_max_size);
        int x = RAND_INT(0, m->width - w - 1);
        int y = RAND_INT(0, m->height - h - 1);

        SDL_Rect new_room = { .x = x, .y = y, .w = w, .h = h };
        bool intersects = false;
        for (int i = 0; i < rooms.len; i++)
        {
            if (room_intersects(&new_room, &rooms.data[i]))
            {
                intersects = true;
                break;
            }
        }
        if (intersects) continue;

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
            map_place_entities(m,
                               &new_room,
                               entities,
                               max_monsters_per_room,
                               items,
                               max_items_per_room);
        }

        memcpy(&rooms.data[rooms.len], &new_room, sizeof(SDL_Rect));
        rooms.len++;
    }

    // Stairs
    SDL_Rect last_room = rooms.data[rooms.len - 1];
    ARRAY_PUSH(entities,
               ((rg_entity){
                 .x = last_room.x + last_room.w / 2,
                 .y = last_room.y + last_room.h / 2,
                 .ch = '>',
                 .color = WHITE,
                 .name = "Stairs",
                 .blocks = false,
                 .type = ENTITY_STAIRS,
                 .render_order = RENDER_ORDER_STAIRS,
               }));
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
    return x < 0 || x >= m->width || y < 0 || y >= m->height ||
           map_get_tile(m, x, y)->blocked;
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

static int val_from_dungeon_level(int current_level,
                                  int len,
                                  int *levels,
                                  int *values)
{
    for (int i = len - 1; i >= 0; i--)
    {
        if (current_level >= levels[i]) return values[i];
    }
    return 0;
}

void map_place_entities(rg_map *m,
                        SDL_Rect *room,
                        rg_entity_array *entities,
                        int max_monsters_per_room,
                        rg_items *items,
                        int max_items_per_room)
{

    int num_monsters = RAND_INT(0, max_monsters_per_room);
    {
        int levels[] = { 1, 4, 6 };
        int vals[] = { 2, 3, 5 };
        num_monsters = val_from_dungeon_level(m->level, 3, levels, vals);
    }
    for (int i = 0; i < num_monsters; i++)
    {
        // FIXME: bug in random max limit???
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

        if (!valid) continue;

        int troll_chances = 0;
        {
            int levels[] = { 3, 5, 7 };
            int vals[] = { 15, 30, 60 };
            troll_chances = val_from_dungeon_level(m->level, 3, levels, vals);
        }

        int monster_chances[] = { 80, troll_chances };
        int midx = rand_int_choice_index(MONSTER_LEN, monster_chances);
        ASSERT_M(midx >= 0);
        ASSERT_M(midx < MONSTER_LEN);
        switch (midx)
        {
        case MONSTER_ORC:
        {
            rg_fighter fighter = {
                .hp = 20, .defence = 0, .power = 4, .xp = 35
            };
            ARRAY_PUSH(entities,
                       ((rg_entity){
                         .x = x,
                         .y = y,
                         .ch = 'o',
                         .color = GREEN,
                         .name = "Orc",
                         .blocks = true,
                         .type = ENTITY_BASIC_MONSTER,
                         .fighter = fighter,
                         .state.type = ENTITY_STATE_FOLLOW_PLAYER,
                         .render_order = RENDER_ORDER_ACTOR,
                       }));
            break;
        }
        case MONSTER_TROLL:
        {
            rg_fighter fighter = {
                .hp = 30, .defence = 2, .power = 8, .xp = 100
            };
            ARRAY_PUSH(entities,
                       ((rg_entity){
                         .x = x,
                         .y = y,
                         .ch = 'T',
                         .color = DARKER_GREEN,
                         .name = "Troll",
                         .blocks = true,
                         .type = ENTITY_BASIC_MONSTER,
                         .fighter = fighter,
                         .state.type = ENTITY_STATE_FOLLOW_PLAYER,
                         .render_order = RENDER_ORDER_ACTOR,
                       }));
            break;
        }
        default:
            ASSERT_M(false); // invalid option
            break;
        }
    }
    int num_items = RAND_INT(0, max_items_per_room);
    {
        int levels[] = { 1, 4 };
        int vals[] = { 1, 2 };
        num_items = val_from_dungeon_level(m->level, 2, levels, vals);
    }
    for (int i = 0; i < num_items; i++)
    {
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

        if (!valid) continue;

        int item_chances[4] = { 0 };
        item_chances[0] = 35;
        {
            int levels[] = { 4 };
            int vals[] = { 25 };
            item_chances[1] = val_from_dungeon_level(m->level, 1, levels, vals);
        }
        {
            int levels[] = { 6 };
            int vals[] = { 25 };
            item_chances[2] = val_from_dungeon_level(m->level, 1, levels, vals);
        }
        {
            int levels[] = { 2 };
            int vals[] = { 10 };
            item_chances[3] = val_from_dungeon_level(m->level, 1, levels, vals);
        }
        int idx = rand_int_choice_index(ITEM_LEN - 1, item_chances);
        ASSERT_M(idx >= 0);
        ASSERT_M(idx < ITEM_LEN - 1);
        // int item_chance = RAND_INT(0, 100);
        switch (idx)
        {
        case ITEM_POTION_HEAL:
        {
            ARRAY_PUSH(items,
                       ((rg_item){
                         .x = x,
                         .y = y,
                         .ch = '!',
                         .color = LIGHTEST_VIOLET,
                         .name = "Healing Potion",
                         .type = ITEM_POTION_HEAL,
                         .heal = { .amount = 40 },
                         .visible_on_map = true,
                       }));
            break;
        }
        case ITEM_FIRE_BALL:
        {
            ARRAY_PUSH(items,
                       ((rg_item){
                         .x = x,
                         .y = y,
                         .ch = '*',
                         .color = FLAME,
                         .name = "Fireball Scroll",
                         .type = ITEM_FIRE_BALL,
                         .fireball = {
                             .damage = 25,
                             .radius = 3,
                             .targeting_msg = "Left-click a target tile for the fireball, or right-click to cancel.",
                             .targeting_msg_color = CYAN,
                             },
                         .visible_on_map = true,
                       }));
            break;
        }
        case ITEM_CAST_CONFUSE:
        {
            ARRAY_PUSH(items,
                       ((rg_item){
                         .x = x,
                         .y = y,
                         .ch = '#',
                         .color = LIGHT_PINK,
                         .name = "Confusion Scroll",
                         .type = ITEM_CAST_CONFUSE,
                         .confuse = {
                             .duration = 10,
                             .targeting_msg = "Left-click an enemy to confuse it, or right-click to cancel.",
                             .targeting_msg_color = CYAN,
                             },
                         .visible_on_map = true,
                       }));
            break;
        }
        case ITEM_LIGHTNING:
        {
            ARRAY_PUSH(items,
                       ((rg_item){
                         .x = x,
                         .y = y,
                         .ch = '#',
                         .color = YELLOW,
                         .name = "Lightning Scroll",
                         .type = ITEM_LIGHTNING,
                         .lightning = {
                             .damage = 40,
                             .maximum_range = 5,
                             },
                         .visible_on_map = true,
                       }));
            break;
        }
        default:
            ASSERT_M(false); // invalid option
            break;
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