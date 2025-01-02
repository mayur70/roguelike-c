#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <SDL.h>
#include <SDL_image.h>

#include "tileset.h"
#include "panic.h"
#include "console.h"
#include "app.h"
#include "terminal.h"

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))
#define ARRAY_PUSH(arr, ele)                                                   \
    do                                                                         \
    {                                                                          \
        assert(arr);                                                           \
        if ((arr)->len + 1 > (arr)->capacity)                                  \
        {                                                                      \
            (arr)->capacity *= 2;                                              \
            (arr)->data = realloc((arr)->data, sizeof(ele) * (arr)->capacity); \
            assert((arr)->data);                                               \
        }                                                                      \
        (arr)->data[(arr)->len] = ele;                                         \
        (arr)->len++;                                                          \
    } while (0)
#define DESIRED_FPS 20.0
#define DESIRED_FRAME_RATE ((1.0 / DESIRED_FPS) * 1000.0)
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

typedef enum rg_action_type
{
    ACTION_NONE,
    ACTION_QUIT,
    ACTION_ESCAPE,
    ACTION_MOVEMENT,
} rg_action_type;

typedef struct rg_action
{
    rg_action_type type;
    union
    {
        struct
        {
            int dx, dy;
        };
    };
} rg_action;

typedef struct rg_entity
{
    int x, y;
    char ch;
    SDL_Color color;
} rg_entity;

typedef struct rg_entity_array
{
    size_t len;
    size_t capacity;
    rg_entity *data;
} rg_entity_array;

void ea_push(rg_entity_array *arr, rg_entity ele)
{
    assert(arr != NULL);
    if ((arr)->len + 1 > (arr)->capacity)
    {
        (arr)->capacity *= 2;
        (arr)->data = realloc((arr)->data, sizeof(ele) * (arr)->capacity);
        assert((arr)->data);
    }
    (arr)->data[(arr)->len] = ele;
    (arr)->len++;
}

void entity_move(rg_entity *e, int dx, int dy)
{
    e->x += dx;
    e->y += dy;
}

void entity_draw(rg_entity *e, rg_console *c)
{
    console_print(c, e->x, e->y, e->ch, e->color);
}

void event_dispatch(SDL_Event *event, rg_action *action)
{
    // Default value
    action->type = ACTION_NONE;

    switch (event->type)
    {
    case SDL_QUIT:
        action->type = ACTION_QUIT;
        break;
    case SDL_KEYDOWN:
    {
        switch (event->key.keysym.sym)
        {
        case SDLK_UP:
        case SDLK_w:
            action->type = ACTION_MOVEMENT;
            action->dx = 0;
            action->dy = -1;
            break;
        case SDLK_DOWN:
        case SDLK_s:
            action->type = ACTION_MOVEMENT;
            action->dx = 0;
            action->dy = 1;
            break;
        case SDLK_LEFT:
        case SDLK_a:
            action->type = ACTION_MOVEMENT;
            action->dx = -1;
            action->dy = 0;
            break;
        case SDLK_RIGHT:
        case SDLK_d:
            action->type = ACTION_MOVEMENT;
            action->dx = 1;
            action->dy = 0;
            break;

        case SDLK_ESCAPE:
            action->type = ACTION_ESCAPE;
            break;
        }
        break;
    }
    default:
        // no-op
        break;
    }
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
#endif

    rg_app app;
    app_create(&app);

    int screen_width = 80;
    int screen_height = 50;

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
        screen_width,
        screen_height,
        &tileset,
        "roguelike",
        true);

    rg_console console;
    console_create(
        &console,
        app.renderer,
        terminal.width,
        terminal.height,
        terminal.tileset);

    rg_entity_array entities = {
        .capacity = 2,
        .data = malloc(sizeof(rg_entity) * 2),
        .len = 0,
    };
    ARRAY_PUSH(&entities, ((rg_entity){screen_width / 2, screen_height / 2, '@', WHITE}));
    ARRAY_PUSH(&entities, ((rg_entity){(screen_width / 2) - 5, screen_height / 2, '@', YELLOW}));
    rg_entity *player = &entities.data[0];
    rg_entity *npc = &entities.data[1];

    bool running = true;
    while (running)
    {

        SDL_Event event = {0};
        SDL_WaitEvent(&event);

        Uint64 frame_start = SDL_GetPerformanceCounter();

        rg_action action;
        event_dispatch(&event, &action);

        if (action.type == ACTION_NONE)
            continue;

        if (action.type == ACTION_MOVEMENT)
        {
            entity_move(player, action.dx, action.dy);
        }
        else if (action.type == ACTION_ESCAPE || action.type == ACTION_QUIT)
            running = false;

        SDL_RenderClear(app.renderer);

        for (int i = 0; i < entities.len; i++)
        {
            entity_draw(&entities.data[i], &console);
        }

        SDL_RenderPresent(app.renderer);

        Uint64 frame_end = SDL_GetPerformanceCounter();
        double dt = (frame_end - frame_start) / (double)SDL_GetPerformanceFrequency() * 1000.0;

        Uint32 sleep_time = (Uint32)floor(DESIRED_FRAME_RATE - dt);
        const Uint32 max_sleep_time = 1000;
        if (sleep_time > max_sleep_time)
            sleep_time = max_sleep_time;
        // SLEEP
        SDL_Delay(sleep_time);
    }

    free(entities.data);
    console_destroy(&console);
    terminal_destroy(&terminal);
    tileset_destroy(&tileset);

    app_destroy(&app);
    return 0;
}