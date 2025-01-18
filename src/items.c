#include "items.h"

#include <stdio.h>

#include "color.h"

void heal(rg_item* item,
          rg_entity* target_entity,
          void* data,
          rg_turn_logs* logs,
          bool* is_consumed)
{
    int amount = item->amount;

    if (target_entity->fighter.hp == target_entity->fighter.max_hp)
    {
        const char* fmt = "You are already at full health";
        int len = snprintf(NULL, 0, fmt);
        char* buf = malloc(sizeof(char) * (len + 1));
        snprintf(buf, len, fmt);
        rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE,
                                    .text = buf,
                                    .color = YELLOW };
        turn_logs_push(logs, &entry);
        *is_consumed = false;
    }
    else
    {
        target_entity->fighter.hp += amount;
        if (target_entity->fighter.hp > target_entity->fighter.max_hp)
            target_entity->fighter.hp = target_entity->fighter.max_hp;

        const char* fmt = "Your wounds start to feel better!";
        int len = snprintf(NULL, 0, fmt);
        char* buf = malloc(sizeof(char) * (len + 1));
        snprintf(buf, len, fmt);
        rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE,
                                    .text = buf,
                                    .color = GREEN };
        turn_logs_push(logs, &entry);
        *is_consumed = true;
    }
}

void item_use(rg_item* item,
              rg_entity* target_entity,
              void* data,
              rg_turn_logs* logs,
              bool* is_consumed)
{
    switch (item->type)
    {
    case ITEM_POTION_HEAL:
        heal(item, target_entity, data, logs, is_consumed);
        break;
    }
}