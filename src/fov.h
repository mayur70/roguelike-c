#ifndef FOV_H
#define FOV_H

#include <stdbool.h>

typedef struct rg_line_data
{
    int step_x, step_y;
    int e;
    int delta_x, delta_y;
    int orig_x, orig_y;
    int dest_x, dest_y;
} rg_line_data;

typedef struct rg_fov_map_cell
{
    bool transparent;
    bool walkable;
    bool fov;
} rg_fov_map_cell;

typedef struct rg_fov_map
{
    int width;
    int height;
    rg_fov_map_cell *cells;
} rg_fov_map;

void line_init(int x0, int y0, int x1, int y1, rg_line_data *data);
bool line_step(int *x, int *y, rg_line_data *data);
void fov_map_create(rg_fov_map *m, int w, int h);
void fov_map_destroy(rg_fov_map *m);
bool fov_map_in_bounds(rg_fov_map *m, int x, int y);
bool fov_map_is_in_fov(rg_fov_map *m, int x, int y);

void fov_map_set_props(rg_fov_map *m,
                       int x,
                       int y,
                       bool transparent,
                       bool walkable);
void fov_map_cast_ray(rg_fov_map *m,
                      int orig_x,
                      int orig_y,
                      int dest_x,
                      int dest_y,
                      int radius_squared,
                      bool light_walls);
void fov_map_postprocess_quad(rg_fov_map *m,
                              int x0,
                              int y0,
                              int x1,
                              int y1,
                              int dx,
                              int dy);

void fov_map_postprocess(rg_fov_map *m,
                         int pov_x,
                         int pov_y,
                         int radius);
void fov_map_compute(rg_fov_map *m,
                     int pov_x,
                     int pov_y,
                     int max_radius,
                     bool light_walls);

#endif