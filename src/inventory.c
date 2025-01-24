#include "inventory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "color.h"
#include "types.h"

void inventory_create(rg_inventory* i, size_t capacity)
{
    memset(i, 0, sizeof(rg_inventory));
    i->capacity = capacity;
    i->len = 0;
    i->data = malloc(sizeof(rg_inventory) * i->capacity);
    ASSERT_M(i->data != NULL);
}

void inventory_destroy(rg_inventory* i)
{
    if (i == NULL) return;
    if (i->data != NULL) free(i->data);
}

void inventory_add_item(rg_inventory* inventory,
                        rg_items* items,
                        size_t item_idx,
                        rg_turn_logs* logs)
{
    if (inventory->len >= inventory->capacity)
    {
        const char* fmt = "You cannot carry any more, your inventory is full";

        int len = snprintf(NULL, 0, fmt);
        char* buf = malloc(sizeof(char) * (len + 1));
        snprintf(buf, len, fmt);
        rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE,
                                    .text = buf,
                                    .color = YELLOW };
        turn_logs_push(logs, &entry);

        return;
    }

    rg_item* item = &items->data[item_idx];
    item->visible_on_map = false;
    const char* fmt = "You pick up the %s!";
    int len = snprintf(NULL, 0, fmt, item->name);
    char* buf = malloc(sizeof(char) * (len + 1));
    snprintf(buf, len, fmt, item->name);
    rg_turn_log_entry entry = { .type = TURN_LOG_MESSAGE,
                                .text = buf,
                                .color = BLUE };
    turn_logs_push(logs, &entry);

    for (int i = 0; i < inventory->len; i++)
        ASSERT_M(inventory->data[i] != item_idx);

    ARRAY_PUSH(inventory, item_idx);
}

void inventory_remove_item(rg_inventory* inventory,
                           rg_items* items,
                           rg_item* item)
{
    int item_idx = -1;
    for (size_t i = 0; i < items->len; i++)
    {
        if (&items->data[i] == item)
        {
            item_idx = (int)i;
            break;
        }
    }
    if (item_idx == -1)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "invalid remove item");
        return;
    }
    int inv_itex = -1;
    size_t idx = item_idx;
    for (int i = 0; i < inventory->len; i++)
    {
        if (inventory->data[i] == idx)
        {
            inv_itex = i;
            break;
        }
    }
    if (inv_itex == -1) return;
    if (inventory->len == 1)
    {
        inventory->len = 0;
        return;
    }
    if (inv_itex == inventory->len - 1)
    {
        inventory->len--;
        return;
    }
    size_t* dest = inventory->data + inv_itex;
    size_t* src = inventory->data + inv_itex + 1;
    size_t sz = inventory->len - inv_itex - 1;
    memmove(dest, src, sz * sizeof(dest));

    inventory->len--;
}

void menu_draw(rg_console* c,
               const char* header,
               int options_count,
               const char* options[],
               int width,
               int* height)
{
    ASSERT_M(options_count <= 26);
    size_t len = strlen(header);
    int header_height = (int)ceil((double)len / (double)width);
    *height = options_count + header_height;

    int y = 0;
    {
        char* buf = malloc(sizeof(char) * width + 1);
        size_t len_remaining = len;
        for (int i = 0; i < header_height; i++)
        {
            // TODO: Split by word?
            size_t sz;
            if (len_remaining > width) sz = width;
            else
                sz = len_remaining;
            memcpy(buf, header + (width * i), sz);
            buf[sz] = '\0';
            console_print_txt(c, 0, y, buf, WHITE);
            y++;
            len_remaining -= sz;
        }
        free(buf);
    }
    int letter_index = (char)'a';
    for (int i = 0; i < options_count; i++)
    {
        const char* opt = options[i];
        int x = 0;
        console_print(c, x, y, '(', WHITE);
        x++;
        console_print(c, x, y, letter_index, WHITE);
        x++;
        console_print(c, x, y, ')', WHITE);
        x++;
        console_print_txt(c, x, y, opt, WHITE);

        y++;
        letter_index++;
    }
}

void inventory_draw(rg_console* c,
                    const char* header,
                    rg_items* items,
                    rg_inventory* inventory,
                    int width,
                    int* height)
{
    if (inventory->len == 0)
    {
        const char* opt = "Inventory is empty.";
        menu_draw(c, header, 1, &opt, width, height);
    }
    else
    {
        char** options = malloc(sizeof(char*) * inventory->len);
        for (int i = 0; i < inventory->len; i++)
        {
            size_t idx = inventory->data[i];
            rg_item* item = &items->data[idx];
            options[i] = strdup(item->name);
        }
        menu_draw(c, header, (int)inventory->len, options, width, height);

        for (int i = 0; i < inventory->len; i++) free(options[i]);
        free(options);
    }
}
