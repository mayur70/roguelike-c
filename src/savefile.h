#ifndef SAVEFILE_H
#define SAVEFILE_H

#include <stdbool.h>

#include "gameplay_state.h"

bool savefile_exists(const char* path);
void savefile_save(rg_game_state_data* data, const char* path);
void savefile_load(rg_game_state_data* data, const char* path);

#endif