#ifndef EVENTS_H
#define EVENTS_H

#include <SDL.h>

typedef enum rg_action_type
{
    ACTION_NONE,
    ACTION_QUIT,
    ACTION_ESCAPE,
    ACTION_MOVEMENT,
    ACTION_PICKUP,
    ACTION_SHOW_INVENTORY,
} rg_action_type;

typedef struct rg_action
{
    rg_action_type type;
    union
    {
        struct
        {
            int dx, dy;
        };
    };
} rg_action;

void event_dispatch(SDL_Event *event, rg_action *action, SDL_Point* mouse_position);

#endif