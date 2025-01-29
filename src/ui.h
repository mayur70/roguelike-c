#ifndef UI_H
#define UI_H

#include "console.h"

void menu_draw(rg_console* c,
               const char* header,
               int options_count,
               const char* options[],
               int width,
               int* height);

#endif