#ifndef ITEMS_H
#define ITEMS_H

#include <stdbool.h>

#include "entity.h"
#include "turn_log.h"


#define MAX_ITEM_NAME 50

typedef enum rg_item_type
{
    ITEM_POTION_HEAL
} rg_item_type;

typedef struct rg_item
{
    int x, y;
    char ch;
    SDL_Color color;
    char name[MAX_ITEM_NAME];
    bool visible_on_map;
    rg_item_type type;
    union
    {
        int amount;
    };
} rg_item;

typedef struct rg_items
{
    size_t capacity;
    size_t len;
    rg_item* data;
} rg_items;

void item_use(rg_item* item, rg_entity* target_entity, void* data, rg_turn_logs* logs, bool* is_consumed);

#endif