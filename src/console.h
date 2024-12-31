#ifndef CONSOLE_H
#define CONSOLE_H

#include <SDL.h>

#include "tileset.h"

typedef struct rg_console
{
    int width;
    int height;
    rg_tileset* tileset;
    SDL_Renderer* renderer;
} rg_console;

void console_create(rg_console* c, SDL_Renderer* r, int w, int h, rg_tileset* ts);
void console_destroy(rg_console* c);

void console_print(rg_console* c, int x, int y, int ch);

#endif