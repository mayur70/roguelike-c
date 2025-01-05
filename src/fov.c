#include "fov.h"

#include <stdlib.h>
#include <string.h>

#include "types.h"

void line_init(int x0, int y0, int x1, int y1, rg_line_data *data)
{
    data->orig_x = x0;
    data->orig_y = y0;
    data->dest_x = x1;
    data->dest_y = y1;
    data->delta_x = x1 - x0;
    data->delta_y = y1 - y0;

    if (data->delta_x > 0) data->step_x = 1;
    else if (data->delta_x < 0)
        data->step_x = -1;
    else
        data->step_x = 0;

    if (data->delta_y > 0) data->step_y = 1;
    else if (data->delta_y < 0)
        data->step_y = -1;
    else
        data->step_y = 0;

    if (data->step_x * data->delta_x > data->step_y * data->delta_y)
        data->e = data->step_x * data->delta_x;
    else
        data->e = data->step_y * data->delta_y;
    data->delta_x *= 2;
    data->delta_y *= 2;
}

bool line_step(int *x, int *y, rg_line_data *data)
{
    if (data->step_x * data->delta_x > data->step_y * data->delta_y)
    {
        if (data->orig_x == data->dest_x) return true;
        data->orig_x += data->step_x;
        data->e -= data->step_y * data->delta_y;
        if (data->e < 0)
        {
            data->orig_y += data->step_y;
            data->e += data->step_x * data->delta_x;
        }
    }
    else
    {
        if (data->orig_y == data->dest_y) return true;
        data->orig_y += data->step_y;
        data->e -= data->step_x * data->delta_x;
        if (data->e < 0)
        {
            data->orig_x += data->step_x;
            data->e += data->step_y * data->delta_y;
        }
    }
    *x = data->orig_x;
    *y = data->orig_y;
    return false;
}

void fov_map_create(rg_fov_map *m, int w, int h)
{
    memset(m, 0, sizeof(*m));
    m->width = w;
    m->height = h;
    m->cells = calloc(m->width * m->height, sizeof(*m->cells));
}

void fov_map_destroy(rg_fov_map *m)
{
    if (m == NULL) return;
    free(m->cells);
}

bool fov_map_in_bounds(rg_fov_map *m, int x, int y)
{
    if (m == NULL) return false;
    return x >= 0 && x < m->width && y >= 0 && y < m->height;
}

bool fov_map_is_in_fov(rg_fov_map *m, int x, int y)
{
    if (!fov_map_in_bounds(m, x, y)) return false;
    return m->cells[x + y * m->width].fov;
}

void fov_map_set_props(rg_fov_map *m,
                       int x,
                       int y,
                       bool transparent,
                       bool walkable)
{
    if (!fov_map_in_bounds(m, x, y)) return;
    size_t idx = x + y * m->width;
    m->cells[idx].transparent = transparent;
    m->cells[idx].walkable = walkable;
}

void fov_map_cast_ray(rg_fov_map *m,
                      int orig_x,
                      int orig_y,
                      int dest_x,
                      int dest_y,
                      int radius_squared,
                      bool light_walls)
{
    rg_line_data data;
    int current_x;
    int current_y;
    line_init(orig_x, orig_y, dest_x, dest_y, &data);
    while (!line_step(&current_x, &current_y, &data))
    {
        if (!fov_map_in_bounds(m, current_x, current_y))
        {
            return; // Out of bounds.
        }
        if (radius_squared > 0)
        {
            const int current_radius =
              (current_x - orig_x) * (current_x - orig_x) +
              (current_y - orig_y) * (current_y - orig_y);
            if (current_radius > radius_squared)
            {
                return; // Outside of radius.
            }
        }
        const int map_index = current_x + current_y * m->width;
        if (!m->cells[map_index].transparent)
        {
            if (light_walls)
            {
                m->cells[map_index].fov = true;
            }
            return; // Blocked by wall.
        }
        // Tile is transparent.
        m->cells[map_index].fov = true;
    }
}

void fov_map_postprocess_quad(rg_fov_map *m,
                              int x0,
                              int y0,
                              int x1,
                              int y1,
                              int dx,
                              int dy)
{
    if (abs(dx) != 1 || abs(dy) != 1)
    {
        return; // Bad parameters.
    }
    for (int cx = x0; cx <= x1; cx++)
    {
        for (int cy = y0; cy <= y1; cy++)
        {
            const int x2 = cx + dx;
            const int y2 = cy + dy;
            const int offset = cx + cy * m->width;
            if (offset < (m->width * m->height) && m->cells[offset].fov == 1 &&
                m->cells[offset].transparent)
            {
                if (x2 >= x0 && x2 <= x1)
                {
                    const int offset2 = x2 + cy * m->width;
                    if (offset2 < (m->width * m->height) &&
                        !m->cells[offset2].transparent)
                    {
                        m->cells[offset2].fov = true;
                    }
                }
                if (y2 >= y0 && y2 <= y1)
                {
                    const int offset2 = cx + y2 * m->width;
                    if (offset2 < (m->width * m->height) &&
                        !m->cells[offset2].transparent)
                    {
                        m->cells[offset2].fov = true;
                    }
                }
                if (x2 >= x0 && x2 <= x1 && y2 >= y0 && y2 <= y1)
                {
                    const int offset2 = x2 + y2 * m->width;
                    if (offset2 < (m->width * m->height) &&
                        !m->cells[offset2].transparent)
                    {
                        m->cells[offset2].fov = true;
                    }
                }
            }
        }
    }
}

void fov_map_postprocess(rg_fov_map *m, int pov_x, int pov_y, int radius)
{
    int x_min = 0;
    int y_min = 0;
    int x_max = m->width;
    int y_max = m->height;
    if (radius > 0)
    {
        x_min = MAX(x_min, pov_x - radius);
        y_min = MAX(y_min, pov_y - radius);
        x_max = MIN(x_max, pov_x + radius + 1);
        y_max = MIN(y_max, pov_y + radius + 1);
    }
    fov_map_postprocess_quad(m, x_min, y_min, pov_x, pov_y, -1, -1);
    fov_map_postprocess_quad(m, pov_x, y_min, x_max - 1, pov_y, 1, -1);
    fov_map_postprocess_quad(m, x_min, pov_y, pov_x, y_max - 1, -1, 1);
    fov_map_postprocess_quad(m, pov_x, pov_y, x_max - 1, y_max - 1, 1, 1);
}

void fov_map_compute(rg_fov_map *m,
                     int pov_x,
                     int pov_y,
                     int max_radius,
                     bool light_walls)
{
    if (m == NULL) return;
    if (!fov_map_in_bounds(m, pov_x, pov_y)) return;
    for (int i = 0; i < m->width * m->height; i++) m->cells[i].fov = false;

    int x_min = 0;
    int y_min = 0;
    int x_max = m->width;
    int y_max = m->height;
    if (max_radius > 0)
    {
        x_min = MAX(x_min, pov_x - max_radius);
        y_min = MAX(y_min, pov_y - max_radius);
        x_max = MIN(x_max, pov_x + max_radius + 1);
        y_max = MIN(y_max, pov_y + max_radius + 1);
    }

    int idx = pov_x + pov_y * m->width;

    m->cells[idx].fov = true;
    // Cast rays along the perimeter.
    const int radius_squared = max_radius * max_radius;
    for (int x = x_min; x < x_max; ++x)
    {
        fov_map_cast_ray(
          m, pov_x, pov_y, x, y_min, radius_squared, light_walls);
    }
    for (int y = y_min + 1; y < y_max; ++y)
    {
        fov_map_cast_ray(
          m, pov_x, pov_y, x_max - 1, y, radius_squared, light_walls);
    }
    for (int x = x_max - 2; x >= x_min; --x)
    {
        fov_map_cast_ray(
          m, pov_x, pov_y, x, y_max - 1, radius_squared, light_walls);
    }
    for (int y = y_max - 2; y > y_min; --y)
    {
        fov_map_cast_ray(
          m, pov_x, pov_y, x_min, y, radius_squared, light_walls);
    }
    if (light_walls)
    {
        fov_map_postprocess(m, pov_x, pov_y, max_radius);
    }
}