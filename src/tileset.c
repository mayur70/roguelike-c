#include "tileset.h"

#include <assert.h>
#include <string.h>

#include <SDL_image.h>
#include <SDL_ttf.h>

#include "color.h"
#include "panic.h"
#include "types.h"

int tcod_tileset_chars[TILESET_CHARS_TOTAL] = {
    0x20,   0x21,   0x22,   0x23,   0x24,   0x25,   0x26,   0x27,   0x28,
    0x29,   0x2A,   0x2B,   0x2C,   0x2D,   0x2E,   0x2F,   0x30,   0x31,
    0x32,   0x33,   0x34,   0x35,   0x36,   0x37,   0x38,   0x39,   0x3A,
    0x3B,   0x3C,   0x3D,   0x3E,   0x3F,   0x40,   0x5B,   0x5C,   0x5D,
    0x5E,   0x5F,   0x60,   0x7B,   0x7C,   0x7D,   0x7E,   0x2591, 0x2592,
    0x2593, 0x2502, 0x2500, 0x253C, 0x2524, 0x2534, 0x251C, 0x252C, 0x2514,
    0x250C, 0x2510, 0x2518, 0x2598, 0x259D, 0x2580, 0x2596, 0x259A, 0x2590,
    0x2597, 0x2191, 0x2193, 0x2190, 0x2192, 0x25B2, 0x25BC, 0x25C4, 0x25BA,
    0x2195, 0x2194, 0x2610, 0x2611, 0x25CB, 0x25C9, 0x2551, 0x2550, 0x256C,
    0x2563, 0x2569, 0x2560, 0x2566, 0x255A, 0x2554, 0x2557, 0x255D, 0x00,
    0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x41,   0x42,   0x43,
    0x44,   0x45,   0x46,   0x47,   0x48,   0x49,   0x4A,   0x4B,   0x4C,
    0x4D,   0x4E,   0x4F,   0x50,   0x51,   0x52,   0x53,   0x54,   0x55,
    0x56,   0x57,   0x58,   0x59,   0x5A,   0x00,   0x00,   0x00,   0x00,
    0x00,   0x00,   0x61,   0x62,   0x63,   0x64,   0x65,   0x66,   0x67,
    0x68,   0x69,   0x6A,   0x6B,   0x6C,   0x6D,   0x6E,   0x6F,   0x70,
    0x71,   0x72,   0x73,   0x74,   0x75,   0x76,   0x77,   0x78,   0x79,
    0x7A,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
    0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
    0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
    0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
    0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
    0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
    0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
    0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
    0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
    0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
    0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
    0x00,   0x00,   0x00,   0x00,
};

void tileset_create(rg_tileset* ts,
                    SDL_Renderer* renderer,
                    const char* file,
                    int tiles_wide,
                    int tiles_high)
{
    memset(ts, 0, sizeof(rg_tileset));
    // Texture
    {
        SDL_Surface* s = IMG_Load(file);
        if (s == NULL) panic("failed to load tileset texture");
        ts->texture = SDL_CreateTextureFromSurface(renderer, s);
        if (ts->texture == NULL)
            panic("failed to create tileset texture from surface");
        SDL_FreeSurface(s);
    }
    int w, h;
    SDL_QueryTexture(ts->texture, NULL, NULL, &w, &h);
    ts->tile_size = w / tiles_wide;
    ts->tiles_wide = tiles_wide;
    ts->tiles_high = tiles_high;

    memcpy(ts->char_indices, tcod_tileset_chars, sizeof(ts->char_indices));

    // SRC Rects
    {
        int x = 0, y = 0;
        for (int i = 0; i < TILESET_CHARS_TOTAL; i++)
        {
            if (x + ts->tile_size > ts->tiles_wide * ts->tile_size)
            {
                x = 0;
                y += ts->tile_size;
                assert(y + ts->tile_size <= ts->tiles_high * ts->tile_size);
            }
            SDL_Rect* r = &ts->srcs[i];
            r->x = x;
            r->y = y;
            r->w = ts->tile_size;
            r->h = ts->tile_size;
            x += ts->tile_size;
        }
    }
}

void tileset_create_from_ttf(rg_tileset* ts,
                             SDL_Renderer* renderer,
                             const char* file,
                             int ptsize,
                             int tiles_wide,
                             int tiles_high)
{
    memset(ts, 0, sizeof(rg_tileset));
    TTF_Init();

    TTF_Font* fnt = TTF_OpenFontDPI(file, ptsize, 96, 96);
    ASSERT_M(fnt != NULL);

    SDL_Surface* tmp = TTF_RenderGlyph32_LCD(fnt, 'M', WHITE, BLACK);
    ASSERT_M(tmp != NULL);

    ts->tile_size = tmp->h;
    ts->tiles_wide = tiles_wide;
    ts->tiles_high = tiles_high;
    int texture_width = tmp->h * tiles_wide;
    int texture_height = tmp->h * tiles_high;
    SDL_Surface* main_surface = SDL_CreateRGBSurfaceWithFormat(
      0, texture_width, texture_height, 32, tmp->format->format);
    ASSERT_M(main_surface != NULL);
    /*ts->texture = SDL_CreateTexture(renderer,
                                    SDL_PIXELFORMAT_RGBA32,
                                    SDL_TEXTUREACCESS_TARGET,
                                    texture_width,
                                    texture_height);*/
    SDL_FreeSurface(tmp);

    //SDL_SetRenderTarget(renderer, ts->texture);
    
    int x = 0, y = 0;
    for (int i = 0; i < TILESET_CHARS_TOTAL; i++)
    {
        int ch = tcod_tileset_chars[i];
        SDL_Surface* s = TTF_RenderGlyph32_LCD(fnt, ch, WHITE, BLACK);
        if (s == NULL)
        {
            x += ts->tile_size;
            continue;
        }
        if (x + ts->tile_size > texture_width)
        {
            x = 0;
            y += ts->tile_size;
        }
        //SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
        SDL_Rect dest;
        dest.x = x + (ts->tile_size / 2 - s->w / 2);
        dest.y = y;
        dest.w = s->w;
        dest.h = s->h;
        if (ts->tile_size == -1) ts->tile_size = s->h;
        //SDL_RenderCopy(renderer, t, NULL, &dest);
        SDL_BlitSurface(s, NULL, main_surface, &dest);
        /*
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_Rect bound = { x, y, s->h, s->h };
        SDL_RenderDrawRect(renderer, &bound);*/

        SDL_FreeSurface(s);
        //SDL_DestroyTexture(t);
        x += ts->tile_size;
    }
    SDL_SetColorKey(
      main_surface, SDL_TRUE, SDL_MapRGB(main_surface->format, 0, 0, 0));
    ts->texture = SDL_CreateTextureFromSurface(renderer, main_surface);
    ASSERT_M(ts->texture != NULL);
    //SDL_SetRenderTarget(renderer, NULL);
    TTF_CloseFont(fnt);
    TTF_Quit();

    memcpy(ts->char_indices, tcod_tileset_chars, sizeof(ts->char_indices));

    // SRC Rects
    {
        int x = 0, y = 0;
        for (int i = 0; i < TILESET_CHARS_TOTAL; i++)
        {
            if (x + ts->tile_size > ts->tiles_wide * ts->tile_size)
            {
                x = 0;
                y += ts->tile_size;
                assert(y + ts->tile_size <= ts->tiles_high * ts->tile_size);
            }
            SDL_Rect* r = &ts->srcs[i];
            r->x = x;
            r->y = y;
            r->w = ts->tile_size;
            r->h = ts->tile_size;
            x += ts->tile_size;
        }
    }
}

void tileset_destroy(rg_tileset* ts)
{
    if (ts == NULL) return;
    if (ts->texture != NULL) SDL_DestroyTexture(ts->texture);
}