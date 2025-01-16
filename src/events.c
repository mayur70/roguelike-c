#include "events.h"

void event_dispatch(SDL_Event *event,
                    rg_action *action,
                    SDL_Point *mouse_position)
{
    // Default value
    action->type = ACTION_NONE;

    switch (event->type)
    {
    case SDL_QUIT:
        action->type = ACTION_QUIT;
        break;
    case SDL_KEYDOWN:
    {
        switch (event->key.keysym.sym)
        {
        case SDLK_UP:
        case SDLK_k:
            action->type = ACTION_MOVEMENT;
            action->dx = 0;
            action->dy = -1;
            break;
        case SDLK_DOWN:
        case SDLK_j:
            action->type = ACTION_MOVEMENT;
            action->dx = 0;
            action->dy = 1;
            break;
        case SDLK_LEFT:
        case SDLK_h:
            action->type = ACTION_MOVEMENT;
            action->dx = -1;
            action->dy = 0;
            break;
        case SDLK_RIGHT:
        case SDLK_l:
            action->type = ACTION_MOVEMENT;
            action->dx = 1;
            action->dy = 0;
            break;
        case SDLK_y:
            action->type = ACTION_MOVEMENT;
            action->dx = -1;
            action->dy = -1;
            break;
        case SDLK_u:
            action->type = ACTION_MOVEMENT;
            action->dx = 1;
            action->dy = -1;
            break;
        case SDLK_b:
            action->type = ACTION_MOVEMENT;
            action->dx = -1;
            action->dy = 1;
            break;
        case SDLK_n:
            action->type = ACTION_MOVEMENT;
            action->dx = 1;
            action->dy = 1;
            break;

        case SDLK_g:
            action->type = ACTION_PICKUP;
            break;
        case SDLK_i:
            action->type = ACTION_SHOW_INVENTORY;
            break;

        case SDLK_ESCAPE:
            action->type = ACTION_ESCAPE;
            break;
        }
        break;
    }
    case SDL_MOUSEMOTION:
    {
        mouse_position->x = event->motion.x;
        mouse_position->y = event->motion.y;
        break;
    }
    default:
        // no-op
        break;
    }
}
