#include "entity.h"

void entity_move(rg_entity* e, int dx, int dy)
{
    e->x += dx;
    e->y += dy;
}

void entity_draw(const rg_entity* e, rg_console* c)
{
    console_print(c, e->x, e->y, e->ch, e->color);
}