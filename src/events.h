#ifndef EVENTS_H
#define EVENTS_H

#include <SDL.h>

#include "app.h"

typedef enum rg_action_type
{
    ACTION_NONE,
    ACTION_QUIT,
    ACTION_ESCAPE,
    ACTION_MOVEMENT,
    ACTION_PICKUP,
    ACTION_SHOW_INVENTORY,
    ACTION_INVENTORY_SELECT,
    ACTION_DROP_INVENTORY,
    ACTION_TARGET_SELECTED,
} rg_action_type;

typedef struct rg_action
{
    rg_action_type type;
    SDL_Event* event;
    union
    {
        struct
        {
            int dx, dy;
        };
        int index;
    };
} rg_action;

void event_dispatch(rg_app* app,
                    SDL_Event* event,
                    rg_action* action,
                    SDL_Point* mouse_position,
                    int tile_width,
                    int tile_height);

#endif