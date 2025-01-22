#include "entity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "color.h"
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

void entity_take_damage(rg_entity* e,
                        int amount,
                        rg_turn_logs* logs,
                        bool* is_dead)
{
    *is_dead = false;
    e->fighter.hp -= amount;

    if (e->fighter.hp <= 0)
    {
        // rg_turn_log_entry entry = { .type = TURN_LOG_DEAD,
        //                           .text = strdup(e->name) };
        // turn_logs_push(logs, &entry);
        *is_dead = true;
    }
}

void entity_attack(rg_entity* e,
                   rg_entity* target,
                   rg_turn_logs* logs,
                   rg_entity** dead_entity)
{
    ASSERT_M(dead_entity != NULL);
    *dead_entity = NULL;
    int damage = e->fighter.power - target->fighter.defence;

    if (damage > 0)
    {
        bool is_dead;
        entity_take_damage(target, damage, logs, &is_dead);

        const char* fmt = "%s attacks %s for %d hit points";

        int len = snprintf(NULL, 0, fmt, e->name, target->name, damage);
        char* buf = malloc(sizeof(char) * (len + 1));
        snprintf(buf, len, fmt, e->name, target->name, damage);

        rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE,
                                    .text = buf,
                                    .color = WHITE };
        turn_logs_push(logs, &entry);

        if (is_dead)
        {
            *dead_entity = target;
        }
    }
    else
    {
        const char* fmt = "%s attacks %s but does no damage";

        long len = snprintf(NULL, 0, fmt, e->name, target->name);
        size_t sz = len + 1;
        char* buf = malloc(sizeof(char) * sz);
        snprintf(buf, len, fmt, e->name, target->name);

        rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE,
                                    .text = buf,
                                    .color = WHITE };
        turn_logs_push(logs, &entry);
    }
}

void entity_kill(rg_entity* e, rg_turn_logs* logs)
{
    ASSERT_M(e != NULL);
    ASSERT_M(logs != NULL);
    e->ch = '%';
    e->color = DARK_RED;
    e->blocks = false;
    e->render_order = RENDER_ORDER_CORPSE;

    rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE };
    if (e->type != ENTITY_PLAYER)
    {

        const char* logfmt = "%s is dead!";
        const int sz1 = snprintf(NULL, 0, logfmt, e->name);
        entry.text = malloc(sizeof(char) * sz1 + 1);
        entry.color = ORANGE;
        snprintf(entry.text, sz1 + 1, logfmt, e->name);
        turn_logs_push(logs, &entry);

        char* buf = strdup(e->name);
        const char* fmt = "remains of %s";
        const int sz = snprintf(NULL, 0, fmt, buf);
        snprintf(e->name, sz + 1, fmt, buf);
        free(buf);
    }
    else
    {
        const char* logfmt = "You died!";
        const int sz1 = snprintf(NULL, 0, logfmt);
        entry.text = malloc(sizeof(char) * sz1 + 1);
        entry.color = RED;
        snprintf(entry.text, sz1 + 1, logfmt);
        turn_logs_push(logs, &entry);
    }
}

float entity_get_distance(rg_entity* a, rg_entity* b)
{
    int dx = b->x - a->x;
    int dy = b->y - a->y;
    return (float)sqrt(dx * dx + dy * dy);
}
