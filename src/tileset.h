#ifndef TILESET_H
#define TILESET_H

#include <SDL.h>

#define TILESET_CHARS_TOTAL 256

typedef struct rg_tileset
{
    int tile_size;
    int tiles_wide;
    int tiles_high;
    int char_indices[TILESET_CHARS_TOTAL];
    SDL_Rect srcs[TILESET_CHARS_TOTAL];
    SDL_Texture *texture;
} rg_tileset;

void tileset_create(rg_tileset *ts,
                  SDL_Renderer *renderer,
                  const char* file,
                  int tiles_wide,
                  int tiles_high);
void tileset_destroy(rg_tileset* ts);

#endif // TILESET_H