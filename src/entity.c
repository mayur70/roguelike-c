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
