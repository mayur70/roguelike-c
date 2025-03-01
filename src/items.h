#ifndef ITEMS_H
#define ITEMS_H

#include <stdbool.h>

#include "entity.h"
#include "turn_log.h"

#define MAX_ITEM_NAME 50
#define MAX_TRGT_MSG_LEN 75

typedef enum rg_item_type
{
    ITEM_POTION_HEAL,
    ITEM_LIGHTNING,
    ITEM_FIRE_BALL,
    ITEM_CAST_CONFUSE,
    ITEM_SWORD,
    ITEM_SHIELD,
    ITEM_EQUIPMENT,
    ITEM_LEN,
} rg_item_type;

typedef enum rg_equipment_slot
{
    EQUIPMENT_SLOT_MAIN_HAND,
    EQUIPMENT_SLOT_OFF_HAND,
} rg_equipment_slot;

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
        struct
        {
            int amount;
        } heal;
        struct
        {
            int damage;
            int maximum_range;
        } lightning;
        struct
        {
            int damage;
            int radius;
            const char targeting_msg[MAX_TRGT_MSG_LEN];
            SDL_Color targeting_msg_color;
        } fireball;
        struct
        {
            int duration;
            const char targeting_msg[MAX_TRGT_MSG_LEN];
            SDL_Color targeting_msg_color;
        } confuse;
        struct
        {
            rg_equipment_slot slot;
            int power_bonus;
            int defense_bonus;
            int max_hp_bonus;
        } equipable;
    };
} rg_item;

typedef struct rg_items
{
    size_t capacity;
    size_t len;
    rg_item* data;
} rg_items;

void item_use(rg_item* item,
              rg_entity* target_entity,
              void* data,
              rg_turn_logs* logs,
              bool* is_consumed);

#endif