#ifndef CONSOLE_H
#define CONSOLE_H

#include <SDL.h>

#include "tileset.h"

typedef struct rg_console
{
    int width;
    int height;
    rg_tileset *tileset;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
} rg_console;

void console_create(rg_console *c,
                    SDL_Renderer *r,
                    int w,
                    int h,
                    rg_tileset *ts);
void console_destroy(rg_console *c);

void console_print(rg_console *c, int x, int y, int ch, SDL_Color color);
void console_print_txt(rg_console *c, int x, int y, const char* txt, SDL_Color color);
void console_fill(rg_console *c, int x, int y, SDL_Color color);
void console_fill_rect(rg_console *c, int x, int y, int w, int h, SDL_Color color);

void console_clear(rg_console *c, SDL_Color color);
void console_begin(rg_console *c);
void console_end(rg_console *c);
void console_flush(rg_console *c, int x, int y);

#endif