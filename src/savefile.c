#include "savefile.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "types.h"

#define TEXT_LEN_M "text_len=%zu"
#define TEXT_M "text="
const char* fmt_color = "color=[%hhu,%hhu,%hhu,%hhu]";
const char* fmt_txt = TEXT_LEN_M " " TEXT_M "%s";

const char* fmt_entity_len = "entity_len=%zu";
const char* fmt_entity = "entity pos=[%d,%d] ch=%c color=[%hhu,%hhu,%hhu,%hhu]";
const char* fmt_entity_name = " name='%s' ";
const char* fmt_entity_rem = "blocks=%d type=%d state=[%d, %ld] "
                             "fighter=[%d,%d,%d,%d] render_order=%d";

const char* fmt_item_len = "item_len=%zu";
const char* fmt_item = "item pos=[%d,%d] ch=%c color=[%hhu,%hhu,%hhu,%hhu]";
const char* fmt_item_name = " name='%s' ";
const char* fmt_item_rem = "visible=%d type=%d";
const char* fmt_item_heal = " amount=%d";
const char* fmt_item_lightning = " damage=%d range=%d";
const char* fmt_item_fireball = " damage=%d radius=%d";
const char* fmt_item_confuse = " duration=%d";

const char* fmt_inventory_len = "inventory_len=%zu";
const char* fmt_inventory = "inventory id=%zu";

const char* fmt_game_map = "map=[%d,%d]";
const char* fmt_tile_len = "tile_len=%zu";
const char* fmt_tile = "tile=[%d,%d,%d]";

const char* fmt_player_index = "playerid=%zu";
const char* fmt_game_state = "game_state=%zu";

const char* fmt_turn_log_len = "log_len=%zu";
const char* fmt_turn_log =
  "log type=%d color=[%hhu,%hhu,%hhu,%hhu] text_size=%zu";
const char* fmt_turn_log_text = " text=%s";

char* next_line(char* start)
{
    char* end = start;
    while (*end != EOF && *end != '\0' && *end != '\n') end++;
    if (*end == '\n') end++;
    return end;
}

static void add_space(FILE* fp)
{
    const char* s = "%c";
    fprintf(fp, s, ' ');
}

static void color_save(SDL_Color* c, FILE* fp)
{
    fprintf(fp, fmt_color, c->r, c->g, c->b, c->a);
}

static char* color_load(SDL_Color* c, char* buf)
{
    int ret = sscanf(buf, fmt_color, &c->r, &c->g, &c->b, &c->a);
    ASSERT_M(ret == 4);
    return strchr(buf, ' ');
}

static void text_save(const char* txt, FILE* fp)
{
    fprintf(fp, fmt_txt, strlen(txt), txt);
}

static char* text_load(char* txt, char* buf)
{
    size_t sz = 0;
    sscanf_s(buf, TEXT_LEN_M, &sz);
    char* start = strstr(buf, TEXT_M) + 5;
    ASSERT_M(start != NULL);
    memcpy(txt, start, sz);
    return start + sz;
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
            e->color.a);
    fprintf(fp, fmt_entity_name, e->name);
    fprintf(fp,
            fmt_entity_rem,
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

void item_save(rg_item* i, FILE* fp)
{
    fprintf(fp,
            fmt_item,
            i->x,
            i->y,
            i->ch,
            i->color.r,
            i->color.g,
            i->color.b,
            i->color.a);
    fprintf(fp, fmt_item_name, i->name);
    fprintf(fp, fmt_item_rem, i->visible_on_map, i->type);
    switch (i->type)
    {
    case ITEM_POTION_HEAL:
        fprintf(fp, fmt_item_heal, i->heal.amount);
        break;
    case ITEM_LIGHTNING:
        fprintf(fp,
                fmt_item_lightning,
                i->lightning.damage,
                i->lightning.maximum_range);
        break;
    case ITEM_FIRE_BALL:
        fprintf(fp, fmt_item_fireball, i->fireball.damage, i->fireball.radius);
        add_space(fp);
        color_save(&i->fireball.targeting_msg_color, fp);
        add_space(fp);
        text_save(i->fireball.targeting_msg, fp);
        add_space(fp);
        break;
    case ITEM_CAST_CONFUSE:
        fprintf(fp, fmt_item_confuse, i->confuse.duration);
        add_space(fp);
        color_save(&i->fireball.targeting_msg_color, fp);
        add_space(fp);
        text_save(i->fireball.targeting_msg, fp);
        add_space(fp);
        break;
    default:
        ASSERT_M(false);
    }
}

void entity_load(rg_entity* e, const char* buf)
{
    int ret = sscanf(buf,
                     fmt_entity,
                     &e->x,
                     &e->y,
                     &e->ch,
                     &e->color.r,
                     &e->color.g,
                     &e->color.b,
                     &e->color.a);
    ASSERT_M(ret == 7);

    char* nstart = strstr(buf, "name='") + 6;
    ASSERT_M(nstart != 0);
    char* nend = strchr(nstart, '\'');
    size_t sz = nend - nstart;
    memcpy(e->name, nstart, sz);
    e->name[sz] = '\0';
    nend += 2;
    //"blocks=1 type=0 state=[0, 0] fighter=[30,30,2,5] render_order=1"
    //"blocks=%d type=%d state=[%d, %ld] fighter=[%d,%d,%d,%d] render_order=%d"
    int blocks = 0;
    ret = sscanf_s(nend,
                   fmt_entity_rem,
                   &blocks,
                   &e->type,
                   &e->state.type,
                   &e->state.data.packed,
                   &e->fighter.max_hp,
                   &e->fighter.hp,
                   &e->fighter.defence,
                   &e->fighter.power,
                   &e->render_order);
    ASSERT_M(ret == 9);
    e->blocks = blocks;
}

char* entities_load(rg_entity_array* entities, char* buf)
{
    memset(entities, sizeof(*entities), 0);
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

char* item_load(rg_item* i, const char* buf)
{
    memset(i, sizeof(*i), 0);
    int visible = 0;
    int ret = sscanf(buf,
                     fmt_item,
                     &i->x,
                     &i->y,
                     &i->ch,
                     &i->color.r,
                     &i->color.g,
                     &i->color.b,
                     &i->color.a);
    ASSERT_M(ret == 7);
    char* nstart = strstr(buf, "name='") + 6;
    ASSERT_M(nstart != 0);
    char* ptr = strchr(nstart, '\'');
    size_t sz = ptr - nstart;
    memcpy(i->name, nstart, sz);
    i->name[sz] = '\0';
    ptr += 2;
    ret = sscanf_s(ptr, fmt_item_rem, &visible, &i->type);
    i->visible_on_map = visible;
    ptr = strstr(ptr, "type=") + 5 + 1 + 1;
    ASSERT_M(ret == 2);

    switch (i->type)
    {
    case ITEM_POTION_HEAL:
    {
        int res = sscanf_s(ptr, fmt_item_heal, &i->heal.amount);
        ASSERT_M(res == 1);
        break;
    }
    case ITEM_LIGHTNING:
    {

        int res = sscanf_s(ptr,
                           fmt_item_lightning,
                           &i->lightning.damage,
                           &i->lightning.maximum_range);
        ASSERT_M(res == 2);
        break;
    }
    case ITEM_FIRE_BALL:
    {
        int res = sscanf_s(
          ptr, fmt_item_fireball, &i->fireball.damage, &i->fireball.radius);
        ASSERT_M(res == 2);
        ptr = strstr(ptr, "color=");
        ptr = color_load(&i->fireball.targeting_msg_color, ptr);
        ptr++;
        ptr = text_load((char*)i->fireball.targeting_msg, ptr);
        break;
    }
    case ITEM_CAST_CONFUSE:
    {
        int res = sscanf_s(ptr, fmt_item_confuse, &i->confuse.duration);
        ASSERT_M(res == 1);
        ptr = strstr(ptr, "color=");
        ptr = color_load(&i->fireball.targeting_msg_color, ptr);
        ptr = text_load((char*)i->fireball.targeting_msg, ptr + 1);
        break;
    }
    default:
        ASSERT_M(false);
    }
    ptr = next_line(ptr);
    return ptr;
}

char* items_load(rg_items* items, char* buf)
{
    memset(items, sizeof(*items), 0);
    const int ret = sscanf_s(buf, fmt_item_len, &items->len);
    ASSERT_M(ret == 1);

    items->capacity = items->len;
    size_t items_sz = sizeof(*items->data) * items->capacity;
    items->data = malloc(items_sz);
    ASSERT_M(items->data != NULL);
    char* line = next_line(buf);
    for (size_t i = 0; i < items->len; i++)
    {
        rg_item* it = &items->data[i];
        line = item_load(it, line);
        if (*line == '\n') line++;
    }
    return line;
}

static char* inventory_load(rg_inventory* iv, char* buf)
{
    memset(iv, sizeof(*iv), 0);
    inventory_create(iv, 26);
    const int ret = sscanf_s(buf, fmt_inventory_len, &iv->len);
    ASSERT_M(ret == 1);
    if (iv->len == 0) return next_line(buf);
    char* line = next_line(buf);
    for (size_t i = 0; i < iv->len; i++)
    {
        size_t* it = &iv->data[i];
        sscanf_s(line, fmt_inventory, it);
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
    memset(m, sizeof(*m), 0);
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
    fprintf(fp, "\n");
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
                sizeof(char) * strlen(e->text));
        fprintf(fp, fmt_turn_log_text, e->text);
        fprintf(fp, "\n");
    }
}

static char* turn_logs_load(rg_turn_logs* logs, char* buf)
{
    memset(logs, sizeof(*logs), 0);
    size_t len;
    sscanf_s(buf, fmt_turn_log_len, &len);
    if (len == 0)
    {
        logs->len = logs->capacity = 0;
        buf = next_line(buf);
        return buf;
    }
    logs->len = len;
    logs->capacity = logs->len;
    if (logs->capacity > 0)
        logs->data = malloc(sizeof(*logs->data) * logs->capacity);
    buf = next_line(buf);
    for (size_t i = 0; i < logs->len; i++)
    {
        rg_turn_log_entry* e = &logs->data[i];
        size_t sz;
        int ret = sscanf_s(buf,
                           fmt_turn_log,
                           &e->type,
                           &e->color.r,
                           &e->color.g,
                           &e->color.b,
                           &e->color.a,
                           &sz);
        ASSERT_M(ret == 6);
        e->text = malloc(sz + 1);
        char* tstart = strstr(buf, "text=") + 5;
        ASSERT_M(tstart != '\0');
        memcpy(e->text, tstart, sz);
        e->text[sz] = '\0';
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
    fprintf(fp, fmt_item_len, data->items.len);
    fprintf(fp, "\n");
    for (size_t i = 0; i < data->items.len; i++)
    {
        item_save(&data->items.data[i], fp);
        fprintf(fp, "\n");
    }
    fprintf(fp, fmt_inventory_len, data->inventory.len);
    fprintf(fp, "\n");
    for (size_t i = 0; i < data->inventory.len; i++)
    {
        fprintf(fp, fmt_inventory, data->inventory.data[i]);
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
    rg_items* items = &data->items;
    rg_map* map = &data->game_map;
    rg_turn_logs* logs = &data->logs;
    rg_inventory* inventory = &data->inventory;
    line = entities_load(entities, line);
    line = items_load(items, line);
    line = inventory_load(inventory, line);
    line = game_map_load(map, line);
    sscanf_s(line, fmt_player_index, &data->player);
    line = next_line(line);
    sscanf_s(line, fmt_game_state, &data->game_state);
    line = next_line(line);
    line = turn_logs_load(logs, line);
    if (logs->capacity == 0)
    {
        turn_logs_create(logs, data->message_width, data->message_height);
    }
    free(buf);
}

bool savefile_exists(const char* path)
{
    FILE* fp = fopen(path, "r+b");
    if (fp == NULL) return false;
    fclose(fp);
    return true;
}