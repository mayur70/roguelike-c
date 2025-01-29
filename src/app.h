#ifndef APP_H
#define APP_H

#include <stdbool.h>

#include <SDL.h>

#include "terminal.h"
#include "tileset.h"
#include "gameplay_state.h"
#include "mainmenu_state.h"

typedef enum app_screen
{
    APP_SCREEN_MENU,
    APP_SCREEN_PLAY,
} app_screen;

typedef struct rg_app
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    int screen_width;
    int screen_height;
    rg_tileset tileset;
    rg_terminal terminal;
    app_screen screen;
    SDL_Texture* main_menu_bg_texture;

    rg_game_state_data game_state_data;
    rg_main_menu_state_data menu_state_data;
} rg_app;

void app_create(rg_app* app,
                int screen_width,
                int screen_height,
                const char* tileset_path,
                int tiles_wide,
                int tiles_high,
                const char* menu_bg_texture_path);
void app_destroy(rg_app* app);

#endif