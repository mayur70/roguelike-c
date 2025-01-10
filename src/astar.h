#ifndef ASTAR_H
#define ASTAR_H

#include <stdbool.h>

#include "fov.h"

enum
{
    NORTH_WEST,
    NORTH,
    NORTH_EAST,
    WEST,
    NONE,
    EAST,
    SOUTH_WEST,
    SOUTH,
    SOUTH_EAST
};
typedef unsigned char dir_t;

/* convert dir_t to dx,dy */
static const int dir_x[] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
static const int dir_y[] = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };

//TODO: remove TCOD LIST & refactor ASTAR Algorith
typedef struct TCOD_List TCOD_List;
typedef struct TCOD_List* TCOD_list_t;


typedef struct astar_path
{
    int ox, oy;       /* coordinates of the creature position */
    int dx, dy;       /* coordinates of the creature's destination */
    TCOD_list_t path; /* list of dir_t to follow the path */
    int w, h;         /* map size */
    float* grid;      /* wxh dijkstra distance grid (covered distance) */
    float* heuristic; /* wxh A* score grid (covered distance + estimated
                         remaining distance) */
    dir_t* prev;      /* wxh 'previous' grid : direction to the previous cell */
    float diagonalCost;
    TCOD_list_t heap; /* min_heap used in the algorithm. stores the offset
                         in
                         grid/heuristic (offset=x+y*w) */
    rg_fov_map* map;
} astar_path;

astar_path* astar_path_new_using_map(rg_fov_map* map, float diagonalCost);
bool astar_path_compute(astar_path* p, int ox, int oy, int dx, int dy);
bool astar_path_is_empty(astar_path* p);
int astar_path_size(astar_path* p);
bool astar_path_walk(astar_path* p,
                    int* x,
                    int* y,
                    bool recalculate_when_needed);
void astar_path_delete(astar_path* p);
#endif