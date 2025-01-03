#ifndef COLOR_H
#define COLOR_H

#include <SDL.h>

#define WHITE \
    (SDL_Color) { 255, 255, 255, 255 }
#define BLACK \
    (SDL_Color) { 0, 0, 0, 255 }
#define RED \
    (SDL_Color) { 255, 0, 0, 255 }
#define BLUE \
    (SDL_Color) { 0, 255, 0, 255 }
#define GREEN \
    (SDL_Color) { 0, 0, 255, 255 }
#define YELLOW \
    (SDL_Color) { 255, 255, 0, 255 }

#define DARK_WALL \
    (SDL_Color) { 0, 0, 100, 255 }
#define DARK_GROUND \
    (SDL_Color) { 50, 50, 150, 255 }

#endif