#include "entity.h"

#include <stdio.h>
#include <string.h>

#include "array.h"
#include "types.h"

void entity_move(rg_entity* e, int dx, int dy)
{
    e->x += dx;
    e->y += dy;
}

void entity_draw(const rg_entity* e, rg_console* c)
{
    console_print(c, e->x, e->y, e->ch, e->color);
}

void entity_get_at_loc(rg_entity_array* entities,
                       int x,
                       int y,
                       rg_entity** entity)
{
    for (int i = 0; i < entities->len; i++)
    {
        rg_entity* e = &entities->data[i];
        if (e->blocks && e->x == x && e->y == y)
        {
            *entity = e;
            return;
        }
    }
    *entity = NULL;
}

void entity_take_damage(rg_entity* e, int amount, rg_turn_logs* logs)
{
    e->fighter.hp -= amount;

    if (e->fighter.hp <= 0)
    {
        rg_turn_log_entry entry = { .type = TURN_LOG_DEAD, .msg = strdup(e->name) };
        turn_logs_push(logs, &entry);
    }
}

void entity_attack(rg_entity* e, rg_entity* target, rg_turn_logs* logs)
{
    int damage = e->fighter.power - target->fighter.defence;

    if (damage > 0)
    {
        entity_take_damage(target, damage, logs);

        const char* fmt = "%s attacks %s for %d hit points";
        // SDL_Log(fmt, e->name, target->name, damage);

        int len = snprintf(NULL, 0, fmt, e->name, target->name, damage);
        char* buf = malloc(sizeof(char) * (len + 1));
        snprintf(buf, len, fmt, e->name, target->name, damage);

        rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE, .msg = buf };
        turn_logs_push(logs, &entry);
    }
    else
    {
        const char* fmt = "%s attacks %s but does no damage";
        // SDL_Log(fmt, e->name, target->name);

        long len = snprintf(NULL, 0, fmt, e->name, target->name);
        size_t sz = len + 1;
        char* buf = malloc(sizeof(char) * sz);
        snprintf(buf, len, fmt, e->name, target->name);

        rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE, .msg = buf };
        turn_logs_push(logs, &entry);
    }
}