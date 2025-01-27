#ifndef ENTITY_H
#define ENTITY_H
#include <stdbool.h>
#include <stddef.h>

#include <SDL.h>

#include "console.h"
#include "turn_log.h"

#define MAX_ENTITY_NAME 50

typedef enum rg_entity_state_type
{
    ENTITY_STATE_NONE,          // IDLE
    ENTITY_STATE_FOLLOW_PLAYER, // ONLY state in game
    ENTITY_STATE_ATTACK,
    ENTITY_STATE_CONFUSED,
} rg_entity_state_type;

typedef struct rg_entity_state
{
    rg_entity_state_type type;
    union
    {
        long packed;
        struct
        {
            int num_turns;
            rg_entity_state_type prev_state;
        } confused;
    } data;
} rg_entity_state;

typedef size_t rg_entity_id;

typedef enum rg_entity_type
{
    ENTITY_PLAYER,
    ENTITY_BASIC_MONSTER,
} rg_entity_type;

typedef enum rg_render_order
{
    RENDER_ORDER_CORPSE,
    RENDER_ORDER_ACTOR
} rg_render_order;

typedef struct rg_fighter
{
    int max_hp;
    int hp;
    int defence;
    int power;
} rg_fighter;

typedef struct rg_entity
{
    int x, y;
    char ch;
    SDL_Color color;
    char name[MAX_ENTITY_NAME];
    bool blocks;
    rg_entity_type type;
    rg_entity_state state;
    rg_fighter fighter;
    rg_render_order render_order;
} rg_entity;

typedef struct rg_entity_array
{
    size_t len;
    size_t capacity;
    rg_entity *data;
} rg_entity_array;

void entity_move(rg_entity *e, int dx, int dy);
void entity_draw(const rg_entity *e, rg_console *c);

void entity_get_at_loc(rg_entity_array *entities,
                       int x,
                       int y,
                       rg_entity **entity);
void entity_take_damage(rg_entity *e,
                        int amount,
                        rg_turn_logs *logs,
                        bool *is_dead);
void entity_attack(rg_entity *e,
                   rg_entity *target,
                   rg_turn_logs *logs,
                   rg_entity **dead_entity);
void entity_kill(rg_entity *e, rg_turn_logs *logs);
float entity_get_distance(rg_entity *a, rg_entity *b);
float entity_distance_to(rg_entity *a, int x, int y);
#endif