#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include "events.h"

typedef struct rg_player_equipments
{
    struct rg_item* main_hand;
    struct rg_item* off_hand;
} rg_player_equipments;

void equipment_toggle_equip(rg_player_equipments* e,
                            struct rg_item* i,
                            int* len,
                            rg_action* actions,
                            struct rg_item** items);
int equipment_get_max_hp_bonus(rg_player_equipments* e);
int equipment_get_power_bonus(rg_player_equipments* e);
int equipment_get_defense_bonus(rg_player_equipments* e);

#endif