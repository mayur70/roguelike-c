#ifndef ENTITY_H
#define ENTITY_H
#include <stdbool.h>
#include <stddef.h>

#include <SDL.h>

#include "console.h"

typedef size_t rg_entity_id;

typedef enum rg_entity_type
{
    ENTITY_PLAYER,
    ENTITY_BASIC_MONSTER
} rg_entity_type;

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
    const char *name;
    bool blocks;
    rg_entity_type type;
    rg_fighter fighter;
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



#endif