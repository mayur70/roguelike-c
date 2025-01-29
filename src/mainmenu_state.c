#include "mainmenu_state.h"

#include "app.h"
#include "color.h"
#include "savefile.h"
#include "ui.h"

void mainmenu_state_create(rg_main_menu_state_data* data, rg_app* app)
{
    data->load_file_error = false;
    console_create(&data->console,
                   app->renderer,
                   app->screen_width,
                   app->screen_height,
                   app->terminal.tileset);
    console_create(&data->menu_options,
                   app->renderer,
                   app->screen_width,
                   app->screen_height,
                   app->terminal.tileset);
    console_create(&data->message_box,
                   app->renderer,
                   app->screen_width,
                   app->screen_height,
                   app->terminal.tileset);
}

void mainmenu_state_destroy(rg_main_menu_state_data* data)
{
    data->load_file_error = false;
    console_destroy(&data->console);
    console_destroy(&data->message_box);
    console_destroy(&data->menu_options);
}

void mainmenu_state_update(struct rg_app* app,
                           SDL_Event* event,
                           rg_main_menu_state_data* data)
{
    switch (event->type)
    {
    case SDL_KEYDOWN:
    {
        switch (event->key.keysym.sym)
        {
        case SDLK_a:
            game_state_create_game(&app->game_state_data, app);
            app->screen = APP_SCREEN_PLAY;
            break;
        case SDLK_b:
            data->load_file_error =
              !game_state_load_game(&app->game_state_data, app);
            if (!data->load_file_error)
            {
                app->screen = APP_SCREEN_PLAY;
            }
            break;
        case SDLK_c:
            // Exit
            app->running = false;
            break;
        }
        break;
    }
    }
}

void mainmenu_state_draw(struct rg_app* app, rg_main_menu_state_data* data)
{

    console_begin(&data->console);
    console_clear(&data->console, ((SDL_Color){ 0, 0, 0, 0 }));
    {
        const char* txt = "TOMBS OF THE ANCIENT KINGS";
        int sz = (int)strlen(txt);
        int x = app->screen_width / 2 - sz / 2;
        int y = app->screen_height / 2 - 4;
        console_print_txt(&data->console, x, y, txt, YELLOW);
    }
    {
        const char* txt = "By Mayur Patil";
        int sz = (int)strlen(txt);
        int x = app->screen_width / 2 - sz / 2;
        int y = app->screen_height - 2;
        console_print_txt(&data->console, x, y, txt, YELLOW);
    }
    console_end(&data->console);

    console_begin(&data->menu_options);
    console_clear(&data->menu_options, ((SDL_Color){ 0, 0, 0, 0 }));
    int menu_options_height;
    {
        const char* options[] = { "Play a new game",
                                  "Continue last game",
                                  "Quit" };
        menu_draw(
          &data->menu_options, "", 3, options, 24, &menu_options_height);
    }
    console_end(&data->menu_options);

    int message_box_height;
    if (data->load_file_error)
    {
        console_begin(&data->message_box);
        console_clear(&data->message_box, ((SDL_Color){ 0, 0, 0, 0 }));
        menu_draw(&data->message_box,
                  "No save game to load",
                  0,
                  NULL,
                  50,
                  &message_box_height);
        console_end(&data->menu_options);
    }

    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    SDL_RenderClear(app->renderer);
    SDL_RenderCopy(app->renderer, app->main_menu_bg_texture, NULL, NULL);
    console_flush(&data->console, 0, 0);
    console_flush(&data->menu_options,
                  app->screen_width / 2 - 24 / 2,
                  app->screen_height / 2 - menu_options_height / 2);
    if (data->load_file_error)
    {
        console_flush(&data->message_box,
                      app->screen_width / 2 - 50 / 2,
                      app->screen_height / 2 - message_box_height / 2);
    }
    SDL_RenderPresent(app->renderer);
}