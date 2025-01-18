#include "events.h"

void event_dispatch(rg_app *app,
                    SDL_Event *event,
                    rg_action *action,
                    SDL_Point *mouse_position,
                    int tile_width,
                    int tile_height)
{
    // Default value
    action->type = ACTION_NONE;
    action->event = event;
    switch (event->type)
    {
    case SDL_QUIT:
        action->type = ACTION_QUIT;
        break;
    case SDL_MOUSEMOTION:
    {
        mouse_position->x = event->motion.x;
        mouse_position->y = event->motion.y;

        int wx, wy;
        SDL_GetMouseState(&wx, &wy);
        float lx, ly;
        SDL_RenderWindowToLogical(app->renderer, wx, wy, &lx, &ly);
        int rw, rh;
        SDL_RenderGetLogicalSize(app->renderer, &rw, &rh);
        if (lx < 0) mouse_position->x = 0;
        else if (lx > rw)
            mouse_position->x = rw;
        else
            mouse_position->x = (int)lx;
        if (ly < 0) mouse_position->y = 0;
        else if (ly > rh)
            mouse_position->y = rh;
        else
            mouse_position->y = (int)ly;

        mouse_position->x = mouse_position->x / tile_width;
        mouse_position->y = mouse_position->y / tile_height;
        break;
    }
    default:
        // no-op
        break;
    }
}
