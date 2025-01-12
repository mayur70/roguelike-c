#ifndef TURN_LOG_H
#define TURN_LOG_H

#include <stddef.h>

#include <SDL.h>

typedef enum rg_turn_log_type
{
    TURN_LOG_MESSAGE,
    TURN_LOG_DEAD,

    TURN_LOG_COUNT
} rg_turn_log_type;

typedef struct rg_turn_log_msg
{
    char *text;
    SDL_Color color;
} rg_turn_log_msg;

typedef struct rg_turn_log_entry
{
    rg_turn_log_type type;
    char *text;
    SDL_Color color;
    // union
    //{
    //rg_turn_log_msg msg;
    // struct rg_entity *entity;
    //};
} rg_turn_log_entry;

typedef struct rg_turn_logs
{
    rg_turn_log_entry *data;
    size_t len;
    size_t capacity;
} rg_turn_logs;

void turn_logs_create(rg_turn_logs *logs);
void turn_logs_destroy(rg_turn_logs *logs);

void turn_logs_clear(rg_turn_logs *logs);
void turn_logs_push(rg_turn_logs *logs, rg_turn_log_entry *entry);

void turn_logs_print(rg_turn_logs *logs);

#endif