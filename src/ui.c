#include "ui.h"

#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "types.h"
#include "color.h"


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