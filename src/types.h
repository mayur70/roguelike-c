#ifndef TYPES_H
#define TYPES_h

#include <assert.h>
#include <time.h>


#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)
#define RAND_INT(min, max) (min + rand() / (RAND_MAX / (max - min + 1) + 1))

#ifdef _WIN32
#include <Windows.h>
#define ASSERT_M(stmt)      \
    do                      \
    {                       \
        if (!(stmt))        \
        {                   \
            __debugbreak(); \
            assert(false);  \
        }                   \
    } while (0)
#else
#define ASSERT_M(stmt) \
    do                 \
    {                  \
        assert(stmt);  \
    } while (0)
#endif

#endif