#include "terminal.h"

#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "app.h"

void terminal_create(rg_terminal* t,
                     rg_app* app,
                     int w,
                     int h,
                     rg_tileset* tileset,
                     const char* title,
                     bool vsync)
{
    memset(t, 0, sizeof(t));
    t->width = w;
    t->height = h;
    t->app = app;
    t->tileset = tileset;

    SDL_SetWindowSize(t->app->window,
                      t->width * t->tileset->tile_size,
                      t->height * t->tileset->tile_size);
    SDL_SetWindowTitle(t->app->window, title);
    SDL_ShowWindow(t->app->window);
    SDL_RenderSetLogicalSize(t->app->renderer,
                             t->width * t->tileset->tile_size,
                             t->height * t->tileset->tile_size);
    SDL_RenderSetVSync(t->app->renderer, vsync);
}

void terminal_destroy(rg_terminal* t)
{
    // no-op
}