#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <SDL.h>
#include <SDL_image.h>

#include "app.h"
#include "array.h"
#include "color.h"
#include "console.h"
#include "entity.h"
#include "events.h"
#include "fov.h"
#include "game_map.h"
#include "panic.h"
#include "terminal.h"
#include "tileset.h"
#include "types.h"

#define DESIRED_FPS 20.0
#define DESIRED_FRAME_RATE ((1.0 / DESIRED_FPS) * 1000.0)

typedef enum rg_game_state
{
    ST_TURN_PLAYER,
    ST_TURN_ENEMY
} rg_game_state;

typedef struct rg_game_state_data
{
    int screen_width;
    int screen_height;
    int map_width;
    int map_height;
    int room_max_size;
    int room_min_size;
    int max_rooms;
    int max_monsters_per_room;
    bool fov_light_walls;
    int fov_radius;
    rg_tileset tileset;
    rg_terminal terminal;
    rg_console console;
    rg_entity_array entities;
    rg_entity_id player;
    rg_map game_map;
    rg_fov_map fov_map;
    bool recompute_fov;
    rg_game_state game_state;
} rg_game_state_data;

//----------A* algo----------------//

struct TCOD_List
{
    /**
     *  A pointer to an array of void pointers.
     */
    void** array;
    /** The current count of items in the array. */
    int fillSize;
    /** The maximum number of items that `array` can currently hold. */
    int allocSize;
};

typedef struct TCOD_List* TCOD_list_t;

TCOD_list_t TCOD_list_new(void)
{
    return (TCOD_list_t)calloc(1, sizeof(struct TCOD_List));
}
/**
 *  Delete a list.
 *
 *  This only frees the list itself, if the list contains any pointers then
 *  those will need to be freed separately.
 */
void TCOD_list_delete(TCOD_list_t l)
{
    if (l)
    {
        free(l->array);
    }
    free(l);
}
int TCOD_list_size(TCOD_list_t l)
{
    return l->fillSize;
}
/**
 *  Return a pointer to the beginning of the list.
 */
void** TCOD_list_begin(TCOD_list_t l)
{
    if (l->fillSize == 0)
    {
        return (void**)NULL;
    }
    return &l->array[0];
}

/**
 *  Remove ALL items from a list.
 */
void TCOD_list_clear(TCOD_list_t l)
{
    l->fillSize = 0;
}

/**
 *  Return a pointer to the end of the list.
 */
void** TCOD_list_end(TCOD_list_t l)
{
    if (l->fillSize == 0)
    {
        return (void**)NULL;
    }
    return &l->array[l->fillSize];
}
/**
 *  Initialize or expand the array of a TCOD_list_t struct.
 *
 *  If `l->allocSize` is zero then a new array is allocated, and allocSize is
 *  set.  If `l->allocSize` is not zero then the allocated size is doubled.
 */
static void TCOD_list_allocate_int(TCOD_list_t l)
{
    void** newArray;
    int newSize = l->allocSize * 2;
    if (newSize == 0)
    {
        newSize = 16;
    }
    newArray = (void**)calloc(newSize, sizeof(void*));
    if (l->array)
    {
        if (l->fillSize > 0)
        {
            memcpy(newArray, l->array, sizeof(void*) * l->fillSize);
        }
        free(l->array);
    }
    l->array = newArray;
    l->allocSize = newSize;
}

/**
 *  Add `elt` to the end of a list.
 */
void TCOD_list_push(TCOD_list_t l, const void* elt)
{
    if (l->fillSize + 1 >= l->allocSize)
    {
        TCOD_list_allocate_int(l);
    }
    l->array[l->fillSize++] = (void*)elt;
}
/**
 *  Remove the last item from a list and return it.
 *
 *  If the list is empty this will return NULL.
 */
void* TCOD_list_pop(TCOD_list_t l)
{
    if (l->fillSize == 0)
    {
        return NULL;
    }
    return l->array[--(l->fillSize)];
}

/**
 *  Return true if this list is empty.
 */
bool TCOD_list_is_empty(TCOD_list_t l)
{
    return (l->fillSize == 0);
}

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

typedef struct TCOD_Path
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
    TCOD_list_t heap; /* min_heap used in the algorithm. stores the offset in
                         grid/heuristic (offset=x+y*w) */
    rg_fov_map* map;
    // TCOD_path_func_t func; //??
    // void* user_data;       //??
} TCOD_path_data_t;

static TCOD_path_data_t* TCOD_path_new_intern(int w, int h)
{
    TCOD_path_data_t* path =
      (TCOD_path_data_t*)calloc(1, sizeof(TCOD_path_data_t));
    path->w = w;
    path->h = h;
    path->grid = calloc(w * h, sizeof(*path->grid));
    path->heuristic = calloc(w * h, sizeof(*path->heuristic));
    path->prev = calloc(w * h, sizeof(*path->prev));
    if (!path->grid || !path->heuristic || !path->prev)
    {
        free(path->grid);
        free(path->heuristic);
        free(path->prev);
        free(path);
        fprintf(
          stderr, "Cannot allocate dijkstra grids of size {%d, %d}", w, h);
        return NULL;
    }
    path->path = TCOD_list_new();
    path->heap = TCOD_list_new();
    return path;
}

TCOD_path_data_t* TCOD_path_new_using_map(rg_fov_map* map, float diagonalCost)
{
    TCOD_path_data_t* path;
    if (map == NULL) return (TCOD_path_data_t*)NULL;
    path = TCOD_path_new_intern(map->width, map->height);
    if (!path) return (TCOD_path_data_t*)NULL;
    path->map = map;
    path->diagonalCost = diagonalCost;
    return path;
}

static void heap_sift_up(TCOD_path_data_t* path, TCOD_list_t heap)
{
    /* sift-up : move the last element of the heap up to its right place */
    int end = TCOD_list_size(heap) - 1;
    int child = end;
    uintptr_t* array = (uintptr_t*)TCOD_list_begin(heap);
    while (child > 0)
    {
        uintptr_t off_child = array[child];
        float child_dist = path->heuristic[off_child];
        int parent = (child - 1) / 2;
        uintptr_t off_parent = array[parent];
        float parent_dist = path->heuristic[off_parent];
        if (parent_dist > child_dist)
        {
            /* get up one level */
            uintptr_t tmp = array[child];
            array[child] = array[parent];
            array[parent] = tmp;
            child = parent;
        }
        else
            return;
    }
}

/* small layer on top of TCOD_list_t to implement a binary heap (min_heap) */
static void heap_sift_down(TCOD_path_data_t* path, TCOD_list_t heap)
{
    /* sift-down : move the first element of the heap down to its right place */
    int cur = 0;
    int end = TCOD_list_size(heap) - 1;
    int child = 1;
    uintptr_t* array = (uintptr_t*)TCOD_list_begin(heap);
    while (child <= end)
    {
        int toSwap = cur;
        uintptr_t off_cur = array[cur];
        float cur_dist = path->heuristic[off_cur];
        float swapValue = cur_dist;
        uintptr_t off_child = array[child];
        float child_dist = path->heuristic[off_child];
        if (child_dist < cur_dist)
        {
            toSwap = child;
            swapValue = child_dist;
        }
        if (child < end)
        {
            /* get the min between child and child+1 */
            uintptr_t off_child2 = array[child + 1];
            float child2_dist = path->heuristic[off_child2];
            if (swapValue > child2_dist)
            {
                toSwap = child + 1;
                swapValue = child2_dist;
            }
        }
        if (toSwap != cur)
        {
            /* get down one level */
            uintptr_t tmp = array[toSwap];
            array[toSwap] = array[cur];
            array[cur] = tmp;
            cur = toSwap;
        }
        else
            return;
        child = cur * 2 + 1;
    }
}

/* add a coordinate pair in the heap so that the heap root always contains the
 * minimum A* score */
static void heap_add(TCOD_path_data_t* path, TCOD_list_t heap, int x, int y)
{
    /* append the new value to the end of the heap */
    uintptr_t off = x + y * path->w;
    TCOD_list_push(heap, (void*)off);
    /* bubble the value up to its real position */
    heap_sift_up(path, heap);
}

/* get the coordinate pair with the minimum A* score from the heap */
static uint32_t heap_get(TCOD_path_data_t* path, TCOD_list_t heap)
{
    /* return the first value of the heap (minimum score) */
    uintptr_t* array = (uintptr_t*)TCOD_list_begin(heap);
    int end = TCOD_list_size(heap) - 1;
    uint32_t off = (uint32_t)(array[0]);
    /* take the last element and put it at first position (heap root) */
    array[0] = array[end];
    TCOD_list_pop(heap);
    /* and bubble it down to its real position */
    heap_sift_down(path, heap);
    return off;
}

/* this is the slow part, when we change the heuristic of a cell already in the
 * heap */
static void heap_reorder(TCOD_path_data_t* path, uint32_t offset)
{
    uintptr_t* array = (uintptr_t*)TCOD_list_begin(path->heap);
    uintptr_t* end = (uintptr_t*)TCOD_list_end(path->heap);
    uintptr_t* cur = array;
    uintptr_t off_idx = 0;
    float value;
    int idx = 0;
    int heap_size = TCOD_list_size(path->heap);
    /* find the node corresponding to offset ... SLOW !! */
    while (cur != end)
    {
        if (*cur == offset) break;
        cur++;
        idx++;
    }
    if (cur == end) return;
    off_idx = array[idx];
    value = path->heuristic[off_idx];
    if (idx > 0)
    {
        int parent = (idx - 1) / 2;
        /* compare to its parent */
        uintptr_t off_parent = array[parent];
        float parent_value = path->heuristic[off_parent];
        if (value < parent_value)
        {
            /* smaller. bubble it up */
            while (idx > 0 && value < parent_value)
            {
                /* swap with parent */
                array[parent] = off_idx;
                array[idx] = off_parent;
                idx = parent;
                if (idx > 0)
                {
                    parent = (idx - 1) / 2;
                    off_parent = array[parent];
                    parent_value = path->heuristic[off_parent];
                }
            }
            return;
        }
    }
    /* compare to its sons */
    while (idx * 2 + 1 < heap_size)
    {
        int child = idx * 2 + 1;
        uintptr_t off_child = array[child];
        int toSwap = idx;
        int child2;
        float swapValue = value;
        if (path->heuristic[off_child] < value)
        {
            /* swap with son1 ? */
            toSwap = child;
            swapValue = path->heuristic[off_child];
        }
        child2 = child + 1;
        if (child2 < heap_size)
        {
            uintptr_t off_child2 = array[child2];
            if (path->heuristic[off_child2] < swapValue)
            {
                /* swap with son2 */
                toSwap = child2;
            }
        }
        if (toSwap != idx)
        {
            /* bigger. bubble it down */
            uintptr_t tmp = array[toSwap];
            array[toSwap] = array[idx];
            array[idx] = tmp;
            idx = toSwap;
        }
        else
            return;
    }
}

/* private stuff */
/* add a new unvisited cells to the cells-to-treat list
 * the list is in fact a min_heap. Cell at index i has its sons at 2*i+1 and
 * 2*i+2
 */
static void TCOD_path_push_cell(TCOD_path_data_t* path, int x, int y)
{
    heap_add(path, path->heap, x, y);
}

/* get the best cell from the heap */
static void TCOD_path_get_cell(TCOD_path_data_t* path,
                               int* x,
                               int* y,
                               float* distance)
{
    uint32_t offset = heap_get(path, path->heap);
    *x = (offset % path->w);
    *y = (offset / path->w);
    *distance = path->grid[offset];
}

/* check if a cell is walkable (from the pathfinder point of view) */
static float TCOD_path_walk_cost(TCOD_path_data_t* path,
                                 int xFrom,
                                 int yFrom,
                                 int xTo,
                                 int yTo)
{
    // if (path->map)
    return fov_map_is_walkable(path->map, xTo, yTo) ? 1.0f : 0.0f;
    // return path->func(xFrom, yFrom, xTo, yTo, path->user_data);
}

/* fill the grid, starting from the origin until we reach the destination */
static void TCOD_path_set_cells(TCOD_path_data_t* path)
{
    while (path->grid[path->dx + path->dy * path->w] == 0 &&
           !TCOD_list_is_empty(path->heap))
    {
        int x, y;
        float distance;
        TCOD_path_get_cell(path, &x, &y, &distance);
        int i_max = (path->diagonalCost == 0.0f ? 4 : 8);
        for (int i = 0; i < i_max; i++)
        {
            /* convert i to dx,dy */
            static int i_dir_x[] = { 0, -1, 1, 0, -1, 1, -1, 1 };
            static int i_dir_y[] = { -1, 0, 0, 1, -1, -1, 1, 1 };
            /* convert i to direction */
            static dir_t previous_dirs[] = { NORTH,      WEST,       EAST,
                                             SOUTH,      NORTH_WEST, NORTH_EAST,
                                             SOUTH_WEST, SOUTH_EAST };
            /* coordinate of the adjacent cell */
            int cx = x + i_dir_x[i];
            int cy = y + i_dir_y[i];
            if (cx >= 0 && cy >= 0 && cx < path->w && cy < path->h)
            {
                float walk_cost = TCOD_path_walk_cost(path, x, y, cx, cy);
                if (walk_cost > 0.0f)
                {
                    /* in of the map and walkable */
                    float covered =
                      distance +
                      walk_cost * (i >= 4 ? path->diagonalCost : 1.0f);
                    float previousCovered = path->grid[cx + cy * path->w];
                    if (previousCovered == 0)
                    {
                        /* put a new cell in the heap */
                        int offset = cx + cy * path->w;
                        /* A* heuristic : remaining distance */
                        float remaining =
                          (float)sqrt((cx - path->dx) * (cx - path->dx) +
                                      (cy - path->dy) * (cy - path->dy));
                        path->grid[offset] = covered;
                        path->heuristic[offset] = covered + remaining;
                        path->prev[offset] = previous_dirs[i];
                        TCOD_path_push_cell(path, cx, cy);
                    }
                    else if (previousCovered > covered)
                    {
                        /* we found a better path to a cell already in the heap
                         */
                        int offset = cx + cy * path->w;
                        path->grid[offset] = covered;
                        path->heuristic[offset] -=
                          (previousCovered - covered); /* fix the A* score */
                        path->prev[offset] = previous_dirs[i];
                        /* reorder the heap */
                        heap_reorder(path, offset);
                    }
                }
            }
        }
    }
}

bool TCOD_path_compute(TCOD_path_data_t* p, int ox, int oy, int dx, int dy)
{
    TCOD_path_data_t* path = p;
    if (p == NULL) return false;
    path->ox = ox;
    path->oy = oy;
    path->dx = dx;
    path->dy = dy;
    TCOD_list_clear(path->path);
    TCOD_list_clear(path->heap);
    if (ox == dx && oy == dy) return true; /* trivial case */
    /* check that origin and destination are inside the map */
    if ((unsigned)ox > (unsigned)path->w || (unsigned)oy > (unsigned)path->h)
        return false;
    if ((unsigned)dx > (unsigned)path->w || (unsigned)dy > (unsigned)path->h)
        return false;

    /* initialize dijkstra grids TODO: do we need this */
    memset(path->grid, 0, sizeof(float) * path->w * path->h);
    memset(path->prev, NONE, sizeof(dir_t) * path->w * path->h);
    path->heuristic[ox + oy * path->w] = 1.0f; /* anything != 0 */
    TCOD_path_push_cell(path, ox, oy); /* put the origin cell as a bootstrap */
    /* fill the dijkstra grid until we reach dx,dy */
    TCOD_path_set_cells(path);
    if (path->grid[dx + dy * path->w] == 0) return false; /* no path found */
    /* there is a path. retrieve it */
    do
    {
        /* walk from destination to origin, using the 'prev' array */
        int step = path->prev[dx + dy * path->w];
        TCOD_list_push(path->path, (void*)(uintptr_t)step);
        dx -= dir_x[step];
        dy -= dir_y[step];
    } while (dx != ox || dy != oy);
    return true;
}

bool TCOD_path_is_empty(TCOD_path_data_t* p)
{
    TCOD_path_data_t* path = p;
    if (p == NULL) return true;
    return TCOD_list_is_empty(path->path);
}

int TCOD_path_size(TCOD_path_data_t* p)
{
    TCOD_path_data_t* path = p;
    if (p == NULL) return 0;
    return TCOD_list_size(path->path);
}

bool TCOD_path_walk(TCOD_path_data_t* p,
                    int* x,
                    int* y,
                    bool recalculate_when_needed)
{
    TCOD_path_data_t* path = (TCOD_path_data_t*)p;
    if (p == NULL) return false;
    if (TCOD_path_is_empty(path)) return false;
    int d = (int)(uintptr_t)TCOD_list_pop(path->path);
    int new_x = path->ox + dir_x[d];
    int new_y = path->oy + dir_y[d];
    /* check if the path is still valid */
    if (TCOD_path_walk_cost(path, path->ox, path->oy, new_x, new_y) <= 0.0f)
    {
        /* path is blocked */
        if (!recalculate_when_needed) return false; /* don't walk */
        /* calculate a new path */
        if (!TCOD_path_compute(path, path->ox, path->oy, path->dx, path->dy))
            return false;                     /* cannot find a new path */
        return TCOD_path_walk(p, x, y, true); /* walk along the new path */
    }
    if (x) *x = new_x;
    if (y) *y = new_y;
    path->ox = new_x;
    path->oy = new_y;
    return true;
}

void TCOD_path_delete(TCOD_path_data_t* p)
{
    TCOD_path_data_t* path = (TCOD_path_data_t*)p;
    if (p == NULL) return;
    if (path->grid) free(path->grid);
    if (path->heuristic) free(path->heuristic);
    if (path->prev) free(path->prev);
    if (path->path) TCOD_list_delete(path->path);
    if (path->heap) TCOD_list_delete(path->heap);
    free(path);
}

//----------A* algo----------------//

float distance_between(rg_entity* a, rg_entity* b)
{
    int dx = b->x - a->x;
    int dy = b->y - a->y;
    return (float)sqrt(dx * dx + dy * dy);
}

void entity_move_towards(rg_entity* e,
                         int x,
                         int y,
                         rg_map* map,
                         rg_entity_array* entities)
{
    int dx = x - e->x;
    int dy = y - e->y;
    float distance = (float)sqrt(dx * dx + dy * dy);
    dx = (int)floor(dx / distance);
    dy = (int)floor(dy / distance);

    if (!map_is_blocked(map, e->x + dx, e->y + dy))
    {
        rg_entity* other = NULL;
        entity_get_at_loc(entities, e->x + dx, e->y + dy, &other);
        if (other == NULL) entity_move(e, dx, dy);
    }
}

void entity_move_astar(rg_entity* e,
                       rg_entity* target,
                       rg_map* game_map,
                       rg_entity_array* entities)
{
    rg_fov_map map;
    fov_map_create(&map, game_map->width, game_map->height);
    for (int y = 0; y < game_map->height; y++)
    {
        for (int x = 0; x < game_map->width; x++)
        {
            rg_tile* t = map_get_tile(game_map, x, y);
            fov_map_set_props(&map, x, y, !t->block_sight, !t->blocked);
        }
    }
    for (int i = 0; i < entities->len; i++)
    {
        rg_entity* t = &entities->data[i];
        if (t->blocks && t != e && t != target)
        {
            fov_map_set_props(&map, t->x, t->y, true, false);
        }
    }

    TCOD_path_data_t* path= TCOD_path_new_using_map(&map, (float)1.41);
    TCOD_path_compute(path, e->x, e->y, target->x, target->y);

    if (!TCOD_path_is_empty(path) && TCOD_path_size(path) < 25)
    {
        int dx, dy;
        bool res = TCOD_path_walk(path, &dx, &dy, true);
        if (res)
        {
            e->x = dx;
            e->y = dy;
        }
    }
    else
    {
        entity_move_towards(e, target->x, target->y, game_map, entities);
    }
    TCOD_path_delete(path);
}

void basic_monster_update(rg_entity* e,
                          rg_entity* target,
                          rg_fov_map* fov_map,
                          rg_map* game_map,
                          rg_entity_array* entities)
{
    if (fov_map_is_in_fov(fov_map, e->x, e->y))
    {
        int distance = (int)distance_between(e, target);
        if (distance >= 2)
        {
            //entity_move_towards(e, target->x, target->y, game_map, entities);
            entity_move_astar(e, target, game_map, entities);
        }
        else if (target->fighter.hp > 0)
            SDL_Log("The '%s' insults you! your ego is damaged!", e->name);
    }
}

void game_recompute_fov(rg_game_state_data* data)
{
    fov_map_compute(&data->fov_map,
                    data->entities.data[data->player].x,
                    data->entities.data[data->player].y,
                    data->fov_radius,
                    data->fov_light_walls);
}

void update(rg_app* app, SDL_Event* event, rg_game_state_data* data)
{
    rg_map* game_map = &data->game_map;
    rg_fov_map* fov_map = &data->fov_map;
    rg_action action;
    event_dispatch(event, &action);

    if (action.type == ACTION_NONE) return;

    if (action.type == ACTION_MOVEMENT && data->game_state == ST_TURN_PLAYER)
    {
        int destination_x = data->entities.data[data->player].x + action.dx;
        int destination_y = data->entities.data[data->player].y + action.dy;
        if (!map_is_blocked(game_map, destination_x, destination_y))
        {
            rg_entity* target = NULL;
            entity_get_at_loc(
              &data->entities, destination_x, destination_y, &target);

            if (target == NULL)
            {
                entity_move(
                  &data->entities.data[data->player], action.dx, action.dy);
                data->recompute_fov = true;
            }
            else
            {
                SDL_Log("You kick the '%s' in shins, much to its annoyance!",
                        target->name);
            }
            data->game_state = ST_TURN_ENEMY;
        }
    }

    if (data->recompute_fov) game_recompute_fov(data);

    if (action.type == ACTION_ESCAPE || action.type == ACTION_QUIT)
        app->running = false;

    if (data->game_state == ST_TURN_ENEMY)
    {
        for (int i = 0; i < data->entities.len; i++)
        {
            if (i != data->player)
            {
                rg_entity* e = &data->entities.data[i];
                rg_entity* player = &data->entities.data[data->player];
                if (e->type == ENTITY_BASIC_MONSTER)
                    basic_monster_update(e,
                                         player,
                                         &data->fov_map,
                                         &data->game_map,
                                         &data->entities);
            }
        }
        data->game_state = ST_TURN_PLAYER;
    }
}

void draw(rg_app* app, rg_game_state_data* data)
{
    rg_map* game_map = &data->game_map;
    rg_fov_map* fov_map = &data->fov_map;
    rg_console* console = &data->console;
    rg_entity_array* entities = &data->entities;

    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    SDL_RenderClear(app->renderer);

    for (int y = 0; y < game_map->height; y++)
    {
        for (int x = 0; x < game_map->width; x++)
        {
            bool visible = fov_map_is_in_fov(fov_map, x, y);
            rg_tile* tile = map_get_tile(game_map, x, y);
            bool is_wall = tile->block_sight;
            if (visible)
            {
                tile->explored = true;
                if (is_wall) console_fill(console, x, y, LIGHT_WALL);
                else
                    console_fill(console, x, y, LIGHT_GROUND);
            }
            else if (tile->explored)
            {
                if (is_wall) console_fill(console, x, y, DARK_WALL);
                else
                    console_fill(console, x, y, DARK_GROUND);
            }
        }
    }
    for (int i = 0; i < entities->len; i++)
    {
        const rg_entity* e = &entities->data[i];
        if (fov_map_is_in_fov(fov_map, e->x, e->y)) entity_draw(e, console);
    }
    SDL_RenderPresent(app->renderer);

    data->recompute_fov = false;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
#endif

    srand((unsigned int)time(NULL));

    rg_app app;
    app_create(&app);

    rg_game_state_data data;
    data.screen_width = 80;
    data.screen_height = 50;
    data.map_width = 80;
    data.map_height = 45;
    data.room_max_size = 10;
    data.room_min_size = 6;
    data.max_rooms = 30;
    data.max_monsters_per_room = 3;
    data.fov_light_walls = true;
    data.fov_radius = 10;
    data.game_state = ST_TURN_PLAYER;

    tileset_create(
      &data.tileset, app.renderer, "res/dejavu10x10_gs_tc.png", 32, 8);

    terminal_create(&data.terminal,
                    &app,
                    data.screen_width,
                    data.screen_height,
                    &data.tileset,
                    "roguelike",
                    true);

    console_create(&data.console,
                   app.renderer,
                   data.terminal.width,
                   data.terminal.height,
                   data.terminal.tileset);

    data.entities.capacity = data.max_monsters_per_room;
    data.entities.data =
      malloc(sizeof(*data.entities.data) * data.entities.capacity);
    ASSERT_M(data.entities.data != NULL);
    data.entities.len = 0;
    rg_fighter fighter = { .hp = 30, .defence = 2, .power = 5 };
    ARRAY_PUSH(&data.entities,
               ((rg_entity){ .x = 0,
                             .y = 0,
                             .ch = '@',
                             .color = WHITE,
                             .name = "player",
                             .blocks = true,
                             .type = ENTITY_PLAYER,
                             .fighter = fighter }));
    data.player = data.entities.len - 1;

    map_create(&data.game_map,
               data.map_width,
               data.map_height,
               data.room_min_size,
               data.room_max_size,
               data.max_rooms,
               data.max_monsters_per_room,
               &data.entities,
               data.player);

    fov_map_create(&data.fov_map, data.map_width, data.map_height);
    for (int y = 0; y < data.map_height; y++)
    {
        for (int x = 0; x < data.map_width; x++)
        {
            rg_tile* tile = map_get_tile(&data.game_map, x, y);
            fov_map_set_props(
              &data.fov_map, x, y, !tile->block_sight, !tile->blocked);
        }
    }

    // first draw before waitevent
    game_recompute_fov(&data);
    draw(&app, &data);

    while (app.running)
    {
        SDL_Event event = { 0 };
        SDL_WaitEvent(&event);

        Uint64 frame_start = SDL_GetPerformanceCounter();

        update(&app, &event, &data);
        draw(&app, &data);

        Uint64 frame_end = SDL_GetPerformanceCounter();
        double dt = (frame_end - frame_start) /
                    (double)SDL_GetPerformanceFrequency() * 1000.0;

        Uint32 sleep_time = (Uint32)floor(DESIRED_FRAME_RATE - dt);
        const Uint32 max_sleep_time = 1000;
        if (sleep_time > max_sleep_time) sleep_time = max_sleep_time;
        // SLEEP
        SDL_Delay(sleep_time);
    }

    map_destroy(&data.game_map);
    free(data.entities.data);
    console_destroy(&data.console);
    terminal_destroy(&data.terminal);
    tileset_destroy(&data.tileset);

    app_destroy(&app);
    return 0;
}