#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <SDL.h>
#include <SDL_image.h>

#include "tileset.h"
#include "panic.h"
#include "console.h"
#include "app.h"
#include "terminal.h"

int main(int argc, char *argv[])
{
    rg_app app;
    app_create(&app);

    rg_tileset tileset;
    tileset_create(
        &tileset,
        app.renderer,
        "res/dejavu10x10_gs_tc.png",
        32,
        8);

    rg_terminal terminal;
    terminal_create(
        &terminal,
        &app,
        80,
        50,
        &tileset,
        "roguelike in c",
        true);

    rg_console console;
    console_create(
        &console,
        app.renderer,
        terminal.width,
        terminal.height,
        terminal.tileset);

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

        SDL_RenderClear(app.renderer);

        console_print(&console, 10, 10, '@');

        SDL_RenderPresent(app.renderer);
    }

    console_destroy(&console);
    terminal_destroy(&terminal);
    tileset_destroy(&tileset);

    app_destroy(&app);
    return 0;
}