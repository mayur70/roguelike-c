#ifndef ENTITY_H
#define ENTITY_H
#include <stddef.h>

#include <SDL.h>

#include "console.h"

typedef struct rg_entity
{
    int x, y;
    char ch;
    SDL_Color color;
} rg_entity;

typedef struct rg_entity_array
{
    size_t len;
    size_t capacity;
    rg_entity *data;
} rg_entity_array;

void entity_move(rg_entity *e, int dx, int dy);
void entity_draw(const rg_entity *e, rg_console *c);

#endif