#include "items.h"

#include <stdio.h>

#include "color.h"
#include "gameplay_state.h"

void heal(rg_item* item,
          rg_entity* target_entity,
          void* data,
          rg_turn_logs* logs,
          bool* is_consumed)
{
    int amount = item->heal.amount;

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

void cast_lightning(rg_item* item,
                    rg_game_state_data* data,
                    rg_turn_logs* logs,
                    bool* is_consumed)
{
    *is_consumed = false;

    rg_entity_array* entities = &data->entities;
    rg_entity* caster = &entities->data[data->player];
    rg_fov_map* fov_map = &data->fov_map;
    int damage = item->lightning.damage;
    int maximum_range = item->lightning.maximum_range;

    rg_entity* target = NULL;
    int closest_distance = maximum_range + 1;
    for (int i = 0; i < entities->len; i++)
    {
        rg_entity* e = &entities->data[i];
        if (e == caster) continue;
        if (e->ch == '%') continue; // dead entity
        if (fov_map_is_in_fov(fov_map, e->x, e->y))
        {
            int distance = (int)entity_get_distance(caster, e);
            if (distance < closest_distance)
            {
                target = e;
                closest_distance = distance;
            }
        }
    }
    if (target != NULL)
    {
        const char* fmt = "A lighting bolt strikes the %s with a loud "
                          "thunder! The damage is %d";
        int len = snprintf(NULL, 0, fmt, target->name, damage);
        char* buf = malloc(sizeof(char) * (len + 1));
        snprintf(buf, len, fmt, target->name, damage);
        rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE,
                                    .text = buf,
                                    .color = WHITE };
        turn_logs_push(logs, &entry);
        bool is_dead;
        entity_take_damage(target, damage, logs, &is_dead);
        if (is_dead)
        {
            entity_kill(target, logs);
        }
        *is_consumed = true;
    }
    else
    {
        const char* fmt = "No enemy is close enough to strike.";
        int len = snprintf(NULL, 0, fmt);
        char* buf = malloc(sizeof(char) * (len + 1));
        snprintf(buf, len, fmt);
        rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE,
                                    .text = buf,
                                    .color = RED };
        turn_logs_push(logs, &entry);
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
    case ITEM_LIGHTNING:
        cast_lightning(item, data, logs, is_consumed);
        break;
    }
}