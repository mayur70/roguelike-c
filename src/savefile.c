#include "savefile.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "types.h"

const char* fmt_entity_len = "entity_len=%zu";
const char* fmt_entity = "entity pos=[%d,%d] ch=%c color=[%hhu,%hhu,%hhu,%hhu] "
                         "name=%s blocks=%d, type=%d state=[%d, %ld] "
                         "fighter=[%d,%d,%d,%d] render_order=%d";

const char* fmt_game_map = "map=[%d,%d]";
const char* fmt_tile_len = "tile_len=%zu";
const char* fmt_tile = "tile=[%d,%d,%d]";

const char* fmt_player_index = "playerid=%zu";
const char* fmt_game_state = "game_state=%zu";

const char* fmt_turn_log_len = "log_len=%zu";
const char* fmt_turn_log = "log type=%d color=[%hhu,%hhu,%hhu,%hhu] text=%s";

char* next_line(char* start)
{
    char* end = start;
    while (*end != EOF && *end != '\0' && *end != '\n') end++;
    if (*end == '\n') end++;
    return end;
}

void entity_save(rg_entity* e, FILE* fp)
{
    fprintf(fp,
            fmt_entity,
            e->x,
            e->y,
            e->ch,
            e->color.r,
            e->color.g,
            e->color.b,
            e->color.a,
            e->name,
            e->blocks,
            e->type,
            e->state.type,
            e->state.data.packed,
            e->fighter.max_hp,
            e->fighter.hp,
            e->fighter.defence,
            e->fighter.power,
            e->render_order);
}

void entity_load(rg_entity* e, const char* buf)
{
    int blocks = 0;
    const int ret = sscanf(buf,
                           fmt_entity,
                           &e->x,
                           &e->y,
                           &e->ch,
                           &e->color.r,
                           &e->color.g,
                           &e->color.b,
                           &e->color.a,
                           &e->name,
                           &blocks,
                           &e->type,
                           &e->state.type,
                           &e->state.data.packed,
                           &e->fighter.max_hp,
                           &e->fighter.hp,
                           &e->fighter.defence,
                           &e->fighter.power,
                           &e->render_order);
    e->blocks = blocks;
    ASSERT_M(ret == 17);
}

char* entities_load(rg_entity_array* entities, char* buf)
{
    const int ret = sscanf_s(buf, fmt_entity_len, &entities->len);
    ASSERT_M(ret == 1);

    entities->capacity = entities->len;
    size_t entities_sz = sizeof(*entities->data) * entities->capacity;
    entities->data = malloc(entities_sz);
    ASSERT_M(entities->data != NULL);
    char* line = next_line(buf);
    for (size_t i = 0; i < entities->len; i++)
    {
        rg_entity* e = &entities->data[i];
        entity_load(e, line);
        line = next_line(line);
    }
    return line;
}

void tile_save(rg_tile* t, FILE* fp)
{
    fprintf(fp, fmt_tile, t->blocked, t->block_sight, t->explored);
}

void game_map_save(rg_map* m, FILE* fp)
{
    fprintf(fp, fmt_game_map, m->width, m->height);
    fprintf(fp, "\n");
    fprintf(fp, fmt_tile_len, m->tiles.len);
    fprintf(fp, "\n");
    for (size_t i = 0; i < m->tiles.len; i++)
    {
        tile_save(&m->tiles.data[i], fp);
        fprintf(fp, "\n");
    }
}

char* game_map_load(rg_map* m, char* buf)
{
    int ret;
    ret = sscanf_s(buf, fmt_game_map, &m->width, &m->height);
    ASSERT_M(ret == 2);
    char* line = next_line(buf);
    ret = sscanf_s(line, fmt_tile_len, &m->tiles.len);
    ASSERT_M(ret == 1);
    
    line = next_line(line);
    m->tiles.capacity = m->tiles.len;
    m->tiles.data = malloc(sizeof(*m->tiles.data) * m->tiles.capacity);
    for (size_t i = 0; i < m->tiles.len; i++)
    {
        int blocked, block_sight, explored;
        ret = sscanf_s(line, fmt_tile, &blocked, &block_sight, &explored);
        ASSERT_M(ret == 3);
        rg_tile* t = &m->tiles.data[i];
        t->blocked = blocked;
        t->block_sight = block_sight;
        t->explored = explored;
        line = next_line(line);
    }
    return line;
}

void turn_log_save(rg_turn_logs* logs, FILE* fp)
{
    fprintf(fp, fmt_turn_log_len, logs->len);
    for (size_t i = 0; i < logs->len; i++)
    {
        rg_turn_log_entry* e = &logs->data[i];
        fprintf(fp,
                fmt_turn_log,
                e->type,
                e->color.r,
                e->color.g,
                e->color.b,
                e->color.a,
                e->text);
        fprintf(fp, "\n");
    }
}

char* turn_logs_load(rg_turn_logs* logs, char* buf)
{
    sscanf_s(buf, fmt_turn_log_len, &logs->len);
    logs->capacity = logs->len;
    if (logs->capacity > 0)
        logs->data = malloc(sizeof(*logs->data) * logs->capacity);
    buf = next_line(buf);
    for (size_t i = 0; i < logs->len; i++)
    {
        rg_turn_log_entry* e = &logs->data[i];
        sscanf_s(buf,
                 fmt_turn_log,
                 e->type,
                 e->color.r,
                 e->color.g,
                 e->color.b,
                 e->color.a,
                 e->text); // TODO: BUG: e->text is not allocated
        buf = next_line(buf);
    }
    return buf;
}

bool verify_save(size_t actual, size_t expected)
{
    if (actual != expected)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "failed to save file %s",
                     SDL_GetError());
        return false;
    }
    return true;
}

void savefile_save(rg_game_state_data* data, const char* path)
{
    FILE* fp = fopen(path, "w+b");
    ASSERT_M(fp != NULL);
    fprintf(fp, fmt_entity_len, data->entities.len);
    fprintf(fp, "\n");
    for (size_t i = 0; i < data->entities.len; i++)
    {
        entity_save(&data->entities.data[i], fp);
        fprintf(fp, "\n");
    }
    game_map_save(&data->game_map, fp);
    fprintf(fp, fmt_player_index, data->player);
    fprintf(fp, "\n");
    fprintf(fp, fmt_game_state, data->game_state);
    fprintf(fp, "\n");
    turn_log_save(&data->logs, fp);
    fclose(fp);
}

void savefile_load(rg_game_state_data* data, const char* path)
{
    FILE* fp = fopen(path, "r+b");
    ASSERT_M(fp != NULL);
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buf = malloc(sizeof(char) * (sz + 1));
    fread(buf, sizeof(char), sz, fp);
    fclose(fp);
    buf[sz] = '\0';

    char* line = buf;
    rg_entity_array* entities = &data->entities;
    rg_map* map = &data->game_map;
    rg_turn_logs* logs = &data->logs;
    line = entities_load(entities, line);
    line = game_map_load(map, line);
    sscanf_s(line, fmt_player_index, &data->player);
    line = next_line(line);
    sscanf_s(line, fmt_game_state, &data->game_state);
    line = next_line(line);
    line = turn_logs_load(logs, line);

    free(buf);
}

bool savefile_exists(const char* path)
{
    FILE* fp = fopen(path, "r+b");
    if (fp == NULL) return false;
    fclose(fp);
    return true;
}