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
    ACTION_SELECT,
    ACTION_DROP_INVENTORY,
    ACTION_TARGET_SELECTED,
    ACTION_LEVEL_UP,
    ACTION_SHOW_CHARACTER,
    ACTION_WAIT,
    ACTION_EQUIPPED,
    ACTION_DEEQUIPPED,
} rg_action_type;

typedef enum rg_level_up_option
{
    LEVEL_UP_OPTION_HP,
    LEVEL_UP_OPTION_STR,
    LEVEL_UP_OPTION_DEF,
} rg_level_up_option;

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
        rg_level_up_option level_up;
    };
} rg_action;

void event_dispatch(struct rg_app* app,
                    SDL_Event* event,
                    rg_action* action,
                    SDL_Point* mouse_position,
                    int tile_width,
                    int tile_height);

#endif