#ifndef MAINMENU_STATE_H
#define MAINMENU_STATE_H

#include <stdbool.h>

#include <SDL.h>

#include "console.h"

typedef struct rg_main_menu_state_data
{
    rg_console console;
    rg_console menu_options;
    rg_console message_box;
    SDL_Point mouse_position;
    bool load_file_error;
} rg_main_menu_state_data;

void mainmenu_state_create(rg_main_menu_state_data* data, struct rg_app* app);
void mainmenu_state_destroy(rg_main_menu_state_data* data);

void mainmenu_state_update(struct rg_app* app,
                           SDL_Event* event,
                           rg_main_menu_state_data* data);
void mainmenu_state_draw(struct rg_app* app,
                           rg_main_menu_state_data* data);


#endif