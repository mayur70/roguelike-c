#include "app.h"

#include <string.h>

#include <SDL_image.h>

#include "panic.h"

void app_create(rg_app *app)
{
    memset(app, 0, sizeof(app));

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        panic("failed to initalize SDL");

    int img_flags = IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) == 0)
        panic("failed to initialize SDL_image");

    app->window = SDL_CreateWindow(
        "roguelike",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800,
        600,
        0);
    if (app->window == NULL)
        panic("failed to create window");

    app->renderer = SDL_CreateRenderer(
        app->window,
        -1,
        0);
    if (app->renderer == NULL)
        panic("failed to create renderer");
}

void app_destroy(rg_app *app)
{
    if (app == NULL)
        return;
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
}