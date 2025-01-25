#include "console.h"
#include <string.h>

#include "color.h"
#include "types.h"

void console_create(rg_console *c,
                    SDL_Renderer *r,
                    int w,
                    int h,
                    rg_tileset *ts)
{
    memset(c, 0, sizeof(c));
    c->renderer = r;
    c->width = w;
    c->height = h;
    c->tileset = ts;
    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(0, &mode);

    int render_w = c->width * c->tileset->tile_size;
    int render_h = c->height * c->tileset->tile_size;
    c->texture = SDL_CreateTexture(
      r, mode.format, SDL_TEXTUREACCESS_TARGET, render_w, render_h);
    SDL_SetTextureBlendMode(c->texture, SDL_BLENDMODE_BLEND);
}

void console_destroy(rg_console *c)
{
    if (c == NULL) return;
    if (c->texture != NULL) SDL_DestroyTexture(c->texture);
}

void console_print(rg_console *c, int x, int y, int ch, SDL_Color color)
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

    SDL_SetTextureColorMod(c->tileset->texture, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(c->tileset->texture, color.a);

    SDL_Rect *src = &c->tileset->srcs[idx];

    SDL_Rect dest = { x * c->tileset->tile_size,
                      y * c->tileset->tile_size,
                      c->tileset->tile_size,
                      c->tileset->tile_size };

    SDL_RenderCopy(c->renderer, c->tileset->texture, src, &dest);
}

void console_print_txt(rg_console *c,
                       int x,
                       int y,
                       const char *txt,
                       SDL_Color color)
{
    ASSERT_M(txt != NULL);
    int penx = x, peny = y;
    while (*txt != '\0')
    {
        console_print(c, penx, peny, *txt, color);
        penx += 1;
        txt++;
    }
}

void console_fill(rg_console *c, int x, int y, SDL_Color color)
{
    console_fill_rect(c, x, y, 1, 1, color);
}

void console_fill_rect(rg_console *c,
                       int x,
                       int y,
                       int w,
                       int h,
                       SDL_Color color)
{
    SDL_SetRenderDrawColor(c->renderer, color.r, color.g, color.b, color.a);
    SDL_Rect dest = { .x = x * c->tileset->tile_size,
                      .y = y * c->tileset->tile_size,
                      .w = w * c->tileset->tile_size,
                      .h = h * c->tileset->tile_size };
    SDL_RenderFillRect(c->renderer, &dest);
}

void console_clear(rg_console *c, SDL_Color color)
{
    SDL_SetRenderDrawColor(c->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(c->renderer);
}

void console_begin(rg_console *c)
{
    ASSERT_M(c->texture != NULL);
    SDL_SetRenderTarget(c->renderer, c->texture);
}

void console_end(rg_console *c)
{
    SDL_SetRenderTarget(c->renderer, NULL);
}

void console_flush(rg_console *c, int x, int y)
{
    ASSERT_M(c->texture != NULL);

    SDL_Rect dest = { .x = x * c->tileset->tile_size,
                      .y = y * c->tileset->tile_size,
                      .w = c->width * c->tileset->tile_size,
                      .h = c->height * c->tileset->tile_size };
    SDL_RenderCopy(c->renderer, c->texture, NULL, &dest);
}