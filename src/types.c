#include "types.h"

int rand_int_choice_index(int len, int* data)
{
    int sum = 0;
    for (int i = 0; i < len; i++) sum += data[i];
    int random_chance = RAND_INT(1, sum);
    int running_sum = 0;
    int choice = 0;
    for (int i = 0; i < len; i++)
    {
        running_sum += data[i];
        if (random_chance <= running_sum) return choice;
        choice++;
    }
    return choice;
}