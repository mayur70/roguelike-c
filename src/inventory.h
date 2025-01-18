#ifndef INVENTORY_H
#define INVENTORY_H

#include <stddef.h>

#include "console.h"
#include "entity.h"
#include "items.h"
#include "turn_log.h"

typedef struct rg_inventory
{
    size_t capacity;
    size_t len;
    size_t* data;
} rg_inventory;

void inventory_create(rg_inventory* i, size_t capacity);
void inventory_destroy(rg_inventory* i);
void inventory_add_item(rg_inventory* inventory,
                        rg_items* items,
                        size_t item_idx,
                        rg_turn_logs* logs);
void inventory_remove_item(rg_inventory* inventory,
                           rg_items* items,
                           rg_item* item);

void inventory_draw(rg_console* c,
                    const char* header,
                    rg_items* items,
                    rg_inventory* inventory,
                    int width,
                    int* height);
#endif