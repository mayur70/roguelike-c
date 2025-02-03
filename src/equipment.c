#include "equipment.h"

#include "items.h"

void equipment_toggle_equip(rg_player_equipments* e,
                            rg_item* i,
                            int* len,
                            rg_action* actions,
                            rg_item** items)
{
    bool res = len != NULL && actions != NULL && items != NULL;
    if (res)
    {
        *len = 0;
    }
    if (i->equipable.slot == EQUIPMENT_SLOT_MAIN_HAND)
    {
        if (e->main_hand == i)
        {
            if (res)
            {
                items[(*len)] = i;
                actions[(*len)].type = ACTION_DEEQUIPPED;
                *len += 1;
            }
            e->main_hand = NULL;
        }
        else
        {
            if (e->main_hand != NULL && res)
            {
                items[(*len)] = e->main_hand;
                actions[(*len)].type = ACTION_DEEQUIPPED;
                *len += 1;
            }

            if (res)
            {

                items[(*len)] = i;
                actions[(*len)].type = ACTION_EQUIPPED;
                *len += 1;
            }
            e->main_hand = i;
        }
    }
    else if (i->equipable.slot == EQUIPMENT_SLOT_OFF_HAND)
    {
        if (e->off_hand == i)
        {
            if (res)
            {
                items[(*len)] = i;
                actions[(*len)].type = ACTION_DEEQUIPPED;
                *len += 1;
            }
            e->off_hand = NULL;
        }
        else
        {
            if (e->off_hand != NULL && res)
            {
                items[(*len)] = e->off_hand;
                actions[(*len)].type = ACTION_DEEQUIPPED;
                *len += 1;
            }

            if (res)
            {
                items[(*len)] = i;
                actions[(*len)].type = ACTION_EQUIPPED;
                *len += 1;
            }
            e->off_hand = i;
        }
    }
}

int equipment_get_max_hp_bonus(rg_player_equipments* e)
{
    int b = 0;
    if (e->main_hand != NULL && e->main_hand->type == ITEM_EQUIPMENT)
        b += e->main_hand->equipable.max_hp_bonus;
    if (e->off_hand != NULL && e->off_hand->type == ITEM_EQUIPMENT)
        b += e->off_hand->equipable.max_hp_bonus;
    return b;
}

int equipment_get_power_bonus(rg_player_equipments* e)
{
    int b = 0;
    if (e->main_hand != NULL && e->main_hand->type == ITEM_EQUIPMENT)
        b += e->main_hand->equipable.power_bonus;
    if (e->off_hand != NULL && e->off_hand->type == ITEM_EQUIPMENT)
        b += e->off_hand->equipable.power_bonus;
    return b;
}

int equipment_get_defense_bonus(rg_player_equipments* e)
{
    int b = 0;
    if (e->main_hand != NULL && e->main_hand->type == ITEM_EQUIPMENT)
        b += e->main_hand->equipable.defense_bonus;
    if (e->off_hand != NULL && e->off_hand->type == ITEM_EQUIPMENT)
        b += e->off_hand->equipable.defense_bonus;
    return b;
}
