#ifndef COLOR_H
#define COLOR_H

#include <SDL.h>

#define WHITE ((SDL_Color){ 255, 255, 255, 255 })
#define BLACK ((SDL_Color){ 0, 0, 0, 255 })
#define RED ((SDL_Color){ 255, 0, 0, 255 })
#define BLUE ((SDL_Color){ 0, 255, 0, 255 })
#define GREEN ((SDL_Color){ 0, 0, 255, 255 })
#define YELLOW ((SDL_Color){ 255, 255, 0, 255 })

#define DARK_WALL ((SDL_Color){ 0, 0, 100, 255 })
#define DARK_GROUND ((SDL_Color){ 50, 50, 150, 255 })
#define LIGHT_WALL ((SDL_Color){ 130, 110, 50, 255 })
#define LIGHT_GROUND ((SDL_Color){ 200, 180, 50, 255 })
#define DESATURATED_GREEN ((SDL_Color){ 63, 127, 63, 255 })
#define DARKER_GREEN ((SDL_Color){ 0, 127, 0, 255 })
#define DARK_RED ((SDL_Color){ 191, 0, 0, 255 })
#define LIGHT_RED ((SDL_Color){ 255, 63, 63, 255 })
#endif