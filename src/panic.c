#include "panic.h"

#include <stdlib.h>

#include <SDL.h>

void panic(const char *msg)
{
    SDL_LogError(
        SDL_LOG_CATEGORY_APPLICATION,
        "%s %s",
        msg,
        SDL_GetError());

    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        "error",
        msg,
        NULL);
    exit(1);
}
