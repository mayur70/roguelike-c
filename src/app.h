#ifndef APP_H
#define APP_H

#include <stdbool.h>

#include <SDL.h>

typedef struct rg_app
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
} rg_app;

void app_create(rg_app* app);
void app_destroy(rg_app* app);

#endif