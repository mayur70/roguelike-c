#include "astar.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


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

TCOD_list_t TCOD_list_new(void);
/**
 *  Delete a list.
 *
 *  This only frees the list itself, if the list contains any pointers then
 *  those will need to be freed separately.
 */
void TCOD_list_delete(TCOD_list_t l);
int TCOD_list_size(TCOD_list_t l);
/**
 *  Return a pointer to the beginning of the list.
 */
void** TCOD_list_begin(TCOD_list_t l);

/**
 *  Remove ALL items from a list.
 */
void TCOD_list_clear(TCOD_list_t l);

/**
 *  Return a pointer to the end of the list.
 */
void** TCOD_list_end(TCOD_list_t l);

/**
 *  Add `elt` to the end of a list.
 */
void TCOD_list_push(TCOD_list_t l, const void* elt);
/**
 *  Remove the last item from a list and return it.
 *
 *  If the list is empty this will return NULL.
 */
void* TCOD_list_pop(TCOD_list_t l);

/**
 *  Return true if this list is empty.
 */
bool TCOD_list_is_empty(TCOD_list_t l);


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
static astar_path* astar_path_new_intern(int w, int h)
{
    astar_path* path =
      (astar_path*)calloc(1, sizeof(astar_path));
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

astar_path* astar_path_new_using_map(rg_fov_map* map, float diagonalCost)
{
    astar_path* path;
    if (map == NULL) return (astar_path*)NULL;
    path = astar_path_new_intern(map->width, map->height);
    if (!path) return (astar_path*)NULL;
    path->map = map;
    path->diagonalCost = diagonalCost;
    return path;
}

static void heap_sift_up(astar_path* path, TCOD_list_t heap)
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
static void heap_sift_down(astar_path* path, TCOD_list_t heap)
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
static void heap_add(astar_path* path, TCOD_list_t heap, int x, int y)
{
    /* append the new value to the end of the heap */
    uintptr_t off = x + y * path->w;
    TCOD_list_push(heap, (void*)off);
    /* bubble the value up to its real position */
    heap_sift_up(path, heap);
}

/* get the coordinate pair with the minimum A* score from the heap */
static uint32_t heap_get(astar_path* path, TCOD_list_t heap)
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
static void heap_reorder(astar_path* path, uint32_t offset)
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
static void astar_path_push_cell(astar_path* path, int x, int y)
{
    heap_add(path, path->heap, x, y);
}

/* get the best cell from the heap */
static void astar_path_get_cell(astar_path* path,
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
static float astar_path_walk_cost(astar_path* path,
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
static void astar_path_set_cells(astar_path* path)
{
    while (path->grid[path->dx + path->dy * path->w] == 0 &&
           !TCOD_list_is_empty(path->heap))
    {
        int x, y;
        float distance;
        astar_path_get_cell(path, &x, &y, &distance);
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
                float walk_cost = astar_path_walk_cost(path, x, y, cx, cy);
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
                        astar_path_push_cell(path, cx, cy);
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

bool astar_path_compute(astar_path* p, int ox, int oy, int dx, int dy)
{
    astar_path* path = p;
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
    astar_path_push_cell(path, ox, oy); /* put the origin cell as a bootstrap */
    /* fill the dijkstra grid until we reach dx,dy */
    astar_path_set_cells(path);
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

bool astar_path_is_empty(astar_path* p)
{
    astar_path* path = p;
    if (p == NULL) return true;
    return TCOD_list_is_empty(path->path);
}

int astar_path_size(astar_path* p)
{
    astar_path* path = p;
    if (p == NULL) return 0;
    return TCOD_list_size(path->path);
}

bool astar_path_walk(astar_path* p,
                    int* x,
                    int* y,
                    bool recalculate_when_needed)
{
    astar_path* path = (astar_path*)p;
    if (p == NULL) return false;
    if (astar_path_is_empty(path)) return false;
    int d = (int)(uintptr_t)TCOD_list_pop(path->path);
    int new_x = path->ox + dir_x[d];
    int new_y = path->oy + dir_y[d];
    /* check if the path is still valid */
    if (astar_path_walk_cost(path, path->ox, path->oy, new_x, new_y) <= 0.0f)
    {
        /* path is blocked */
        if (!recalculate_when_needed) return false; /* don't walk */
        /* calculate a new path */
        if (!astar_path_compute(path, path->ox, path->oy, path->dx, path->dy))
            return false;                     /* cannot find a new path */
        return astar_path_walk(p, x, y, true); /* walk along the new path */
    }
    if (x) *x = new_x;
    if (y) *y = new_y;
    path->ox = new_x;
    path->oy = new_y;
    return true;
}

void astar_path_delete(astar_path* p)
{
    astar_path* path = (astar_path*)p;
    if (p == NULL) return;
    if (path->grid) free(path->grid);
    if (path->heuristic) free(path->heuristic);
    if (path->prev) free(path->prev);
    if (path->path) TCOD_list_delete(path->path);
    if (path->heap) TCOD_list_delete(path->heap);
    free(path);
}