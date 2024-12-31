#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <SDL.h>
#include <SDL_image.h>

#define TILESET_CHARS_TOTAL 256

static int tcod_tileset_chars[TILESET_CHARS_TOTAL] = {
    0x20,
    0x21,
    0x22,
    0x23,
    0x24,
    0x25,
    0x26,
    0x27,
    0x28,
    0x29,
    0x2A,
    0x2B,
    0x2C,
    0x2D,
    0x2E,
    0x2F,
    0x30,
    0x31,
    0x32,
    0x33,
    0x34,
    0x35,
    0x36,
    0x37,
    0x38,
    0x39,
    0x3A,
    0x3B,
    0x3C,
    0x3D,
    0x3E,
    0x3F,
    0x40,
    0x5B,
    0x5C,
    0x5D,
    0x5E,
    0x5F,
    0x60,
    0x7B,
    0x7C,
    0x7D,
    0x7E,
    0x2591,
    0x2592,
    0x2593,
    0x2502,
    0x2500,
    0x253C,
    0x2524,
    0x2534,
    0x251C,
    0x252C,
    0x2514,
    0x250C,
    0x2510,
    0x2518,
    0x2598,
    0x259D,
    0x2580,
    0x2596,
    0x259A,
    0x2590,
    0x2597,
    0x2191,
    0x2193,
    0x2190,
    0x2192,
    0x25B2,
    0x25BC,
    0x25C4,
    0x25BA,
    0x2195,
    0x2194,
    0x2610,
    0x2611,
    0x25CB,
    0x25C9,
    0x2551,
    0x2550,
    0x256C,
    0x2563,
    0x2569,
    0x2560,
    0x2566,
    0x255A,
    0x2554,
    0x2557,
    0x255D,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x41,
    0x42,
    0x43,
    0x44,
    0x45,
    0x46,
    0x47,
    0x48,
    0x49,
    0x4A,
    0x4B,
    0x4C,
    0x4D,
    0x4E,
    0x4F,
    0x50,
    0x51,
    0x52,
    0x53,
    0x54,
    0x55,
    0x56,
    0x57,
    0x58,
    0x59,
    0x5A,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x61,
    0x62,
    0x63,
    0x64,
    0x65,
    0x66,
    0x67,
    0x68,
    0x69,
    0x6A,
    0x6B,
    0x6C,
    0x6D,
    0x6E,
    0x6F,
    0x70,
    0x71,
    0x72,
    0x73,
    0x74,
    0x75,
    0x76,
    0x77,
    0x78,
    0x79,
    0x7A,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};

void panic(const char *msg)
{
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s %s", msg, SDL_GetError());
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        "error",
        msg,
        NULL);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        panic("failed to initalize SDL");

    int img_flags = IMG_INIT_PNG;
    if (IMG_Init(img_flags) & img_flags == 0)
        panic("failed to initialize SDL_image");

    SDL_Window *window = NULL;
    window = SDL_CreateWindow(
        "roguelike",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800,
        600,
        0);
    if (window == NULL)
        panic("failed to create window");

    SDL_Renderer *renderer = NULL;
    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL)
        panic("failed to create renderer");

    SDL_Texture *tileset_texture = NULL;
    {
        SDL_Surface *s = IMG_Load("res/dejavu10x10_gs_tc.png");
        if (s == NULL)
            panic("failed to load tileset texture");
        tileset_texture = SDL_CreateTextureFromSurface(
            renderer,
            s);
        if (tileset_texture == NULL)
            panic("failed to create tileset texture from surface");
        SDL_FreeSurface(s);
    }

    int tile_size = 10;
    int tiles_wide = 32;
    int tiles_high = 8;
    SDL_Rect tileset_src[TILESET_CHARS_TOTAL];
    {
        int x = 0, y = 0;
        for (int i = 0; i < TILESET_CHARS_TOTAL; i++)
        {
            if (x + tile_size > tiles_wide * tile_size)
            {
                x = 0;
                y += tile_size;
                assert(y + tile_size <= tiles_high * tile_size);
            }
            SDL_Rect *r = &tileset_src[i];
            r->x = x;
            r->y = y;
            r->w = tile_size;
            r->h = tile_size;

            x += tile_size;
        }
    }

    bool running = true;
    while (running)
    {
        SDL_Event event = {0};
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        const char *msg = "Hello world!";

        int x = 0, y = 0;
        while (*msg != '\0')
        {
            int idx = -1;
            for (int i = 0; i < TILESET_CHARS_TOTAL; i++)
            {
                if (*msg == tcod_tileset_chars[i])
                {
                    idx = i;
                    break;
                }
            }
            if (idx == -1)
                continue;
            SDL_Rect *src = &tileset_src[idx];

            SDL_Rect dest = {
                x,
                y,
                10,
                10};

            SDL_RenderCopy(
                renderer,
                tileset_texture,
                src,
                &dest);

            x += dest.w;
            msg++;
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(tileset_texture);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}