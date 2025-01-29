#include "app.h"

#include <string.h>

#include <SDL_image.h>

#include "panic.h"

void app_create(rg_app *app,
                int screen_width,
                int screen_height,
                const char *tileset_path,
                int tiles_wide,
                int tiles_high,
                const char *menu_bg_texture_path)
{
    memset(app, 0, sizeof(app));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) panic("failed to initalize SDL");

    int img_flags = IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) == 0)
        panic("failed to initialize SDL_image");

    app->running = true;
    app->screen_width = screen_width;
    app->screen_height = screen_height;
    app->window = SDL_CreateWindow("roguelike",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   800,
                                   600,
                                   SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    if (app->window == NULL) panic("failed to create window");

    app->renderer = SDL_CreateRenderer(app->window, -1, 0);
    if (app->renderer == NULL) panic("failed to create renderer");
    app->screen = APP_SCREEN_MENU;

    tileset_create(
      &app->tileset, app->renderer, tileset_path, tiles_wide, tiles_high);

    terminal_create(&app->terminal,
                    app,
                    app->screen_width,
                    app->screen_height,
                    &app->tileset,
                    "roguelike",
                    true);
    // Texture
    {
        SDL_Surface *s = IMG_Load(menu_bg_texture_path);
        if (s == NULL) panic("failed to load menu_bg_texture");
        app->main_menu_bg_texture = SDL_CreateTextureFromSurface(app->renderer, s);
        if (app->main_menu_bg_texture == NULL)
            panic("failed to create menu_bg_texture from surface");
        SDL_FreeSurface(s);
    }
}

void app_destroy(rg_app *app)
{
    if (app == NULL) return;
    SDL_DestroyTexture(app->main_menu_bg_texture);
    tileset_destroy(&app->tileset);
    terminal_destroy(&app->terminal);
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
}