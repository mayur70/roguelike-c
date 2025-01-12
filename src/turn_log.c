#include "turn_log.h"

#include <string.h>
#include <stdint.h>

#include <SDL2/SDL.h>

#include "array.h"
#include "types.h"

void turn_logs_create(rg_turn_logs* logs, int width, int height)
{
    logs->capacity = height;
    logs->data = malloc(sizeof(*logs->data) * logs->capacity);
    ASSERT_M(logs->data != NULL);
    logs->len = 0;
    logs->width = width;
    logs->height = height;
}

void turn_logs_destroy(rg_turn_logs* logs)
{
    if (logs == NULL) return;
    turn_logs_clear(logs);
    free(logs->data);
    logs->len = 0;
    logs->capacity = 0;
}

void turn_logs_clear(rg_turn_logs* logs)
{
    for (int i = 0; i < logs->len; i++)
    {
        if (logs->data[i].type == TURN_LOG_MESSAGE)
        {
            char* msg = logs->data[i].text;
            if (msg == NULL) continue;
            free(msg);
        }
    }
    logs->len = 0;
}

void turn_logs_push(rg_turn_logs* logs, rg_turn_log_entry* entry)
{
    if (entry->text != NULL)
    {
        int ch = entry->text[0];
        if (ch > 96 && ch < 122)
        {
            entry->text[0] = ch - 32;
        }
    }

    //TODO: Split lines by width
    int lines = 1;

    if (logs->len + lines > logs->height)
    {
        rg_turn_log_entry* src = logs->data + lines;
        size_t count = (logs->len - lines) * sizeof(rg_turn_log_entry);
        memmove(logs->data, src, count);
        logs->len -= lines;
    }
    ARRAY_PUSH(logs, *entry);
}

void turn_logs_print(rg_turn_logs* logs)
{
    for (int i = 0; i < logs->len; i++)
    {
        if (logs->data[i].type == TURN_LOG_MESSAGE)
        {
            if (logs->data[i].text == NULL) continue;
            SDL_Log("%s", logs->data[i].text);
        }
    }
}