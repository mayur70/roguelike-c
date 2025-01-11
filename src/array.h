#ifndef ARRAY_H
#define ARRAY_H

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include "types.h"

#define ARRAY_PUSH(arr, ele)                                                   \
    do                                                                         \
    {                                                                          \
        assert(arr);                                                           \
        if ((arr)->len + 1 > (arr)->capacity)                                  \
        {                                                                      \
            (arr)->capacity *= 2;                                              \
            (arr)->data = realloc((arr)->data, sizeof(ele) * (arr)->capacity); \
            ASSERT_M((arr)->data != NULL);                                     \
        }                                                                      \
        (arr)->data[(arr)->len] = ele;                                         \
        (arr)->len++;                                                          \
    } while (0)

#endif