#ifndef ARRAY_H
#define ARRAY_H

#define ARRAY_PUSH(arr, ele)                                                   \
    do                                                                         \
    {                                                                          \
        assert(arr);                                                           \
        if ((arr)->len + 1 > (arr)->capacity)                                  \
        {                                                                      \
            (arr)->capacity *= 2;                                              \
            (arr)->data = realloc((arr)->data, sizeof(ele) * (arr)->capacity); \
            assert((arr)->data);                                               \
        }                                                                      \
        (arr)->data[(arr)->len] = ele;                                         \
        (arr)->len++;                                                          \
    } while (0)

#endif