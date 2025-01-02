#include "console.h"
#include <string.h>

void console_create(rg_console *c, SDL_Renderer *r, int w, int h, rg_tileset *ts)
{
    memset(c, 0, sizeof(c));
    c->renderer = r;
    c->width = w;
    c->height = h;
    c->tileset = ts;
}

void console_destroy(rg_console *c)
{
}

void console_print(rg_console *c,
                   int x,
                   int y,
                   int ch,
                   SDL_Color color)
{
    int idx = -1;
    for (int i = 0; i < TILESET_CHARS_TOTAL; i++)
    {
        if (ch == c->tileset->char_indices[i])
        {
            idx = i;
            break;
        }
    }
    if (idx == -1)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "unsupported char %c supplied to print.",
                    ch);
        return;
    }

    SDL_SetTextureColorMod(c->tileset->texture,
                           color.r,
                           color.g,
                           color.b);
    SDL_SetTextureAlphaMod(c->tileset->texture,
                           color.a);

    SDL_Rect *src = &c->tileset->srcs[idx];

    SDL_Rect dest = {
        x * c->tileset->tile_size,
        y * c->tileset->tile_size,
        c->tileset->tile_size,
        c->tileset->tile_size};

    SDL_RenderCopy(
        c->renderer,
        c->tileset->texture,
        src,
        &dest);
}