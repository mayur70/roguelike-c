#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>

#include "app.h"
#include "tileset.h"

typedef struct rg_terminal
{
    int width;
    int height;
    rg_app *app;
    rg_tileset *tileset;
} rg_terminal;

void terminal_create(rg_terminal *t,
                     rg_app *app,
                     int w,
                     int h,
                     rg_tileset *tileset,
                     const char *title,
                     bool vsync);
void terminal_destroy(rg_terminal *t);

#endif // TERMINAL_H